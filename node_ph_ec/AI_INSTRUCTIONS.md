# 🤖 AI INSTRUCTIONS - NODE pH/EC

## 🎯 Назначение узла

**NODE pH/EC (ESP32-S3 #3+)** - КРИТИЧНЫЙ узел с автономной работой

### ⚠️ ЭТО САМЫЙ ВАЖНЫЙ УЗЕЛ СИСТЕМЫ!

### Основные функции:
- 📊 Измерение pH и EC (I2C датчики Trema)
- 💧 Управление 5 насосами (pH UP/DOWN, EC A/B/C)
- 🤖 AI PID контроллер (адаптивный)
- 📺 OLED дисплей 128x64 (SSD1306) - локальная индикация
- 🔋 **АВТОНОМНАЯ РАБОТА** при потере связи с ROOT
- 💾 Локальный ring buffer (1000 записей)
- 🔴 LED + Buzzer индикация
- 🔘 Кнопка MODE (переключение режимов)

### 🚨 КРИТИЧНЫЕ ПРАВИЛА:

1. ✅ **ВСЕГДА автономная работа**
   - Если ROOT offline > 30 сек → автономный режим
   - PID контроллер НИКОГДА не останавливается
   - Используй настройки из NVS

2. ✅ **ВСЕГДА сохранение в NVS**
   - Все настройки (pH target, EC target, PID параметры)
   - Калибровки датчиков
   - История насосов (мл за день)

3. ✅ **ВСЕГДА локальный buffer**
   - Ring buffer 1000 записей
   - При восстановлении связи → отправка батчами

4. ✅ **ВСЕГДА watchdog**
   - Сброс каждые 10 сек
   - Перезагрузка при зависании

5. ❌ **НИКОГДА не блокировать PID**
   - PID цикл каждые 10 сек
   - Не ждать ответа от ROOT
   - Mesh отправка в отдельной задаче

---

## 🏗️ Архитектура

```
NODE pH/EC (ESP32-S3 #3)
│
├── Sensor Manager
│   ├── pH sensor (I2C 0x0A) - Trema
│   ├── EC sensor (I2C 0x08) - Trema
│   ├── Калибровка из NVS
│   └── Retry логика (3 попытки)
│
├── Pump Manager
│   ├── 5 насосов (GPIO 1-5)
│   ├── Safety: max time, cooldown
│   ├── Статистика (мл за день/неделю)
│   └── Emergency stop
│
├── Adaptive PID (AI)
│   ├── pH UP/DOWN контроллеры
│   ├── EC A/B/C контроллеры
│   ├── Адаптация параметров
│   └── Anti-windup
│
├── OLED Display (SSD1306)
│   ├── pH: 6.5  EC: 1.8
│   ├── ● ONLINE / ⚡ AUTONOMOUS
│   ├── Насосы (мл за час)
│   └── Статус/алерты
│
├── Connection Monitor
│   ├── Проверка связи с ROOT
│   ├── Переключение режимов:
│   │   - ONLINE (норма)
│   │   - DEGRADED (нестабильно)
│   │   - AUTONOMOUS (автономия)
│   │   - EMERGENCY (авария)
│   └── Логика переходов
│
├── Local Storage
│   ├── Ring buffer (1000 записей)
│   ├── Timestamp каждой записи
│   └── Синхронизация с ROOT
│
└── Buzzer + LED + Button
    ├── LED: зеленый/желтый/красный
    ├── Buzzer: сигналы при авариях
    └── Кнопка MODE: смена режимов
```

---

## 📦 Компоненты

### Обязательные (common):
- `mesh_manager` - Mesh NODE режим
- `mesh_protocol` - JSON протокол
- `node_config` - NVS конфигурация
- `sensor_base` - Retry, validation
- `actuator_base` - Safety checks

### Из hydro1.0 (портировать):
- `sensor_manager` - pH/EC датчики
- `pump_manager` - 5 насосов
- `adaptive_pid` - AI PID контроллер

### Новые (реализовать):
- `oled_display` - SSD1306 128x64
- `connection_monitor` - Мониторинг связи
- `local_storage` - Ring buffer
- `buzzer_led` - Индикация
- `node_controller` - Главная логика

---

## 🔌 Распиновка

| GPIO | Назначение | Компонент |
|------|------------|-----------|
| **I2C шина:** | | |
| 17 | I2C SCL | pH, EC, OLED |
| 18 | I2C SDA | pH, EC, OLED |
| - | OLED 0x3C | SSD1306 128x64 |
| - | pH 0x0A | Trema pH |
| - | EC 0x08 | Trema EC |
| **Насосы (5 штук):** | | |
| 1 | Насос pH UP | GPIO HIGH = вкл |
| 2 | Насос pH DOWN | GPIO HIGH = вкл |
| 3 | Насос EC A | GPIO HIGH = вкл |
| 4 | Насос EC B | GPIO HIGH = вкл |
| 5 | Насос EC C | GPIO HIGH = вкл |
| **Индикация:** | | |
| 15 | LED статус | RGB или 3×LED (R/G/Y) |
| 16 | Buzzer | Пищалка 3.3V |
| 19 | Кнопка MODE | Переключение режимов |

---

## 💻 Примеры кода

### Главный цикл

```c
void node_ph_ec_main_task(void *arg) {
    ph_ec_node_config_t config;
    float ph, ec;
    
    // Загрузка конфигурации из NVS
    node_config_load(&config, sizeof(config), "ph_ec_ns");
    
    while (1) {
        // 1. Чтение датчиков
        if (sensor_manager_read_ph(&ph) == ESP_OK &&
            sensor_manager_read_ec(&ec) == ESP_OK) {
            
            sensor_cache_value("ph", ph);
            sensor_cache_value("ec", ec);
            
            // 2. OLED обновление
            oled_display_update(ph, ec, get_connection_state());
            
            // 3. PID контроль (ВСЕГДА работает!)
            if (config.base.auto_mode || is_autonomous()) {
                adaptive_pid_update(ph, ec, &config);
            }
            
            // 4. Отправка телеметрии (если online)
            if (is_online()) {
                send_telemetry(ph, ec);
            } else {
                // Автономный режим - буферизация
                local_storage_add(ph, ec);
            }
            
            // 5. Проверка аварийных ситуаций
            if (ph < config.ph_emergency_low) {
                handle_emergency_ph_low(ph, &config);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000));  // Каждые 10 сек
    }
}
```

### Автономная работа

```c
void connection_monitor_task(void *arg) {
    uint64_t last_root_msg = 0;
    connection_state_t state = STATE_ONLINE;
    
    while (1) {
        uint64_t now = esp_timer_get_time() / 1000;
        uint64_t elapsed = now - last_root_msg;
        
        switch (state) {
            case STATE_ONLINE:
                if (elapsed > 10000) {  // 10 сек без сообщений
                    state = STATE_DEGRADED;
                    oled_display_set_status("DEGRADED");
                    set_led_yellow();
                }
                break;
                
            case STATE_DEGRADED:
                if (elapsed > 30000) {  // 30 сек без сообщений
                    state = STATE_AUTONOMOUS;
                    oled_display_set_status("AUTONOMOUS");
                    set_led_yellow_blink();
                    
                    // Загрузить последнюю конфигурацию из NVS
                    load_config_from_nvs();
                    
                    ESP_LOGW(TAG, "Entering AUTONOMOUS mode");
                }
                break;
                
            case STATE_AUTONOMOUS:
                if (elapsed < 10000) {  // Связь восстановлена
                    state = STATE_ONLINE;
                    oled_display_set_status("ONLINE");
                    set_led_green();
                    
                    // Синхронизация буфера
                    sync_local_buffer_to_root();
                    
                    ESP_LOGI(TAG, "Back to ONLINE mode");
                }
                break;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### Аварийная ситуация

```c
void handle_emergency_ph_low(float ph, ph_ec_node_config_t *config) {
    ESP_LOGE(TAG, "EMERGENCY: pH critically low: %.2f", ph);
    
    // 1. Визуальная индикация
    set_led_red_fast_blink();
    buzzer_alarm(3);  // 3 сигнала
    oled_display_show_emergency("pH TOO LOW!", ph);
    
    // 2. Агрессивная коррекция
    float correction_ml = calculate_emergency_correction(ph, config->ph_target);
    pump_manager_run("ph_up", correction_ml * 1000, true);  // true = emergency
    
    // 3. Попытка отправить SOS
    if (is_online()) {
        send_emergency_event("pH critically low", ph);
    } else {
        // Сохранить в buffer с высоким приоритетом
        local_storage_add_priority(ph, EC_EMERGENCY_TAG);
    }
    
    // 4. Логирование
    log_emergency_action("ph_low", ph, correction_ml);
}
```

### OLED дисплей

```c
void oled_display_update(float ph, float ec, connection_state_t state) {
    char line1[32], line2[32], line3[32], line4[32];
    
    // Строка 1: Заголовок
    snprintf(line1, sizeof(line1), "pH/EC Zone 1");
    
    // Строка 2: Показания
    snprintf(line2, sizeof(line2), "pH:%.1f  EC:%.1f", ph, ec);
    
    // Строка 3: Статус
    const char *status_str = get_status_string(state);
    const char *mode_str = config.auto_mode ? "AUTO" : "MANUAL";
    snprintf(line3, sizeof(line3), "%s  %s", status_str, mode_str);
    
    // Строка 4: Насосы (за последний час)
    pump_stats_t stats = pump_manager_get_stats();
    snprintf(line4, sizeof(line4), "↑%dml ↓%dml →%dml", 
             stats.ph_up_ml_hour, 
             stats.ph_down_ml_hour, 
             stats.ec_ml_hour);
    
    // Отправка на дисплей
    ssd1306_clear_screen();
    ssd1306_draw_string(0, 0, line1, 1);
    ssd1306_draw_string(0, 16, line2, 1);
    ssd1306_draw_string(0, 32, line3, 1);
    ssd1306_draw_string(0, 48, line4, 1);
    ssd1306_refresh_gram();
}
```

### Кнопка MODE

```c
void button_task(void *arg) {
    uint32_t press_start = 0;
    bool pressed = false;
    
    while (1) {
        bool current = gpio_get_level(GPIO_BUTTON_MODE) == 0;
        
        if (current && !pressed) {
            // Нажатие
            press_start = xTaskGetTickCount();
            pressed = true;
            
        } else if (!current && pressed) {
            // Отпускание
            uint32_t duration = xTaskGetTickCount() - press_start;
            pressed = false;
            
            if (duration < 1000) {
                // Короткое нажатие - переключить AUTO/MANUAL
                config.auto_mode = !config.auto_mode;
                oled_show_mode(config.auto_mode ? "AUTO" : "MANUAL");
                
            } else if (duration < 3000) {
                // 2 нажатия - сброс Buzzer
                buzzer_stop();
                
            } else if (duration < 10000) {
                // Долгое - сброс Emergency
                reset_emergency_state();
                oled_show_message("Emergency Reset");
                
            } else {
                // Очень долгое - Factory Reset
                oled_show_message("Factory Reset...");
                node_config_erase_all();
                esp_restart();
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
```

---

## 🚫 Что НЕЛЬЗЯ делать

1. ❌ **НЕ ОСТАНАВЛИВАЙ PID при потере связи**
   ```c
   // ПЛОХО:
   if (!is_online()) {
       return;  // ❌ PID остановлен!
   }
   pid_update();
   
   // ХОРОШО:
   pid_update();  // ✅ Работает всегда
   if (is_online()) {
       send_data();
   }
   ```

2. ❌ **НЕ ЖДАТЬ ответа от ROOT**
   ```c
   // ПЛОХО:
   send_command_and_wait();  // ❌ Блокировка!
   
   // ХОРОШО:
   send_command_async();  // ✅ Неблокирующая отправка
   ```

3. ❌ **НЕ ИГНОРИРУЙ аварии**
   - pH < 5.0 или > 8.0 → EMERGENCY
   - EC > 3.0 → EMERGENCY
   - Всегда логируй и сигнализируй

4. ❌ **НЕ ПЕРЕПОЛНЯЙ buffer**
   - Ring buffer 1000 записей
   - Старые вытесняются новыми

---

## ✅ Что НУЖНО делать

1. ✅ **Сохраняй ВСЁ в NVS**
   ```c
   // При изменении конфигурации
   node_config_save(&config, sizeof(config), "ph_ec_ns");
   ```

2. ✅ **Буферизуй при offline**
   ```c
   if (!is_online()) {
       local_storage_add(ph, ec);
   }
   ```

3. ✅ **Обновляй OLED постоянно**
   - Каждые 10 сек
   - Даже в автономном режиме

4. ✅ **Мониторь насосы**
   - Статистика мл/день
   - Проверка max_daily_volume
   - Cooldown между включениями

---

## 🧪 Тестирование

### Тест 1: Нормальная работа

```bash
cd node_ph_ec
idf.py set-target esp32s3
idf.py build
idf.py -p COM5 flash monitor
```

**Ожидается:**
- ✅ Чтение pH и EC
- ✅ OLED показывает данные
- ✅ PID корректирует значения
- ✅ Отправка в mesh (если ROOT online)

### Тест 2: Автономный режим

1. Прошей узел
2. Выключи ROOT
3. Подожди 30 сек

**Ожидается:**
- ⚡ OLED: "AUTONOMOUS"
- ⚡ LED желтый мигает
- ✅ PID продолжает работу
- ✅ Данные буферизуются локально

4. Включи ROOT обратно

**Ожидается:**
- ● OLED: "ONLINE"
- ● LED зеленый
- ✅ Синхронизация буфера

### Тест 3: Аварийная ситуация

Симулируй pH 4.5 (замени данные в коде временно):

**Ожидается:**
- 🔴 LED красный быстро мигает
- 🔊 Buzzer: 3 сигнала каждые 10 сек
- 🚨 OLED: "EMERGENCY pH 4.5!"
- 💧 Агрессивная коррекция pH UP
- 📡 SOS на ROOT (если online)

### Тест 4: Кнопка MODE

1. Короткое нажатие → переключение AUTO/MANUAL
2. Долгое (3 сек) → сброс Emergency
3. Очень долгое (10 сек) → Factory Reset

---

## 📊 Чек-лист разработки

- [ ] Портировать sensor_manager из hydro1.0
- [ ] Портировать pump_manager из hydro1.0
- [ ] Портировать adaptive_pid из hydro1.0
- [ ] Реализовать oled_display (SSD1306)
- [ ] Реализовать connection_monitor
- [ ] Реализовать local_storage (ring buffer)
- [ ] Реализовать buzzer_led
- [ ] Реализовать button обработчик
- [ ] NVS сохранение/загрузка конфигурации
- [ ] Тестирование нормальной работы
- [ ] Тестирование автономного режима
- [ ] Тестирование аварийных ситуаций
- [ ] Тестирование восстановления связи
- [ ] Стресс-тест (24 часа непрерывной работы)

**NODE pH/EC - критичный узел. Автономия превыше всего!** 🛡️

