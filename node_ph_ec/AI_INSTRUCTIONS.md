# 🤖 ПОЛНАЯ ИНСТРУКЦИЯ ДЛЯ ИИ: NODE pH/EC

**⚠️ КРИТИЧНЫЙ УЗЕЛ - Автономное управление pH и EC с PID контроллером**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE pH/EC (ESP32-S3 #3)** - самый важный узел системы, управляет качеством питательного раствора.

### 💧 Ключевые функции:

1. **Мониторинг pH/EC** - каждые 10 секунд чтение I2C датчиков
2. **PID управление** - автоматическая коррекция через 5 насосов
3. **Автономная работа** - продолжает работу при потере связи с ROOT
4. **Emergency protection** - аварийная остановка при критичных значениях
5. **Локальный буфер** - 1000 записей в ring buffer
6. **OLED дисплей** - локальная индикация pH, EC, статуса
7. **Статистика насосов** - мл за час/день/неделю

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ✅ **PID РАБОТАЕТ ВСЕГДА** - даже при offline ROOT!
2. ✅ **АВТОНОМНЫЙ РЕЖИМ** - через 30 сек после потери ROOT
3. ✅ **SAFETY FIRST** - emergency stop при критичных значениях
4. ✅ **БУФЕРИЗАЦИЯ** - сохраняй все данные локально
5. ❌ **НЕ ОСТАНАВЛИВАЙ PID** - насосы должны работать 24/7

---

## 📦 ДАТЧИКИ И ИСПОЛНИТЕЛИ

### Датчики (I2C):
| Датчик | Адрес | Назначение | Диапазон |
|--------|-------|------------|----------|
| **Trema pH** | 0x4D | pH раствора | 0.0-14.0 |
| **Trema EC** | 0x64 | Электропроводность | 0-5.0 mS/cm |
| **DS18B20** | 1-Wire | Температура (опц.) | -55...+125°C |

### Насосы (PWM):
| Насос | GPIO | Назначение | Дозировка |
|-------|------|------------|-----------|
| **pH UP** | 4 | Повышение pH (KOH) | 0-100% PWM |
| **pH DOWN** | 5 | Понижение pH (H3PO4) | 0-100% PWM |
| **EC A** | 6 | Удобрение A | 0-100% PWM |
| **EC B** | 7 | Удобрение B | 0-100% PWM |
| **EC C** | 15 | Микроэлементы | 0-100% PWM |

### Индикация:
| Компонент | GPIO | Назначение |
|-----------|------|------------|
| **OLED SSD1306** | I2C (17,18) | Локальный дисплей |
| **LED Status** | 21 | Зеленый=OK, Желтый=Degraded, Красный=Emergency |
| **Buzzer** | 22 | Аварийные сигналы |
| **Button MODE** | 23 | Смена режимов |

---

## 🔌 РАСПИНОВКА ESP32-S3 #3

| GPIO | Назначение | Примечание |
|------|------------|------------|
| **I2C шина (датчики + OLED):** | | |
| 17 | I2C SCL | Clock (100 kHz) |
| 18 | I2C SDA | Data |
| **PWM насосы (перистальтические):** | | |
| 4 | Pump pH UP | PWM 1000 Hz, max 60 сек |
| 5 | Pump pH DOWN | PWM 1000 Hz, max 60 сек |
| 6 | Pump EC A | PWM 1000 Hz, max 60 сек |
| 7 | Pump EC B | PWM 1000 Hz, max 60 сек |
| 15 | Pump EC C | PWM 1000 Hz, max 60 сек |
| **Индикация и управление:** | | |
| 21 | LED RGB | WS2812 или обычный LED |
| 22 | Buzzer | 5V активный |
| 23 | Button MODE | Подтяжка к GND |

---

## 🏗️ АРХИТЕКТУРА ПО КОМПОНЕНТАМ

```
node_ph_ec/
├── components/
│   ├── ph_sensor/           # Драйвер Trema pH (I2C 0x4D)
│   ├── ec_sensor/           # Драйвер Trema EC (I2C 0x64)
│   ├── temp_sensor/         # DS18B20 (1-Wire)
│   ├── pump_controller/     # Управление 5 насосами (PWM + safety)
│   ├── adaptive_pid/        # AI PID контроллер (адаптивные Kp, Ki, Kd)
│   ├── ph_ec_manager/       # Главный менеджер (координация всех компонентов)
│   ├── connection_monitor/  # Мониторинг связи с ROOT
│   ├── local_storage/       # Ring buffer 1000 записей
│   ├── oled_display/        # SSD1306 128x64 (локальная индикация)
│   ├── buzzer_led/          # Аварийные сигналы
│   └── node_controller/     # Главная логика и режимы
│
├── main/
│   └── app_main.c           # Точка входа + mesh callback
│
└── AI_INSTRUCTIONS.md       # ← ЭТА ИНСТРУКЦИЯ
```

---

## 📨 ТИПЫ СООБЩЕНИЙ

### 1. 📤 TELEMETRY (каждые 10 сек)

**NODE → ROOT → MQTT → Server**

```json
{
  "type": "telemetry",
  "node_id": "ph_ec_001",
  "timestamp": 1729346400,
  "data": {
    "ph": 6.5,
    "ec": 1.8,
    "temperature": 22.5,
    "rssi_to_parent": -45,
    "pump_stats": {
      "ph_up_ml_today": 120,
      "ph_down_ml_today": 85,
      "ec_a_ml_today": 250,
      "ec_b_ml_today": 250,
      "ec_c_ml_today": 50
    },
    "mode": "online"  // online/degraded/autonomous/emergency
  }
}
```

**Код:**
```c
static void send_telemetry(float ph, float ec, float temp) {
    cJSON *data = cJSON_CreateObject();
    cJSON_AddNumberToObject(data, "ph", ph);
    cJSON_AddNumberToObject(data, "ec", ec);
    cJSON_AddNumberToObject(data, "temperature", temp);
    cJSON_AddNumberToObject(data, "rssi_to_parent", mesh_manager_get_parent_rssi());
    
    // Статистика насосов
    cJSON *pump_stats = cJSON_CreateObject();
    cJSON_AddNumberToObject(pump_stats, "ph_up_ml_today", pump_controller_get_daily_ml(PUMP_PH_UP));
    cJSON_AddNumberToObject(pump_stats, "ph_down_ml_today", pump_controller_get_daily_ml(PUMP_PH_DOWN));
    // ...
    cJSON_AddItemToObject(data, "pump_stats", pump_stats);
    
    cJSON_AddStringToObject(data, "mode", connection_monitor_get_state_str());
    
    char json_buf[1024];
    mesh_protocol_create_telemetry(s_config->base.node_id, data, json_buf, sizeof(json_buf));
    mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
    
    cJSON_Delete(data);
}
```

---

### 2. 📥 COMMAND (Server → MQTT → ROOT → NODE)

**Топик:** `hydro/command/ph_ec_001`

**Примеры команд:**

#### a) Изменение target значений:
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "set_targets",
  "params": {
    "ph_target": 6.5,
    "ec_target": 1.8
  }
}
```

#### b) Ручное управление насосом (emergency):
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "manual_pump",
  "params": {
    "pump": "ph_up",
    "duration_sec": 5,
    "power_percent": 100
  }
}
```

#### c) Калибровка датчиков:
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "calibrate_ph",
  "params": {
    "ph_value": 7.0
  }
}
```

#### d) Переключение режима:
```json
{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "set_mode",
  "params": {
    "mode": "autonomous"  // online/autonomous/manual
  }
}
```

**Обработка:**
```c
void node_controller_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);
    
    if (strcmp(command, "set_targets") == 0) {
        cJSON *ph = cJSON_GetObjectItem(params, "ph_target");
        cJSON *ec = cJSON_GetObjectItem(params, "ec_target");
        
        if (ph && cJSON_IsNumber(ph)) {
            s_config->ph_target = (float)ph->valuedouble;
        }
        if (ec && cJSON_IsNumber(ec)) {
            s_config->ec_target = (float)ec->valuedouble;
        }
        
        // Сохранение в NVS
        node_config_save(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
        ESP_LOGI(TAG, "Targets updated: pH=%.2f, EC=%.2f", s_config->ph_target, s_config->ec_target);
    }
    else if (strcmp(command, "manual_pump") == 0) {
        // Ручное управление насосом (только в manual режиме!)
        if (s_autonomous_mode) {
            ESP_LOGW(TAG, "Manual pump rejected - in autonomous mode");
            return;
        }
        // ...
    }
    else if (strcmp(command, "calibrate_ph") == 0) {
        // Калибровка pH датчика
        // ...
    }
}
```

---

### 3. 🔔 EVENT (критичные события)

**NODE → ROOT → MQTT → Server**

```json
{
  "type": "event",
  "node_id": "ph_ec_001",
  "timestamp": 1729346400,
  "level": "critical",  // info/warning/critical/emergency
  "message": "pH out of safe range",
  "data": {
    "current_ph": 5.2,
    "target_ph": 6.5,
    "action": "pump_ph_up_activated"
  }
}
```

**Когда отправлять:**
- pH/EC вне безопасного диапазона
- Датчик не отвечает
- Насос работает слишком долго
- Emergency stop активирован
- Переход в autonomous режим

---

### 4. 💓 HEARTBEAT (каждые 10 сек)

```json
{
  "type": "heartbeat",
  "node_id": "ph_ec_001",
  "uptime": 3600,
  "heap_free": 180000,
  "rssi_to_parent": -45
}
```

---

## 🧠 PID КОНТРОЛЛЕР (АДАПТИВНЫЙ)

### Логика работы:

```c
static void run_pid_control(float ph, float ec) {
    // 1. pH контроль
    float ph_error = s_config->ph_target - ph;
    
    if (fabs(ph_error) > PH_DEADBAND) {  // Deadband = 0.05
        if (ph_error > 0) {
            // pH слишком низкий → нужен pH UP
            float duty_cycle = adaptive_pid_calculate(&pid_ph_up, ph_error);
            uint32_t duration_ms = (uint32_t)(duty_cycle * 1000.0f);
            pump_controller_run(PUMP_PH_UP, duration_ms, 100);
        } else {
            // pH слишком высокий → нужен pH DOWN
            float duty_cycle = adaptive_pid_calculate(&pid_ph_down, -ph_error);
            uint32_t duration_ms = (uint32_t)(duty_cycle * 1000.0f);
            pump_controller_run(PUMP_PH_DOWN, duration_ms, 100);
        }
    }
    
    // 2. EC контроль (аналогично)
    float ec_error = s_config->ec_target - ec;
    
    if (fabs(ec_error) > EC_DEADBAND) {  // Deadband = 0.05
        if (ec_error > 0) {
            // EC слишком низкая → добавить удобрения
            float duty_cycle = adaptive_pid_calculate(&pid_ec, ec_error);
            uint32_t duration_ms = (uint32_t)(duty_cycle * 1000.0f);
            
            // Пропорциональное распределение A:B:C = 10:10:1
            pump_controller_run(PUMP_EC_A, duration_ms, 100);
            pump_controller_run(PUMP_EC_B, duration_ms, 100);
            pump_controller_run(PUMP_EC_C, duration_ms / 10, 100);
        }
    }
}
```

### PID параметры (начальные):

```c
typedef struct {
    float kp;           // Пропорциональный коэффициент
    float ki;           // Интегральный
    float kd;           // Дифференциальный
    float integral;     // Накопленная ошибка
    float prev_error;   // Предыдущая ошибка
    float output_min;   // Минимальный выход (0.0)
    float output_max;   // Максимальный выход (1.0)
} adaptive_pid_t;

// Начальные значения (адаптируются автоматически):
adaptive_pid_t pid_ph_up = {
    .kp = 0.5,
    .ki = 0.05,
    .kd = 0.1,
    .output_min = 0.0,
    .output_max = 1.0
};
```

---

## 🔄 РЕЖИМЫ РАБОТЫ

### 1. 🟢 ONLINE (норма)
- ROOT доступен
- Отправка телеметрии каждые 10 сек
- PID работает
- Heartbeat каждые 10 сек
- Команды от сервера обрабатываются

### 2. 🟡 DEGRADED (нестабильная связь)
- ROOT пропадает периодически
- Буферизация данных в ring buffer
- PID работает
- Попытки переотправки буфера

### 3. 🟠 AUTONOMOUS (автономия)
- ROOT offline > 30 сек
- **PID работает с настройками из NVS**
- Все данные в ring buffer
- OLED показывает "⚡ AUTONOMOUS"
- Buzzer 1 короткий сигнал при переходе

### 4. 🔴 EMERGENCY (авария)
- pH/EC вне критичных пределов
- **ВСЕ НАСОСЫ ОСТАНАВЛИВАЮТСЯ**
- Buzzer непрерывный сигнал
- LED красный мигает
- OLED показывает "🔴 EMERGENCY"
- Отправка event (если ROOT доступен)

---

## 📨 ОБРАБОТКА КОМАНД

### Callback в app_main.c:

```c
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    // Создаём NULL-terminated копию
    char *data_copy = malloc(len + 1);
    if (!data_copy) return;
    memcpy(data_copy, data, len);
    data_copy[len] = '\0';
    
    mesh_message_t msg;
    if (!mesh_protocol_parse(data_copy, &msg)) {
        free(data_copy);
        return;
    }
    
    // Проверка что для нас
    if (strcmp(msg.node_id, g_config.base.node_id) != 0) {
        free(data_copy);
        mesh_protocol_free_message(&msg);
        return;
    }

    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            if (cmd && cJSON_IsString(cmd)) {
                node_controller_handle_command(cmd->valuestring, msg.data);
            }
            break;
        }

        case MESH_MSG_CONFIG:
            node_controller_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    free(data_copy);
    mesh_protocol_free_message(&msg);
}
```

---

## 🔐 SAFETY CHECKS

### Проверки ПЕРЕД включением насоса:

```c
esp_err_t pump_controller_run(pump_type_t pump, uint32_t duration_ms, uint8_t power_percent) {
    // 1. Проверка таймаута с последнего запуска
    if (time_since_last_run < PUMP_COOLDOWN_MS) {
        ESP_LOGW(TAG, "Pump cooldown active");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 2. Проверка максимальной длительности
    if (duration_ms > PUMP_MAX_DURATION_MS) {
        ESP_LOGW(TAG, "Duration too long: %d ms (max %d)", duration_ms, PUMP_MAX_DURATION_MS);
        duration_ms = PUMP_MAX_DURATION_MS;
    }
    
    // 3. Проверка лимита в сутки
    float ml_today = get_pump_ml_today(pump);
    if (ml_today > PUMP_MAX_ML_PER_DAY) {
        ESP_LOGE(TAG, "Daily limit reached: %.1f ml", ml_today);
        node_controller_send_event(MESH_EVENT_CRITICAL, "Pump daily limit reached");
        return ESP_ERR_INVALID_STATE;
    }
    
    // 4. Запуск насоса
    return pump_start_pwm(pump, duration_ms, power_percent);
}
```

---

## 🌐 MESH INTEGRATION

### Инициализация (NODE режим):

```c
void app_main(void) {
    // ...
    
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,  // ← ВСЕГДА NODE!
        .mesh_id = MESH_NETWORK_ID,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = 6,
        .router_ssid = MESH_ROUTER_SSID,
        .router_password = MESH_ROUTER_PASSWORD,
        .router_bssid = NULL
    };
    
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);  // ← Callback для команд!
    ESP_ERROR_CHECK(mesh_manager_start());
    
    // ...
}
```

### Проверка статуса mesh:

```c
// В главной задаче (каждые 10 сек):
bool mesh_connected = mesh_manager_is_connected();
int8_t rssi = mesh_manager_get_parent_rssi();
int layer = mesh_manager_get_layer();

ESP_LOGI(TAG, "Mesh: %s, Layer: %d, RSSI: %d dBm",
         mesh_connected ? "CONNECTED" : "OFFLINE", layer, rssi);
```

---

## 🔄 АВТОНОМНЫЙ РЕЖИМ (КЛЮЧЕВАЯ ФИШКА!)

### Переход в автономию:

```c
static void on_connection_state_changed(connection_state_t new_state, connection_state_t old_state) {
    if (new_state == CONN_STATE_AUTONOMOUS && old_state != CONN_STATE_AUTONOMOUS) {
        ESP_LOGW(TAG, "⚡ Entering AUTONOMOUS mode (ROOT offline > 30s)");
        
        // 1. Звуковой сигнал
        buzzer_beep(1, 200);
        
        // 2. Загрузка настроек из NVS (на случай изменений)
        node_config_load(s_config, sizeof(ph_ec_node_config_t), "ph_ec_ns");
        
        // 3. Переход в автономный режим
        s_autonomous_mode = true;
        
        // 4. Обновление OLED
        oled_display_show_autonomous();
    }
    
    if (new_state == CONN_STATE_ONLINE && old_state == CONN_STATE_AUTONOMOUS) {
        ESP_LOGI(TAG, "✅ Returning to ONLINE mode (ROOT reconnected)");
        
        s_autonomous_mode = false;
        
        // Отправка накопленного буфера
        local_storage_sync_to_root();
        
        // 2 коротких сигнала
        buzzer_beep(2, 100);
    }
}
```

### ⚠️ ВАЖНО: PID работает В ОБОИХ РЕЖИМАХ!

```c
// ГЛАВНАЯ ЗАДАЧА (каждые 10 сек):
static void node_controller_main_task(void *arg) {
    while (1) {
        float ph = read_ph_sensor();
        float ec = read_ec_sensor();
        float temp = read_temp_sensor();
        
        // PID управление ВСЕГДА активно!
        run_pid_control(ph, ec);  // ← Работает и в ONLINE, и в AUTONOMOUS!
        
        // Отправка данных зависит от режима
        if (connection_monitor_get_state() == CONN_STATE_ONLINE) {
            send_telemetry(ph, ec, temp);
        } else {
            local_storage_add(ph, ec, temp);  // ← Буферизация
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
```

---

## 🚨 EMERGENCY PROTECTION

### Критичные пределы:

```c
#define PH_EMERGENCY_LOW      4.0   // pH < 4.0 = авария!
#define PH_EMERGENCY_HIGH     9.0   // pH > 9.0 = авария!
#define EC_EMERGENCY_HIGH     5.0   // EC > 5.0 = авария!
#define PUMP_MAX_DURATION_SEC 60    // Макс время работы насоса
#define PUMP_MAX_ML_PER_DAY   5000  // Макс мл за сутки
```

### Emergency обработчик:

```c
void node_controller_handle_emergency(const char *reason, float value) {
    ESP_LOGE(TAG, "🚨 EMERGENCY: %s (value=%.2f)", reason, value);
    
    // 1. ОСТАНОВКА ВСЕХ НАСОСОВ!
    pump_controller_emergency_stop();
    
    // 2. Buzzer сигнал (непрерывный)
    buzzer_continuous(true);
    
    // 3. LED красный мигающий
    led_set_mode(LED_MODE_EMERGENCY);
    
    // 4. OLED большими буквами
    oled_display_show_emergency(reason);
    
    // 5. Отправка event на сервер (если ROOT доступен)
    if (mesh_manager_is_connected()) {
        cJSON *event_data = cJSON_CreateObject();
        cJSON_AddStringToObject(event_data, "reason", reason);
        cJSON_AddNumberToObject(event_data, "value", value);
        
        char event_buf[512];
        mesh_protocol_create_event(s_config->base.node_id, MESH_EVENT_EMERGENCY,
                                   reason, event_data, event_buf, sizeof(event_buf));
        mesh_manager_send_to_root((uint8_t *)event_buf, strlen(event_buf));
        
        cJSON_Delete(event_data);
    }
    
    // 6. Переход в EMERGENCY режим (требует ручного сброса!)
    s_emergency_mode = true;
}
```

---

## 💾 ЛОКАЛЬНОЕ ХРАНИЛИЩЕ

### Ring Buffer (1000 записей):

```c
typedef struct {
    uint64_t timestamp;
    float ph;
    float ec;
    float temperature;
    bool synced;  // Отправлено на ROOT?
} ph_ec_record_t;

// API:
void local_storage_add(float ph, float ec, float temp);
int local_storage_get_unsynced_count(void);
bool local_storage_get_next_unsynced(ph_ec_record_t *record);
void local_storage_mark_synced(uint64_t timestamp);
void local_storage_sync_to_root(void);  // Отправка всех несинхронизированных
```

### Синхронизация при восстановлении связи:

```c
void local_storage_sync_to_root(void) {
    int unsynced = local_storage_get_unsynced_count();
    
    if (unsynced == 0) return;
    
    ESP_LOGI(TAG, "Syncing %d records to ROOT...", unsynced);
    
    // Отправка батчами по 10 записей
    for (int i = 0; i < unsynced && i < 100; i++) {  // Макс 100 за раз
        ph_ec_record_t record;
        if (local_storage_get_next_unsynced(&record)) {
            send_buffered_telemetry(&record);
            local_storage_mark_synced(record.timestamp);
            
            vTaskDelay(pdMS_TO_TICKS(100));  // 100ms между отправками
        }
    }
    
    ESP_LOGI(TAG, "Sync complete");
}
```

---

## 📺 OLED ДИСПЛЕЙ (SSD1306 128x64)

### Главный экран:

```
┌──────────────────────┐
│  pH: 6.5  EC: 1.8    │  ← Текущие значения
│  T: 22.5°C           │
│                      │
│  ● ONLINE            │  ← Статус: ●=online, ⚡=autonomous, 🔴=emergency
│  RSSI: -45 dBm       │
│                      │
│  Pumps (ml/h):       │
│  ↑120 ↓85  A250 B250 │  ← Статистика насосов за час
└──────────────────────┘
```

### Экран аварии:

```
┌──────────────────────┐
│    🔴 EMERGENCY!     │
│                      │
│  pH TOO LOW: 4.2     │
│  Target: 6.5         │
│                      │
│  ALL PUMPS STOPPED   │
│  MANUAL RESET REQ!   │
└──────────────────────┘
```

---

## ❌ ЧТО НЕ ДЕЛАТЬ

### 1. ❌ НЕ останавливай PID при offline ROOT

```c
// ПЛОХО ❌
if (!mesh_manager_is_connected()) {
    return;  // ❌ PID остановлен!
}
run_pid_control(ph, ec);

// ХОРОШО ✅
run_pid_control(ph, ec);  // ✅ Работает всегда!

if (mesh_manager_is_connected()) {
    send_telemetry(...);
} else {
    local_storage_add(...);  // Буферизация
}
```

### 2. ❌ НЕ блокируй главный цикл ожиданием mesh

```c
// ПЛОХО ❌
esp_err_t err = mesh_manager_send_to_root(...);
while (err != ESP_OK) {  // ❌ Зависание!
    vTaskDelay(1000);
    err = mesh_manager_send_to_root(...);
}

// ХОРОШО ✅
esp_err_t err = mesh_manager_send_to_root(...);
if (err != ESP_OK) {
    local_storage_add(...);  // ✅ Буферизация
}
```

### 3. ❌ НЕ запускай насос без safety checks

```c
// ПЛОХО ❌
gpio_set_level(PUMP_PH_UP_GPIO, 1);
vTaskDelay(pdMS_TO_TICKS(5000));
gpio_set_level(PUMP_PH_UP_GPIO, 0);  // ❌ Нет контроля!

// ХОРОШО ✅
esp_err_t err = pump_controller_run(PUMP_PH_UP, 5000, 100);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Pump safety check failed: %s", esp_err_to_name(err));
}
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест автономии (симуляция потери ROOT):

1. Запусти NODE pH/EC
2. Выключи ROOT
3. **Проверь через 30 сек:**
   - ✅ OLED показывает "⚡ AUTONOMOUS"
   - ✅ PID продолжает работать
   - ✅ Buzzer 1 короткий сигнал
   - ✅ Данные сохраняются в buffer
4. Включи ROOT обратно
5. **Проверь:**
   - ✅ OLED "● ONLINE"
   - ✅ Buffer синхронизируется
   - ✅ Buzzer 2 коротких сигнала

### Тест команды от сервера:

```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/ph_ec_001 -m '{
  "type": "command",
  "node_id": "ph_ec_001",
  "command": "set_targets",
  "params": {"ph_target": 6.2, "ec_target": 2.0}
}'
```

**Ожидаемый лог:**
```
I (12000) ph_ec: Message from ROOT: type=1
I (12000) node_controller: Command received: set_targets
I (12000) node_controller: Targets updated: pH=6.20, EC=2.00
```

---

## 📋 CHECKLIST ДЛЯ ИИ

Перед коммитом проверь:

- [ ] `MESH_MODE_NODE` (НЕ ROOT!)
- [ ] PID работает ВСЕГДА (не зависит от mesh)
- [ ] Автономный режим через 30 сек offline
- [ ] Ring buffer 1000 записей
- [ ] Safety checks для всех насосов
- [ ] Emergency stop при критичных значениях
- [ ] OLED показывает статус
- [ ] Buzzer сигналы при событиях
- [ ] Калибровка сохраняется в NVS
- [ ] Статистика насосов (мл за день)
- [ ] Команды обрабатываются
- [ ] Telemetry включает pump_stats

---

## 🎯 ПРИОРИТЕТЫ РАЗРАБОТКИ

### Сейчас реализовано:
✅ Базовая структура компонентов
✅ Конфигурация в NVS
✅ Mesh integration (NODE режим)

### TODO (по приоритету):

1. **HIGH:** Реализация sensor_manager (pH/EC/Temp чтение)
2. **HIGH:** Реализация pump_controller (PWM + safety)
3. **HIGH:** Реализация adaptive_pid (базовый PID)
4. **MEDIUM:** Реализация connection_monitor (мониторинг ROOT)
5. **MEDIUM:** Реализация local_storage (ring buffer)
6. **MEDIUM:** Реализация node_controller (главная логика)
7. **LOW:** Реализация oled_display (SSD1306)
8. **LOW:** Реализация buzzer_led (индикация)

---

**ГОТОВО! pH/EC NODE - АВТОНОМНОЕ УПРАВЛЕНИЕ С PID!** 💧🤖

