/**
 * @file pid_controller.c
 * @brief PID controller implementation
 */

#include "pid_controller.h"
#include "esp_timer.h"
#include <string.h>

void pid_init(pid_controller_t *pid, float kp, float ki, float kd) {
    if (pid == NULL) return;
    
    memset(pid, 0, sizeof(pid_controller_t));
    
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = 0.0f;
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->output_min = 0.0f;
    pid->output_max = 10.0f;
    pid->enabled = true;
    pid->last_time_us = esp_timer_get_time();
}

float pid_compute(pid_controller_t *pid, float current, float dt) {
    if (pid == NULL || !pid->enabled || dt <= 0.0f) {
        return 0.0f;
    }
    
    // Ошибка
    float error = pid->setpoint - current;
    
    // Пропорциональная составляющая
    float p_term = pid->kp * error;
    
    // Интегральная составляющая
    pid->integral += error * dt;
    
    // Anti-windup: ограничение интеграла
    float max_integral = pid->output_max / pid->ki;
    if (pid->integral > max_integral) {
        pid->integral = max_integral;
    } else if (pid->integral < -max_integral) {
        pid->integral = -max_integral;
    }
    
    float i_term = pid->ki * pid->integral;
    
    // Дифференциальная составляющая
    float derivative = (error - pid->prev_error) / dt;
    float d_term = pid->kd * derivative;
    
    // Сохранение для следующего цикла
    pid->prev_error = error;
    
    // Итоговый выход
    float output = p_term + i_term + d_term;
    
    // Ограничение выхода
    if (output > pid->output_max) {
        output = pid->output_max;
    } else if (output < pid->output_min) {
        output = pid->output_min;
    }
    
    return output;
}

void pid_reset(pid_controller_t *pid) {
    if (pid == NULL) return;
    
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->last_time_us = esp_timer_get_time();
}

void pid_set_setpoint(pid_controller_t *pid, float setpoint) {
    if (pid == NULL) return;
    pid->setpoint = setpoint;
}

void pid_set_output_limits(pid_controller_t *pid, float min, float max) {
    if (pid == NULL) return;
    pid->output_min = min;
    pid->output_max = max;
}

