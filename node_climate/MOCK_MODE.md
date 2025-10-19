# 🧪 MOCK MODE - Режим работы без датчиков

## 📝 Описание

В проекте активирован **MOCK MODE** - все датчики работают с фиктивными данными вместо реального чтения по I2C.

## ✅ Что сделано

### 1. **app_main.c**
- ❌ Отключена инициализация I2C (строки 116-125)
- ✅ Датчики инициализируются с кодом `ESP_OK` (без реального оборудования)
- 📝 Добавлены логи `"I2C DISABLED - Running in MOCK mode"`

### 2. **sht3x_driver.c** (Температура + Влажность)
- 🔧 Добавлен флаг `#define SHT3X_MOCK_MODE 1`
- 📊 Моковые данные:
  - Температура: **22°C ± 1°C** (случайные колебания)
  - Влажность: **60% ± 2%** (случайные колебания)
- ✅ Функция `sht3x_read()` возвращает синтетические данные

### 3. **ccs811_driver.c** (CO2 + TVOC)
- 🔧 Добавлен флаг `#define CCS811_MOCK_MODE 1`
- 📊 Моковые данные:
  - CO2: **450 ppm ± 50 ppm** (диапазон 400-2000)
  - TVOC: **50 ppb ± 10 ppb** (диапазон 0-1000)
- ✅ Функция `ccs811_read()` возвращает синтетические данные

### 4. **lux_sensor.c** (Освещенность)
- 🔧 Добавлен флаг `#define LUX_SENSOR_MOCK_MODE 1`
- 📊 Моковые данные:
  - Lux: **500 lux ± 100 lux** (диапазон 0-2000)
- ✅ Функция `lux_sensor_read()` возвращает синтетические данные

## 🎯 Преимущества MOCK MODE

1. ✅ **Тестирование без оборудования** - можно проверить логику без датчиков
2. ✅ **Нет ошибок I2C** - не требуется подключение физических устройств
3. ✅ **Стабильные данные** - предсказуемые значения для отладки
4. ✅ **Быстрая разработка** - работа с кодом без аппаратной настройки

## 📊 Ожидаемый вывод

```
I CLIMATE: [Step 3/7] Initializing I2C...
W CLIMATE: I2C DISABLED - Running in MOCK mode
I CLIMATE: I2C MOCK: Skipping hardware init
I CLIMATE: [Step 4/7] Initializing Sensors...
W CLIMATE: SENSORS DISABLED - Running in MOCK mode
I CLIMATE:   - SHT3x (temp + humidity)... MOCK
I CLIMATE:   - CCS811 (CO2)... MOCK
I CLIMATE:   - Lux sensor... MOCK
I CLIMATE: All sensors initialized in MOCK mode
```

При чтении данных:
```
D sht3x: SHT3x MOCK: 22.34°C, 61.2%
D ccs811: CCS811 MOCK: CO2=475 ppm, TVOC=55 ppb
D lux_sensor: Lux MOCK: 520
```

## 🔄 Как вернуться к реальным датчикам

Чтобы включить работу с реальными датчиками:

### 1. В `app_main.c` (строка 115):
Раскомментируйте инициализацию I2C:
```c
// Удалите строку:
ESP_LOGW(TAG, "I2C DISABLED - Running in MOCK mode");

// Раскомментируйте:
i2c_config_t i2c_conf = {
    .mode = I2C_MODE_MASTER,
    .sda_io_num = GPIO_I2C_SDA,
    .scl_io_num = GPIO_I2C_SCL,
    .sda_pullup_en = GPIO_PULLUP_ENABLE,
    .scl_pullup_en = GPIO_PULLUP_ENABLE,
    .master.clk_speed = I2C_FREQ_HZ,
};
ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &i2c_conf));
ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
```

### 2. В драйверах датчиков:
Измените флаги на `0`:
```c
// sht3x_driver.c (строка 14)
#define SHT3X_MOCK_MODE 0

// ccs811_driver.c (строка 13)
#define CCS811_MOCK_MODE 0

// lux_sensor.c (строка 13)
#define LUX_SENSOR_MOCK_MODE 0
```

### 3. Пересоберите проект:
```bash
idf.py build
idf.py -p COM4 flash monitor
```

## 🧪 Использование

**Текущий режим:** ✅ **MOCK MODE активен**

NODE Climate будет:
- ✅ Работать без физических датчиков
- ✅ Генерировать реалистичные данные
- ✅ Отправлять телеметрию на ROOT
- ✅ Подключаться к Mesh сети

**Идеально для:**
- 🔍 Тестирования Mesh протокола
- 🔍 Отладки логики контроллера
- 🔍 Проверки связи с ROOT
- 🔍 MQTT публикаций

---

**Дата изменений:** 19.10.2025  
**Версия:** v1.0 (MOCK MODE)

