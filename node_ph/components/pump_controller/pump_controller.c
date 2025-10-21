/**
 * @file pump_controller.c
 * @brief Pump controller implementation for node_ph (2 pumps)
 */

#include "pump_controller.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include <string.h>

static const char *TAG = "pump_ctrl";

// Конфигурация PWM
#define PWM_FREQ_HZ       1000
#define PWM_RESOLUTION    LEDC_TIMER_8_BIT
#define PWM_MAX_DUTY      255
#define PWM_TIMER         LEDC_TIMER_0
#define PWM_MODE          LEDC_LOW_SPEED_MODE

// Безопасность
#define MAX_RUN_TIME_MS   10000  // Макс 10 секунд непрерывной работы
#define DEFAULT_ML_PER_SEC 2.0f  // Дефолтная производительность

// GPIO пины для насосов (ESP32-C3)
static const int PUMP_GPIO[PUMP_MAX] = {2, 3};

// Структура насоса
typedef struct {
    bool is_running;
    uint64_t start_time;
    uint32_t run_duration_ms;
    pump_stats_t stats;
    float ml_per_sec;
    TimerHandle_t timer;
} pump_state_t;

static pump_state_t s_pumps[PUMP_MAX];
static bool s_initialized = false;

// Forward declarations
static void pump_timer_callback(TimerHandle_t timer);
static esp_err_t pump_start_internal(pump_id_t pump, uint32_t duration_ms);
static esp_err_t pump_stop_internal(pump_id_t pump);

esp_err_t pump_controller_init(void) {
    if (s_initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing pump controller (2 pumps for pH)...");
    
    // Настройка PWM таймера
    ledc_timer_config_t timer_conf = {
        .speed_mode = PWM_MODE,
        .duty_resolution = PWM_RESOLUTION,
        .timer_num = PWM_TIMER,
        .freq_hz = PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));
    
    // Инициализация каждого насоса
    for (int i = 0; i < PUMP_MAX; i++) {
        // Настройка PWM канала
        ledc_channel_config_t channel_conf = {
            .gpio_num = PUMP_GPIO[i],
            .speed_mode = PWM_MODE,
            .channel = (ledc_channel_t)i,
            .timer_sel = PWM_TIMER,
            .duty = 0,
            .hpoint = 0
        };
        ESP_ERROR_CHECK(ledc_channel_config(&channel_conf));
        
        // Инициализация состояния
        memset(&s_pumps[i], 0, sizeof(pump_state_t));
        s_pumps[i].ml_per_sec = DEFAULT_ML_PER_SEC;
        s_pumps[i].is_running = false;
        
        // Создание таймера для автоостановки
        char timer_name[16];
        snprintf(timer_name, sizeof(timer_name), "pump%d", i);
        s_pumps[i].timer = xTimerCreate(timer_name, pdMS_TO_TICKS(1000), 
                                        pdFALSE, (void *)(uintptr_t)i, 
                                        pump_timer_callback);
        
        ESP_LOGI(TAG, "Pump %d initialized (GPIO %d)", i, PUMP_GPIO[i]);
    }
    
    s_initialized = true;
    ESP_LOGI(TAG, "Pump controller ready (pH UP/DOWN)");
    return ESP_OK;
}

esp_err_t pump_controller_run(pump_id_t pump, uint32_t duration_ms) {
    if (pump >= PUMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (duration_ms > MAX_RUN_TIME_MS) {
        ESP_LOGW(TAG, "Duration %lu ms exceeds max %d ms", 
                 (unsigned long)duration_ms, MAX_RUN_TIME_MS);
        duration_ms = MAX_RUN_TIME_MS;
    }
    
    return pump_start_internal(pump, duration_ms);
}

esp_err_t pump_controller_run_dose(pump_id_t pump, float dose_ml) {
    if (pump >= PUMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (dose_ml <= 0.0f) {
        ESP_LOGW(TAG, "Invalid dose: %.2f ml", dose_ml);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Расчёт времени работы по калибровке
    uint32_t duration_ms = (uint32_t)((dose_ml / s_pumps[pump].ml_per_sec) * 1000.0f);
    
    // Ограничение макс времени
    if (duration_ms > MAX_RUN_TIME_MS) {
        ESP_LOGW(TAG, "Dose %.2f ml requires %lu ms, limiting to %d ms",
                 dose_ml, (unsigned long)duration_ms, MAX_RUN_TIME_MS);
        duration_ms = MAX_RUN_TIME_MS;
    }
    
    ESP_LOGI(TAG, "Pump %d: dose %.2f ml = %lu ms", 
             pump, dose_ml, (unsigned long)duration_ms);
    
    return pump_start_internal(pump, duration_ms);
}

esp_err_t pump_controller_stop(pump_id_t pump) {
    if (pump >= PUMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return pump_stop_internal(pump);
}

esp_err_t pump_controller_emergency_stop(void) {
    ESP_LOGW(TAG, "EMERGENCY STOP - all pumps");
    
    for (int i = 0; i < PUMP_MAX; i++) {
        pump_stop_internal((pump_id_t)i);
    }
    
    return ESP_OK;
}

esp_err_t pump_controller_get_stats(pump_id_t pump, pump_stats_t *stats) {
    if (pump >= PUMP_MAX || stats == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memcpy(stats, &s_pumps[pump].stats, sizeof(pump_stats_t));
    return ESP_OK;
}

float pump_controller_get_total_ml(pump_id_t pump) {
    if (pump >= PUMP_MAX) {
        return 0.0f;
    }
    
    return s_pumps[pump].stats.total_ml;
}

esp_err_t pump_controller_reset_stats(pump_id_t pump) {
    if (pump >= PUMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    memset(&s_pumps[pump].stats, 0, sizeof(pump_stats_t));
    ESP_LOGI(TAG, "Pump %d stats reset", pump);
    return ESP_OK;
}

esp_err_t pump_controller_set_calibration(pump_id_t pump, float ml_per_sec) {
    if (pump >= PUMP_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (ml_per_sec <= 0.0f || ml_per_sec > 10.0f) {
        ESP_LOGE(TAG, "Invalid calibration: %.2f ml/s", ml_per_sec);
        return ESP_ERR_INVALID_ARG;
    }
    
    s_pumps[pump].ml_per_sec = ml_per_sec;
    ESP_LOGI(TAG, "Pump %d calibration: %.2f ml/s", pump, ml_per_sec);
    return ESP_OK;
}

bool pump_controller_is_running(pump_id_t pump) {
    if (pump >= PUMP_MAX) {
        return false;
    }
    
    return s_pumps[pump].is_running;
}

// Внутренние функции
static esp_err_t pump_start_internal(pump_id_t pump, uint32_t duration_ms) {
    if (s_pumps[pump].is_running) {
        ESP_LOGW(TAG, "Pump %d already running", pump);
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Pump %d START (%lu ms)", pump, (unsigned long)duration_ms);
    
    // Включение PWM (100% duty)
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, (ledc_channel_t)pump, PWM_MAX_DUTY));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, (ledc_channel_t)pump));
    
    // Обновление состояния
    s_pumps[pump].is_running = true;
    s_pumps[pump].start_time = esp_timer_get_time() / 1000; // мс
    s_pumps[pump].run_duration_ms = duration_ms;
    
    // Запуск таймера автоостановки
    xTimerChangePeriod(s_pumps[pump].timer, pdMS_TO_TICKS(duration_ms), 0);
    xTimerStart(s_pumps[pump].timer, 0);
    
    return ESP_OK;
}

static esp_err_t pump_stop_internal(pump_id_t pump) {
    if (!s_pumps[pump].is_running) {
        return ESP_OK; // Уже остановлен
    }
    
    // Выключение PWM
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, (ledc_channel_t)pump, 0));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, (ledc_channel_t)pump));
    
    // Остановка таймера
    xTimerStop(s_pumps[pump].timer, 0);
    
    // Обновление статистики
    uint64_t current_time = esp_timer_get_time() / 1000; // мс
    uint64_t actual_time = current_time - s_pumps[pump].start_time;
    
    s_pumps[pump].stats.total_runs++;
    s_pumps[pump].stats.total_time_ms += actual_time;
    s_pumps[pump].stats.last_run_time = current_time;
    
    // Расчёт объёма
    float ml = (actual_time / 1000.0f) * s_pumps[pump].ml_per_sec;
    s_pumps[pump].stats.total_ml += ml;
    
    ESP_LOGI(TAG, "Pump %d STOP (%.2f ml, %llu ms)", 
             pump, ml, (unsigned long long)actual_time);
    
    s_pumps[pump].is_running = false;
    
    return ESP_OK;
}

// Callback таймера автоостановки
static void pump_timer_callback(TimerHandle_t timer) {
    pump_id_t pump = (pump_id_t)(uintptr_t)pvTimerGetTimerID(timer);
    
    ESP_LOGD(TAG, "Timer callback for pump %d", pump);
    pump_stop_internal(pump);
}

