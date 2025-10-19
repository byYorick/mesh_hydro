# 🤖 ПОЛНАЯ ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Water

**Узел управления водоснабжением (насос, клапаны, датчик уровня)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Water (ESP32)** - управление подачей и сливом воды в гидропонную систему.

### 💧 Ключевые функции:

1. **Мониторинг уровня воды** - ультразвуковой датчик HC-SR04
2. **Управление главным насосом** - подача воды из бака
3. **Управление дренажем** - клапан слива
4. **Защита от переполнения** - автоматическая остановка насоса
5. **Защита от сухого хода** - остановка при низком уровне
6. **Статистика потребления** - литры за час/день/неделю

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ❌ **НЕТ АВТОНОМИИ!** - Water работает ТОЛЬКО по командам от ROOT
2. ✅ **SAFETY FIRST** - защита от переполнения и сухого хода
3. ✅ **TIMEOUT PROTECTION** - максимальное время работы насоса
4. ✅ **EMERGENCY STOP** - немедленная остановка при аварии
5. ❌ **НЕ принимай решений** - только исполняй команды

---

## 📦 ДАТЧИКИ И ИСПОЛНИТЕЛИ

### Датчики:
| Датчик | Интерфейс | GPIO | Назначение | Диапазон |
|--------|-----------|------|------------|----------|
| **HC-SR04** | Ultrasonic | Trig=12, Echo=13 | Уровень воды в баке | 2-400 cm |
| **Float Switch Low** | Digital | 14 | Минимальный уровень | ON/OFF |
| **Float Switch High** | Digital | 27 | Максимальный уровень | ON/OFF |

### Исполнители:
| Актуатор | GPIO | Тип | Назначение |
|----------|------|-----|------------|
| **Main Pump** | 25 | Relay | Насос подачи воды (12V, 20W) |
| **Drain Valve** | 26 | Solenoid | Клапан слива (12V) |
| **Fill Valve** | 33 | Solenoid | Клапан наполнения (12V) |

### Индикация:
| Компонент | GPIO | Назначение |
|-----------|------|------------|
| **LED Status** | 2 | Зеленый=OK, Красный=Error |
| **Buzzer** | 15 | Аварийные сигналы |

---

## 🔌 РАСПИНОВКА ESP32

| GPIO | Назначение | Тип | Примечание |
|------|------------|-----|------------|
| **Датчики уровня:** | | | |
| 12 | HC-SR04 Trig | Output | Ультразвуковой |
| 13 | HC-SR04 Echo | Input | Ультразвуковой |
| 14 | Float Switch Low | Input | Pullup, LOW=вода есть |
| 27 | Float Switch High | Input | Pullup, LOW=переполнение |
| **Актуаторы:** | | | |
| 25 | Main Pump | Relay | HIGH=включен |
| 26 | Drain Valve | Relay | HIGH=открыт |
| 33 | Fill Valve | Relay | HIGH=открыт |
| **Индикация:** | | | |
| 2 | Status LED | GPIO | Встроенный LED |
| 15 | Buzzer | GPIO | 5V активный |

---

## 🏗️ АРХИТЕКТУРА

```
NODE Water (ESP32)
│
├── Sensor Manager
│   ├── HC-SR04 ultrasonic driver
│   ├── Float switches (digital)
│   ├── Water level calculation (cm)
│   └── Average filtering (5 samples)
│
├── Pump Controller
│   ├── Main pump (ON/OFF)
│   ├── Safety timeout (max 10 min)
│   ├── Dry run protection
│   └── Statistics (liters/hour/day)
│
├── Valve Controller
│   ├── Drain valve (ON/OFF)
│   ├── Fill valve (ON/OFF)
│   ├── Interlock (не открывать одновременно!)
│   └── Timeout protection
│
├── Command Handler
│   ├── Команды от ROOT
│   ├── Validation
│   ├── Execution
│   └── Status response
│
├── Mesh Client (NODE mode)
│   ├── Receive commands ← ROOT
│   ├── Send status → ROOT
│   ├── Heartbeat every 10 sec
│   └── Emergency events
│
└── Safety Monitor
    ├── Overflow protection (float high)
    ├── Dry run protection (float low)
    ├── Timeout checks
    └── Emergency stop
```

---

## 📨 ТИПЫ СООБЩЕНИЙ

### 1. 📤 STATUS (каждые 10 сек или после команды)

**NODE → ROOT → MQTT → Server**

```json
{
  "type": "telemetry",
  "node_id": "water_001",
  "timestamp": 1729346400,
  "data": {
    "water_level_cm": 45,
    "water_level_percent": 75,
    "float_low": true,
    "float_high": false,
    "pump_running": false,
    "drain_open": false,
    "fill_open": false,
    "pump_runtime_today_sec": 1800,
    "water_used_today_liters": 150,
    "rssi_to_parent": -48
  }
}
```

**Код:**
```c
static void send_status(void) {
    float level_cm = water_level_read_cm();
    float level_percent = (level_cm / TANK_HEIGHT_CM) * 100.0f;
    
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "water_level_cm", level_cm);
    cJSON_AddNumberToObject(data, "water_level_percent", level_percent);
    cJSON_AddBoolToObject(data, "float_low", gpio_get_level(FLOAT_LOW_GPIO) == 0);
    cJSON_AddBoolToObject(data, "float_high", gpio_get_level(FLOAT_HIGH_GPIO) == 0);
    cJSON_AddBoolToObject(data, "pump_running", pump_is_running());
    cJSON_AddBoolToObject(data, "drain_open", valve_is_open(VALVE_DRAIN));
    cJSON_AddBoolToObject(data, "fill_open", valve_is_open(VALVE_FILL));
    cJSON_AddNumberToObject(data, "pump_runtime_today_sec", pump_get_runtime_today());
    cJSON_AddNumberToObject(data, "water_used_today_liters", pump_get_liters_today());
    cJSON_AddNumberToObject(data, "rssi_to_parent", mesh_manager_get_parent_rssi());
    
    char json_buf[512];
    mesh_protocol_create_telemetry(s_config->base.node_id, data, json_buf, sizeof(json_buf));
    mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
    
    cJSON_Delete(data);
}
```

---

### 2. 📥 COMMAND (Server → MQTT → ROOT → NODE)

**Топик:** `hydro/command/water_001`

#### a) Запуск насоса:
```json
{
  "type": "command",
  "node_id": "water_001",
  "command": "run_pump",
  "params": {
    "duration_sec": 60
  }
}
```

#### b) Открыть дренаж:
```json
{
  "type": "command",
  "node_id": "water_001",
  "command": "open_drain",
  "params": {
    "duration_sec": 30
  }
}
```

#### c) Наполнить бак:
```json
{
  "type": "command",
  "node_id": "water_001",
  "command": "fill_tank",
  "params": {
    "target_level_cm": 50  // Наполнить до 50 см
  }
}
```

#### d) Emergency stop:
```json
{
  "type": "command",
  "node_id": "water_001",
  "command": "emergency_stop",
  "params": {}
}
```

**Обработка:**
```c
void water_controller_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);
    
    if (strcmp(command, "run_pump") == 0) {
        cJSON *duration = cJSON_GetObjectItem(params, "duration_sec");
        
        if (duration && cJSON_IsNumber(duration)) {
            uint32_t duration_sec = (uint32_t)duration->valueint;
            
            esp_err_t err = pump_controller_run(duration_sec);
            
            if (err == ESP_OK) {
                ESP_LOGI(TAG, "Pump started for %d sec", duration_sec);
            } else {
                ESP_LOGE(TAG, "Pump start failed: %s", esp_err_to_name(err));
                send_error_event("Pump start failed");
            }
            
            send_status();
        }
    }
    else if (strcmp(command, "open_drain") == 0) {
        cJSON *duration = cJSON_GetObjectItem(params, "duration_sec");
        
        if (duration && cJSON_IsNumber(duration)) {
            valve_controller_open(VALVE_DRAIN, duration->valueint);
            send_status();
        }
    }
    else if (strcmp(command, "fill_tank") == 0) {
        cJSON *target = cJSON_GetObjectItem(params, "target_level_cm");
        
        if (target && cJSON_IsNumber(target)) {
            float target_cm = (float)target->valuedouble;
            water_controller_fill_to_level(target_cm);
        }
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        water_controller_emergency_stop();
    }
}
```

---

### 3. 💓 HEARTBEAT (каждые 10 сек)

```json
{
  "type": "heartbeat",
  "node_id": "water_001",
  "uptime": 3600,
  "heap_free": 200000,
  "rssi_to_parent": -48
}
```

---

### 4. 🔔 EVENT (аварии)

**Когда отправлять:**
- Переполнение бака (float_high triggered)
- Низкий уровень (float_low triggered)
- Насос работает > 10 мин
- Dry run protection сработала
- Клапаны открыты одновременно (interlock error)

```json
{
  "type": "event",
  "node_id": "water_001",
  "timestamp": 1729346400,
  "level": "critical",
  "message": "Tank overflow detected",
  "data": {
    "water_level_cm": 65,
    "float_high": true,
    "action": "pump_stopped"
  }
}
```

---

## 💧 УПРАВЛЕНИЕ ВОДОЙ

### Логика запуска насоса:

```c
esp_err_t pump_controller_run(uint32_t duration_sec) {
    // 1. Проверка float switches
    if (gpio_get_level(FLOAT_LOW_GPIO) == 1) {  // HIGH = нет воды
        ESP_LOGE(TAG, "Cannot run pump - water level too low (dry run protection)");
        send_error_event("Dry run protection");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (gpio_get_level(FLOAT_HIGH_GPIO) == 0) {  // LOW = переполнение
        ESP_LOGW(TAG, "Cannot run pump - tank is full");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 2. Проверка максимального времени
    if (duration_sec > PUMP_MAX_DURATION_SEC) {
        ESP_LOGW(TAG, "Duration too long: %d sec (max %d)", duration_sec, PUMP_MAX_DURATION_SEC);
        duration_sec = PUMP_MAX_DURATION_SEC;
    }
    
    // 3. Проверка лимита в сутки
    if (s_pump_runtime_today_sec > PUMP_MAX_RUNTIME_PER_DAY_SEC) {
        ESP_LOGE(TAG, "Daily runtime limit exceeded");
        send_error_event("Pump daily limit exceeded");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 4. Запуск насоса
    s_pump_running = true;
    s_pump_start_time = xTaskGetTickCount();
    gpio_set_level(PUMP_GPIO, 1);
    
    ESP_LOGI(TAG, "Pump started (duration=%d sec)", duration_sec);
    
    // 5. Создание задачи мониторинга
    xTaskCreate(pump_monitor_task, "pump_mon", 2048, (void *)(intptr_t)duration_sec, 5, &s_pump_monitor_task);
    
    return ESP_OK;
}

// Задача мониторинга насоса:
static void pump_monitor_task(void *arg) {
    uint32_t duration_sec = (uint32_t)(intptr_t)arg;
    uint32_t elapsed_sec = 0;
    
    while (elapsed_sec < duration_sec) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        elapsed_sec++;
        
        // Проверка переполнения КАЖДУЮ СЕКУНДУ
        if (gpio_get_level(FLOAT_HIGH_GPIO) == 0) {
            ESP_LOGW(TAG, "Tank full - stopping pump");
            pump_controller_stop();
            send_warning_event("Tank full - pump auto-stopped");
            vTaskDelete(NULL);
            return;
        }
        
        // Проверка сухого хода
        if (gpio_get_level(FLOAT_LOW_GPIO) == 1) {
            ESP_LOGE(TAG, "Water level too low - stopping pump");
            pump_controller_stop();
            send_error_event("Dry run protection activated");
            vTaskDelete(NULL);
            return;
        }
    }
    
    // Нормальная остановка по таймеру
    pump_controller_stop();
    ESP_LOGI(TAG, "Pump stopped (duration complete)");
    
    vTaskDelete(NULL);
}
```

---

### Измерение уровня воды (HC-SR04):

```c
float water_level_read_cm(void) {
    const int NUM_SAMPLES = 5;
    float samples[NUM_SAMPLES];
    int valid_count = 0;
    
    // Берем 5 измерений
    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Отправка импульса (10 мкс)
        gpio_set_level(TRIG_GPIO, 0);
        esp_rom_delay_us(2);
        gpio_set_level(TRIG_GPIO, 1);
        esp_rom_delay_us(10);
        gpio_set_level(TRIG_GPIO, 0);
        
        // Ожидание echo (timeout 30ms)
        int64_t start = esp_timer_get_time();
        while (gpio_get_level(ECHO_GPIO) == 0) {
            if ((esp_timer_get_time() - start) > 30000) break;
        }
        
        start = esp_timer_get_time();
        while (gpio_get_level(ECHO_GPIO) == 1) {
            if ((esp_timer_get_time() - start) > 30000) break;
        }
        int64_t duration_us = esp_timer_get_time() - start;
        
        // Расчет расстояния (см)
        float distance_cm = (duration_us * 0.0343f) / 2.0f;
        
        // Валидация (2-400 см)
        if (distance_cm > 2.0f && distance_cm < 400.0f) {
            samples[valid_count++] = distance_cm;
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));  // 100ms между измерениями
    }
    
    // Среднее значение
    if (valid_count == 0) {
        ESP_LOGE(TAG, "All HC-SR04 measurements failed");
        return -1.0f;
    }
    
    float sum = 0.0f;
    for (int i = 0; i < valid_count; i++) {
        sum += samples[i];
    }
    
    float avg_distance = sum / (float)valid_count;
    
    // Уровень воды = высота бака - расстояние до датчика
    float water_level = TANK_HEIGHT_CM - avg_distance;
    
    ESP_LOGD(TAG, "Water level: %.1f cm (%.0f%%)", water_level, (water_level / TANK_HEIGHT_CM) * 100.0f);
    
    return water_level;
}
```

---

### Управление клапанами:

```c
esp_err_t valve_controller_open(valve_type_t valve, uint32_t duration_sec) {
    // Проверка interlock (не открывать drain и fill одновременно!)
    if (valve == VALVE_DRAIN && s_valve_fill_open) {
        ESP_LOGE(TAG, "Cannot open drain - fill valve is open!");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (valve == VALVE_FILL && s_valve_drain_open) {
        ESP_LOGE(TAG, "Cannot open fill - drain valve is open!");
        return ESP_ERR_INVALID_STATE;
    }
    
    int gpio = (valve == VALVE_DRAIN) ? DRAIN_GPIO : FILL_GPIO;
    bool *is_open = (valve == VALVE_DRAIN) ? &s_valve_drain_open : &s_valve_fill_open;
    
    // Открытие клапана
    gpio_set_level(gpio, 1);
    *is_open = true;
    
    ESP_LOGI(TAG, "Valve %s opened (duration=%d sec)", 
             valve == VALVE_DRAIN ? "DRAIN" : "FILL", duration_sec);
    
    // Создание задачи auto-close
    if (duration_sec > 0) {
        xTaskCreate(valve_auto_close_task, "valve_close", 2048, 
                    (void *)(intptr_t)valve, 5, NULL);
    }
    
    return ESP_OK;
}

esp_err_t valve_controller_close(valve_type_t valve) {
    int gpio = (valve == VALVE_DRAIN) ? DRAIN_GPIO : FILL_GPIO;
    bool *is_open = (valve == VALVE_DRAIN) ? &s_valve_drain_open : &s_valve_fill_open;
    
    gpio_set_level(gpio, 0);
    *is_open = false;
    
    ESP_LOGI(TAG, "Valve %s closed", valve == VALVE_DRAIN ? "DRAIN" : "FILL");
    return ESP_OK;
}
```

---

## 📨 ПРИМЕРЫ КОМАНД

### 1. Наполнить бак до 50 см:

**Команда:**
```json
{
  "type": "command",
  "node_id": "water_001",
  "command": "fill_tank",
  "params": {
    "target_level_cm": 50
  }
}
```

**Логика:**
```c
void water_controller_fill_to_level(float target_cm) {
    float current_level = water_level_read_cm();
    
    if (current_level >= target_cm) {
        ESP_LOGI(TAG, "Tank already at target level");
        return;
    }
    
    ESP_LOGI(TAG, "Filling tank: %.1f → %.1f cm", current_level, target_cm);
    
    // Открыть fill valve
    valve_controller_open(VALVE_FILL, 0);  // 0 = без auto-close
    
    // Мониторинг в отдельной задаче
    xTaskCreate(fill_monitor_task, "fill_mon", 2048, 
                (void *)(intptr_t)(target_cm * 10), 5, NULL);
}

static void fill_monitor_task(void *arg) {
    float target_cm = (float)((int)(intptr_t)arg) / 10.0f;
    
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));  // Проверка каждые 2 сек
        
        float current_level = water_level_read_cm();
        
        // Достигли уровня?
        if (current_level >= target_cm) {
            valve_controller_close(VALVE_FILL);
            ESP_LOGI(TAG, "Tank filled to %.1f cm", current_level);
            send_status();
            vTaskDelete(NULL);
            return;
        }
        
        // Timeout 10 минут
        static int counter = 0;
        if (counter++ > 300) {  // 300 * 2 сек = 10 мин
            valve_controller_close(VALVE_FILL);
            ESP_LOGE(TAG, "Fill timeout!");
            send_error_event("Fill tank timeout");
            vTaskDelete(NULL);
            return;
        }
        
        // Отправка progress
        if (counter % 15 == 0) {  // Каждые 30 сек
            send_status();
        }
    }
}
```

---

### 2. Слив воды:

**Команда:**
```json
{
  "type": "command",
  "node_id": "water_001",
  "command": "drain_tank",
  "params": {
    "target_level_cm": 10  // Слить до 10 см (не полностью)
  }
}
```

---

## 🚨 SAFETY PROTECTION

### 1. Overflow protection:

```c
// ISR от float switch HIGH
void IRAM_ATTR float_high_isr(void *arg) {
    // Немедленная остановка насоса и fill valve!
    gpio_set_level(PUMP_GPIO, 0);
    gpio_set_level(FILL_VALVE_GPIO, 0);
    
    s_overflow_detected = true;
    
    ESP_LOGE(TAG, "OVERFLOW DETECTED!");
}

// В главной задаче:
if (s_overflow_detected) {
    s_overflow_detected = false;
    send_emergency_event("Tank overflow - pump auto-stopped");
}
```

### 2. Dry run protection:

```c
// ISR от float switch LOW
void IRAM_ATTR float_low_isr(void *arg) {
    // Немедленная остановка насоса!
    gpio_set_level(PUMP_GPIO, 0);
    
    s_dry_run_detected = true;
    
    ESP_LOGE(TAG, "DRY RUN DETECTED!");
}

// В главной задаче:
if (s_dry_run_detected) {
    s_dry_run_detected = false;
    send_emergency_event("Water level too low - pump auto-stopped");
}
```

### 3. Interlock protection:

```c
// НЕ ОТКРЫВАТЬ drain и fill одновременно!
if (valve_is_open(VALVE_DRAIN) && valve_is_open(VALVE_FILL)) {
    ESP_LOGE(TAG, "INTERLOCK VIOLATION!");
    valve_controller_close(VALVE_DRAIN);
    valve_controller_close(VALVE_FILL);
    send_emergency_event("Valve interlock violation");
}
```

---

## 📊 СТАТИСТИКА

### Отслеживание потребления воды:

```c
typedef struct {
    uint32_t pump_runtime_today_sec;    // Время работы за сутки
    uint32_t pump_cycles_today;          // Количество запусков
    float water_used_today_liters;       // Потребление за сутки
    uint32_t last_reset_timestamp;       // Когда сбросили счётчики
} water_stats_t;

// Расчет расхода (примерный, на основе времени работы):
// Производительность насоса: ~5 л/мин
void pump_controller_update_stats(uint32_t runtime_sec) {
    const float PUMP_FLOW_RATE_LPM = 5.0f;  // л/мин
    
    s_stats.pump_runtime_today_sec += runtime_sec;
    s_stats.pump_cycles_today++;
    
    float liters = (runtime_sec / 60.0f) * PUMP_FLOW_RATE_LPM;
    s_stats.water_used_today_liters += liters;
    
    ESP_LOGI(TAG, "Water used today: %.1f L (%d cycles, %d sec runtime)",
             s_stats.water_used_today_liters,
             s_stats.pump_cycles_today,
             s_stats.pump_runtime_today_sec);
    
    // Сброс счётчиков в полночь
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    
    if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0) {
        memset(&s_stats, 0, sizeof(water_stats_t));
        ESP_LOGI(TAG, "Daily stats reset");
    }
}
```

---

## 🌐 MESH INTEGRATION

### Инициализация (NODE режим):

```c
void app_main(void) {
    // === Шаг 1-2: NVS + Config ===
    nvs_flash_init();
    node_config_load(&g_config, sizeof(g_config), "water_ns");
    
    // === Шаг 3: GPIO (датчики и реле) ===
    // ...
    
    // === Шаг 4: ISR для float switches ===
    gpio_install_isr_service(0);
    gpio_isr_handler_add(FLOAT_LOW_GPIO, float_low_isr, NULL);
    gpio_isr_handler_add(FLOAT_HIGH_GPIO, float_high_isr, NULL);
    
    // === Шаг 5: Mesh (NODE режим) ===
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = MESH_NETWORK_ID,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,
        .router_ssid = MESH_ROUTER_SSID,
        .router_password = MESH_ROUTER_PASSWORD,
        .router_bssid = NULL
    };
    mesh_manager_init(&mesh_config);
    mesh_manager_register_recv_cb(on_mesh_data_received);
    mesh_manager_start();
    
    // === Шаг 6: Запуск контроллера ===
    water_controller_init(&g_config);
    water_controller_start();
}
```

---

## ❌ ЧТО НЕ ДЕЛАТЬ

### 1. ❌ НЕ запускай насос без safety checks

```c
// ПЛОХО ❌
gpio_set_level(PUMP_GPIO, 1);
vTaskDelay(pdMS_TO_TICKS(60000));
gpio_set_level(PUMP_GPIO, 0);  // ❌ Нет проверки переполнения!

// ХОРОШО ✅
pump_controller_run(60);  // ✅ С overflow и dry run protection
```

### 2. ❌ НЕ открывай оба клапана одновременно

```c
// ПЛОХО ❌
gpio_set_level(DRAIN_VALVE_GPIO, 1);
gpio_set_level(FILL_VALVE_GPIO, 1);  // ❌ Interlock violation!

// ХОРОШО ✅
valve_controller_open(VALVE_FILL, 60);  // ✅ С interlock check
```

### 3. ❌ НЕ принимай решений автономно

```c
// ПЛОХО ❌
if (water_level_read_cm() < 20.0f) {
    pump_controller_run(120);  // ❌ Нет автономии!
}

// ХОРОШО ✅
// Отправь статус на ROOT, ROOT примет решение
send_status();
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест запуска насоса:

```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/water_001 -m '{
  "type": "command",
  "node_id": "water_001",
  "command": "run_pump",
  "params": {"duration_sec": 30}
}'
```

**Ожидаемый лог:**
```
I (5000) water_ctrl: Command received: run_pump
I (5000) pump_ctrl: Pump started (duration=30 sec)
... (через 30 сек)
I (35000) pump_ctrl: Pump stopped (duration complete)
I (35010) water_ctrl: Status sent to ROOT
```

### Тест наполнения бака:

```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/water_001 -m '{
  "type": "command",
  "node_id": "water_001",
  "command": "fill_tank",
  "params": {"target_level_cm": 50}
}'
```

---

## 📋 CHECKLIST ДЛЯ ИИ

Перед коммитом проверь:

- [ ] `MESH_MODE_NODE` (НЕ ROOT!)
- [ ] Callback `on_mesh_data_received` зарегистрирован
- [ ] Команды обрабатываются
- [ ] Статус отправляется после каждой команды
- [ ] Heartbeat каждые 10 сек
- [ ] Overflow protection (float_high ISR)
- [ ] Dry run protection (float_low ISR)
- [ ] Timeout для насоса (макс 10 мин)
- [ ] Timeout для клапанов
- [ ] Interlock для клапанов (не открывать вместе)
- [ ] Статистика (литры за день)
- [ ] НЕТ автономной логики!
- [ ] НЕТ датчиков (кроме уровня воды)!

---

## 🎯 КРИТЕРИИ УСПЕХА

**NODE Water работает правильно если:**

1. ✅ Подключается к ROOT как NODE (layer 2)
2. ✅ Исполняет команды от ROOT/Server
3. ✅ Отправляет статус каждые 10 сек
4. ✅ Overflow protection работает (насос останавливается)
5. ✅ Dry run protection работает
6. ✅ Interlock защищает от одновременного открытия клапанов
7. ✅ Статистика потребления отправляется
8. ✅ НЕ работает автономно (только по командам)

---

**ГОТОВО! Water NODE - УПРАВЛЕНИЕ ВОДОСНАБЖЕНИЕМ ПО КОМАНДАМ!** 💧🚰

