# 🚰 Отчет: Система событий насосов с данными PID

## 📋 Обзор

Реализована комплексная система событий для отслеживания работы насосов pH/EC с полными данными PID контроллеров. Система отправляет события на сервер при включении, выключении и аварийных остановках насосов.

## 🏗️ Архитектура системы

### **Компоненты:**

1. **`pump_events`** - Основной компонент системы событий
2. **`pump_controller`** - Интеграция с контроллером насосов
3. **`ph_ec_manager`** - Интеграция с PID управлением

### **Структура данных:**

```c
typedef struct {
    pump_event_type_t type;         // Тип события
    pump_id_t pump_id;              // ID насоса
    uint32_t timestamp;             // Время события
    uint32_t duration_ms;           // Длительность работы
    float dose_ml;                  // Доза в мл
    float ml_per_second;            // Производительность
    pump_event_pid_data_t pid_data; // Данные PID
    float current_ph;               // Текущий pH
    float current_ec;               // Текущий EC
    float ph_target;                // Целевой pH
    float ec_target;                // Целевой EC
    bool emergency_mode;            // Режим аварии
    bool autonomous_mode;           // Автономный режим
    int8_t rssi;                    // RSSI к родительскому узлу
} pump_event_t;
```

## 🔧 Реализованные функции

### **1. Типы событий:**
- `PUMP_EVENT_START` - Насос включен
- `PUMP_EVENT_STOP` - Насос выключен
- `PUMP_EVENT_EMERGENCY_STOP` - Аварийная остановка
- `PUMP_EVENT_TIMEOUT` - Таймаут работы
- `PUMP_EVENT_CALIBRATION_START` - Начало калибровки
- `PUMP_EVENT_CALIBRATION_END` - Конец калибровки

### **2. Данные PID в событиях:**
```c
typedef struct {
    float kp;           // Пропорциональный коэффициент
    float ki;           // Интегральный коэффициент
    float kd;           // Дифференциальный коэффициент
    float setpoint;     // Заданное значение
    float current_value; // Текущее значение
    float error;        // Ошибка (setpoint - current)
    float output;       // Выход PID
    float integral;     // Интегральная составляющая
    float derivative;   // Дифференциальная составляющая
    bool enabled;       // Включен ли PID
} pump_event_pid_data_t;
```

### **3. API функции:**

#### **Инициализация:**
```c
esp_err_t pump_events_init(void);
```

#### **Отправка событий:**
```c
esp_err_t pump_events_send_start_event(
    pump_id_t pump_id,
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ph, float ec,
    float ph_target, float ec_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);

esp_err_t pump_events_send_stop_event(
    pump_id_t pump_id,
    uint32_t duration_ms,
    float dose_ml,
    const pump_event_pid_data_t *pid_data,
    float ph, float ec,
    float ph_target, float ec_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);

esp_err_t pump_events_send_emergency_stop_event(
    pump_id_t pump_id,
    const char *reason,
    float ph, float ec,
    float ph_target, float ec_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);
```

#### **Получение данных PID:**
```c
esp_err_t pump_events_get_pid_data(
    const pid_controller_t *pid,
    float current_value,
    pump_event_pid_data_t *pid_data
);
```

## 🔄 Интеграция с существующими компонентами

### **1. pump_controller.c:**
- ✅ Добавлена отправка событий при включении насосов
- ✅ Добавлена отправка событий при выключении насосов
- ✅ Добавлена отправка событий при аварийной остановке
- ✅ Интеграция с системой событий

### **2. ph_ec_manager.c:**
- ✅ Обновлена функция `control_ph_ec()` для отправки событий с реальными данными PID
- ✅ Получение актуальных данных pH/EC и PID параметров
- ✅ Отправка событий при каждом включении насоса через PID

### **3. CMakeLists.txt:**
- ✅ Добавлены зависимости для `pump_events`
- ✅ Обновлены требования компонентов

## 📊 Формат JSON событий

### **Пример события включения насоса:**
```json
{
  "type": "event",
  "node_id": "ph_ec_001",
  "node_type": "ph_ec",
  "timestamp": 1703123456,
  "level": "info",
  "data": {
    "event_type": "pump_start",
    "pump_id": 0,
    "duration_ms": 5000,
    "dose_ml": 2.5,
    "ml_per_second": 2.0,
    "pid_data": {
      "kp": 1.2,
      "ki": 0.1,
      "kd": 0.5,
      "setpoint": 6.5,
      "current_value": 6.0,
      "error": 0.5,
      "output": 2.5,
      "integral": 0.0,
      "derivative": 0.0,
      "enabled": true
    },
    "current_ph": 6.0,
    "current_ec": 1.2,
    "ph_target": 6.5,
    "ec_target": 1.5,
    "emergency_mode": false,
    "autonomous_mode": false,
    "rssi": -45
  }
}
```

## 🎯 Преимущества системы

### **1. Полная трассируемость:**
- 📈 Отслеживание каждого включения/выключения насоса
- 🔍 Детальные данные PID для анализа
- ⏱️ Точное время и длительность работы
- 📊 Статистика производительности

### **2. Диагностика системы:**
- 🐛 Выявление проблем с PID настройками
- 📡 Мониторинг качества связи (RSSI)
- 🚨 Отслеживание аварийных ситуаций
- 🔧 Анализ эффективности управления

### **3. Интеграция с сервером:**
- 🌐 Автоматическая отправка на сервер
- 📱 Real-time уведомления
- 📊 Сбор статистики и аналитики
- 🔔 Алерты при критических событиях

## 🧪 Тестирование

### **Тестовые файлы:**
- `test_pump_events.json` - Примеры событий для тестирования

### **Сценарии тестирования:**
1. **Нормальная работа:** Включение/выключение насосов через PID
2. **Аварийная остановка:** При превышении лимитов pH/EC
3. **Автономный режим:** Работа без связи с сервером
4. **Калибровка:** События калибровки насосов

## 🚀 Готовность к использованию

### **✅ Реализовано:**
- [x] Система событий насосов
- [x] Интеграция с PID контроллерами
- [x] Отправка на сервер через MQTT
- [x] Полные данные системы
- [x] Обработка аварийных ситуаций

### **🔄 Следующие шаги:**
1. Тестирование на реальном оборудовании
2. Настройка обработки событий на сервере
3. Создание дашборда для мониторинга
4. Настройка алертов и уведомлений

## 📝 Заключение

Система событий насосов полностью интегрирована с PID контроллерами и готова к использованию. Она обеспечивает полную трассируемость работы насосов с детальными данными для анализа и диагностики системы гидропоники.

**Система готова к развертыванию! 🎉**
