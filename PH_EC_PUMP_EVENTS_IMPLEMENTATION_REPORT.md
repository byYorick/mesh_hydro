# 🚰 Отчет: Система событий насосов для нод pH и EC

## 📋 Обзор

Реализована система событий насосов для отдельных нод `node_ph` и `node_ec` с полными данными PID контроллеров. Каждая нода отправляет события при включении/выключении своих насосов с детальной информацией о состоянии системы.

## 🏗️ Архитектура системы

### **Компоненты для node_ph:**
- **`node_ph/components/pump_events/`** - Система событий для pH насосов
- **`ph_manager`** - Интеграция с pH менеджером
- **Поддержка насосов:** PH_UP, PH_DOWN

### **Компоненты для node_ec:**
- **`node_ec/components/pump_events/`** - Система событий для EC насосов  
- **`ec_manager`** - Интеграция с EC менеджером
- **Поддержка насосов:** EC_A, EC_B, EC_C

## 🔧 Реализованные функции

### **1. Система событий для node_ph:**

#### **Структура события pH:**
```c
typedef struct {
    pump_event_type_t type;         // Тип события
    pump_id_t pump_id;              // ID насоса (PH_UP или PH_DOWN)
    uint32_t timestamp;             // Время события
    uint32_t duration_ms;           // Длительность работы
    float dose_ml;                  // Доза в мл
    float ml_per_second;            // Производительность
    pump_event_pid_data_t pid_data; // Данные PID
    float current_ph;               // Текущий pH
    float ph_target;                // Целевой pH
    bool emergency_mode;            // Режим аварии
    bool autonomous_mode;           // Автономный режим
    int8_t rssi;                    // RSSI к родительскому узлу
} pump_event_t;
```

#### **API функции для pH:**
```c
esp_err_t pump_events_send_start_event(
    pump_id_t pump_id,              // PH_UP или PH_DOWN
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ph,
    float ph_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);

esp_err_t pump_events_send_stop_event(
    pump_id_t pump_id,
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ph,
    float ph_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);
```

### **2. Система событий для node_ec:**

#### **Структура события EC:**
```c
typedef struct {
    pump_event_type_t type;         // Тип события
    pump_id_t pump_id;              // ID насоса (EC_A, EC_B, EC_C)
    uint32_t timestamp;             // Время события
    uint32_t duration_ms;           // Длительность работы
    float dose_ml;                  // Доза в мл
    float ml_per_second;            // Производительность
    pump_event_pid_data_t pid_data; // Данные PID
    float current_ec;               // Текущий EC
    float ec_target;                // Целевой EC
    bool emergency_mode;            // Режим аварии
    bool autonomous_mode;           // Автономный режим
    int8_t rssi;                    // RSSI к родительскому узлу
} pump_event_t;
```

#### **API функции для EC:**
```c
esp_err_t pump_events_send_start_event(
    pump_id_t pump_id,              // EC_A, EC_B или EC_C
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ec,
    float ec_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);
```

## 🔄 Интеграция с менеджерами

### **1. ph_manager.c:**
- ✅ Добавлен include `pump_events.h`
- ✅ Обновлена функция `control_ph()` для отправки событий
- ✅ Отправка событий при включении PH_UP и PH_DOWN насосов
- ✅ Получение реальных данных PID из адаптивного контроллера
- ✅ Обновлен CMakeLists.txt с зависимостью `pump_events`

### **2. ec_manager.c:**
- ✅ Добавлен include `pump_events.h`
- ✅ Обновлена функция `control_ec()` для отправки событий
- ✅ Отправка событий для всех 3 насосов EC (A, B, C)
- ✅ Распределение доз: A=50%, B=40%, C=10%
- ✅ Получение реальных данных PID из адаптивного контроллера
- ✅ Обновлен CMakeLists.txt с зависимостью `pump_events`

## 📊 Формат JSON событий

### **Пример события pH насоса:**
```json
{
  "type": "event",
  "node_id": "ph_001",
  "node_type": "ph",
  "timestamp": 1703123456,
  "level": "info",
  "data": {
    "event_type": "pump_start",
    "pump_id": 0,
    "duration_ms": 3000,
    "dose_ml": 1.5,
    "ml_per_second": 2.0,
    "pid_data": {
      "kp": 1.5,
      "ki": 0.2,
      "kd": 0.8,
      "setpoint": 6.5,
      "current_value": 6.0,
      "error": 0.5,
      "output": 1.5,
      "integral": 0.0,
      "derivative": 0.0,
      "enabled": true
    },
    "current_ph": 6.0,
    "ph_target": 6.5,
    "emergency_mode": false,
    "autonomous_mode": false,
    "rssi": -45
  }
}
```

### **Пример события EC насоса:**
```json
{
  "type": "event",
  "node_id": "ec_001",
  "node_type": "ec",
  "timestamp": 1703123456,
  "level": "info",
  "data": {
    "event_type": "pump_start",
    "pump_id": 0,
    "duration_ms": 2000,
    "dose_ml": 2.5,
    "ml_per_second": 2.0,
    "pid_data": {
      "kp": 1.2,
      "ki": 0.15,
      "kd": 0.6,
      "setpoint": 1.5,
      "current_value": 1.2,
      "error": 0.3,
      "output": 2.5,
      "integral": 0.0,
      "derivative": 0.0,
      "enabled": true
    },
    "current_ec": 1.2,
    "ec_target": 1.5,
    "emergency_mode": false,
    "autonomous_mode": false,
    "rssi": -45
  }
}
```

## 🎯 Особенности реализации

### **1. node_ph (pH нода):**
- 🔄 **2 насоса:** PH_UP и PH_DOWN
- 📊 **PID данные:** Адаптивный PID с зонами управления
- 🎯 **Целевое значение:** pH target
- 📈 **Мониторинг:** Текущий pH vs целевой pH

### **2. node_ec (EC нода):**
- 🔄 **3 насоса:** EC_A, EC_B, EC_C
- 📊 **PID данные:** Адаптивный PID с зонами управления
- 🎯 **Целевое значение:** EC target
- 📈 **Мониторинг:** Текущий EC vs целевой EC
- ⚖️ **Распределение доз:** A=50%, B=40%, C=10%

## 🧪 Тестирование

### **Тестовые файлы:**
- `test_ph_pump_events.json` - Примеры событий pH ноды
- `test_ec_pump_events.json` - Примеры событий EC ноды

### **Сценарии тестирования:**
1. **pH нода:** Включение/выключение PH_UP и PH_DOWN насосов
2. **EC нода:** Включение/выключение всех 3 насосов EC
3. **Аварийные остановки:** При превышении лимитов
4. **Автономный режим:** Работа без связи с сервером

## 📁 Созданные файлы

### **node_ph:**
- `node_ph/components/pump_events/pump_events.h`
- `node_ph/components/pump_events/pump_events.c`
- `node_ph/components/pump_events/CMakeLists.txt`

### **node_ec:**
- `node_ec/components/pump_events/pump_events.h`
- `node_ec/components/pump_events/pump_events.c`
- `node_ec/components/pump_events/CMakeLists.txt`

### **Тестовые файлы:**
- `test_ph_pump_events.json`
- `test_ec_pump_events.json`

### **Обновленные файлы:**
- `node_ph/components/ph_manager/ph_manager.c`
- `node_ph/components/ph_manager/CMakeLists.txt`
- `node_ec/components/ec_manager/ec_manager.c`
- `node_ec/components/ec_manager/CMakeLists.txt`

## 🚀 Готовность к использованию

### **✅ Реализовано:**
- [x] Система событий для node_ph
- [x] Система событий для node_ec
- [x] Интеграция с ph_manager
- [x] Интеграция с ec_manager
- [x] Отправка на сервер через MQTT
- [x] Полные данные PID
- [x] Обработка аварийных ситуаций

### **🔄 Следующие шаги:**
1. Тестирование на реальном оборудовании
2. Настройка обработки событий на сервере
3. Создание дашборда для мониторинга pH и EC нод
4. Настройка алертов для критических событий

## 📝 Заключение

Система событий насосов полностью реализована для нод pH и EC. Каждая нода теперь автоматически отправляет детальные события при работе насосов с полными данными PID контроллеров, что обеспечивает полную трассируемость и диагностику системы гидропоники.

**Система готова к развертыванию! 🎉**
