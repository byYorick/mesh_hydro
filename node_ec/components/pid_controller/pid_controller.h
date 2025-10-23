/**
 * @file pid_controller.h
 * @brief Простой PID контроллер для pH/EC
 */

#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Структура PID контроллера
 */
typedef struct {
    float kp;                    // Пропорциональный коэффициент
    float ki;                    // Интегральный коэффициент
    float kd;                    // Дифференциальный коэффициент
    float setpoint;              // Целевое значение
    float integral;              // Интегральная составляющая
    float prev_error;            // Предыдущая ошибка
    float output_min;            // Минимальный выход
    float output_max;            // Максимальный выход
    uint64_t last_time_us;       // Время последнего расчёта
    bool enabled;                // Включен ли контроллер
} pid_controller_t;

/**
 * @brief Инициализация PID контроллера
 * 
 * @param pid Указатель на структуру PID
 * @param kp Пропорциональный коэффициент
 * @param ki Интегральный коэффициент
 * @param kd Дифференциальный коэффициент
 */
void pid_init(pid_controller_t *pid, float kp, float ki, float kd);

/**
 * @brief Расчёт PID
 * 
 * @param pid Указатель на структуру PID
 * @param current Текущее значение
 * @param dt Временной шаг (секунды)
 * @return Выходное значение
 */
float pid_compute(pid_controller_t *pid, float current, float dt);

/**
 * @brief Сброс PID (интеграл и ошибка)
 * 
 * @param pid Указатель на структуру PID
 */
void pid_reset(pid_controller_t *pid);

/**
 * @brief Установка уставки
 * 
 * @param pid Указатель на структуру PID
 * @param setpoint Целевое значение
 */
void pid_set_setpoint(pid_controller_t *pid, float setpoint);

/**
 * @brief Установка лимитов выхода
 * 
 * @param pid Указатель на структуру PID
 * @param min Минимум
 * @param max Максимум
 */
void pid_set_output_limits(pid_controller_t *pid, float min, float max);

#ifdef __cplusplus
}
#endif

#endif // PID_CONTROLLER_H

