# 🧪 Руководство по тестированию ручного управления насосами (node_ph)

**Дата:** 22 октября 2025  
**Узел:** node_ph (ESP32-S3 #3 или ESP32-C3)  
**Насосы:** 2 (pH UP / pH DOWN)

---

## ✅ Что было сделано

### 1. Исправлены GPIO пины насосов
```c
// node_ph/components/pump_controller/pump_controller.c
// Было: GPIO 2,3
// Стало: GPIO 1,2 (согласно MESH_PINOUT_ALL_NODES.md)
static const int PUMP_GPIO[PUMP_MAX] = {1, 2};
```

**GPIO маппинг:**
- GPIO 1 = Насос 0 (pH UP)
- GPIO 2 = Насос 1 (pH DOWN)

### 2. Добавлен Mock режим для pH датчика
```c
// node_ph/components/ph_sensor/ph_sensor.c
// Если датчик не найден - автоматически включается MOCK MODE
// pH генерируется в диапазоне 6.0-7.5 с плавными изменениями
```

**Логи Mock режима:**
```
W (82) ph_sensor: pH sensor not found at 0x0A - MOCK MODE enabled
W (83) ph_sensor: Mock pH will vary 6.0-7.5 for testing
I (92) ph_mgr: pH Manager initialized
D (135) ph_sensor: pH MOCK: 6.51  // Плавно меняется каждое чтение
```

---

## 🔗 Полная цепочка управления

### Frontend → Backend → MQTT → ROOT → Mesh → node_ph → GPIO

```
┌─────────────┐
│  Frontend   │  http://192.168.0.167:3000
│  (Vue.js)   │  PhNode.vue компонент
└──────┬──────┘
       │ POST /api/nodes/{node_id}/pump/run
       ↓
┌─────────────┐
│  Backend    │  http://192.168.0.167:8000
│  (Laravel)  │  NodeController::runPump()
└──────┬──────┘
       │ MQTT Publish: hydro/command/{node_id}
       ↓
┌─────────────┐
│  MQTT       │  mqtt://192.168.0.167:1883
│  (Mosquitto)│  Topic: hydro/command/ph_xxx
└──────┬──────┘
       │ Subscription
       ↓
┌─────────────┐
│  ROOT Node  │  ESP32-S3 (192.168.0.104)
│             │  MQTT Bridge + Data Router
└──────┬──────┘
       │ ESP-WIFI-MESH (Mesh ID: HYDRO1)
       ↓
┌─────────────┐
│  node_ph    │  ESP32-C3/S3
│             │  pH Manager
└──────┬──────┘
       │ GPIO Control
       ↓
┌─────────────┐
│  Pumps      │  GPIO 1,2
│  (Hardware) │  pH UP / pH DOWN
└─────────────┘
```

---

## 📡 API Эндпоинты

### 1. Запуск насоса вручную
```http
POST /api/nodes/{node_id}/pump/run
Content-Type: application/json

{
  "pump_id": 0,           // 0 = pH UP, 1 = pH DOWN
  "duration_sec": 5.0     // 0.1 - 30.0 секунд
}
```

**Ответ:**
```json
{
  "success": true,
  "message": "Pump 0 started for 5 seconds"
}
```

### 2. Калибровка насоса
```http
POST /api/nodes/{node_id}/pump/calibrate
Content-Type: application/json

{
  "pump_id": 0,
  "duration_sec": 10.0,
  "volume_ml": 15.5
}
```

**Что происходит:**
1. Насос запускается на 10 секунд
2. Вы измеряете фактический объем (например, 15.5 мл)
3. Система вычисляет: `ml_per_sec = 15.5 / 10.0 = 1.55 мл/с`
4. Калибровка сохраняется в БД и отправляется на узел

### 3. Получить калибровки
```http
GET /api/nodes/{node_id}/pump/calibrations
```

**Ответ:**
```json
{
  "calibrations": [
    {
      "pump_id": 0,
      "ml_per_second": 1.55,
      "calibration_volume_ml": 15.5,
      "calibration_time_ms": 10000,
      "is_calibrated": true,
      "calibrated_at": "2025-10-22T10:30:00Z"
    }
  ]
}
```

---

## 🧪 MQTT команды (для прямого тестирования)

### Команда 1: Запуск насоса вручную

**Publish:**
```bash
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ph_f5fde8" -m '{
  "type": "command",
  "node_id": "ph_f5fde8",
  "command": "run_pump_manual",
  "params": {
    "pump_id": 0,
    "duration_sec": 3.0
  }
}'
```

**Что произойдёт:**
```
ROOT → получает MQTT → пересылает в Mesh → node_ph
node_ph → ph_manager_handle_command()
        → pump_controller_run(PUMP_PH_UP, 3000 ms)
        → GPIO 1 = HIGH на 3 секунды
        → Насос работает 3 сек
        → GPIO 1 = LOW
        → Статистика обновлена
```

### Команда 2: Установка целевого pH

**Publish:**
```bash
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ph_f5fde8" -m '{
  "type": "command",
  "node_id": "ph_f5fde8",
  "command": "set_ph_target",
  "params": {
    "target": 6.5
  }
}'
```

### Команда 3: Emergency Stop

**Publish:**
```bash
mosquitto_pub -h 192.168.0.167 -t "hydro/command/ph_f5fde8" -m '{
  "type": "command",
  "node_id": "ph_f5fde8",
  "command": "emergency_stop"
}'
```

**Что произойдёт:**
- Все насосы мгновенно останавливаются
- Автономный режим отключается
- LED/Buzzer сигнализируют аварийный режим

---

## 🖥️ Тестирование через Frontend

### 1. Откройте веб-интерфейс
```
http://192.168.0.167:3000
```

### 2. Найдите узел pH
- Должен отображаться как **node_ph** или **ph_f5fde8**
- Статус: **Online** (зелёный чип)

### 3. Секция "Ручное управление насосами"

#### Pump 0 (pH UP):
- Иконка: ⬆️ (зеленая стрелка вверх)
- Название: **pH UP**
- Поле ввода: Длительность (сек)
- Кнопка: **Запустить** (зеленая)

#### Pump 1 (pH DOWN):
- Иконка: ⬇️ (красная стрелка вниз)
- Название: **pH DOWN**
- Поле ввода: Длительность (сек)
- Кнопка: **Запустить** (красная)

### 4. Тест запуска насоса

**Шаг 1:** Введите длительность (например, 2.5 секунд)  
**Шаг 2:** Нажмите **Запустить**  
**Ожидаемый результат:**
- ✅ Кнопка показывает loading индикатор
- ✅ Toast уведомление: "Насос 0 запущен на 2.5 сек"
- ✅ GPIO 1 переходит в HIGH на 2.5 секунды
- ✅ Если подключен насос - он работает
- ✅ Логи ESP32: `I pump_ctrl: Pump 0 START (2500 ms)`

---

## 📊 Мониторинг через логи

### ESP32 логи (node_ph)
```bash
# Подключитесь к node_ph через USB
cd node_ph
idf.py monitor
```

**Ожидаемые логи при запуске насоса:**
```
I (12345) ph_mgr: Command received: run_pump_manual
I (12346) ph_mgr: Manual pump run: PUMP_UP for 3.0 sec
I (12347) pump_ctrl: Pump 0 START (3000 ms)
I (15348) pump_ctrl: Pump 0 STOP (6.00 ml, 3000 ms)
```

### MQTT Listener логи (Backend)
```bash
docker logs hydro_mqtt_listener -f
```

**При отправке команды:**
```
📤 Publishing to hydro/command/ph_f5fde8
   {"type":"command","node_id":"ph_f5fde8","command":"run_pump_manual",...}
```

---

## 🔧 Калибровка насосов (важно для точности!)

### Процедура калибровки:

#### 1. Подготовка:
- Мерный стакан (или шприц) на 50-100 мл
- Жидкость для теста (вода)
- Узел подключен и Online

#### 2. В веб-интерфейсе:
**Секция "Калибровка насосов"**

**Для Pump 0 (pH UP):**
1. Введите время работы: `10.0` секунд
2. Нажмите кнопку калибровки
3. **ВАЖНО:** Насос запустится на 10 секунд!
4. Измерьте фактический объем (например, 18.2 мл)
5. Введите измеренный объем: `18.2` мл
6. Нажмите "Сохранить"

**Результат:**
```
Производительность = 18.2 мл / 10 сек = 1.82 мл/с
```

#### 3. Проверка калибровки:
После калибровки рядом с насосом появится зелёный чип:
```
✅ 1.82 мл/с
```

#### 4. Использование калибровки:
Теперь при автономном управлении:
```
Нужная доза = 5.0 мл
Время работы = 5.0 мл / 1.82 мл/с ≈ 2.7 секунды
```

---

## ⚠️ Безопасность и ограничения

### Программные ограничения:
| Параметр | Значение | Примечание |
|----------|----------|------------|
| Макс время работы | 10 секунд | `MAX_RUN_TIME_MS` в pump_controller.c |
| Макс доза | 5.0 мл | Adaptive PID safety limit |
| Мин интервал между дозами | 60 секунд | Adaptive PID safety |
| Мин длительность | 0.1 сек | API validation |
| Макс длительность (API) | 30 сек | API validation |

### Аварийная защита:
- ✅ Автоостановка по таймеру
- ✅ Emergency stop command
- ✅ Mesh disconnect → stop all pumps
- ✅ Critical pH → emergency mode

---

## 🧪 Пошаговый тест (для первого запуска)

### Тест 1: Проверка Mock pH
```bash
cd node_ph
idf.py monitor
```

**Ожидаемо:**
```
W ph_sensor: pH sensor not found - MOCK MODE enabled
I ph_mgr: pH Manager initialized
D ph_sensor: pH MOCK: 6.51
D ph_sensor: pH MOCK: 6.52
D ph_sensor: pH MOCK: 6.53
```

### Тест 2: Регистрация узла
**В браузере:**
```
http://192.168.0.167:3000
```

**Должен появиться:**
- Узел `ph_xxxxxx`
- Статус: Online ✅
- Тип: pH
- Actuators: pump_ph_up, pump_ph_down

### Тест 3: Запуск насоса через Frontend
1. Найдите узел pH
2. Введите 2 секунды
3. Нажмите "Запустить" для pH UP
4. Проверьте логи ESP32

**Ожидаемо в логах:**
```
I ph_mgr: Command received: run_pump_manual
I pump_ctrl: Pump 0 START (2000 ms)
I pump_ctrl: Pump 0 STOP (4.00 ml, 2000 ms)
```

### Тест 4: Подключите LED к GPIO 1
- Подключите LED + резистор 220 Ом к GPIO 1
- Запустите насос через веб-интерфейс
- **LED должен загореться на указанное время!** ✅

### Тест 5: Подключите реальный насос
- **ВНИМАНИЕ:** Убедитесь что питание насоса отдельное!
- ESP32 GPIO → транзистор/реле → насос
- Запустите тест на 1 секунду
- Измерьте объем → откалибруйте

---

## 📝 Troubleshooting

### Проблема: Узел не появляется во frontend

**Проверка:**
```bash
# 1. Узел подключен к Mesh?
docker logs hydro_mqtt_listener | grep discovery

# 2. Узел в БД?
docker exec hydro_backend php artisan tinker
>>> \App\Models\Node::where('node_type', 'ph')->get()
```

### Проблема: Команда не доходит до узла

**Проверка:**
```bash
# 1. MQTT топик
mosquitto_sub -h 192.168.0.167 -t "hydro/#" -v

# 2. ROOT узел получает?
# Проверьте логи ROOT узла

# 3. Mesh связь есть?
# В логах node_ph должно быть: "Mesh connected"
```

### Проблема: Насос не работает

**Проверка:**
1. **GPIO правильный?** GPIO 1,2 (не 2,3!)
2. **Питание насоса?** Должно быть отдельное!
3. **Транзистор/реле?** Проверьте схему
4. **Тест LED?** Сначала протестируйте с LED

---

## ✅ Чеклист готовности к production

- [ ] node_ph прошит и подключен к Mesh
- [ ] Узел появился в веб-интерфейсе
- [ ] Mock pH работает (если нет реального датчика)
- [ ] LED тест на GPIO 1,2 прошёл успешно
- [ ] Насосы подключены через транзисторы/реле
- [ ] Калибровка насосов выполнена
- [ ] Ручной запуск работает через Frontend
- [ ] Emergency stop протестирован
- [ ] Автономный режим настроен (если нужен)

---

**Готово к тестированию! 🚀**

**Следующий шаг:** Подключите реальный pH датчик Trema (I2C 0x0A) для перехода из Mock в Real режим.

