# 🤖 AI INSTRUCTIONS - NODE Display

## 🎯 Назначение узла

**NODE Display (ESP32-S3 #2)** - TFT дисплей с LVGL UI

### Основные функции:
- 📺 TFT дисплей 240x320 (ILI9341 через SPI)
- 🎨 LVGL 8.x UI framework
- 🔄 Ротационный энкодер (навигация)
- 📊 Показ данных ВСЕХ узлов mesh-сети
- 🔍 Запрос данных от ROOT через mesh
- 📈 Графики в реальном времени

### Критичные правила:
- ✅ **НЕТ датчиков** - только отображение
- ✅ **НЕТ исполнителей** - только мониторинг
- ✅ **Требует PSRAM** - LVGL нужно много памяти
- ✅ Запросы данных через mesh (не напрямую от узлов)

---

## 🏗️ Архитектура

```
NODE Display (ESP32-S3 #2 с PSRAM)
│
├── LCD ILI9341 (SPI)
│   ├── 240x320 пикселей
│   ├── 16-bit color
│   └── Backlight PWM
│
├── LVGL UI
│   ├── Dashboard экран (главный)
│   ├── Node Detail экран (детали узла)
│   ├── Node List экран (список узлов)
│   └── System Menu экран (настройки)
│
├── Encoder Input
│   ├── CLK, DT (направление)
│   ├── SW (кнопка)
│   └── Debouncing
│
└── Display Controller
    ├── Запрос данных от ROOT
    ├── Парсинг ответов
    └── Обновление UI
```

---

## 🔌 Распиновка

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| **LCD (SPI):** | | |
| 11 | LCD MOSI | ILI9341 |
| 12 | LCD SCLK | ILI9341 |
| 10 | LCD CS | ILI9341 |
| 9 | LCD DC | ILI9341 |
| 14 | LCD RST | ILI9341 |
| 15 | LCD Backlight | PWM |
| **Encoder:** | | |
| 4 | Encoder A (CLK) | Ротационный |
| 5 | Encoder B (DT) | Ротационный |
| 6 | Encoder SW | Кнопка |

---

## 💻 Примеры кода

### Запрос данных от ROOT

```c
void display_request_all_nodes_data(void) {
    char json_buf[256];
    
    // Создать запрос
    mesh_protocol_create_request("display_001", "all_nodes_data", 
                                  json_buf, sizeof(json_buf));
    
    // Отправить на ROOT
    mesh_manager_send_to_root((uint8_t*)json_buf, strlen(json_buf));
}

void on_root_response(const char *json_data) {
    mesh_message_t msg;
    if (!mesh_protocol_parse(json_data, &msg)) return;
    
    // Обновить UI данными
    update_dashboard_ui(msg.data);
    
    mesh_protocol_free_message(&msg);
}
```

### LVGL экраны

```c
// Dashboard - главный экран
void create_dashboard_screen(void) {
    lv_obj_t *scr = lv_obj_create(NULL);
    
    // Карточки узлов
    create_node_card(scr, 10, 10, "pH/EC Zone 1", "6.5", "1.8");
    create_node_card(scr, 130, 10, "Climate", "24°C", "65%");
    
    // Кнопки навигации
    lv_obj_t *btn_nodes = lv_btn_create(scr);
    lv_obj_set_pos(btn_nodes, 10, 200);
    lv_obj_add_event_cb(btn_nodes, on_nodes_clicked, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(scr);
}
```

### Портирование из hydro1.0

```c
// Используй компоненты из hydro1.0:
// - lvgl_ui/
// - lcd_ili9341/
// - encoder/

// Но адаптируй для mesh-сети:
// - Запросы через mesh (не напрямую датчики)
// - Обновление по таймеру (каждые 5 сек)
```

---

## 📚 Документация

См. `node_display/README.md` и `doc/MESH_HYDRO_V2_FINAL_PLAN.md`

**Фаза 6 реализации (3-4 дня)**

