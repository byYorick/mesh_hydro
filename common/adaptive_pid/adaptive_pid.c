/**
 * @file adaptive_pid.c
 * @brief Реализация адаптивного PID контроллера
 */

#include "adaptive_pid.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <string.h>
#include <math.h>

static const char *TAG = "adaptive_pid";

// Вспомогательные функции
static pid_zone_t determine_zone(const adaptive_pid_t *pid, float error);
static void adapt_coefficients(adaptive_pid_t *pid, float error, float output);
static bool check_safety_interval(const adaptive_pid_t *pid);

esp_err_t adaptive_pid_init(adaptive_pid_t *pid, float setpoint,
                            float kp_close, float ki_close, float kd_close) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(pid, 0, sizeof(adaptive_pid_t));
    
    // Базовые параметры
    pid->setpoint = setpoint;
    pid->output_min = 0.0f;
    pid->output_max = 10.0f;
    
    // Коэффициенты для близкой зоны (базовые)
    pid->coeffs_close.kp = kp_close;
    pid->coeffs_close.ki = ki_close;
    pid->coeffs_close.kd = kd_close;
    
    // Мёртвая зона - нулевые коэффициенты
    pid->coeffs_dead.kp = 0.0f;
    pid->coeffs_dead.ki = 0.0f;
    pid->coeffs_dead.kd = 0.0f;
    
    // Дальняя зона - увеличенные коэффициенты
    pid->coeffs_far.kp = kp_close * 2.0f;
    pid->coeffs_far.ki = ki_close * 1.5f;
    pid->coeffs_far.kd = kd_close * 0.5f;
    
    // Зоны по умолчанию (для pH/EC)
    pid->zones.dead_zone = 0.1f;
    pid->zones.close_zone = 0.3f;
    pid->zones.far_zone = 1.0f;
    pid->current_zone = ZONE_DEAD;
    
    // Safety параметры
    pid->max_dose_ml = 5.0f;
    pid->min_interval_ms = 60000;  // 1 минута
    
    // Адаптация
    pid->auto_tune_enabled = true;
    pid->adaptation_rate = 0.05f;  // 5% за итерацию
    
    // Флаги
    pid->enabled = true;
    pid->emergency_stop = false;
    
    pid->last_time_us = esp_timer_get_time();
    pid->last_dose_time_us = 0;
    
    ESP_LOGI(TAG, "Adaptive PID initialized: setpoint=%.2f, Kp=%.3f, Ki=%.3f, Kd=%.3f",
             setpoint, kp_close, ki_close, kd_close);
    
    return ESP_OK;
}

esp_err_t adaptive_pid_set_zones(adaptive_pid_t *pid,
                                 float dead_zone, float close_zone, float far_zone) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->zones.dead_zone = dead_zone;
    pid->zones.close_zone = close_zone;
    pid->zones.far_zone = far_zone;
    
    ESP_LOGI(TAG, "Zones set: dead=%.2f, close=%.2f, far=%.2f",
             dead_zone, close_zone, far_zone);
    
    return ESP_OK;
}

esp_err_t adaptive_pid_set_zone_coeffs(adaptive_pid_t *pid, pid_zone_t zone,
                                       float kp, float ki, float kd) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid_coeffs_t *coeffs;
    switch (zone) {
        case ZONE_DEAD:
            coeffs = &pid->coeffs_dead;
            break;
        case ZONE_CLOSE:
            coeffs = &pid->coeffs_close;
            break;
        case ZONE_FAR:
            coeffs = &pid->coeffs_far;
            break;
        default:
            return ESP_ERR_INVALID_ARG;
    }
    
    coeffs->kp = kp;
    coeffs->ki = ki;
    coeffs->kd = kd;
    
    ESP_LOGI(TAG, "Zone %s coeffs set: Kp=%.3f, Ki=%.3f, Kd=%.3f",
             adaptive_pid_zone_to_str(zone), kp, ki, kd);
    
    return ESP_OK;
}

esp_err_t adaptive_pid_set_safety(adaptive_pid_t *pid,
                                  float max_dose_ml, uint32_t min_interval_ms) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->max_dose_ml = max_dose_ml;
    pid->min_interval_ms = min_interval_ms;
    
    ESP_LOGI(TAG, "Safety set: max_dose=%.2f ml, min_interval=%lu ms",
             max_dose_ml, min_interval_ms);
    
    return ESP_OK;
}

esp_err_t adaptive_pid_set_output_limits(adaptive_pid_t *pid, float min, float max) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->output_min = min;
    pid->output_max = max;
    
    return ESP_OK;
}

esp_err_t adaptive_pid_set_setpoint(adaptive_pid_t *pid, float setpoint) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    float prev_setpoint = pid->setpoint;
    pid->setpoint = setpoint;
    
    // Если setpoint значительно изменился - сброс интеграла
    if (fabsf(setpoint - prev_setpoint) > 0.5f) {
        pid->integral = 0.0f;
        ESP_LOGI(TAG, "Setpoint changed significantly (%.2f -> %.2f), integral reset",
                 prev_setpoint, setpoint);
    }
    
    return ESP_OK;
}

esp_err_t adaptive_pid_compute(adaptive_pid_t *pid, float current, float dt, float *output) {
    if (!pid || !output) {
        return ESP_ERR_INVALID_ARG;
    }
    
    *output = 0.0f;
    
    // Проверки
    if (!pid->enabled || pid->emergency_stop) {
        return ESP_OK;
    }
    
    if (dt <= 0.0f || dt > 10.0f) {
        ESP_LOGW(TAG, "Invalid dt: %.3f", dt);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Проверка safety интервала
    if (!check_safety_interval(pid)) {
        ESP_LOGD(TAG, "Safety interval not elapsed");
        return ESP_OK;
    }
    
    // Вычисление ошибки
    float error = pid->setpoint - current;
    
    // Определение зоны
    pid_zone_t prev_zone = pid->current_zone;
    pid->current_zone = determine_zone(pid, error);
    
    if (prev_zone != pid->current_zone) {
        ESP_LOGI(TAG, "Zone changed: %s -> %s (error=%.3f)",
                 adaptive_pid_zone_to_str(prev_zone),
                 adaptive_pid_zone_to_str(pid->current_zone), error);
    }
    
    // Обновление статистики зон
    uint32_t dt_ms = (uint32_t)(dt * 1000.0f);
    switch (pid->current_zone) {
        case ZONE_DEAD:
            pid->stats.time_in_dead_zone_ms += dt_ms;
            break;
        case ZONE_CLOSE:
            pid->stats.time_in_close_zone_ms += dt_ms;
            break;
        case ZONE_FAR:
            pid->stats.time_in_far_zone_ms += dt_ms;
            break;
    }
    
    // Если в мёртвой зоне - не корректируем
    if (pid->current_zone == ZONE_DEAD) {
        return ESP_OK;
    }
    
    // Выбор коэффициентов для текущей зоны
    pid_coeffs_t *coeffs;
    switch (pid->current_zone) {
        case ZONE_CLOSE:
            coeffs = &pid->coeffs_close;
            break;
        case ZONE_FAR:
            coeffs = &pid->coeffs_far;
            break;
        default:
            return ESP_OK;
    }
    
    // Пропорциональная составляющая
    float p_term = coeffs->kp * error;
    
    // Интегральная составляющая
    pid->integral += error * dt;
    
    // Anti-windup: умное ограничение интеграла
    float max_integral = pid->output_max / (coeffs->ki + 0.001f);  // Защита от деления на 0
    if (pid->integral > max_integral) {
        pid->integral = max_integral;
    } else if (pid->integral < -max_integral) {
        pid->integral = -max_integral;
    }
    
    float i_term = coeffs->ki * pid->integral;
    
    // Дифференциальная составляющая
    float derivative = (error - pid->prev_error) / dt;
    float d_term = coeffs->kd * derivative;
    
    // Итоговый выход
    float raw_output = p_term + i_term + d_term;
    
    // Ограничение выхода
    if (raw_output > pid->output_max) {
        raw_output = pid->output_max;
    } else if (raw_output < pid->output_min) {
        raw_output = pid->output_min;
    }
    
    // Применение max_dose_ml ограничения
    if (raw_output > pid->max_dose_ml) {
        raw_output = pid->max_dose_ml;
    }
    
    // Если выход слишком мал - не дозируем (гистерезис)
    if (fabsf(raw_output) < 0.05f) {
        raw_output = 0.0f;
    }
    
    // Сохранение для следующей итерации
    pid->prev_error = error;
    pid->prev_output = raw_output;
    
    // Обновление статистики
    if (raw_output > 0.0f) {
        pid->stats.corrections_count++;
        pid->stats.total_output += raw_output;
        pid->last_dose_time_us = esp_timer_get_time();
    }
    
    if (fabsf(error) > pid->stats.max_error) {
        pid->stats.max_error = fabsf(error);
    }
    
    // Скользящее среднее ошибки
    pid->stats.avg_error = (pid->stats.avg_error * 0.9f) + (fabsf(error) * 0.1f);
    
    // Адаптация коэффициентов (если включена)
    if (pid->auto_tune_enabled && raw_output > 0.0f) {
        adapt_coefficients(pid, error, raw_output);
    }
    
    *output = raw_output;
    
    ESP_LOGD(TAG, "PID: error=%.3f, zone=%s, P=%.3f, I=%.3f, D=%.3f, out=%.3f ml",
             error, adaptive_pid_zone_to_str(pid->current_zone),
             p_term, i_term, d_term, raw_output);
    
    return ESP_OK;
}

esp_err_t adaptive_pid_reset(adaptive_pid_t *pid) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->integral = 0.0f;
    pid->prev_error = 0.0f;
    pid->prev_output = 0.0f;
    pid->stable_count = 0;
    pid->last_time_us = esp_timer_get_time();
    
    memset(&pid->stats, 0, sizeof(pid_stats_t));
    
    ESP_LOGI(TAG, "PID reset");
    
    return ESP_OK;
}

esp_err_t adaptive_pid_set_auto_tune(adaptive_pid_t *pid, bool enable, float adaptation_rate) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->auto_tune_enabled = enable;
    pid->adaptation_rate = adaptation_rate;
    
    ESP_LOGI(TAG, "Auto-tune %s (rate=%.3f)", enable ? "enabled" : "disabled", adaptation_rate);
    
    return ESP_OK;
}

esp_err_t adaptive_pid_emergency_stop(adaptive_pid_t *pid) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->emergency_stop = true;
    pid->integral = 0.0f;
    
    ESP_LOGW(TAG, "EMERGENCY STOP activated");
    
    return ESP_OK;
}

esp_err_t adaptive_pid_resume(adaptive_pid_t *pid) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->emergency_stop = false;
    
    ESP_LOGI(TAG, "PID resumed");
    
    return ESP_OK;
}

pid_zone_t adaptive_pid_get_zone(const adaptive_pid_t *pid) {
    return pid ? pid->current_zone : ZONE_DEAD;
}

const pid_stats_t* adaptive_pid_get_stats(const adaptive_pid_t *pid) {
    return pid ? &pid->stats : NULL;
}

esp_err_t adaptive_pid_reset_stats(adaptive_pid_t *pid) {
    if (!pid) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(&pid->stats, 0, sizeof(pid_stats_t));
    
    return ESP_OK;
}

const char* adaptive_pid_zone_to_str(pid_zone_t zone) {
    switch (zone) {
        case ZONE_DEAD:  return "DEAD";
        case ZONE_CLOSE: return "CLOSE";
        case ZONE_FAR:   return "FAR";
        default:         return "UNKNOWN";
    }
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

static pid_zone_t determine_zone(const adaptive_pid_t *pid, float error) {
    float abs_error = fabsf(error);
    
    if (abs_error <= pid->zones.dead_zone) {
        return ZONE_DEAD;
    } else if (abs_error <= pid->zones.close_zone) {
        return ZONE_CLOSE;
    } else {
        return ZONE_FAR;
    }
}

static bool check_safety_interval(const adaptive_pid_t *pid) {
    if (pid->last_dose_time_us == 0) {
        return true;  // Первая доза
    }
    
    uint64_t now = esp_timer_get_time();
    uint64_t elapsed_us = now - pid->last_dose_time_us;
    uint64_t min_interval_us = (uint64_t)pid->min_interval_ms * 1000;
    
    return (elapsed_us >= min_interval_us);
}

static void adapt_coefficients(adaptive_pid_t *pid, float error, float output) {
    // Простая адаптация: если ошибка растёт - увеличиваем Kp, уменьшаем Ki
    // Если ошибка уменьшается - наоборот
    
    float abs_error = fabsf(error);
    float error_trend = abs_error - pid->stats.avg_error;
    
    pid_coeffs_t *coeffs = (pid->current_zone == ZONE_CLOSE) ? 
                           &pid->coeffs_close : &pid->coeffs_far;
    
    if (error_trend > 0.05f) {
        // Ошибка растёт - нужна более агрессивная коррекция
        coeffs->kp *= (1.0f + pid->adaptation_rate);
        coeffs->ki *= (1.0f - pid->adaptation_rate * 0.5f);
        
        // Ограничения
        if (coeffs->kp > 10.0f) coeffs->kp = 10.0f;
        if (coeffs->ki < 0.001f) coeffs->ki = 0.001f;
        
        ESP_LOGD(TAG, "Adapted (error growing): Kp=%.3f, Ki=%.3f", coeffs->kp, coeffs->ki);
        
    } else if (error_trend < -0.05f) {
        // Ошибка уменьшается - можно снизить агрессивность
        coeffs->kp *= (1.0f - pid->adaptation_rate * 0.5f);
        coeffs->ki *= (1.0f + pid->adaptation_rate * 0.3f);
        
        // Ограничения
        if (coeffs->kp < 0.1f) coeffs->kp = 0.1f;
        if (coeffs->ki > 1.0f) coeffs->ki = 1.0f;
        
        ESP_LOGD(TAG, "Adapted (error decreasing): Kp=%.3f, Ki=%.3f", coeffs->kp, coeffs->ki);
    }
}

esp_err_t adaptive_pid_set_target(adaptive_pid_t *pid, float target) {
    if (pid == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (target < 0.0f || target > 14.0f) {
        ESP_LOGW(TAG, "Invalid target value: %.2f (must be 0.0-14.0)", target);
        return ESP_ERR_INVALID_ARG;
    }
    
    pid->setpoint = target;
    ESP_LOGI(TAG, "PID target set to %.2f", target);
    
    return ESP_OK;
}

