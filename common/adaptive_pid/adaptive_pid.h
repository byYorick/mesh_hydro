/**
 * @file adaptive_pid.h
 * @brief Адаптивный PID контроллер с зонированием и auto-tuning
 * 
 * Особенности:
 * - 3 зоны работы (dead/close/far)
 * - Адаптивные коэффициенты Kp, Ki, Kd
 * - Anti-windup с умным ограничением интеграла
 * - Safety механизмы (макс доза, макс частота)
 * - Учет инерционности системы
 * - Статистика и диагностика
 */

#ifndef ADAPTIVE_PID_H
#define ADAPTIVE_PID_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Зоны работы PID контроллера
 */
typedef enum {
    ZONE_DEAD = 0,      ///< Мёртвая зона (±0.1 от target) - не корректируем
    ZONE_CLOSE,         ///< Близкая зона (±0.3 от target) - консервативная коррекция
    ZONE_FAR            ///< Дальняя зона (> 0.3 от target) - агрессивная коррекция
} pid_zone_t;

/**
 * @brief Конфигурация зон
 */
typedef struct {
    float dead_zone;    ///< Размер мёртвой зоны (обе стороны от target)
    float close_zone;   ///< Размер близкой зоны
    float far_zone;     ///< Размер дальней зоны
} pid_zones_config_t;

/**
 * @brief Набор коэффициентов PID
 */
typedef struct {
    float kp;           ///< Пропорциональный
    float ki;           ///< Интегральный
    float kd;           ///< Дифференциальный
} pid_coeffs_t;

/**
 * @brief Статистика работы PID
 */
typedef struct {
    uint32_t corrections_count;     ///< Количество коррекций
    float total_output;             ///< Суммарный выход
    float max_error;                ///< Максимальная ошибка
    float avg_error;                ///< Средняя ошибка
    uint32_t time_in_dead_zone_ms;  ///< Время в мёртвой зоне
    uint32_t time_in_close_zone_ms; ///< Время в близкой зоне
    uint32_t time_in_far_zone_ms;   ///< Время в дальней зоне
} pid_stats_t;

/**
 * @brief Структура адаптивного PID контроллера
 */
typedef struct {
    // Базовые параметры
    float setpoint;                     ///< Целевое значение
    float output_min;                   ///< Минимальный выход (мл)
    float output_max;                   ///< Максимальный выход (мл)
    
    // Коэффициенты для разных зон
    pid_coeffs_t coeffs_dead;           ///< Коэффициенты для мёртвой зоны
    pid_coeffs_t coeffs_close;          ///< Коэффициенты для близкой зоны
    pid_coeffs_t coeffs_far;            ///< Коэффициенты для дальней зоны
    
    // Зоны
    pid_zones_config_t zones;
    pid_zone_t current_zone;
    
    // Внутреннее состояние
    float integral;                     ///< Интегральная составляющая
    float prev_error;                   ///< Предыдущая ошибка
    uint64_t last_time_us;              ///< Время последнего расчёта
    float prev_output;                  ///< Предыдущий выход
    
    // Safety
    float max_dose_ml;                  ///< Максимальная доза за раз (мл)
    uint32_t min_interval_ms;           ///< Минимальный интервал между дозами (мс)
    uint64_t last_dose_time_us;         ///< Время последней дозы
    
    // Адаптация
    bool auto_tune_enabled;             ///< Включена ли автоподстройка
    float adaptation_rate;              ///< Скорость адаптации (0.0-1.0)
    uint32_t stable_count;              ///< Счётчик стабильных итераций
    
    // Статистика
    pid_stats_t stats;
    
    // Флаги
    bool enabled;                       ///< Включен ли контроллер
    bool emergency_stop;                ///< Аварийная остановка
} adaptive_pid_t;

/**
 * @brief Инициализация адаптивного PID контроллера
 * 
 * @param pid Указатель на структуру PID
 * @param setpoint Целевое значение
 * @param kp_close Kp для близкой зоны
 * @param ki_close Ki для близкой зоны
 * @param kd_close Kd для близкой зоны
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_init(adaptive_pid_t *pid, float setpoint, 
                            float kp_close, float ki_close, float kd_close);

/**
 * @brief Установка зон работы
 * 
 * @param pid Указатель на структуру PID
 * @param dead_zone Размер мёртвой зоны (например 0.1 для pH)
 * @param close_zone Размер близкой зоны (например 0.3 для pH)
 * @param far_zone Размер дальней зоны (например 1.0 для pH)
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_set_zones(adaptive_pid_t *pid, 
                                 float dead_zone, float close_zone, float far_zone);

/**
 * @brief Установка коэффициентов для конкретной зоны
 * 
 * @param pid Указатель на структуру PID
 * @param zone Зона
 * @param kp Пропорциональный коэффициент
 * @param ki Интегральный коэффициент
 * @param kd Дифференциальный коэффициент
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_set_zone_coeffs(adaptive_pid_t *pid, pid_zone_t zone,
                                       float kp, float ki, float kd);

/**
 * @brief Установка safety параметров
 * 
 * @param pid Указатель на структуру PID
 * @param max_dose_ml Максимальная доза за раз (мл)
 * @param min_interval_ms Минимальный интервал между дозами (мс)
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_set_safety(adaptive_pid_t *pid, 
                                  float max_dose_ml, uint32_t min_interval_ms);

/**
 * @brief Установка лимитов выхода
 * 
 * @param pid Указатель на структуру PID
 * @param min Минимум (мл)
 * @param max Максимум (мл)
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_set_output_limits(adaptive_pid_t *pid, float min, float max);

/**
 * @brief Установка уставки (target)
 * 
 * @param pid Указатель на структуру PID
 * @param setpoint Целевое значение
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_set_setpoint(adaptive_pid_t *pid, float setpoint);

/**
 * @brief Расчёт PID (основная функция)
 * 
 * @param pid Указатель на структуру PID
 * @param current Текущее значение
 * @param dt Временной шаг (секунды)
 * @param output Указатель для выходного значения (мл)
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_compute(adaptive_pid_t *pid, float current, float dt, float *output);

/**
 * @brief Сброс PID (интеграл, ошибка, статистика)
 * 
 * @param pid Указатель на структуру PID
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_reset(adaptive_pid_t *pid);

/**
 * @brief Включение/выключение автоподстройки
 * 
 * @param pid Указатель на структуру PID
 * @param enable true - включить, false - выключить
 * @param adaptation_rate Скорость адаптации (0.01-0.2)
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_set_auto_tune(adaptive_pid_t *pid, bool enable, float adaptation_rate);

/**
 * @brief Аварийная остановка PID
 * 
 * @param pid Указатель на структуру PID
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_emergency_stop(adaptive_pid_t *pid);

/**
 * @brief Возобновление работы после emergency stop
 * 
 * @param pid Указатель на структуру PID
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_resume(adaptive_pid_t *pid);

/**
 * @brief Получение текущей зоны
 * 
 * @param pid Указатель на структуру PID
 * @return Текущая зона
 */
pid_zone_t adaptive_pid_get_zone(const adaptive_pid_t *pid);

/**
 * @brief Получение статистики
 * 
 * @param pid Указатель на структуру PID
 * @return Указатель на статистику
 */
const pid_stats_t* adaptive_pid_get_stats(const adaptive_pid_t *pid);

/**
 * @brief Сброс статистики
 * 
 * @param pid Указатель на структуру PID
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_reset_stats(adaptive_pid_t *pid);

/**
 * @brief Получение строкового названия зоны
 * 
 * @param zone Зона
 * @return Строка с названием
 */
const char* adaptive_pid_zone_to_str(pid_zone_t zone);

#ifdef __cplusplus
}
#endif

#endif // ADAPTIVE_PID_H

