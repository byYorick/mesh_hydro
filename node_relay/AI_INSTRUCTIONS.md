# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Relay

**Управление исполнительными устройствами (форточки, вентиляция, свет)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Relay (ESP32)** - узел управления климатом через исполнительные устройства.

### ⚡ Ключевые функции:

1. **Форточки** - 2× линейные актуаторы 12V
2. **Вентиляция** - вентилятор через реле
3. **Освещение** - LED лента с PWM диммированием (0-100%)
4. **Команды от ROOT** - выполнение команд управления

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ❌ **НЕТ ДАТЧИКОВ!** - только исполнительные устройства
2. ❌ **НЕТ ЛОГИКИ!** - команды приходят от ROOT
3. ✅ **ТОЛЬКО ВЫПОЛНЕНИЕ** - открыть/закрыть/включить/выключить
4. ✅ **SAFETY** - таймауты на актуаторы (макс 30 сек)
5. ✅ **PWM для света** - плавная регулировка яркости

---

## 🔌 РАСПИНОВКА

| GPIO | Назначение | Управление |
|------|------------|------------|
| **Форточки:** | | |
| 1 | Актуатор окно 1 | HIGH=открыто, LOW=закрыто |
| 2 | Актуатор окно 2 | HIGH=открыто, LOW=закрыто |
| **Вентиляция:** | | |
| 3 | Вентилятор | HIGH=вкл, LOW=выкл |
| **Освещение:** | | |
| 7 | LED свет (PWM) | 0-255 (0-100%) |

**Примечание:** Линейные актуаторы имеют встроенные концевики и останавливаются автоматически.

---

## 💻 РЕАЛИЗАЦИЯ

### main/app_main.c

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

static const char *TAG = "RELAY";

// GPIO пины
#define GPIO_ACTUATOR_1    1
#define GPIO_ACTUATOR_2    2
#define GPIO_FAN           3
#define GPIO_LED_PWM       7

// PWM для света
#define LED_PWM_FREQ       5000
#define LED_PWM_TIMER      LEDC_TIMER_0
#define LED_PWM_CHANNEL    LEDC_CHANNEL_0

static relay_node_config_t g_config;

// Инициализация GPIO
void gpio_init_all(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_ACTUATOR_1) | 
                        (1ULL << GPIO_ACTUATOR_2) |
                        (1ULL << GPIO_FAN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    
    // Все выключены по умолчанию
    gpio_set_level(GPIO_ACTUATOR_1, 0);
    gpio_set_level(GPIO_ACTUATOR_2, 0);
    gpio_set_level(GPIO_FAN, 0);
}

// Инициализация PWM для света
void led_pwm_init(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LED_PWM_TIMER,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = LED_PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));
    
    ledc_channel_config_t channel_conf = {
        .gpio_num = GPIO_LED_PWM,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LED_PWM_CHANNEL,
        .timer_sel = LED_PWM_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
}

// Установка яркости света 0-100%
void set_light_brightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    
    uint32_t duty = (percent * 255) / 100;
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, LED_PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, LED_PWM_CHANNEL));
    
    ESP_LOGI(TAG, "Light brightness set to %d%%", percent);
}

// Обработка команд от ROOT
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
        
        // Выполнение команд
        if (strcmp(command, "open_windows") == 0) {
            gpio_set_level(GPIO_ACTUATOR_1, 1);
            gpio_set_level(GPIO_ACTUATOR_2, 1);
            ESP_LOGI(TAG, "Windows opening...");
            
        } else if (strcmp(command, "close_windows") == 0) {
            gpio_set_level(GPIO_ACTUATOR_1, 0);
            gpio_set_level(GPIO_ACTUATOR_2, 0);
            ESP_LOGI(TAG, "Windows closing...");
            
        } else if (strcmp(command, "start_fan") == 0) {
            gpio_set_level(GPIO_FAN, 1);
            ESP_LOGI(TAG, "Fan started");
            
        } else if (strcmp(command, "stop_fan") == 0) {
            gpio_set_level(GPIO_FAN, 0);
            ESP_LOGI(TAG, "Fan stopped");
            
        } else if (strcmp(command, "set_light_brightness") == 0) {
            cJSON *brightness = cJSON_GetObjectItem(msg.data, "brightness");
            if (brightness && cJSON_IsNumber(brightness)) {
                set_light_brightness((uint8_t)brightness->valueint);
            }
            
        } else {
            ESP_LOGW(TAG, "Unknown command: %s", command);
        }
    }
    
    mesh_protocol_free_message(&msg);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== NODE Relay Starting ===");
    
    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Загрузка конфигурации
    if (node_config_load(&g_config, sizeof(g_config), "relay_ns") != ESP_OK) {
        node_config_reset_to_default(&g_config, "relay");
        strcpy(g_config.base.node_id, "relay_001");
        node_config_save(&g_config, sizeof(g_config), "relay_ns");
    }
    
    // 3. Инициализация GPIO и PWM
    gpio_init_all();
    led_pwm_init();
    
    // 4. Mesh (NODE режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_2025",
        .channel = 1
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    
    ESP_LOGI(TAG, "=== NODE Relay Running ===");
    ESP_LOGI(TAG, "Waiting for commands from ROOT...");
}
```

---

## 🚫 ЧТО НЕЛЬЗЯ ДЕЛАТЬ

### 1. ❌ НЕ принимай решения самостоятельно

```c
// ПЛОХО ❌
void some_task(void *arg) {
    if (some_condition) {
        gpio_set_level(GPIO_FAN, 1);  // ❌ Самостоятельное решение!
    }
}

// ХОРОШО ✅
void on_mesh_data_received(...) {
    // Только выполняем команды от ROOT
    if (strcmp(command, "start_fan") == 0) {
        gpio_set_level(GPIO_FAN, 1);  // ✅ По команде ROOT
    }
}
```

### 2. ❌ НЕ добавляй датчики

Relay узел - это "руки", но не "глаза" системы.

---

## ✅ ЧТО НУЖНО ДЕЛАТЬ

### 1. ✅ Safety таймеры для актуаторов

```c
TimerHandle_t actuator_timer;

void start_actuator_with_timeout(gpio_num_t gpio, uint32_t timeout_ms) {
    gpio_set_level(gpio, 1);
    
    // Таймер безопасности (автоматически остановит через timeout_ms)
    xTimerStart(actuator_timer, pdMS_TO_TICKS(timeout_ms));
}

void actuator_timeout_callback(TimerHandle_t timer) {
    // Аварийная остановка актуаторов
    gpio_set_level(GPIO_ACTUATOR_1, 0);
    gpio_set_level(GPIO_ACTUATOR_2, 0);
    ESP_LOGW(TAG, "Actuator safety timeout!");
}
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест через MQTT (с ROOT узлом)

```bash
# Открыть форточки
mosquitto_pub -h 192.168.1.100 -t "hydro/command/relay_001" \
  -m '{"type":"command","node_id":"relay_001","command":"open_windows"}'

# Включить вентилятор
mosquitto_pub -h 192.168.1.100 -t "hydro/command/relay_001" \
  -m '{"type":"command","node_id":"relay_001","command":"start_fan"}'

# Установить яркость света 80%
mosquitto_pub -h 192.168.1.100 -t "hydro/command/relay_001" \
  -m '{"type":"command","node_id":"relay_001","command":"set_light_brightness","brightness":80}'
```

---

## 📊 ЧЕКЛИСТ РАЗРАБОТКИ

- [ ] GPIO инициализация
- [ ] PWM инициализация для света
- [ ] Обработка команды open_windows
- [ ] Обработка команды close_windows
- [ ] Обработка команды start_fan
- [ ] Обработка команды stop_fan
- [ ] Обработка команды set_light_brightness
- [ ] Safety таймеры (опционально)
- [ ] Mesh NODE режим
- [ ] Тестирование через MQTT

---

**NODE Relay - исполнитель команд ROOT!** ⚡

**Приоритет:** 🟡 СРЕДНИЙ (Фаза 7)

**Время реализации:** 2-3 дня
