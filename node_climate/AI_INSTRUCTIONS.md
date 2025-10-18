# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Climate

**Датчики климата (температура, влажность, CO2, освещенность)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Climate (ESP32)** - узел мониторинга климатических условий в теплице.

### 🌡️ Ключевые функции:

1. **Чтение датчиков** - температура, влажность, CO2, освещенность
2. **Отправка телеметрии** - каждые 10 секунд на ROOT
3. **Heartbeat** - подтверждение работоспособности
4. **Хранение данных** - локальный буфер при отключении mesh

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** - Climate node только читает датчики
2. ❌ **НЕТ ЛОГИКИ УПРАВЛЕНИЯ!** - решения принимает ROOT
3. ✅ **ТОЛЬКО МОНИТОРИНГ** - температура, влажность, CO2, lux
4. ✅ **СТАБИЛЬНОЕ ЧТЕНИЕ** - retry логика при ошибках датчиков
5. ✅ **БУФЕРИЗАЦИЯ** - сохранять данные при потере связи

---

## 📦 ДАТЧИКИ

| Датчик | Интерфейс | I2C адрес | Назначение |
|--------|-----------|-----------|------------|
| **SHT3x** | I2C | 0x44 | Температура + влажность |
| **CCS811** | I2C | 0x5A | CO2 + TVOC |
| **Trema Lux** | I2C | 0x12 | Освещенность (lux) |

---

## 🔌 РАСПИНОВКА

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина:** | | |
| 17 | I2C SCL | Clock |
| 18 | I2C SDA | Data |
| **Индикация (опц.):** | | |
| 15 | LED статус | Зеленый=OK, желтый=degraded |

---

## 🏗️ АРХИТЕКТУРА

```
NODE Climate (ESP32)
│
├── Sensor Manager
│   ├── SHT3x driver (temp + humidity)
│   ├── CCS811 driver (CO2)
│   ├── Trema Lux driver (освещенность)
│   ├── Retry логика (3 попытки)
│   └── Валидация данных
│
├── Mesh Client (NODE режим)
│   ├── Отправка телеметрии → ROOT
│   ├── Heartbeat каждые 10 сек
│   └── Обработка команд от ROOT
│
├── Local Buffer
│   ├── Ring buffer (500 записей)
│   ├── Сохранение при offline
│   └── Синхронизация при online
│
└── Node Controller
    ├── Главный цикл (10 сек)
    ├── Обработка команд
    └── Мониторинг связи
```

---

## 💻 РЕАЛИЗАЦИЯ

### main/app_main.c

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

#include "sht3x_driver.h"
#include "ccs811_driver.h"
#include "trema_lux_driver.h"

static const char *TAG = "CLIMATE";

// Конфигурация
static climate_node_config_t g_config;

void send_telemetry(float temp, float humidity, uint16_t co2, uint16_t lux) {
    if (!mesh_manager_is_connected()) {
        ESP_LOGW(TAG, "Mesh offline, buffering data");
        // TODO: local_buffer_add()
        return;
    }
    
    // Создание JSON телеметрии
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "temp", temp);
    cJSON_AddNumberToObject(data, "humidity", humidity);
    cJSON_AddNumberToObject(data, "co2", co2);
    cJSON_AddNumberToObject(data, "lux", lux);
    
    char json_buf[512];
    if (mesh_protocol_create_telemetry(g_config.base.node_id, data, 
                                        json_buf, sizeof(json_buf))) {
        mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));
        ESP_LOGI(TAG, "Telemetry sent: %.1f°C, %.0f%%, %dppm, %dlux", 
                 temp, humidity, co2, lux);
    }
    
    cJSON_Delete(data);
}

void climate_main_task(void *arg) {
    float temp, humidity;
    uint16_t co2, lux;
    
    while (1) {
        // Чтение датчиков
        esp_err_t ret_temp = sht3x_read(&temp, &humidity);
        esp_err_t ret_co2 = ccs811_read(&co2);
        esp_err_t ret_lux = trema_lux_read(&lux);
        
        if (ret_temp == ESP_OK && ret_co2 == ESP_OK && ret_lux == ESP_OK) {
            // Отправка телеметрии
            send_telemetry(temp, humidity, co2, lux);
        } else {
            ESP_LOGE(TAG, "Sensor read error: temp=%d, co2=%d, lux=%d", 
                     ret_temp, ret_co2, ret_lux);
        }
        
        // Интервал чтения
        vTaskDelay(pdMS_TO_TICKS(g_config.read_interval_ms));
    }
}

void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    if (!mesh_protocol_parse((const char*)data, &msg)) {
        return;
    }
    
    if (strcmp(msg.node_id, g_config.base.node_id) != 0) {
        mesh_protocol_free_message(&msg);
        return;  // Не для нас
    }
    
    switch (msg.type) {
        case MESH_MSG_COMMAND:
            ESP_LOGI(TAG, "Command received");
            // Обработка команд (например, изменение интервала)
            break;
            
        case MESH_MSG_CONFIG:
            ESP_LOGI(TAG, "Config update received");
            node_config_update_from_json(&g_config, msg.data, "climate");
            node_config_save(&g_config, sizeof(g_config), "climate_ns");
            break;
    }
    
    mesh_protocol_free_message(&msg);
}

void app_main(void) {
    ESP_LOGI(TAG, "=== NODE Climate Starting ===");
    
    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Загрузка конфигурации
    if (node_config_load(&g_config, sizeof(g_config), "climate_ns") != ESP_OK) {
        // Первый запуск - значения по умолчанию
        node_config_reset_to_default(&g_config, "climate");
        strcpy(g_config.base.node_id, "climate_001");
        strcpy(g_config.base.zone, "Greenhouse Zone 1");
        node_config_save(&g_config, sizeof(g_config), "climate_ns");
    }
    
    // 3. Инициализация I2C
    i2c_config_t i2c_conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = 18,
        .scl_io_num = 17,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
    
    // 4. Инициализация датчиков
    ESP_ERROR_CHECK(sht3x_init());
    ESP_ERROR_CHECK(ccs811_init());
    ESP_ERROR_CHECK(trema_lux_init());
    
    // 5. Mesh (NODE режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_2025",
        .channel = 1
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    
    // 6. Запуск главной задачи
    xTaskCreate(climate_main_task, "climate_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "=== NODE Climate Running ===");
}
```

---

## 🚫 ЧТО НЕЛЬЗЯ ДЕЛАТЬ

### 1. ❌ НЕ управляй исполнителями

```c
// ПЛОХО ❌
void climate_task(void *arg) {
    float temp = read_temperature();
    
    if (temp > 28.0) {
        gpio_set_level(FAN_GPIO, 1);  // ❌ НЕТ! Это делает ROOT!
    }
}

// ХОРОШО ✅
void climate_task(void *arg) {
    float temp = read_temperature();
    
    // Просто отправляем данные на ROOT
    send_telemetry(temp, ...);
    // ROOT примет решение о включении вентилятора
}
```

### 2. ❌ НЕ принимай решения

Climate узел - это "глаза", но не "мозг" системы.

---

## ✅ ЧТО НУЖНО ДЕЛАТЬ

### 1. ✅ Стабильное чтение датчиков

```c
esp_err_t read_sensors_with_retry(float *temp, float *humidity, 
                                   uint16_t *co2, uint16_t *lux) {
    const int max_retries = 3;
    
    for (int i = 0; i < max_retries; i++) {
        if (sht3x_read(temp, humidity) == ESP_OK &&
            ccs811_read(co2) == ESP_OK &&
            trema_lux_read(lux) == ESP_OK) {
            return ESP_OK;
        }
        
        ESP_LOGW(TAG, "Sensor read failed, retry %d/%d", i+1, max_retries);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    return ESP_FAIL;
}
```

### 2. ✅ Валидация данных

```c
bool validate_sensor_data(float temp, float humidity, uint16_t co2) {
    // Проверка разумности данных
    if (temp < -10.0 || temp > 50.0) {
        ESP_LOGW(TAG, "Invalid temperature: %.1f", temp);
        return false;
    }
    
    if (humidity < 0.0 || humidity > 100.0) {
        ESP_LOGW(TAG, "Invalid humidity: %.0f", humidity);
        return false;
    }
    
    if (co2 > 5000) {
        ESP_LOGW(TAG, "Invalid CO2: %d", co2);
        return false;
    }
    
    return true;
}
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест 1: Чтение датчиков

```bash
cd node_climate
idf.py set-target esp32
idf.py build flash monitor
```

**Ожидаемый лог:**
```
I CLIMATE: === NODE Climate Starting ===
I CLIMATE: SHT3x initialized
I CLIMATE: CCS811 initialized
I CLIMATE: Trema Lux initialized
I mesh_manager: NODE mode
I CLIMATE: === NODE Climate Running ===
I CLIMATE: Telemetry sent: 24.5°C, 65%, 450ppm, 1200lux
```

### Тест 2: Подключение к ROOT

Убедись, что ROOT прошит и работает, затем:
```bash
idf.py monitor
```

**В логах Climate должно быть:**
```
I mesh_manager: Parent connected
I CLIMATE: Telemetry sent: 24.3°C, 67%, 445ppm, 1180lux
```

**В логах ROOT:**
```
I node_registry: New node added: climate_001
I data_router: Telemetry from climate_001 → MQTT
```

---

## 📊 ЧЕКЛИСТ РАЗРАБОТКИ

- [ ] I2C инициализация
- [ ] SHT3x driver
- [ ] CCS811 driver
- [ ] Trema Lux driver
- [ ] Retry логика для датчиков
- [ ] Mesh NODE режим
- [ ] Отправка телеметрии
- [ ] Обработка команд от ROOT
- [ ] Локальный буфер (опционально)
- [ ] Тестирование с реальными датчиками
- [ ] Тестирование с ROOT узлом

---

**NODE Climate - простой и надежный мониторинг климата!** 🌡️

**Приоритет:** 🟡 СРЕДНИЙ (Фаза 7)

**Время реализации:** 2-3 дня
