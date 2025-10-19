# 🤖 ПОЛНАЯ ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Relay

**Узел управления климатом (актуаторы, вентиляция, освещение)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Relay (ESP32)** - исполнительный узел для управления климатом в теплице.

### ⚙️ Ключевые функции:

1. **Управление форточками** - 2 линейных актуатора (открыть/закрыть)
2. **Управление вентиляцией** - вентилятор ON/OFF или PWM скорость
3. **Управление освещением** - LED свет с PWM диммированием (0-100%)
4. **Обработка команд** - от ROOT/Server (НЕ автономно!)
5. **Отправка статуса** - состояние всех реле

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ❌ **НЕТ ДАТЧИКОВ!** - Relay НЕ читает температуру/влажность/CO2
2. ❌ **НЕТ ЛОГИКИ!** - Relay НЕ принимает решений (кроме safety)
3. ❌ **НЕТ АВТОНОМИИ!** - Relay работает ТОЛЬКО по командам от ROOT
4. ✅ **ТОЛЬКО ИСПОЛНЕНИЕ** - получил команду → выполнил → отправил статус
5. ✅ **SAFETY CHECKS** - ограничения по времени работы актуаторов
6. ✅ **СТАТУС В РЕАЛЬНОМ ВРЕМЕНИ** - отправка после каждого изменения

---

## 📦 ИСПОЛНИТЕЛИ

### Линейные актуаторы форточек:
| Актуатор | GPIO | Управление | Время хода | Назначение |
|----------|------|------------|------------|------------|
| **Window 1** | 1 | Relay (открыть/закрыть) | ~30 сек | Верхняя форточка |
| **Window 2** | 2 | Relay (открыть/закрыть) | ~30 сек | Боковая форточка |

**⚠️ ВАЖНО:** 
- Актуаторы имеют концевики (limit switches)
- Макс время работы: 45 сек (защита от зависания)
- Cooldown между операциями: 5 сек

### Вентилятор:
| Вентилятор | GPIO | Управление | Мощность |
|------------|------|------------|----------|
| **Fan** | 3 | ON/OFF или PWM | 0-100% |

### LED освещение:
| Освещение | GPIO | Управление | Диапазон |
|-----------|------|------------|----------|
| **LED Grow Light** | 7 | PWM 1000 Hz | 0-100% |

---

## 🔌 РАСПИНОВКА ESP32

| GPIO | Назначение | Тип | Примечание |
|------|------------|-----|------------|
| **Актуаторы форточек:** | | | |
| 1 | Window 1 Open | Relay | HIGH=открыть |
| 2 | Window 2 Open | Relay | HIGH=открыть |
| **Вентиляция и свет:** | | | |
| 3 | Fan | Relay/PWM | ON/OFF или PWM скорость |
| 7 | LED Light | PWM | 0-100% яркость |
| **Опциональная индикация:** | | | |
| 15 | Status LED | GPIO | Зеленый=OK, Красный=Error |

---

## 🏗️ АРХИТЕКТУРА

```
NODE Relay (ESP32)
│
├── Actuator Manager
│   ├── Window 1 controller (linear actuator)
│   ├── Window 2 controller (linear actuator)
│   ├── Limit switch monitoring
│   ├── Timeout protection (45 sec max)
│   └── Cooldown logic (5 sec)
│
├── Fan Controller
│   ├── ON/OFF control
│   ├── PWM speed control (optional)
│   └── State tracking
│
├── LED Controller
│   ├── PWM dimming (0-100%)
│   ├── Smooth transitions
│   └── Daily schedule (optional)
│
├── Command Handler
│   ├── Парсинг команд от ROOT
│   ├── Validation parameters
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
    ├── Overcurrent protection
    ├── Timeout checks
    └── Emergency stop
```

---

## 📨 ТИПЫ СООБЩЕНИЙ

### 1. 📥 COMMAND (Server → MQTT → ROOT → NODE)

**Топик:** `hydro/command/relay_001`

#### a) Управление форточкой:
```json
{
  "type": "command",
  "node_id": "relay_001",
  "command": "set_window",
  "params": {
    "window": 1,        // 1 или 2
    "action": "open"    // "open" или "close"
  }
}
```

#### b) Управление вентилятором:
```json
{
  "type": "command",
  "node_id": "relay_001",
  "command": "set_fan",
  "params": {
    "state": "on",      // "on" или "off"
    "speed": 100        // 0-100% (если PWM)
  }
}
```

#### c) Управление LED освещением:
```json
{
  "type": "command",
  "node_id": "relay_001",
  "command": "set_light",
  "params": {
    "brightness": 75    // 0-100%
  }
}
```

#### d) Emergency stop:
```json
{
  "type": "command",
  "node_id": "relay_001",
  "command": "emergency_stop",
  "params": {}
}
```

**Обработка команд:**
```c
void relay_controller_handle_command(const char *command, cJSON *params) {
    ESP_LOGI(TAG, "Command received: %s", command);
    
    if (strcmp(command, "set_window") == 0) {
        cJSON *window_num = cJSON_GetObjectItem(params, "window");
        cJSON *action = cJSON_GetObjectItem(params, "action");
        
        if (window_num && action && cJSON_IsString(action)) {
            int window = window_num->valueint;
            const char *action_str = action->valuestring;
            
            if (strcmp(action_str, "open") == 0) {
                actuator_manager_open_window(window);
            } else if (strcmp(action_str, "close") == 0) {
                actuator_manager_close_window(window);
            }
            
            // Отправка статуса после выполнения
            send_status();
        }
    }
    else if (strcmp(command, "set_fan") == 0) {
        cJSON *state = cJSON_GetObjectItem(params, "state");
        cJSON *speed = cJSON_GetObjectItem(params, "speed");
        
        if (state && cJSON_IsString(state)) {
            if (strcmp(state->valuestring, "on") == 0) {
                int fan_speed = speed ? speed->valueint : 100;
                fan_controller_set_speed(fan_speed);
            } else {
                fan_controller_turn_off();
            }
            
            send_status();
        }
    }
    else if (strcmp(command, "set_light") == 0) {
        cJSON *brightness = cJSON_GetObjectItem(params, "brightness");
        
        if (brightness && cJSON_IsNumber(brightness)) {
            int bright = brightness->valueint;
            led_controller_set_brightness(bright);
            
            send_status();
        }
    }
    else if (strcmp(command, "emergency_stop") == 0) {
        // Остановка ВСЕХ актуаторов
        actuator_manager_emergency_stop();
        fan_controller_turn_off();
        led_controller_set_brightness(0);
        
        // Отправка emergency event
        send_emergency_event("Emergency stop activated");
    }
}
```

---

### 2. 📤 STATUS (после каждой команды)

**NODE → ROOT → MQTT → Server**

```json
{
  "type": "telemetry",
  "node_id": "relay_001",
  "timestamp": 1729346400,
  "data": {
    "window_1": "open",      // "open"/"close"/"moving"
    "window_2": "closed",
    "fan": "on",
    "fan_speed": 100,
    "led_brightness": 75,
    "rssi_to_parent": -50
  }
}
```

**Код:**
```c
static void send_status(void) {
    cJSON *data = cJSON_CreateObject();
    
    // Состояние форточек
    cJSON_AddStringToObject(data, "window_1", actuator_manager_get_state(1));
    cJSON_AddStringToObject(data, "window_2", actuator_manager_get_state(2));
    
    // Вентилятор
    cJSON_AddStringToObject(data, "fan", fan_controller_is_on() ? "on" : "off");
    cJSON_AddNumberToObject(data, "fan_speed", fan_controller_get_speed());
    
    // LED свет
    cJSON_AddNumberToObject(data, "led_brightness", led_controller_get_brightness());
    
    // RSSI
    cJSON_AddNumberToObject(data, "rssi_to_parent", mesh_manager_get_parent_rssi());
    
    char json_buf[512];
    mesh_protocol_create_telemetry(s_config->base.node_id, data, json_buf, sizeof(json_buf));
    mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
    
    cJSON_Delete(data);
}
```

---

### 3. 💓 HEARTBEAT (каждые 10 сек)

```json
{
  "type": "heartbeat",
  "node_id": "relay_001",
  "uptime": 3600,
  "heap_free": 190000,
  "rssi_to_parent": -50
}
```

---

### 4. 🔔 EVENT (аварии)

**Когда отправлять:**
- Актуатор работает > 45 сек (timeout)
- Концевик не сработал
- Overcurrent detected
- Emergency stop активирован

```json
{
  "type": "event",
  "node_id": "relay_001",
  "timestamp": 1729346400,
  "level": "critical",
  "message": "Window actuator timeout",
  "data": {
    "window": 1,
    "action": "open",
    "duration_sec": 50
  }
}
```

---

## 💻 ГЛАВНАЯ ЛОГИКА

### app_main.c (с mesh callback):

```c
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "relay_controller.h"

static relay_node_config_t g_config;

// Callback для команд от ROOT
static void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    char *data_copy = malloc(len + 1);
    if (!data_copy) return;
    memcpy(data_copy, data, len);
    data_copy[len] = '\0';
    
    mesh_message_t msg;
    if (!mesh_protocol_parse(data_copy, &msg)) {
        free(data_copy);
        return;
    }
    
    if (strcmp(msg.node_id, g_config.base.node_id) != 0) {
        free(data_copy);
        mesh_protocol_free_message(&msg);
        return;
    }

    switch (msg.type) {
        case MESH_MSG_COMMAND: {
            cJSON *cmd = cJSON_GetObjectItem(msg.data, "command");
            if (cmd && cJSON_IsString(cmd)) {
                relay_controller_handle_command(cmd->valuestring, msg.data);
            }
            break;
        }

        case MESH_MSG_CONFIG:
            relay_controller_handle_config_update(msg.data);
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    free(data_copy);
    mesh_protocol_free_message(&msg);
}

void app_main(void) {
    // === Шаг 1: NVS ===
    nvs_flash_init();
    
    // === Шаг 2: Загрузка конфигурации ===
    node_config_load(&g_config, sizeof(g_config), "relay_ns");
    
    // === Шаг 3: Инициализация GPIO ===
    // Актуаторы (Relay)
    gpio_config_t actuator_conf = {
        .pin_bit_mask = (1ULL << 1) | (1ULL << 2),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&actuator_conf);
    
    // Вентилятор (Relay или PWM)
    gpio_config_t fan_conf = {
        .pin_bit_mask = (1ULL << 3),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&fan_conf);
    
    // LED свет (PWM)
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .freq_hz = 1000,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = 7,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
    
    // === Шаг 4: Mesh (NODE режим) ===
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
    mesh_manager_register_recv_cb(on_mesh_data_received);  // ← Callback!
    mesh_manager_start();
    
    // === Шаг 5: Инициализация контроллеров ===
    relay_controller_init(&g_config);
    relay_controller_start();
}
```

---

## ⚙️ УПРАВЛЕНИЕ АКТУАТОРАМИ

### Логика актуаторов форточек:

```c
typedef enum {
    WINDOW_CLOSED = 0,
    WINDOW_OPENING,
    WINDOW_OPEN,
    WINDOW_CLOSING,
    WINDOW_ERROR
} window_state_t;

esp_err_t actuator_open_window(int window_num) {
    if (window_num < 1 || window_num > 2) {
        return ESP_ERR_INVALID_ARG;
    }
    
    int gpio = (window_num == 1) ? 1 : 2;
    window_state_t *state = &s_window_states[window_num - 1];
    
    // Проверка текущего состояния
    if (*state == WINDOW_OPEN) {
        ESP_LOGI(TAG, "Window %d already open", window_num);
        return ESP_OK;
    }
    
    if (*state == WINDOW_OPENING || *state == WINDOW_CLOSING) {
        ESP_LOGW(TAG, "Window %d is moving", window_num);
        return ESP_ERR_INVALID_STATE;
    }
    
    // Запуск актуатора
    *state = WINDOW_OPENING;
    gpio_set_level(gpio, 1);
    
    // Создание задачи мониторинга timeout (45 сек)
    xTaskCreate(window_timeout_task, "win_timeout", 2048, (void *)(intptr_t)window_num, 5, NULL);
    
    ESP_LOGI(TAG, "Window %d opening...", window_num);
    return ESP_OK;
}

// Timeout task:
static void window_timeout_task(void *arg) {
    int window_num = (int)(intptr_t)arg;
    
    // Ждем 45 сек
    vTaskDelay(pdMS_TO_TICKS(45000));
    
    // Если всё ещё открывается/закрывается - timeout!
    window_state_t state = s_window_states[window_num - 1];
    if (state == WINDOW_OPENING || state == WINDOW_CLOSING) {
        ESP_LOGE(TAG, "Window %d TIMEOUT!", window_num);
        
        // Остановка
        int gpio = (window_num == 1) ? 1 : 2;
        gpio_set_level(gpio, 0);
        s_window_states[window_num - 1] = WINDOW_ERROR;
        
        // Отправка emergency event
        send_emergency_event("Window actuator timeout");
    }
    
    vTaskDelete(NULL);
}

// Callback от концевика (если есть):
void IRAM_ATTR window_limit_switch_isr(void *arg) {
    int window_num = (int)(intptr_t)arg;
    
    // Остановка актуатора
    int gpio = (window_num == 1) ? 1 : 2;
    gpio_set_level(gpio, 0);
    
    // Обновление состояния
    if (s_window_states[window_num - 1] == WINDOW_OPENING) {
        s_window_states[window_num - 1] = WINDOW_OPEN;
    } else if (s_window_states[window_num - 1] == WINDOW_CLOSING) {
        s_window_states[window_num - 1] = WINDOW_CLOSED;
    }
    
    ESP_LOGI(TAG, "Window %d limit switch triggered", window_num);
}
```

---

### Управление вентилятором:

```c
esp_err_t fan_controller_set_speed(uint8_t speed_percent) {
    if (speed_percent > 100) {
        speed_percent = 100;
    }
    
    if (speed_percent == 0) {
        // Выключить
        gpio_set_level(FAN_GPIO, 0);
        s_fan_state = false;
        s_fan_speed = 0;
    } else if (speed_percent == 100) {
        // Полная мощность (ON/OFF)
        gpio_set_level(FAN_GPIO, 1);
        s_fan_state = true;
        s_fan_speed = 100;
    } else {
        // PWM (если поддерживается)
        uint32_t duty = (speed_percent * 255) / 100;
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, duty);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
        s_fan_state = true;
        s_fan_speed = speed_percent;
    }
    
    ESP_LOGI(TAG, "Fan: %s (speed=%d%%)", s_fan_state ? "ON" : "OFF", s_fan_speed);
    return ESP_OK;
}
```

---

### Управление LED светом:

```c
esp_err_t led_controller_set_brightness(uint8_t brightness_percent) {
    if (brightness_percent > 100) {
        brightness_percent = 100;
    }
    
    // PWM duty cycle (0-255)
    uint32_t duty = (brightness_percent * 255) / 100;
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    
    s_led_brightness = brightness_percent;
    
    ESP_LOGI(TAG, "LED brightness: %d%%", brightness_percent);
    return ESP_OK;
}
```

---

## 🌐 MESH INTEGRATION

### Главная задача (для heartbeat и мониторинга):

```c
static void relay_main_task(void *arg) {
    ESP_LOGI(TAG, "Main task running (heartbeat every 10 sec)");
    
    while (1) {
        // 1. Отправка heartbeat
        if (mesh_manager_is_connected()) {
            send_heartbeat();
        }
        
        // 2. Проверка timeout актуаторов (если нет концевиков)
        check_actuator_timeouts();
        
        // 3. Периодическая отправка статуса (каждые 60 сек)
        static int counter = 0;
        if (counter % 6 == 0) {  // Каждые 60 сек
            send_status();
        }
        counter++;
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // 10 сек
    }
}
```

---

## ❌ ЧТО НЕ ДЕЛАТЬ

### 1. ❌ НЕ принимай решений самостоятельно

```c
// ПЛОХО ❌
void check_temperature(float temp) {
    if (temp > 28.0f) {
        fan_controller_turn_on();  // ❌ Relay НЕ принимает решений!
    }
}

// ХОРОШО ✅
// Relay ТОЛЬКО исполняет команды от ROOT
// ROOT получает данные от climate NODE и принимает решение
```

### 2. ❌ НЕ работай без команд от ROOT

```c
// ПЛОХО ❌
void app_main(void) {
    // Автоматическое включение света по расписанию
    if (hour >= 6 && hour <= 20) {
        led_controller_set_brightness(100);  // ❌ Нет автономии!
    }
}

// ХОРОШО ✅
// Ждём команду от ROOT/Server:
// "set_light" с brightness=100
```

### 3. ❌ НЕ игнорируй safety checks

```c
// ПЛОХО ❌
gpio_set_level(WINDOW_1_GPIO, 1);  // ❌ Без timeout!
vTaskDelay(pdMS_TO_TICKS(60000));  // ❌ 60 сек - слишком долго!
gpio_set_level(WINDOW_1_GPIO, 0);

// ХОРОШО ✅
actuator_open_window(1);  // ✅ С timeout 45 сек и концевиками
```

---

## 🔧 КОМПОНЕНТЫ (структура)

### components/actuator_manager/:
```c
// Управление линейными актуаторами
esp_err_t actuator_manager_init(void);
esp_err_t actuator_manager_open_window(int window_num);
esp_err_t actuator_manager_close_window(int window_num);
window_state_t actuator_manager_get_state(int window_num);
void actuator_manager_emergency_stop(void);
```

### components/fan_controller/:
```c
// Управление вентилятором
esp_err_t fan_controller_init(void);
esp_err_t fan_controller_turn_on(void);
esp_err_t fan_controller_turn_off(void);
esp_err_t fan_controller_set_speed(uint8_t speed_percent);
bool fan_controller_is_on(void);
uint8_t fan_controller_get_speed(void);
```

### components/led_controller/:
```c
// Управление LED освещением
esp_err_t led_controller_init(void);
esp_err_t led_controller_set_brightness(uint8_t brightness_percent);
uint8_t led_controller_get_brightness(void);
esp_err_t led_controller_smooth_transition(uint8_t from, uint8_t to, uint32_t duration_ms);
```

### components/relay_controller/:
```c
// Главная логика
esp_err_t relay_controller_init(relay_node_config_t *config);
esp_err_t relay_controller_start(void);
void relay_controller_handle_command(const char *command, cJSON *params);
void relay_controller_handle_config_update(cJSON *config_json);
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Тест команды открыть форточку:

```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/relay_001 -m '{
  "type": "command",
  "node_id": "relay_001",
  "command": "set_window",
  "params": {"window": 1, "action": "open"}
}'
```

**Ожидаемый лог:**
```
I (5000) relay_ctrl: Command received: set_window
I (5000) actuator_mgr: Window 1 opening...
I (35000) actuator_mgr: Window 1 OPEN (limit switch triggered)
I (35010) relay_ctrl: Status sent to ROOT
```

### Тест управления вентилятором:

```bash
mosquitto_pub -h 192.168.1.100 -t hydro/command/relay_001 -m '{
  "type": "command",
  "node_id": "relay_001",
  "command": "set_fan",
  "params": {"state": "on", "speed": 75}
}'
```

**Ожидаемый лог:**
```
I (10000) relay_ctrl: Command received: set_fan
I (10000) fan_ctrl: Fan: ON (speed=75%)
I (10010) relay_ctrl: Status sent to ROOT
```

---

## 📊 ВЗАИМОДЕЙСТВИЕ С ДРУГИМИ УЗЛАМИ

### Типичный сценарий:

```
1. climate_001 отправляет:
   {"temperature": 29.5, "humidity": 70} → ROOT

2. ROOT (climate_logic) принимает решение:
   "Температура высокая → открыть форточку + включить вентилятор"

3. ROOT отправляет команды:
   hydro/command/relay_001 → "set_window" window=1 action=open
   hydro/command/relay_001 → "set_fan" state=on speed=100

4. relay_001 исполняет:
   ✅ Открывает форточку 1
   ✅ Включает вентилятор на 100%
   ✅ Отправляет статус обратно на ROOT

5. ROOT видит статус:
   {"window_1": "open", "fan": "on"}
   ✅ Действие выполнено успешно
```

---

## 🔐 SAFETY FEATURES

### 1. Timeout protection:
```c
#define ACTUATOR_MAX_TIME_SEC   45  // Макс время работы
#define ACTUATOR_COOLDOWN_SEC   5   // Пауза между операциями
```

### 2. Emergency stop:
```c
void relay_controller_emergency_stop(void) {
    // Остановка ВСЕХ актуаторов
    gpio_set_level(1, 0);  // Window 1
    gpio_set_level(2, 0);  // Window 2
    gpio_set_level(3, 0);  // Fan
    ledc_set_duty(..., 0); // LED
    
    ESP_LOGE(TAG, "EMERGENCY STOP ACTIVATED!");
}
```

### 3. Overcurrent detection (если есть датчик тока):
```c
// Мониторинг тока актуаторов
if (current > MAX_CURRENT_MA) {
    ESP_LOGE(TAG, "Overcurrent detected: %d mA", current);
    relay_controller_emergency_stop();
    send_emergency_event("Overcurrent protection");
}
```

---

## 📋 CHECKLIST ДЛЯ ИИ

Перед коммитом проверь:

- [ ] `MESH_MODE_NODE` (НЕ ROOT!)
- [ ] Callback `on_mesh_data_received` зарегистрирован
- [ ] Команды обрабатываются (`handle_command`)
- [ ] Статус отправляется после каждой команды
- [ ] Heartbeat каждые 10 сек
- [ ] Timeout protection для актуаторов (45 сек)
- [ ] Cooldown между операциями (5 сек)
- [ ] Emergency stop функция
- [ ] PWM для LED света (0-100%)
- [ ] НЕТ автономной логики (только команды!)
- [ ] НЕТ датчиков (только исполнители!)

---

## 🎯 КРИТЕРИИ УСПЕХА

**NODE Relay работает правильно если:**

1. ✅ Подключается к ROOT как NODE (layer 2)
2. ✅ Исполняет команды от ROOT/Server
3. ✅ Отправляет статус после каждой операции
4. ✅ Актуаторы останавливаются через 45 сек (timeout)
5. ✅ Концевики останавливают актуаторы (если есть)
6. ✅ Вентилятор управляется PWM (0-100%)
7. ✅ LED свет управляется PWM (0-100%)
8. ✅ Emergency stop работает
9. ✅ НЕ принимает решений самостоятельно

---

**ГОТОВО! Relay NODE - ТОЛЬКО ИСПОЛНИТЕЛЬ, БЕЗ ЛОГИКИ!** ⚙️🔌

