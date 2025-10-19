# ⚠️ NODE pH/EC - MOCK MODE (БЕЗ ДАТЧИКОВ)

**Текущая конфигурация: Симуляция данных**

---

## 🎯 ЧТО АКТИВИРОВАНО

### ✅ MOCK MODE:

- 🔴 **I2C ОТКЛЮЧЁН** - не требуется физическое подключение датчиков
- 🟢 **Моковые значения** - генерируются программно
- 🟢 **PID работает** - корректирует моковые значения
- 🟢 **Насосы работают** - PWM выходы активны (но можно не подключать физические насосы)
- 🟢 **Mesh работает** - отправка данных на ROOT
- 🟢 **Telemetry работает** - полный JSON с моковыми данными

---

## 📊 МОКОВЫЕ ЗНАЧЕНИЯ

### pH:
```
Базовое значение: 6.5
Вариация: ±0.3
Диапазон: 6.2 - 6.8
Изменение: случайное каждую секунду
```

### EC:
```
Базовое значение: 2.5
Вариация: ±0.2
Диапазон: 2.3 - 2.7
Изменение: случайное каждую секунду
```

### Temperature:
```
Фиксированное: 22.5°C
```

---

## 🔧 КОД МОКОВОЙ СИМУЛЯЦИИ

### app_main.c (строки 79-94):

```c
// [Step 3/9] I2C init - ОТКЛЮЧЁН ДЛЯ ТЕСТИРОВАНИЯ БЕЗ ДАТЧИКОВ
ESP_LOGI(TAG, "[Step 3/9] I2C init - DISABLED (Mock mode)");
ESP_LOGW(TAG, "  ⚠️ I2C ОТКЛЮЧЁН - используются моковые значения!");
// ESP_ERROR_CHECK(i2c_master_init());  // ← Закомментировано

// [Step 4/9] Sensors init - ОТКЛЮЧЕНЫ
ESP_LOGI(TAG, "[Step 4/9] Sensors init - MOCK MODE");
ESP_LOGW(TAG, "  ⚠️ ДАТЧИКИ ОТКЛЮЧЕНЫ - симуляция данных!");
ESP_LOGW(TAG, "  Mock pH: 6.5 ± 0.3");
ESP_LOGW(TAG, "  Mock EC: 2.5 ± 0.2");

ESP_LOGI(TAG, "  ✅ MOCK MODE активирован - датчики не требуются!");
```

### ph_ec_manager.c read_sensors() (строки 220-265):

```c
static void read_sensors(void) {
    // Генерация случайных отклонений
    static float ph_variation = 0.0f;
    static float ec_variation = 0.0f;
    
    // Случайное изменение ±0.05 каждую секунду
    ph_variation += ((float)(esp_random() % 100) - 50.0f) / 1000.0f;
    ec_variation += ((float)(esp_random() % 100) - 50.0f) / 1000.0f;
    
    // Ограничение вариаций
    if (ph_variation > 0.3f) ph_variation = 0.3f;
    if (ph_variation < -0.3f) ph_variation = -0.3f;
    if (ec_variation > 0.2f) ec_variation = 0.2f;
    if (ec_variation < -0.2f) ec_variation = -0.2f;
    
    // Моковые значения
    s_current_ph = 6.5f + ph_variation;
    s_current_ec = 2.5f + ec_variation;
}
```

---

## 🎮 ЧТО БУДЕТ РАБОТАТЬ БЕЗ ДАТЧИКОВ

### ✅ Полностью работает:

1. **Mesh Network**
   - Подключение к ROOT
   - Layer 2
   - RSSI мониторинг

2. **Heartbeat**
   - Каждые 5 секунд
   - Отправка на ROOT → MQTT → Backend

3. **Telemetry**
   - Каждые 10 секунд
   - Моковые pH/EC данные
   - Статистика насосов

4. **PID Контроллер**
   - Расчёт дозировки
   - Активация насосов
   - Логирование действий

5. **Команды от ROOT**
   - set_targets
   - manual_pump
   - set_mode
   - emergency_stop

6. **Автономный режим**
   - Переключение при offline ROOT
   - Буферизация данных
   - Восстановление связи

---

## 📈 ПРИМЕР ЛОГОВ (Mock Mode)

### При запуске:
```
I (79) ph_ec_node: [Step 3/9] I2C init - DISABLED (Mock mode)
W (80) ph_ec_node:   ⚠️ I2C ОТКЛЮЧЁН - используются моковые значения!
I (85) ph_ec_node: [Step 4/9] Sensors init - MOCK MODE
W (90) ph_ec_node:   ⚠️ ДАТЧИКИ ОТКЛЮЧЕНЫ - симуляция данных!
W (95) ph_ec_node:   Mock pH: 6.5 ± 0.3
W (100) ph_ec_node:   Mock EC: 2.5 ± 0.2
I (105) ph_ec_node:   ✅ MOCK MODE активирован - датчики не требуются!
```

### Во время работы (каждую секунду):
```
D (1000) ph_ec_mgr: 📊 Mock sensors: pH=6.48, EC=2.52
D (1001) ph_ec_mgr: ✓ pH OK (target 6.50), EC OK (target 2.50)

D (2000) ph_ec_mgr: 📊 Mock sensors: pH=6.53, EC=2.47
D (2001) ph_ec_mgr: ✓ pH OK (target 6.50), EC OK (target 2.50)

D (3000) ph_ec_mgr: 📊 Mock sensors: pH=6.45, EC=2.55
D (3001) ph_ec_mgr: ✓ pH OK (target 6.50), EC OK (target 2.50)
```

### PID активация (когда pH выходит за deadband ±0.1):
```
I (5000) ph_ec_mgr: 📊 Mock sensors: pH=6.35, EC=2.48
I (5005) ph_ec_mgr: pH UP: 0.3 ml (current=6.35, target=6.50)
I (5010) pump_ctrl: Pump pH UP running (dose=0.3 ml, 600 ms)
I (5615) pump_ctrl: Pump pH UP stopped (total: 0.3 ml)
```

---

## 🔄 КАК ВКЛЮЧИТЬ РЕАЛЬНЫЕ ДАТЧИКИ

### Шаг 1: Раскомментируй I2C

В `node_ph_ec/main/app_main.c` (строка 82):

```c
// БЫЛО (mock):
// ESP_ERROR_CHECK(i2c_master_init());  // ← Закомментировано

// СТАЛО (production):
ESP_ERROR_CHECK(i2c_master_init());  // ← Раскомментировать!
```

### Шаг 2: Раскомментируй датчики

В `node_ph_ec/main/app_main.c` (строки 91-92):

```c
// БЫЛО (mock):
// esp_err_t ret_ph = ph_sensor_init(I2C_MASTER_NUM);  // ← Закомментировано
// esp_err_t ret_ec = ec_sensor_init(I2C_MASTER_NUM);  // ← Закомментировано

// СТАЛО (production):
ESP_LOGI(TAG, "  - pH sensor (0x%02X)...", PH_SENSOR_ADDR);
esp_err_t ret_ph = ph_sensor_init(I2C_MASTER_NUM);
if (ret_ph != ESP_OK) {
    ESP_LOGW(TAG, "    WARNING: pH sensor init failed.");
} else {
    ESP_LOGI(TAG, "    OK");
}

ESP_LOGI(TAG, "  - EC sensor (0x%02X)...", EC_SENSOR_ADDR);
esp_err_t ret_ec = ec_sensor_init(I2C_MASTER_NUM);
if (ret_ec != ESP_OK) {
    ESP_LOGW(TAG, "    WARNING: EC sensor init failed.");
} else {
    ESP_LOGI(TAG, "    OK");
}
```

### Шаг 3: Раскомментируй реальное чтение

В `node_ph_ec/components/ph_ec_manager/ph_ec_manager.c` (строки 244-265):

```c
// БЫЛО (mock):
s_current_ph = 6.5f + ph_variation;
s_current_ec = 2.5f + ec_variation;

// СТАЛО (production):
// Закомментируй моковые значения и раскомментируй:
esp_err_t ret;

// Чтение pH
ret = ph_sensor_read(&s_current_ph);
if (ret != ESP_OK) {
    ESP_LOGW(TAG, "pH sensor read failed");
}

// Чтение EC
ret = ec_sensor_read(&s_current_ec);
if (ret != ESP_OK) {
    ESP_LOGW(TAG, "EC sensor read failed");
}

// Компенсация температуры для EC
ec_sensor_set_temperature(25.0f);
```

### Шаг 4: Пересобери

```batch
cd node_ph_ec
C:\Espressif\idf_cmd_init.bat esp-idf-1dcc643656a1439837fdf6ab63363005
idf.py fullclean
idf.py build
idf.py -p COM9 flash monitor
```

---

## 🧪 ТЕСТИРОВАНИЕ MOCK MODE

### Запусти узел:

```batch
tools\flash_ph_ec.bat
```

### Ожидаемые логи:

```
I (80) ph_ec_node:   ⚠️ I2C ОТКЛЮЧЁН - используются моковые значения!
I (85) ph_ec_node: [Step 4/9] Sensors init - MOCK MODE
W (90) ph_ec_node:   ⚠️ ДАТЧИКИ ОТКЛЮЧЕНЫ - симуляция данных!
I (105) ph_ec_node:   ✅ MOCK MODE активирован - датчики не требуются!
...
I (5000) mesh_manager: ✓ MESH Parent connected!
I (5005) mesh_manager: Layer: 2
...
D (10000) ph_ec_mgr: 📊 Mock sensors: pH=6.48, EC=2.52
I (10010) ph_ec_mgr: 📊 Telemetry: pH=6.48, EC=2.52, RSSI=-45
I (15000) ph_ec_mgr: 💓 Heartbeat sent (uptime=10s, heap=165KB, RSSI=-43)
```

### Проверь Dashboard:

Открой: http://localhost:3000

**Должен появиться:**
- ✅ `ph_ec_001` (ph_ec) - **ONLINE**
- ✅ pH: 6.5 ± 0.3
- ✅ EC: 2.5 ± 0.2

---

## 🎯 ПРЕИМУЩЕСТВА MOCK MODE

### ✅ Для разработки:
- Не нужны физические датчики Trema (дорогие!)
- Не нужны насосы и растворы
- Можно тестировать mesh и backend
- Можно проверять Dashboard
- Можно тестировать команды

### ✅ Для отладки:
- Детерминированное поведение
- Легко воспроизвести сценарии
- Быстрая итерация (не ждать изменения pH в растворе)
- Безопасно (не портится раствор)

### ✅ Для демонстрации:
- Показать систему без установки оборудования
- Демо для заказчика
- Обучение новых разработчиков

---

## 🔌 ПОДКЛЮЧЕНИЕ РЕАЛЬНЫХ ДАТЧИКОВ (PRODUCTION)

### Когда будешь готов:

1. **Купи датчики:**
   - Trema pH Sensor (I2C 0x4D) ~$30
   - Trema EC Sensor (I2C 0x64) ~$30

2. **Подключи к ESP32-S3:**
   ```
   pH Sensor:
     VCC → 3.3V
     GND → GND
     SDA → GPIO 18
     SCL → GPIO 17
   
   EC Sensor:
     VCC → 3.3V
     GND → GND
     SDA → GPIO 18 (shared)
     SCL → GPIO 17 (shared)
   ```

3. **Раскомментируй код** (см. инструкцию выше)

4. **Калибруй датчики:**
   ```bash
   # pH 7.0 буфер:
   mosquitto_pub -h 192.168.1.100 -t hydro/command/ph_ec_001 -m '{
     "type": "command",
     "node_id": "ph_ec_001",
     "command": "calibrate_ph",
     "params": {"buffer_ph": 7.0}
   }'
   
   # EC 2.77 mS/cm раствор:
   mosquitto_pub -h 192.168.1.100 -t hydro/command/ph_ec_001 -m '{
     "type": "command",
     "node_id": "ph_ec_001",
     "command": "calibrate_ec",
     "params": {"solution_ec": 2.77}
   }'
   ```

5. **Пересобери и прошей:**
   ```batch
   tools\flash_ph_ec.bat
   ```

---

## 📋 СРАВНЕНИЕ: Mock vs Production

| Функция | Mock Mode | Production Mode |
|---------|-----------|-----------------|
| **I2C** | ❌ Отключён | ✅ Активен |
| **pH Sensor** | 🟡 Симуляция | ✅ Реальное чтение |
| **EC Sensor** | 🟡 Симуляция | ✅ Реальное чтение |
| **Насосы PWM** | ✅ Работают | ✅ Работают |
| **PID контроллер** | ✅ Работает | ✅ Работает |
| **Mesh сеть** | ✅ Работает | ✅ Работает |
| **Telemetry** | ✅ Моковые данные | ✅ Реальные данные |
| **Команды** | ✅ Обрабатываются | ✅ Обрабатываются |
| **Калибровка** | ❌ Не нужна | ✅ Обязательна |

---

## 🎉 СЕЙЧАС МОЖНО ТЕСТИРОВАТЬ:

### Без физического оборудования:

```batch
# 1. Запусти Backend
tools\server_start.bat

# 2. Прошей ROOT
tools\flash_root.bat

# 3. Прошей pH/EC (MOCK MODE)
tools\flash_ph_ec.bat

# 4. Открой Dashboard
start http://localhost:3000
```

**Увидишь:**
- ✅ `root_98a316f5fde8` - ONLINE
- ✅ `climate_001` - ONLINE (если прошит)
- ✅ `ph_ec_001` - ONLINE (моковые данные!)

**На карточке pH/EC:**
```
pH: 6.5 (target 6.5)
EC: 2.5 (target 2.5)
Status: ONLINE
Mode: Online
```

---

## 🔮 ПЛАНЫ НА PRODUCTION

### Когда будут датчики:

1. Раскомментируй 3 блока кода (см. выше)
2. Пересобери: `idf.py fullclean && idf.py build`
3. Прошей: `idf.py -p COM9 flash monitor`
4. Калибруй pH (буфер 7.0 и 4.0)
5. Калибруй EC (раствор 2.77 mS/cm)
6. Готово! Реальные датчики работают.

---

**СЕЙЧАС: РАБОТАЕТ БЕЗ ДАТЧИКОВ! МОЖНО ТЕСТИРОВАТЬ MESH И BACKEND!** 🎮✅

