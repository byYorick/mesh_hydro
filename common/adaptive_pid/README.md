# 🎯 Адаптивный PID Контроллер

**Профессиональная реализация PID контроллера с автоподстройкой, зонированием и safety механизмами**

---

## 🌟 Особенности

### ✅ Реализовано:

1. **Зонирование (3 зоны работы)**
   - **DEAD** - мёртвая зона (±0.1-0.2 от target) - не корректируем
   - **CLOSE** - близкая зона (±0.3-0.5 от target) - консервативная коррекция
   - **FAR** - дальняя зона (> 0.5 от target) - агрессивная коррекция

2. **Адаптивные коэффициенты**
   - Разные Kp, Ki, Kd для каждой зоны
   - Автоподстройка на основе тренда ошибки
   - Скорость адаптации настраиваемая (0.01-0.2)

3. **Anti-windup**
   - Умное ограничение интеграла
   - Предотвращение переполнения I-составляющей

4. **Safety механизмы**
   - Максимальная доза за раз (например 5 мл)
   - Минимальный интервал между дозами (например 60 сек)
   - Emergency stop режим

5. **Статистика**
   - Количество коррекций
   - Суммарный выход
   - Максимальная/средняя ошибка
   - Время в каждой зоне

---

## 📊 Как это работает?

### Зоны работы:

```
pH = 7.0 (target)

│←─ DEAD ZONE ─→│
├───────────────┤
6.9 ← → 7.1     (±0.1)  ✅ Не корректируем

│←─ CLOSE ZONE ─→│
├────────────────┤
6.7 ← → 7.3      (±0.3)  🟡 Консервативно: Kp=0.5, Ki=0.01, Kd=0.1

│←─ FAR ZONE ─→│
├──────────────┤
<6.7 или >7.3    (±1.0+)  🔴 Агрессивно: Kp=1.0, Ki=0.015, Kd=0.05
```

### Пример коррекции:

```c
// pH = 6.5 (target = 7.0, error = -0.5)
// Зона: FAR (так как error > 0.3)

// Используются агрессивные коэффициенты:
Kp = 1.0, Ki = 0.015, Kd = 0.05

// PID расчет:
P_term = 1.0 * (-0.5) = -0.5
I_term = 0.015 * integral = 0.02
D_term = 0.05 * derivative = 0.03

Output = -0.5 + 0.02 + 0.03 = -0.45 ml  (отрицательно = pH DOWN не нужен)

// Для pH UP:
Output_UP = 0.45 ml  (инвертируем для pH UP)

// Проверка safety:
- Output_UP < max_dose (5.0 ml) ✅
- Interval > min_interval (60 сек) ✅

// Запуск насоса:
pump_controller_run_dose(PUMP_PH_UP, 0.45);
```

---

## 🔧 Использование

### 1. Инициализация:

```c
#include "adaptive_pid.h"

adaptive_pid_t pid_ph_up;

// Инициализация с базовыми коэффициентами (для CLOSE зоны)
adaptive_pid_init(&pid_ph_up, 
                 7.0f,      // setpoint (target)
                 0.5f,      // Kp
                 0.01f,     // Ki
                 0.1f);     // Kd

// Настройка зон (для pH)
adaptive_pid_set_zones(&pid_ph_up, 
                      0.1f,   // dead_zone
                      0.3f,   // close_zone
                      1.0f);  // far_zone

// Safety параметры
adaptive_pid_set_safety(&pid_ph_up, 
                       5.0f,    // max_dose_ml
                       60000);  // min_interval_ms (1 минута)

// Лимиты выхода
adaptive_pid_set_output_limits(&pid_ph_up, 0.0f, 5.0f);
```

### 2. Настройка коэффициентов для зон:

```c
// FAR зона - агрессивные коэффициенты
adaptive_pid_set_zone_coeffs(&pid_ph_up, ZONE_FAR,
                             1.0f,    // Kp (x2 от CLOSE)
                             0.015f,  // Ki (x1.5 от CLOSE)
                             0.05f);  // Kd (x0.5 от CLOSE)

// CLOSE зона уже установлена при init()

// DEAD зона - нулевые коэффициенты (не корректируем)
adaptive_pid_set_zone_coeffs(&pid_ph_up, ZONE_DEAD,
                             0.0f, 0.0f, 0.0f);
```

### 3. Расчет PID (каждые 10 секунд):

```c
float current_ph = 6.5f;  // Текущее значение с датчика
float output_ml = 0.0f;

esp_err_t err = adaptive_pid_compute(&pid_ph_up, 
                                    current_ph,  // Текущее значение
                                    10.0f,       // dt (секунды)
                                    &output_ml); // Выход (мл)

if (err == ESP_OK && output_ml > 0.0f) {
    // Получение текущей зоны для логирования
    pid_zone_t zone = adaptive_pid_get_zone(&pid_ph_up);
    
    ESP_LOGI(TAG, "pH UP: %.2f ml [%s zone]", 
             output_ml, adaptive_pid_zone_to_str(zone));
    
    // Запуск насоса
    pump_controller_run_dose(PUMP_PH_UP, output_ml);
}
```

### 4. Автоподстройка:

```c
// Включение auto-tuning
adaptive_pid_set_auto_tune(&pid_ph_up, 
                          true,    // enable
                          0.05f);  // adaptation_rate (5% за итерацию)

// Коэффициенты будут автоматически адаптироваться:
// - Если ошибка растёт → увеличиваем Kp, уменьшаем Ki
// - Если ошибка уменьшается → уменьшаем Kp, увеличиваем Ki
```

### 5. Статистика:

```c
const pid_stats_t* stats = adaptive_pid_get_stats(&pid_ph_up);

ESP_LOGI(TAG, "PID Stats:");
ESP_LOGI(TAG, "  Corrections: %lu", stats->corrections_count);
ESP_LOGI(TAG, "  Total output: %.2f ml", stats->total_output);
ESP_LOGI(TAG, "  Max error: %.3f", stats->max_error);
ESP_LOGI(TAG, "  Avg error: %.3f", stats->avg_error);
ESP_LOGI(TAG, "  Time in DEAD: %lu ms", stats->time_in_dead_zone_ms);
ESP_LOGI(TAG, "  Time in CLOSE: %lu ms", stats->time_in_close_zone_ms);
ESP_LOGI(TAG, "  Time in FAR: %lu ms", stats->time_in_far_zone_ms);

// Сброс статистики
adaptive_pid_reset_stats(&pid_ph_up);
```

### 6. Emergency режим:

```c
// Аварийная остановка
adaptive_pid_emergency_stop(&pid_ph_up);
// Все насосы останавливаются, интеграл сбрасывается

// Возобновление работы
adaptive_pid_resume(&pid_ph_up);
```

---

## 🎛️ Настройка параметров

### Для pH узла:

```c
// pH target: 6.0-7.0
// Зоны:
dead_zone  = 0.1   // ±0.1 (pH 6.9-7.1)
close_zone = 0.3   // ±0.3 (pH 6.7-7.3)
far_zone   = 1.0   // > 0.3

// Коэффициенты CLOSE:
Kp = 0.5, Ki = 0.01, Kd = 0.1

// Коэффициенты FAR:
Kp = 1.0, Ki = 0.015, Kd = 0.05

// Safety:
max_dose = 5.0 ml
min_interval = 60000 ms (1 минута)
```

### Для EC узла:

```c
// EC target: 1.5-2.5 mS/cm
// Зоны:
dead_zone  = 0.2   // ±0.2
close_zone = 0.5   // ±0.5
far_zone   = 1.5   // > 0.5

// Коэффициенты CLOSE:
Kp = 0.8, Ki = 0.02, Kd = 0.2

// Коэффициенты FAR:
Kp = 1.5, Ki = 0.03, Kd = 0.1

// Safety:
max_dose = 10.0 ml (распределяется на 3 насоса)
min_interval = 60000 ms (1 минута)
```

---

## 📈 Преимущества над простым PID

| Функция | Простой PID | Адаптивный PID |
|---------|-------------|----------------|
| **Зонирование** | ❌ Нет | ✅ 3 зоны |
| **Мёртвая зона** | ❌ Жёстко заданная | ✅ Настраиваемая |
| **Коэффициенты** | ⚠️ Одни для всех | ✅ Разные для зон |
| **Автоподстройка** | ❌ Нет | ✅ Есть |
| **Anti-windup** | ⚠️ Простой | ✅ Умный |
| **Safety** | ❌ Нет | ✅ Есть |
| **Статистика** | ❌ Нет | ✅ Полная |
| **Emergency stop** | ❌ Нет | ✅ Есть |

---

## 🔬 Тестирование

### Тест 1: Dead Zone

```c
// pH = 7.05 (target = 7.0, error = 0.05)
// Ожидается: output = 0.0 (в мёртвой зоне)

float output = 0.0f;
adaptive_pid_compute(&pid, 7.05f, 10.0f, &output);

assert(output == 0.0f);
assert(adaptive_pid_get_zone(&pid) == ZONE_DEAD);
```

### Тест 2: Close Zone

```c
// pH = 7.25 (target = 7.0, error = 0.25)
// Ожидается: output > 0, zone = CLOSE

float output = 0.0f;
adaptive_pid_compute(&pid, 7.25f, 10.0f, &output);

assert(output > 0.0f && output < 1.0f);
assert(adaptive_pid_get_zone(&pid) == ZONE_CLOSE);
```

### Тест 3: Far Zone

```c
// pH = 7.8 (target = 7.0, error = 0.8)
// Ожидается: output > 1.0, zone = FAR

float output = 0.0f;
adaptive_pid_compute(&pid, 7.8f, 10.0f, &output);

assert(output > 1.0f);
assert(adaptive_pid_get_zone(&pid) == ZONE_FAR);
```

### Тест 4: Safety Interval

```c
// Две дозы подряд < min_interval
// Ожидается: вторая доза НЕ выдана

float output1 = 0.0f, output2 = 0.0f;

adaptive_pid_compute(&pid, 6.5f, 10.0f, &output1);
assert(output1 > 0.0f);  // Первая доза выдана

vTaskDelay(pdMS_TO_TICKS(1000));  // 1 секунда (< 60 сек)

adaptive_pid_compute(&pid, 6.5f, 10.0f, &output2);
assert(output2 == 0.0f);  // Вторая доза НЕ выдана (safety interval)
```

---

## 📝 Примечания

1. **dt (временной шаг)** должен быть постоянным для корректной работы PID
2. **Коэффициенты** подбираются эмпирически для каждой системы
3. **Auto-tuning** может занять несколько часов для стабилизации
4. **Safety интервал** критично важен для предотвращения overdose
5. **Статистику** рекомендуется логировать раз в час для анализа

---

## 🚀 Интеграция в node_ph и node_ec

Адаптивный PID уже интегрирован в:
- ✅ `node_ph/components/ph_manager/`
- ✅ `node_ec/components/ec_manager/`

**Использование:** Автоматически при запуске нод!

---

**Версия:** 1.0  
**Дата:** 21.10.2025  
**Автор:** Mesh Hydro Team

