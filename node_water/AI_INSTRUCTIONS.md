# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Water

**Управление поливом (насосы + клапаны)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Water (ESP32-C3)** - узел управления системой полива.

### 💧 Ключевые функции:

1. **Насос подачи** - подача воды из резервуара
2. **Насос слива** - слив воды (опционально)
3. **3 клапана** - соленоидные клапаны для зон полива
4. **Датчик уровня** - проверка наличия воды (опционально)
5. **Команды от ROOT** - выполнение команд полива

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ✅ **SAFETY** - макс время насоса 60 сек
2. ✅ **ПРОВЕРКА УРОВНЯ** - не включать насос без воды
3. ✅ **ЛОГИРОВАНИЕ** - запись всех операций (мл, время)
4. ❌ **НЕ РАБОТАЙ БЕЗ КОМАНДЫ** - только по команде ROOT или расписанию

---

## 🔌 РАСПИНОВКА

| GPIO | Назначение | Управление |
|------|------------|------------|
| 1 | Насос подачи | HIGH=вкл |
| 2 | Насос слива | HIGH=вкл |
| 3 | Клапан зона 1 | HIGH=открыт |
| 4 | Клапан зона 2 | HIGH=открыт |
| 5 | Клапан зона 3 | HIGH=открыт |
| 6 | Датчик уровня воды | LOW=есть вода |

---

## 💻 РЕАЛИЗАЦИЯ

### main/app_main.c

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

static const char *TAG = "WATER";

// GPIO
#define GPIO_PUMP_IN       1
#define GPIO_PUMP_OUT      2
#define GPIO_VALVE_1       3
#define GPIO_VALVE_2       4
#define GPIO_VALVE_3       5
#define GPIO_WATER_LEVEL   6

static water_node_config_t g_config;

// Проверка уровня воды
bool check_water_level(void) {
    if (!g_config.water_level_check) {
        return true;  // Датчик отключен
    }
    
    int level = gpio_get_level(GPIO_WATER_LEVEL);
    return (level == 0);  // LOW = есть вода
}

// Активация зоны полива
void water_zone_activate(int zone_num, uint32_t duration_ms) {
    if (zone_num < 0 || zone_num > 2) {
        ESP_LOGE(TAG, "Invalid zone: %d", zone_num);
        return;
    }
    
    // Проверка уровня воды
    if (!check_water_level()) {
        ESP_LOGE(TAG, "Low water level! Cannot activate zone %d", zone_num + 1);
        return;
    }
    
    // Проверка активности зоны
    if (!(g_config.active_zones & (1 << zone_num))) {
        ESP_LOGW(TAG, "Zone %d is disabled", zone_num + 1);
        return;
    }
    
    // Ограничение времени работы
    if (duration_ms > g_config.max_pump_time_ms) {
        ESP_LOGW(TAG, "Duration %d ms limited to %d ms", 
                 duration_ms, g_config.max_pump_time_ms);
        duration_ms = g_config.max_pump_time_ms;
    }
    
    gpio_num_t valve_gpio = GPIO_VALVE_1 + zone_num;
    
    ESP_LOGI(TAG, "Activating zone %d for %d ms", zone_num + 1, duration_ms);
    
    // Открыть клапан зоны
    gpio_set_level(valve_gpio, 1);
    
    // Включить насос подачи
    gpio_set_level(GPIO_PUMP_IN, 1);
    
    // Работа указанное время
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    
    // Выключить
    gpio_set_level(GPIO_PUMP_IN, 0);
    gpio_set_level(valve_gpio, 0);
    
    ESP_LOGI(TAG, "Zone %d completed", zone_num + 1);
    
    // TODO: Логирование в NVS (мл, время)
}

void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    if (!mesh_protocol_parse((const char*)data, &msg)) {
        return;
    }
    
    if (strcmp(msg.node_id, g_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        return;
    }
    
    if (msg.type == MESH_MSG_COMMAND) {
        cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
        if (!cmd || !cJSON_IsString(cmd)) {
            mesh_protocol_free_message(&msg);
            return;
        }
        
        const char *command = cmd->valuestring;
        ESP_LOGI(TAG, "Command received: %s", command);
        
        if (strcmp(command, "water_zone") == 0) {
            cJSON *zone = cJSON_GetObjectItem(msg.data, "zone");
            cJSON *duration = cJSON_GetObjectItem(msg.data, "duration_ms");
            
            if (zone && duration && cJSON_IsNumber(zone) && cJSON_IsNumber(duration)) {
                int zone_num = zone->valueint - 1;  // 1-3 → 0-2
                uint32_t duration_ms = (uint32_t)duration->valueint;
                
                water_zone_activate(zone_num, duration_ms);
            }
        }
    }
    
    mesh_protocol_free_message(&msg);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== NODE Water Starting ===");
    
    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Загрузка конфигурации
    if (node_config_load(&g_config, sizeof(g_config), "water_ns") != ESP_OK) {
        node_config_reset_to_default(&g_config, "water");
        strcpy(g_config.base.node_id, "water_001");
        node_config_save(&g_config, sizeof(g_config), "water_ns");
    }
    
    // 3. GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_PUMP_IN) | (1ULL << GPIO_PUMP_OUT) |
                        (1ULL << GPIO_VALVE_1) | (1ULL << GPIO_VALVE_2) | 
                        (1ULL << GPIO_VALVE_3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Датчик уровня (вход)
    gpio_config_t input_conf = {
        .pin_bit_mask = (1ULL << GPIO_WATER_LEVEL),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    ESP_ERROR_CHECK(gpio_config(&input_conf));
    
    // 4. Mesh
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_2025",
        .channel = 1
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    
    ESP_LOGI(TAG, "=== NODE Water Running ===");
}
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест через MQTT

```bash
# Полив зоны 1 в течение 5 секунд
mosquitto_pub -h 192.168.1.100 -t "hydro/command/water_001" \
  -m '{"type":"command","node_id":"water_001","command":"water_zone","zone":1,"duration_ms":5000}'
```

---

## 📊 ЧЕКЛИСТ РАЗРАБОТКИ

- [ ] GPIO инициализация
- [ ] Проверка уровня воды
- [ ] Функция water_zone_activate()
- [ ] Safety ограничения
- [ ] Обработка команд от ROOT
- [ ] Логирование операций
- [ ] Mesh NODE режим
- [ ] Тестирование через MQTT

---

**NODE Water - автоматизированный полив!** 💧

**Приоритет:** 🟢 НИЗКИЙ (Фаза 8)

**Время реализации:** 2-3 дня
