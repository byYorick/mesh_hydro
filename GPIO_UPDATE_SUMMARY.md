# ⚡ Обновление GPIO для node_ph

**Дата:** 22 октября 2025

---

## ✅ GPIO обновлены на реальную распайку

### Было (по документации):
```c
GPIO 1  = Pump 0 (pH UP)
GPIO 2  = Pump 1 (pH DOWN)
```

### Стало (реальная плата ESP32):
```c
GPIO 12 = Pump 0 (pH UP)
GPIO 13 = Pump 1 (pH DOWN)
```

---

## 📝 Обновлённые файлы

1. ✅ `node_ph/components/pump_controller/pump_controller.c`
   ```c
   static const int PUMP_GPIO[PUMP_MAX] = {12, 13};
   ```

2. ✅ `node_ph/main/app_main.c`
   ```c
   ESP_LOGI(TAG, "  - 2 pumps ready (GPIO 12,13 - pH UP/DOWN)");
   ```

3. ✅ `node_ph/QUICK_TEST_GUIDE.md` - обновлены инструкции

4. ✅ `node_ph/GPIO_CONFIG.md` - создана новая документация

---

## 🔧 Схема подключения

```
┌──────────┐
│  ESP32   │
└─────┬────┘
      │
      ├─ GPIO 12 → MOSFET → Pump pH UP (12V)
      │              ↓
      │         Питание 12V
      │
      └─ GPIO 13 → MOSFET → Pump pH DOWN (12V)
                     ↓
                Питание 12V
```

### Рекомендуемая схема с MOSFET:

```
ESP32 GPIO 12 (3.3V)
     │
     ├─ 10kΩ → GND (pull-down)
     │
     └─ MOSFET Gate (IRLZ44N)
           │
           ├─ Drain → Pump - (минус)
           └─ Source → GND

Pump + (плюс) → 12V питание
```

---

## 🧪 Быстрый тест

### 1. Тест LED:
```
GPIO 12 → LED + резистор 220Ω → GND
GPIO 13 → LED + резистор 220Ω → GND
```

### 2. Запуск через веб-интерфейс:
```
http://192.168.0.167:3000
→ Найти узел pH
→ Pump 0 → 2 секунды → Запустить
→ LED на GPIO 12 загорится! ✅
```

### 3. Проверка в логах:
```bash
cd node_ph
idf.py monitor
```

Должно быть:
```
I (93) ph_node: [Step 5/8] Pumps init (2x PWM)...
I (95) pump_ctrl: Pump 0 initialized (GPIO 12)
I (96) pump_ctrl: Pump 1 initialized (GPIO 13)
I (97) pump_ctrl: Pump controller ready (pH UP/DOWN)
I (98) ph_node:   - 2 pumps ready (GPIO 12,13 - pH UP/DOWN)
```

---

## 📋 I2C настройки (не изменены)

```c
GPIO 9  = SCL (I2C Clock)
GPIO 8  = SDA (I2C Data)
```

**Устройства:**
- Trema pH (0x0A) - датчик pH
- OLED SSD1306 (0x3C) - дисплей (опционально)

---

## ⚠️ Важные примечания

### GPIO 12 Bootloader warning:
GPIO 12 используется при загрузке для определения напряжения Flash (3.3V vs 1.8V).

**Если возникают проблемы с загрузкой:**
- При включении GPIO 12 должен быть LOW
- Или переключитесь на другие GPIO (например, 25, 26)

**Для текущего использования это не проблема**, так как:
- Насосы по умолчанию выключены (GPIO LOW)
- PWM начинается с 0% (LOW)

---

## 🚀 Пересборка и прошивка

```bash
cd node_ph
rm -rf build  # Очистка старой сборки (опционально)
idf.py build
idf.py flash monitor
```

**После прошивки проверьте:**
```
I pump_ctrl: Pump 0 initialized (GPIO 12)  ✅
I pump_ctrl: Pump 1 initialized (GPIO 13)  ✅
```

---

## ✅ Готово!

Теперь node_ph использует **GPIO 12 и 13** для насосов.

**Следующий шаг:** Подключите LED к GPIO 12 и протестируйте через веб-интерфейс!

