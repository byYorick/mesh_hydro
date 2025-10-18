# 🤖 ИНСТРУКЦИЯ ДЛЯ ИИ: NODE Display

**TFT дисплей с LVGL UI для мониторинга всей системы**

---

## 🎯 НАЗНАЧЕНИЕ УЗЛА

**NODE Display (ESP32-S3 #2 с PSRAM)** - визуальный интерфейс для мониторинга mesh-системы.

### 📺 Ключевые функции:

1. **TFT дисплей** - 240x320 ILI9341 через SPI
2. **LVGL UI** - современный графический интерфейс
3. **Ротационный энкодер** - навигация по меню
4. **Запрос данных** - получение данных от ROOT через mesh
5. **Показ всех узлов** - статус pH/EC, Climate, Relay, Water

### ⚠️ КРИТИЧНЫЕ ПРАВИЛА:

1. ✅ **ТРЕБУЕТ PSRAM!** - LVGL + TFT = много памяти
2. ❌ **НЕТ ДАТЧИКОВ!** - только отображение
3. ❌ **НЕТ ИСПОЛНИТЕЛЕЙ!** - только мониторинг
4. ✅ **ЗАПРОСЫ ЧЕРЕЗ ROOT** - не напрямую от узлов
5. ✅ **ОБНОВЛЕНИЕ ПО ТАЙМЕРУ** - каждые 5 секунд

---

## 🔌 РАСПИНОВКА

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| **LCD (SPI):** | | |
| 11 | MOSI | ILI9341 |
| 12 | SCLK | ILI9341 |
| 10 | CS | ILI9341 |
| 9 | DC | ILI9341 |
| 14 | RST | ILI9341 |
| 15 | Backlight | PWM |
| **Encoder:** | | |
| 4 | CLK (A) | Ротационный |
| 5 | DT (B) | Ротационный |
| 6 | SW | Кнопка |

---

## 🏗️ АРХИТЕКТУРА

```
NODE Display (ESP32-S3 #2 с PSRAM 8MB)
│
├── LCD Driver (ILI9341 SPI)
│   ├── 240x320 пикселей
│   ├── 16-bit color (RGB565)
│   └── Backlight PWM
│
├── LVGL UI Framework
│   ├── Dashboard экран (главный)
│   ├── Node Detail экран
│   ├── Node List экран
│   └── Settings экран
│
├── Encoder Handler
│   ├── Обработка вращения (навигация)
│   ├── Обработка нажатия (выбор)
│   └── Debouncing
│
├── Display Controller
│   ├── Запрос данных от ROOT
│   ├── Парсинг ответов
│   ├── Обновление UI (5 сек)
│   └── Кэширование данных
│
└── Mesh Client (NODE режим)
    ├── Отправка request → ROOT
    ├── Прием response от ROOT
    └── Heartbeat
```

---

## 💻 РЕАЛИЗАЦИЯ

### main/app_main.c

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lvgl.h"
#include "lcd_ili9341.h"
#include "encoder_driver.h"

#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"

#include "ui_screens.h"

static const char *TAG = "DISPLAY";

// Кэш данных узлов
typedef struct {
    char node_id[32];
    char node_type[16];
    bool online;
    cJSON *data;  // Последние данные
} node_cache_t;

static node_cache_t g_nodes_cache[10];
static int g_cache_count = 0;

// Запрос данных от ROOT
void request_all_nodes_data(void) {
    char json_buf[256];
    
    if (mesh_protocol_create_request("display_001", "all_nodes_data", 
                                      json_buf, sizeof(json_buf))) {
        ESP_LOGI(TAG, "Requesting data from ROOT...");
        mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));
    }
}

// Обработка ответа от ROOT
void handle_root_response(mesh_message_t *msg) {
    ESP_LOGI(TAG, "Response from ROOT received");
    
    cJSON *data = cJSON_GetObjectItem(msg->data, "data");
    if (!data || !cJSON_IsArray(data)) {
        return;
    }
    
    // Очистка старого кэша
    for (int i = 0; i < g_cache_count; i++) {
        if (g_nodes_cache[i].data) {
            cJSON_Delete(g_nodes_cache[i].data);
        }
    }
    g_cache_count = 0;
    
    // Заполнение нового кэша
    int array_size = cJSON_GetArraySize(data);
    for (int i = 0; i < array_size && i < 10; i++) {
        cJSON *node_obj = cJSON_GetArrayItem(data, i);
        if (node_obj) {
            cJSON *node_id = cJSON_GetObjectItem(node_obj, "node_id");
            cJSON *node_type = cJSON_GetObjectItem(node_obj, "type");
            cJSON *online = cJSON_GetObjectItem(node_obj, "online");
            
            if (node_id && node_type) {
                strncpy(g_nodes_cache[i].node_id, node_id->valuestring, 31);
                strncpy(g_nodes_cache[i].node_type, node_type->valuestring, 15);
                g_nodes_cache[i].online = online ? cJSON_IsTrue(online) : false;
                g_nodes_cache[i].data = cJSON_Duplicate(node_obj, true);
                
                g_cache_count++;
            }
        }
    }
    
    // Обновление UI
    ui_update_dashboard(g_nodes_cache, g_cache_count);
}

void on_mesh_data_received(const uint8_t *src, const uint8_t *data, size_t len) {
    mesh_message_t msg;
    if (!mesh_protocol_parse((const char*)data, &msg)) {
        return;
    }
    
    if (msg.type == MESH_MSG_RESPONSE) {
        handle_root_response(&msg);
    }
    
    mesh_protocol_free_message(&msg);
}

// Задача обновления дисплея
void display_update_task(void *arg) {
    while (1) {
        // Запрос данных каждые 5 секунд
        request_all_nodes_data();
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== NODE Display Starting ===");
    
    // 1. NVS
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // 2. Инициализация LCD
    ESP_LOGI(TAG, "Initializing LCD...");
    lcd_ili9341_init();
    
    // 3. Инициализация LVGL
    ESP_LOGI(TAG, "Initializing LVGL...");
    lv_init();
    lv_port_disp_init();  // Display driver
    lv_port_indev_init(); // Input driver (encoder)
    
    // 4. Инициализация энкодера
    ESP_LOGI(TAG, "Initializing encoder...");
    encoder_init();
    
    // 5. Создание UI экранов
    ESP_LOGI(TAG, "Creating UI screens...");
    ui_create_dashboard_screen();
    
    // 6. Mesh (NODE режим)
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_NODE,
        .mesh_id = "HYDRO1",
        .mesh_password = "hydro_mesh_2025",
        .channel = 1
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    mesh_manager_register_recv_cb(on_mesh_data_received);
    ESP_ERROR_CHECK(mesh_manager_start());
    
    // 7. Запуск задачи обновления
    xTaskCreate(display_update_task, "disp_update", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "=== NODE Display Running ===");
    
    // Главный цикл LVGL
    while (1) {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### ui_screens.c - примеры экранов

```c
#include "lvgl.h"
#include "ui_screens.h"

static lv_obj_t *dashboard_screen;
static lv_obj_t *node_labels[10];

void ui_create_dashboard_screen(void) {
    dashboard_screen = lv_obj_create(NULL);
    
    // Заголовок
    lv_obj_t *title = lv_label_create(dashboard_screen);
    lv_label_set_text(title, "HYDRO SYSTEM");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Карточки узлов (будут заполнены позже)
    for (int i = 0; i < 10; i++) {
        node_labels[i] = lv_label_create(dashboard_screen);
        lv_obj_set_pos(node_labels[i], 10, 40 + i * 25);
    }
    
    lv_scr_load(dashboard_screen);
}

void ui_update_dashboard(node_cache_t *nodes, int count) {
    for (int i = 0; i < 10; i++) {
        if (i < count) {
            char text[128];
            const char *status = nodes[i].online ? "●" : "○";
            snprintf(text, sizeof(text), "%s %s: %s", 
                     status, nodes[i].node_id, nodes[i].node_type);
            
            lv_label_set_text(node_labels[i], text);
            lv_obj_clear_flag(node_labels[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(node_labels[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}
```

---

## 📦 КОМПОНЕНТЫ

### Из common/:
- `mesh_manager`
- `mesh_protocol`
- `node_config`

### Из hydro1.0 (портировать):
- `lvgl_ui/` - LVGL компоненты
- `lcd_ili9341/` - драйвер дисплея
- `encoder/` - драйвер энкодера

### Новые:
- `display_controller` - запросы и обработка данных
- `ui_screens` - экраны интерфейса

---

## 🚫 ЧТО НЕЛЬЗЯ ДЕЛАТЬ

### 1. ❌ НЕ запрашивай данные напрямую от узлов

```c
// ПЛОХО ❌
void request_data(void) {
    uint8_t ph_ec_mac[6] = {...};
    mesh_manager_send(ph_ec_mac, request, len);  // ❌ Напрямую к узлу!
}

// ХОРОШО ✅
void request_data(void) {
    // Запрос через ROOT (он соберет данные всех узлов)
    mesh_protocol_create_request("display_001", "all_nodes_data", ...);
    mesh_manager_send_to_root(...);  // ✅ Через ROOT
}
```

### 2. ❌ НЕ блокируй UI

```c
// ПЛОХО ❌
void button_click_handler(void) {
    wait_for_response();  // ❌ Блокировка UI!
}

// ХОРОШО ✅
void button_click_handler(void) {
    send_request_async();  // ✅ Асинхронно
    // Ответ придет в callback
}
```

---

## ✅ ЧТО НУЖНО ДЕЛАТЬ

### 1. ✅ Портировать компоненты из hydro1.0

```bash
# Копировать из hydro1.0/components/ в node_display/components/
cp -r ../hydro1.0/components/lvgl_ui node_display/components/
cp -r ../hydro1.0/components/lcd_ili9341 node_display/components/
cp -r ../hydro1.0/components/encoder node_display/components/
```

### 2. ✅ Адаптировать для mesh

Вместо прямого чтения датчиков - запросы через mesh.

---

## 🧪 ТЕСТИРОВАНИЕ

```bash
cd node_display
idf.py set-target esp32s3
idf.py build flash monitor
```

**Ожидаемый лог:**
```
I DISPLAY: === NODE Display Starting ===
I DISPLAY: Initializing LCD...
I lcd: ILI9341 initialized
I DISPLAY: Initializing LVGL...
I lvgl: LVGL initialized
I DISPLAY: Creating UI screens...
I DISPLAY: === NODE Display Running ===
I DISPLAY: Requesting data from ROOT...
I DISPLAY: Response from ROOT received
I DISPLAY: 5 nodes in cache
```

---

## 📊 ЧЕКЛИСТ РАЗРАБОТКИ

- [ ] Портировать lcd_ili9341 из hydro1.0
- [ ] Портировать encoder из hydro1.0
- [ ] Портировать lvgl_ui из hydro1.0
- [ ] Адаптировать для запросов через mesh
- [ ] Создать Dashboard экран
- [ ] Создать Node Detail экран
- [ ] Реализовать кэширование данных
- [ ] Mesh NODE режим
- [ ] Тестирование с ROOT
- [ ] Навигация через энкодер

---

**NODE Display - визуальный мониторинг системы!** 📺

**Приоритет:** 🟡 СРЕДНИЙ (Фаза 6)

**Время реализации:** 3-4 дня

**ВАЖНО:** Требуется ESP32-S3 с PSRAM 8MB!
