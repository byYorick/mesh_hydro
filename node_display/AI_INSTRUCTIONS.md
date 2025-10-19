# 🤖 ПОЛНАЯ ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Display

**Локальный дисплей для мониторинга всей mesh-системы (без смартфона/компьютера)**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Display (ESP32-S3 #2 с PSRAM 8MB)** - визуальный интерфейс для мониторинга и управления системой.

### 📺 Ключевые функции:

1. **TFT дисплей 320x240** - ILI9341 через SPI (полноцветный)
2. **LVGL GUI** - современный графический интерфейс с анимациями
3. **Ротационный энкодер** - навигация и выбор (с кнопкой)
4. **Запрос данных** - REQUEST к ROOT → RESPONSE со всеми узлами
5. **Кэширование данных** - локальное хранение для быстрого отображения
6. **Управление узлами** - отправка команд через ROOT
7. **Heartbeat** - подтверждение работоспособности

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ✅ **ТРЕБУЕТ PSRAM 8MB!** - LVGL + TFT буферы требуют много памяти
2. ❌ **НЕТ ДАТЧИКОВ!** - Display НЕ читает физические датчики
3. ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** - Display НЕ управляет реле/насосами напрямую
4. ✅ **ЗАПРОСЫ ЧЕРЕЗ ROOT** - все данные через REQUEST/RESPONSE протокол
5. ✅ **КОМАНДЫ ЧЕРЕЗ ROOT** - отправка команд другим узлам через ROOT
6. ✅ **ОБНОВЛЕНИЕ ПО ТАЙМЕРУ** - запрос данных каждые 5 секунд

---

## 📦 ОБОРУДОВАНИЕ

### TFT Дисплей:
| Параметр | Значение |
|----------|----------|
| **Модель** | ILI9341 |
| **Разрешение** | 320x240 пикселей |
| **Интерфейс** | SPI (40 MHz) |
| **Цвета** | 16-bit RGB565 (65K colors) |
| **Подсветка** | PWM 0-100% |
| **Touch** | Опционально (XPT2046) |

### Ротационный энкодер:
| Параметр | Значение |
|----------|----------|
| **Тип** | KY-040 или аналог |
| **Выходы** | CLK (A), DT (B), SW (button) |
| **Разрешение** | 20 импульсов на оборот |
| **Debouncing** | Программный (10ms) |

---

## 🔌 РАСПИНОВКА ESP32-S3 #2

| GPIO | Назначение | Компонент | Примечание |
|------|------------|-----------|------------|
| **SPI для TFT (VSPI):** | | | |
| 23 | MOSI | ILI9341 | Master Out Slave In |
| 18 | SCK | ILI9341 | Clock (40 MHz) |
| 5 | CS | ILI9341 | Chip Select |
| 2 | DC | ILI9341 | Data/Command |
| 4 | RST | ILI9341 | Reset |
| 15 | Backlight | PWM | 0-100% яркость |
| **Ротационный энкодер:** | | | |
| 32 | Encoder CLK | KY-040 pin A | Pullup внутренний |
| 33 | Encoder DT | KY-040 pin B | Pullup внутренний |
| 25 | Encoder SW | Button | Pullup внутренний |
| **Опциональный Touch:** | | | |
| 19 | Touch MISO | XPT2046 | Если используется |
| 26 | Touch IRQ | XPT2046 | Interrupt |

---

## 🏗️ АРХИТЕКТУРА ПО КОМПОНЕНТАМ

```
node_display/
├── components/
│   ├── lcd_driver/           # ILI9341 SPI driver
│   ├── lvgl_port/            # LVGL интеграция с ESP32
│   ├── encoder_driver/       # Ротационный энкодер + button
│   ├── ui_screens/           # LVGL экраны
│   │   ├── ui_dashboard.c   # Главный экран (все узлы)
│   │   ├── ui_node_detail.c # Детали узла
│   │   ├── ui_node_list.c   # Список всех узлов
│   │   └── ui_settings.c    # Настройки Display
│   ├── display_controller/   # Логика запросов и кэширования
│   └── command_sender/       # Отправка команд узлам
│
├── main/
│   └── app_main.c            # Mesh callback + LVGL init
│
└── AI_INSTRUCTIONS_COMPLETE.md  # ← ЭТА ИНСТРУКЦИЯ
```

---

## 📨 ТИПЫ СООБЩЕНИЙ

### 1. 📤 REQUEST (Display → ROOT)

**Запрос всех данных узлов:**

```json
{
  "type": "request",
  "from": "display_001",
  "request": "all_nodes_data"
}
```

**Код отправки:**
```c
void request_all_nodes_data(void) {
    char json_buf[256];
    
    if (mesh_protocol_create_request(s_config->base.node_id, "all_nodes_data",
                                      json_buf, sizeof(json_buf))) {
        esp_err_t err = mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        if (err == ESP_OK) {
            ESP_LOGD(TAG, "Data request sent to ROOT");
        } else {
            ESP_LOGW(TAG, "Failed to send request: %s", esp_err_to_name(err));
        }
    }
}
```

**Частота:** Каждые 5 секунд (автоматически в задаче)

---

### 2. 📥 RESPONSE (ROOT → Display)

**Ответ от ROOT со всеми узлами:**

```json
{
  "type": "response",
  "to": "display_001",
  "data": {
    "nodes": [
      {
        "node_id": "climate_001",
        "node_type": "climate",
        "online": true,
        "data": {
          "temperature": 24.5,
          "humidity": 65,
          "co2": 820,
          "lux": 450
        }
      },
      {
        "node_id": "ph_ec_001",
        "node_type": "ph_ec",
        "online": true,
        "data": {
          "ph": 6.5,
          "ec": 1.8,
          "temperature": 22.5
        }
      },
      {
        "node_id": "relay_001",
        "node_type": "relay",
        "online": true,
        "data": {
          "window_1": "closed",
          "window_2": "open",
          "fan": "on",
          "led_brightness": 75
        }
      },
      {
        "node_id": "water_001",
        "node_type": "water",
        "online": true,
        "data": {
          "water_level_cm": 45,
          "water_level_percent": 75,
          "pump_running": false
        }
      }
    ]
  }
}
```

**Обработка:**
```c
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
    
    if (strcmp(msg.node_id, s_config->base.node_id) != 0) {
        free(data_copy);
        mesh_protocol_free_message(&msg);
        return;
    }

    if (msg.type == MESH_MSG_RESPONSE) {
        display_controller_handle_response(&msg);
    }

    free(data_copy);
    mesh_protocol_free_message(&msg);
}

void display_controller_handle_response(mesh_message_t *msg) {
    cJSON *nodes_array = cJSON_GetObjectItem(msg->data, "nodes");
    if (!nodes_array || !cJSON_IsArray(nodes_array)) {
        ESP_LOGE(TAG, "Invalid response format");
        return;
    }
    
    // Обновление кэша узлов
    display_cache_update(nodes_array);
    
    // Обновление UI
    ui_update_all_screens();
    
    ESP_LOGI(TAG, "Received data for %d nodes", cJSON_GetArraySize(nodes_array));
}
```

---

### 3. 📤 COMMAND (Display → ROOT → Target NODE)

**Display может отправлять команды другим узлам через ROOT:**

```json
{
  "type": "command",
  "node_id": "relay_001",
  "command": "set_fan",
  "params": {
    "state": "on",
    "speed": 75
  },
  "from": "display_001"
}
```

**Код:**
```c
void display_send_command_to_node(const char *target_node_id, 
                                   const char *command, 
                                   cJSON *params) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "command");
    cJSON_AddStringToObject(root, "node_id", target_node_id);
    cJSON_AddStringToObject(root, "command", command);
    cJSON_AddItemToObject(root, "params", cJSON_Duplicate(params, true));
    cJSON_AddStringToObject(root, "from", s_config->base.node_id);
    
    char json_buf[512];
    char *json_str = cJSON_PrintUnformatted(root);
    if (json_str) {
        strncpy(json_buf, json_str, sizeof(json_buf) - 1);
        free(json_str);
        
        // Отправка на ROOT (он перешлёт target узлу)
        mesh_manager_send_to_root((uint8_t *)json_buf, strlen(json_buf));
        
        ESP_LOGI(TAG, "Command sent to %s: %s", target_node_id, command);
    }
    
    cJSON_Delete(root);
}
```

---

### 4. 💓 HEARTBEAT (каждые 10 сек)

```json
{
  "type": "heartbeat",
  "node_id": "display_001",
  "uptime": 3600,
  "heap_free": 3500000,
  "rssi_to_parent": -45
}
```

---

## 📺 LVGL UI SCREENS

### 1. Dashboard (главный экран):

```
┌──────────────────────────────────┐
│  🌿 HYDRO MESH SYSTEM v2.0       │
├──────────────────────────────────┤
│  ╭─ Climate ─────────────● ON ─╮ │
│  │  🌡 24.5°C   💧 65%          │ │
│  │  CO2: 820ppm   Lux: 450      │ │
│  ╰─────────────────────────────╯ │
│                                  │
│  ╭─ pH/EC ────────────────● ON ─╮│
│  │  pH: 6.5   EC: 1.8 mS/cm    │ │
│  │  Temp: 22.5°C                │ │
│  ╰─────────────────────────────╯ │
│                                  │
│  ╭─ Relay ────────────────● ON ─╮│
│  │  Fan: ON 100%  Light: 75%   │ │
│  │  Window: OPEN                │ │
│  ╰─────────────────────────────╯ │
│                                  │
│  ╭─ Water ────────────────● ON ─╮│
│  │  Level: 45cm (75%)           │ │
│  │  Pump: OFF                   │ │
│  ╰─────────────────────────────╯ │
│                                  │
│  Last update: 2s ago             │
└──────────────────────────────────┘
```

**LVGL код:**
```c
void ui_create_dashboard_screen(void) {
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // Заголовок
    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "🌿 HYDRO MESH SYSTEM v2.0");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Карточки узлов
    lv_obj_t *climate_card = create_node_card(screen, "Climate", 0, 40);
    lv_obj_t *ph_ec_card = create_node_card(screen, "pH/EC", 0, 110);
    lv_obj_t *relay_card = create_node_card(screen, "Relay", 0, 180);
    lv_obj_t *water_card = create_node_card(screen, "Water", 0, 250);
    
    // Статус бар
    lv_obj_t *status = lv_label_create(screen);
    lv_label_set_text(status, "Last update: 0s ago");
    lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_scr_load(screen);
}

lv_obj_t *create_node_card(lv_obj_t *parent, const char *name, int x, int y) {
    // Контейнер карточки
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 300, 60);
    lv_obj_set_pos(card, x + 10, y);
    lv_obj_set_style_radius(card, 10, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x2C3E50), 0);
    
    // Название узла
    lv_obj_t *label = lv_label_create(card);
    lv_label_set_text(label, name);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_16, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 5);
    
    // Индикатор ONLINE
    lv_obj_t *status_led = lv_led_create(card);
    lv_obj_set_size(status_led, 12, 12);
    lv_obj_align(status_led, LV_ALIGN_TOP_RIGHT, -10, 8);
    lv_led_set_color(status_led, lv_color_hex(0x00FF00));  // Зеленый
    lv_led_on(status_led);
    
    // Данные узла
    lv_obj_t *data_label = lv_label_create(card);
    lv_label_set_text(data_label, "Loading...");
    lv_obj_align(data_label, LV_ALIGN_CENTER, 0, 10);
    
    return card;
}
```

---

### 2. Node Detail (детальный экран узла):

```
┌──────────────────────────────────┐
│  ← Back          Climate Node    │
├──────────────────────────────────┤
│  🌡 Temperature:  24.5°C         │
│     ├─ Target: 24.0°C            │
│     └─ Status: ✅ Normal         │
│                                  │
│  💧 Humidity:  65%               │
│     ├─ Target: 65%               │
│     └─ Status: ✅ Normal         │
│                                  │
│  CO2:  820 ppm                   │
│     ├─ Target: 800 ppm           │
│     └─ Status: ⚠️ Slightly high  │
│                                  │
│  Lux:  450                       │
│     └─ Target: 500               │
│                                  │
│  📡 RSSI: -42 dBm (Excellent)    │
│  ⏱ Uptime: 2h 15m                │
│  💾 Free heap: 158 KB            │
│                                  │
│  [Settings] [Commands]           │
└──────────────────────────────────┘
```

---

### 3. Command Panel (управление узлом):

```
┌──────────────────────────────────┐
│  Control: climate_001            │
├──────────────────────────────────┤
│  📝 Commands:                    │
│                                  │
│  ┌─────────────────────────────┐ │
│  │ Set Read Interval           │ │
│  │ → 30 seconds                │ │
│  └─────────────────────────────┘ │
│                                  │
│  ┌─────────────────────────────┐ │
│  │ Restart Sensor              │ │
│  └─────────────────────────────┘ │
│                                  │
│  ┌─────────────────────────────┐ │
│  │ Reset Statistics            │ │
│  └─────────────────────────────┘ │
│                                  │
│  ⚠️ Use encoder to select        │
│  Press button to confirm         │
└──────────────────────────────────┘
```

---

## 💻 ГЛАВНАЯ ЛОГИКА

### app_main.c:

```c
#include "lvgl.h"
#include "lcd_driver.h"
#include "encoder_driver.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "display_controller.h"
#include "ui_screens.h"

static display_node_config_t g_config;

// Mesh callback
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
        case MESH_MSG_RESPONSE:
            display_controller_handle_response(&msg);
            break;

        case MESH_MSG_CONFIG:
            display_controller_handle_config(&msg);
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
    node_config_load(&g_config, sizeof(g_config), "display_ns");
    
    // === Шаг 3: Инициализация LCD (SPI) ===
    lcd_driver_init();
    
    // === Шаг 4: Инициализация LVGL ===
    lv_init();
    
    // Создание буферов (ТРЕБУЕТ PSRAM!)
    static lv_color_t buf1[LCD_WIDTH * 40];  // 320*40*2 = 25.6 KB
    static lv_color_t buf2[LCD_WIDTH * 40];
    
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(disp, lcd_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
    
    // === Шаг 5: Инициализация энкодера ===
    encoder_driver_init();
    
    // Input device для LVGL
    lv_indev_t *encoder_indev = lv_indev_create();
    lv_indev_set_type(encoder_indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(encoder_indev, encoder_read_cb);
    
    // === Шаг 6: Создание UI ===
    ui_create_dashboard_screen();
    
    // === Шаг 7: Mesh (NODE режим) ===
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
    
    // === Шаг 8: Запуск контроллера ===
    display_controller_init(&g_config);
    display_controller_start();
    
    // === Шаг 9: LVGL timer task ===
    xTaskCreate(lvgl_timer_task, "lvgl_timer", 4096, NULL, 5, NULL);
}
```

---

## 🔄 ОБНОВЛЕНИЕ ДАННЫХ

### Задача автоматического обновления:

```c
static void display_update_task(void *arg) {
    ESP_LOGI(TAG, "Update task running (request data every 5 sec)");
    
    // Начальная задержка (ждём подключения к mesh)
    vTaskDelay(pdMS_TO_TICKS(10000));
    
    while (1) {
        if (mesh_manager_is_connected()) {
            // Запрос данных у ROOT
            request_all_nodes_data();
            
            // Обновление индикации связи
            ui_update_connection_status(true);
        } else {
            // Показ offline статуса
            ui_update_connection_status(false);
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Каждые 5 секунд
    }
}
```

---

## 🎮 НАВИГАЦИЯ ЭНКОДЕРОМ

### Обработка энкодера:

```c
typedef enum {
    SCREEN_DASHBOARD = 0,
    SCREEN_CLIMATE_DETAIL,
    SCREEN_PH_EC_DETAIL,
    SCREEN_RELAY_DETAIL,
    SCREEN_WATER_DETAIL,
    SCREEN_SETTINGS
} screen_type_t;

static screen_type_t current_screen = SCREEN_DASHBOARD;
static int selected_item = 0;

void encoder_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    static int last_enc_value = 0;
    int enc_value = encoder_get_value();
    
    // Вращение
    int diff = enc_value - last_enc_value;
    last_enc_value = enc_value;
    
    data->enc_diff = diff;
    
    // Кнопка
    data->state = encoder_is_pressed() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void encoder_event_handler(int encoder_value, bool button_pressed) {
    if (current_screen == SCREEN_DASHBOARD) {
        // Выбор узла (0-4)
        selected_item = (encoder_value / 4) % 5;  // 5 узлов (ROOT + 4 NODE)
        ui_highlight_item(selected_item);
        
        // Нажатие - переход на детальный экран
        if (button_pressed) {
            switch (selected_item) {
                case 0:
                    ui_load_screen(SCREEN_CLIMATE_DETAIL);
                    break;
                case 1:
                    ui_load_screen(SCREEN_PH_EC_DETAIL);
                    break;
                // ...
            }
        }
    }
}
```

---

## ❌ ЧТО НЕ ДЕЛАТЬ

### 1. ❌ НЕ запрашивай данные у NODE напрямую

```c
// ПЛОХО ❌
// Попытка отправить request напрямую на climate_001
mesh_manager_send(climate_mac, request, len);  // ❌ NODE не обрабатывает REQUEST!

// ХОРОШО ✅
// Запрос у ROOT, он соберёт данные всех узлов
mesh_manager_send_to_root(request, len);  // ✅ ROOT обработает REQUEST
```

### 2. ❌ НЕ блокируй UI ожиданием ответа

```c
// ПЛОХО ❌
void button_click() {
    send_request();
    while (!response_received) {  // ❌ Зависание UI!
        vTaskDelay(100);
    }
    update_ui();
}

// ХОРОШО ✅
void button_click() {
    send_request();  // ✅ Асинхронно
    // Ответ придет в callback on_mesh_data_received
    // UI обновится автоматически
}
```

### 3. ❌ НЕ создавай LVGL объекты без PSRAM

```c
// ПЛОХО ❌
// На ESP32 без PSRAM:
static lv_color_t buf[LCD_WIDTH * LCD_HEIGHT];  // ❌ 320*240*2 = 153 KB - не влезет!

// ХОРОШО ✅
// С PSRAM (ESP32-S3):
static lv_color_t buf1[LCD_WIDTH * 40];  // 25.6 KB в PSRAM
static lv_color_t buf2[LCD_WIDTH * 40];  // Двойная буферизация
```

---

## 🔧 КОМПОНЕНТЫ (структура)

### components/lcd_driver/:
```c
esp_err_t lcd_driver_init(void);
void lcd_driver_set_backlight(uint8_t brightness);
void lcd_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p);
```

### components/encoder_driver/:
```c
esp_err_t encoder_driver_init(void);
int encoder_get_value(void);
bool encoder_is_pressed(void);
void encoder_reset(void);
```

### components/display_controller/:
```c
esp_err_t display_controller_init(display_node_config_t *config);
esp_err_t display_controller_start(void);
void display_controller_request_data(void);
void display_controller_handle_response(mesh_message_t *msg);
void display_controller_send_command(const char *node_id, const char *command, cJSON *params);
```

### components/ui_screens/:
```c
void ui_create_dashboard_screen(void);
void ui_create_node_detail_screen(const char *node_id);
void ui_update_node_card(const char *node_id, cJSON *data);
void ui_show_error(const char *message);
```

---

## 🧪 ТЕСТИРОВАНИЕ

### Проверка REQUEST/RESPONSE:

1. Запусти Display NODE
2. Запусти ROOT
3. **Проверь лог Display:**

```
I (10000) DISPLAY: Mesh connected
I (10010) display_ctrl: Requesting data from ROOT...
I (10200) DISPLAY: Response from ROOT received
I (10210) display_ctrl: 4 nodes in cache
I (10220) ui: Dashboard updated
```

4. **Проверь лог ROOT:**

```
I (10100) data_router: Request from display_001 (Display)
I (10110) data_router: Request type: all_nodes_data
I (10120) data_router: Sent response to Display (4 nodes, 1024 bytes)
```

---

## 📊 КЭШИРОВАНИЕ ДАННЫХ

### Локальный кэш узлов:

```c
#define MAX_CACHED_NODES 10

typedef struct {
    char node_id[32];
    char node_type[16];
    bool online;
    uint64_t last_update;
    cJSON *data;  // Последние данные узла
} cached_node_t;

static cached_node_t g_nodes_cache[MAX_CACHED_NODES];
static int g_cache_count = 0;
static SemaphoreHandle_t g_cache_mutex;

void display_cache_update(cJSON *nodes_array) {
    xSemaphoreTake(g_cache_mutex, portMAX_DELAY);
    
    // Очистка старого кэша
    for (int i = 0; i < g_cache_count; i++) {
        if (g_nodes_cache[i].data) {
            cJSON_Delete(g_nodes_cache[i].data);
        }
    }
    
    g_cache_count = 0;
    
    // Заполнение нового кэша
    int array_size = cJSON_GetArraySize(nodes_array);
    for (int i = 0; i < array_size && i < MAX_CACHED_NODES; i++) {
        cJSON *node = cJSON_GetArrayItem(nodes_array, i);
        
        cJSON *node_id = cJSON_GetObjectItem(node, "node_id");
        cJSON *node_type = cJSON_GetObjectItem(node, "node_type");
        cJSON *online = cJSON_GetObjectItem(node, "online");
        
        if (node_id && node_type) {
            strncpy(g_nodes_cache[i].node_id, node_id->valuestring, 31);
            strncpy(g_nodes_cache[i].node_type, node_type->valuestring, 15);
            g_nodes_cache[i].online = online ? cJSON_IsTrue(online) : false;
            g_nodes_cache[i].last_update = esp_timer_get_time() / 1000000;
            g_nodes_cache[i].data = cJSON_Duplicate(node, true);
            
            g_cache_count++;
        }
    }
    
    xSemaphoreGive(g_cache_mutex);
    
    ESP_LOGI(TAG, "Cache updated: %d nodes", g_cache_count);
}

cached_node_t *display_cache_get(const char *node_id) {
    xSemaphoreTake(g_cache_mutex, portMAX_DELAY);
    
    for (int i = 0; i < g_cache_count; i++) {
        if (strcmp(g_nodes_cache[i].node_id, node_id) == 0) {
            xSemaphoreGive(g_cache_mutex);
            return &g_nodes_cache[i];
        }
    }
    
    xSemaphoreGive(g_cache_mutex);
    return NULL;
}
```

---

## 🌐 MESH INTEGRATION

### Инициализация (NODE режим):

```c
mesh_manager_config_t mesh_config = {
    .mode = MESH_MODE_NODE,  // ← Display это NODE!
    .mesh_id = MESH_NETWORK_ID,
    .mesh_password = MESH_NETWORK_PASSWORD,
    .channel = MESH_NETWORK_CHANNEL,
    .max_connection = 6,
    .router_ssid = MESH_ROUTER_SSID,
    .router_password = MESH_ROUTER_PASSWORD,
    .router_bssid = NULL
};

mesh_manager_init(&mesh_config);
mesh_manager_register_recv_cb(on_mesh_data_received);  // ← Callback для RESPONSE!
mesh_manager_start();
```

---

## 📋 CHECKLIST ДЛЯ ИИ

Перед коммитом проверь:

- [ ] ESP32-S3 с PSRAM 8MB (обязательно!)
- [ ] `MESH_MODE_NODE` (НЕ ROOT!)
- [ ] Callback `on_mesh_data_received` для RESPONSE
- [ ] REQUEST каждые 5 секунд
- [ ] Heartbeat каждые 10 секунд
- [ ] Кэширование данных узлов
- [ ] LVGL буферы в PSRAM
- [ ] Encoder debouncing
- [ ] Backlight PWM (0-100%)
- [ ] UI обновляется асинхронно
- [ ] НЕТ прямых запросов к NODE!
- [ ] Команды через ROOT

---

## 🎯 КРИТЕРИИ УСПЕХА

**NODE Display работает правильно если:**

1. ✅ Подключается к ROOT как NODE (layer 2)
2. ✅ Запрашивает данные каждые 5 сек
3. ✅ Получает RESPONSE от ROOT
4. ✅ Отображает ВСЕ узлы (climate, ph_ec, relay, water)
5. ✅ Энкодер работает (навигация плавная)
6. ✅ UI обновляется без зависаний
7. ✅ Может отправлять команды узлам через ROOT
8. ✅ Heap стабильный (~3.5 MB в PSRAM)

---

**ГОТОВО! Display NODE - ВИЗУАЛЬНЫЙ МОНИТОРИНГ ЧЕРЕЗ MESH!** 📺🎮

