/**
 * @file adaptive_pid.h
 * @brief Адаптивный PID контроллер с зонами управления
 */

#ifndef ADAPTIVE_PID_H
#define ADAPTIVE_PID_H

#include "esp_err.h"
#include "pid_controller.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Зоны управления PID
 */
typedef enum {
    ZONE_DEAD = 0,      // Мертвая зона - незначительная ошибка
    ZONE_CLOSE,         // Близкая зона - умеренная коррекция
    ZONE_FAR            // Дальняя зона - агрессивная коррекция
} pid_zone_t;

/**
 * @brief Структура адаптивного PID контроллера
 */
typedef struct {
    pid_controller_t pid;           // Базовый PID контроллер
    
    // Зоны управления
    float dead_zone;                // Мертвая зона
    float close_zone;               // Близкая зона
    float far_zone;                 // Дальняя зона
    
    // Безопасность
    float max_dose_ml;              // Максимальная доза за раз
    uint32_t min_cooldown_ms;       // Минимальное время между дозами
    uint64_t last_dose_time;        // Время последней дозы
    
    // Состояние
    pid_zone_t current_zone;        // Текущая зона
    bool safety_blocked;            // Заблокирован ли по безопасности
} adaptive_pid_t;

/**
 * @brief Инициализация адаптивного PID контроллера
 * 
 * @param apid Указатель на адаптивный PID
 * @param setpoint Целевое значение
 * @param kp Пропорциональный коэффициент
 * @param ki Интегральный коэффициент
 * @param kd Дифференциальный коэффициент
 */
void adaptive_pid_init(adaptive_pid_t *apid, float setpoint, float kp, float ki, float kd);

/**
 * @brief Установка зон управления
 * 
 * @param apid Указатель на адаптивный PID
 * @param dead_zone Мертвая зона
 * @param close_zone Близкая зона
 * @param far_zone Дальняя зона
 */
void adaptive_pid_set_zones(adaptive_pid_t *apid, float dead_zone, float close_zone, float far_zone);

/**
 * @brief Установка параметров безопасности
 * 
 * @param apid Указатель на адаптивный PID
 * @param max_dose_ml Максимальная доза за раз
 * @param min_cooldown_ms Минимальное время между дозами
 */
void adaptive_pid_set_safety(adaptive_pid_t *apid, float max_dose_ml, uint32_t min_cooldown_ms);

/**
 * @brief Установка лимитов выхода
 * 
 * @param apid Указатель на адаптивный PID
 * @param min Минимальный выход
 * @param max Максимальный выход
 */
void adaptive_pid_set_output_limits(adaptive_pid_t *apid, float min, float max);

/**
 * @brief Установка целевого значения
 * 
 * @param apid Указатель на адаптивный PID
 * @param setpoint Целевое значение
 */
void adaptive_pid_set_setpoint(adaptive_pid_t *apid, float setpoint);

/**
 * @brief Расчёт адаптивного PID
 * 
 * @param apid Указатель на адаптивный PID
 * @param current Текущее значение
 * @param dt Временной шаг (секунды)
 * @param output Указатель для выхода
 * @return ESP_OK при успехе
 */
esp_err_t adaptive_pid_compute(adaptive_pid_t *apid, float current, float dt, float *output);

/**
 * @brief Получение текущей зоны
 * 
 * @param apid Указатель на адаптивный PID
 * @return Текущая зона
 */
pid_zone_t adaptive_pid_get_zone(adaptive_pid_t *apid);

/**
 * @brief Преобразование зоны в строку
 * 
 * @param zone Зона
 * @return Строковое представление зоны
 */
const char* adaptive_pid_zone_to_str(pid_zone_t zone);

/**
 * @brief Проверка блокировки по безопасности
 * 
 * @param apid Указатель на адаптивный PID
 * @return true если заблокирован
 */
bool adaptive_pid_is_safety_blocked(adaptive_pid_t *apid);

#ifdef __cplusplus
}
#endif

#endif // ADAPTIVE_PID_H
