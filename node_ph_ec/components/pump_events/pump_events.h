/**
 * @file pump_events.h
 * @brief Система событий для насосов с данными PID
 */

#ifndef PUMP_EVENTS_H
#define PUMP_EVENTS_H

#include "esp_err.h"
#include "pump_controller.h"
#include "pid_controller.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Типы событий насосов
 */
typedef enum {
    PUMP_EVENT_START,    // Насос включен
    PUMP_EVENT_STOP,     // Насос выключен
    PUMP_EVENT_EMERGENCY_STOP,  // Аварийная остановка
    PUMP_EVENT_TIMEOUT,  // Таймаут работы
    PUMP_EVENT_CALIBRATION_START,  // Начало калибровки
    PUMP_EVENT_CALIBRATION_END    // Конец калибровки
} pump_event_type_t;

/**
 * @brief Структура данных PID для события
 */
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

/**
 * @brief Структура события насоса
 */
typedef struct {
    pump_event_type_t type;         // Тип события
    pump_id_t pump_id;              // ID насоса
    uint32_t timestamp;             // Время события
    uint32_t duration_ms;           // Длительность работы (для STOP событий)
    float dose_ml;                  // Доза в мл
    float ml_per_second;            // Производительность мл/сек
    pump_event_pid_data_t pid_data; // Данные PID
    float current_ph;               // Текущий pH
    float current_ec;               // Текущий EC
    float ph_target;                // Целевой pH
    float ec_target;                // Целевой EC
    bool emergency_mode;            // Режим аварии
    bool autonomous_mode;           // Автономный режим
    int8_t rssi;                    // RSSI к родительскому узлу
} pump_event_t;

/**
 * @brief Инициализация системы событий насосов
 * 
 * @return ESP_OK при успехе
 */
esp_err_t pump_events_init(void);

/**
 * @brief Отправка события насоса на сервер
 * 
 * @param event Структура события
 * @return ESP_OK при успехе
 */
esp_err_t pump_events_send_event(const pump_event_t *event);

/**
 * @brief Создание события включения насоса
 * 
 * @param pump_id ID насоса
 * @param duration_ms Длительность работы
 * @param dose_ml Доза в мл
 * @param pid_data Данные PID
 * @param ph Текущий pH
 * @param ec Текущий EC
 * @param ph_target Целевой pH
 * @param ec_target Целевой EC
 * @param emergency_mode Режим аварии
 * @param autonomous_mode Автономный режим
 * @param rssi RSSI
 * @return ESP_OK при успехе
 */
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

/**
 * @brief Создание события выключения насоса
 * 
 * @param pump_id ID насоса
 * @param duration_ms Длительность работы
 * @param dose_ml Доза в мл
 * @param pid_data Данные PID
 * @param ph Текущий pH
 * @param ec Текущий EC
 * @param ph_target Целевой pH
 * @param ec_target Целевой EC
 * @param emergency_mode Режим аварии
 * @param autonomous_mode Автономный режим
 * @param rssi RSSI
 * @return ESP_OK при успехе
 */
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

/**
 * @brief Создание события аварийной остановки насоса
 * 
 * @param pump_id ID насоса
 * @param reason Причина остановки
 * @param ph Текущий pH
 * @param ec Текущий EC
 * @param ph_target Целевой pH
 * @param ec_target Целевой EC
 * @param emergency_mode Режим аварии
 * @param autonomous_mode Автономный режим
 * @param rssi RSSI
 * @return ESP_OK при успехе
 */
esp_err_t pump_events_send_emergency_stop_event(
    pump_id_t pump_id,
    const char *reason,
    float ph, float ec,
    float ph_target, float ec_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);

/**
 * @brief Создание события таймаута насоса
 * 
 * @param pump_id ID насоса
 * @param ph Текущий pH
 * @param ec Текущий EC
 * @param ph_target Целевой pH
 * @param ec_target Целевой EC
 * @param emergency_mode Режим аварии
 * @param autonomous_mode Автономный режим
 * @param rssi RSSI
 * @return ESP_OK при успехе
 */
esp_err_t pump_events_send_timeout_event(
    pump_id_t pump_id,
    float ph, float ec,
    float ph_target, float ec_target,
    bool emergency_mode, bool autonomous_mode,
    int8_t rssi
);

/**
 * @brief Получение данных PID из контроллера
 * 
 * @param pid PID контроллер
 * @param current_value Текущее значение
 * @param pid_data Выходная структура с данными PID
 * @return ESP_OK при успехе
 */
esp_err_t pump_events_get_pid_data(
    const pid_controller_t *pid,
    float current_value,
    pump_event_pid_data_t *pid_data
);

#ifdef __cplusplus
}
#endif

#endif // PUMP_EVENTS_H
