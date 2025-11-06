/**
 * @file ph_sensor.c
 * @brief Trema pH sensor driver implementation
 */

#include "ph_sensor.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ph_sensor";

static i2c_port_t s_i2c_port = I2C_NUM_0;
static bool s_initialized = false;
static float s_last_value = 7.0f;
static bool s_mock_mode = false;  // Mock режим для тестирования
static ph_sensor_mode_t s_sensor_mode = PH_SENSOR_MODE_REAL;  // Режим работы датчика
static uint32_t s_error_count = 0;  // Счетчик ошибок чтения
static uint32_t s_last_error_time = 0;  // Время последней ошибки

// Mock режим - глобальные переменные для обновления через дозирование
static float s_mock_ph_base = 6.5f;  // Базовое значение pH в mock режиме
static int s_mock_direction = 1;      // Направление дрифта
static bool s_mock_initialized = false;
static uint64_t s_mock_last_update_time = 0;

// Регистры Trema pH
#define REG_PH_VALUE    0x1D
#define REG_PH_CALIB    0x10
#define REG_PH_ERROR    0x1F

// Внутренние функции
static esp_err_t ph_read_register(uint8_t reg, uint8_t *data, size_t len);
static esp_err_t ph_write_register(uint8_t reg, uint8_t *data, size_t len);

esp_err_t ph_sensor_init(i2c_port_t i2c_port) {
    s_i2c_port = i2c_port;
    
    // Проверка подключения
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PH_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "pH sensor not found at 0x%02X", PH_SENSOR_ADDR);
        // Если реальный датчик не найден, но режим не установлен явно - используем режим из конфига
        // или включаем mock режим по умолчанию
        if (s_sensor_mode == PH_SENSOR_MODE_REAL) {
            s_sensor_mode = PH_SENSOR_MODE_MOCK_REACTIVE;  // По умолчанию mock с реакцией
            ESP_LOGW(TAG, "Real sensor not found, switching to mock reactive mode");
        }
        s_initialized = true;
        s_mock_mode = (s_sensor_mode != PH_SENSOR_MODE_REAL);
        s_last_value = 6.5f;
        ESP_LOGI(TAG, "MOCK mode initialized with pH=%.2f (mode=%d)", s_last_value, s_sensor_mode);
        return ESP_OK;
    }
    
    s_initialized = true;
    // Если реальный датчик найден, проверяем режим
    if (s_sensor_mode == PH_SENSOR_MODE_REAL) {
        s_mock_mode = false;
        ESP_LOGI(TAG, "pH sensor initialized (REAL MODE)");
    } else {
        s_mock_mode = true;
        ESP_LOGW(TAG, "pH sensor found but using MOCK mode (mode=%d)", s_sensor_mode);
    }
    return ESP_OK;
}

esp_err_t ph_sensor_read(float *ph) {
    if (ph == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Если датчик не инициализирован или в mock режиме
    if (!s_initialized || s_mock_mode) {
        // Инициализация MOCK режима
        if (!s_mock_initialized) {
            s_mock_ph_base = 6.5f;
            s_mock_direction = 1;
            s_mock_initialized = true;
            s_mock_last_update_time = esp_timer_get_time();
            ESP_LOGI(TAG, "MOCK pH generator initialized at %.2f", s_mock_ph_base);
        }
        
        // Медленное естественное изменение ±0.001 каждые 10 секунд (имитация дрифта)
        // Только для режима MOCK_NON_REACTIVE, для MOCK_REACTIVE дрифт минимальный
        if (s_sensor_mode == PH_SENSOR_MODE_MOCK_NON_REACTIVE) {
            uint64_t now = esp_timer_get_time();
            if (now - s_mock_last_update_time > 10000000ULL) {  // 10 секунд
                s_mock_ph_base += (s_mock_direction ? 0.01f : -0.01f);  // Больший дрифт для non-reactive
                s_mock_last_update_time = now;
                
                // Разворачиваемся на границах (более широкие границы для естественного дрифта)
                if (s_mock_ph_base >= 7.5f) s_mock_direction = 0;
                if (s_mock_ph_base <= 6.0f) s_mock_direction = 1;
            }
        } else if (s_sensor_mode == PH_SENSOR_MODE_MOCK_REACTIVE) {
            // Для reactive режима дрифт минимальный - только шум
            uint64_t now = esp_timer_get_time();
            if (now - s_mock_last_update_time > 30000000ULL) {  // 30 секунд - очень медленный дрифт
                s_mock_ph_base += (s_mock_direction ? 0.001f : -0.001f);
                s_mock_last_update_time = now;
                
                if (s_mock_ph_base >= 7.5f) s_mock_direction = 0;
                if (s_mock_ph_base <= 6.0f) s_mock_direction = 1;
            }
        }
        
        // Добавляем небольшой шум (более безопасный)
        int32_t noise_raw = esp_random() % 21;  // 0-20
        float noise = (noise_raw - 10) / 1000.0f;  // ±0.01
        float final_ph = s_mock_ph_base + noise;
        
        // ВАЖНО: Валидация mock данных тоже!
        if (final_ph < 0.0f || final_ph > 14.0f) {
            ESP_LOGE(TAG, "MOCK ERROR: Invalid pH %.2f, using safe value", final_ph);
            final_ph = 6.5f;  // Безопасное значение
        }
        
        *ph = final_ph;
        
        // Обновляем s_last_value для совместимости
        s_last_value = *ph;
        
        ESP_LOGI(TAG, "pH MOCK: %.2f (target: 6.50)", *ph);
        return ESP_OK;
    }
    
    uint8_t data[2];
    esp_err_t ret = ph_read_register(REG_PH_VALUE, data, 2);
    
    if (ret == ESP_OK) {
        // Преобразование: pH = (data[0] << 8 | data[1]) / 100.0
        uint16_t raw = (data[0] << 8) | data[1];
        float ph_value = raw / 100.0f;
        
        // ВАЖНО: Валидация данных pH
        if (ph_value < 0.0f || ph_value > 14.0f || raw == 0xFFFF || raw == 0x0000) {
            s_error_count++;
            s_last_error_time = xTaskGetTickCount();
            ESP_LOGE(TAG, "INVALID pH data: raw=0x%04X (%.2f) - error #%lu", raw, ph_value, s_error_count);
            
            // Автоматическое переключение в mock режим после 5 ошибок подряд
            if (s_error_count >= 5 && !s_mock_mode) {
                ESP_LOGW(TAG, "Too many pH errors (%lu) - switching to MOCK mode", s_error_count);
                s_mock_mode = true;
                s_last_value = 6.5f;  // Устанавливаем разумное значение
            }
            
            *ph = s_last_value;
            ESP_LOGW(TAG, "Using last valid pH: %.2f", s_last_value);
        } else {
            // Сброс счетчика ошибок при успешном чтении
            if (s_error_count > 0) {
                ESP_LOGI(TAG, "pH sensor recovered after %lu errors", s_error_count);
                s_error_count = 0;
            }
            
            s_last_value = ph_value;
            *ph = ph_value;
            ESP_LOGD(TAG, "pH read: %.2f (raw=0x%04X)", ph_value, raw);
        }
    } else {
        // При ошибке возвращаем последнее значение
        *ph = s_last_value;
        ESP_LOGW(TAG, "pH read failed, using last value: %.2f", *ph);
    }
    
    return ret;
}

esp_err_t ph_sensor_calibrate(float reference_ph) {
    if (!s_initialized) {
        ESP_LOGW(TAG, "Cannot calibrate - sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (reference_ph < 0.0f || reference_ph > 14.0f) {
        ESP_LOGE(TAG, "Invalid reference pH: %.2f", reference_ph);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Запись эталонного значения
    uint16_t ref_raw = (uint16_t)(reference_ph * 100.0f);
    uint8_t data[2] = {(ref_raw >> 8) & 0xFF, ref_raw & 0xFF};
    
    esp_err_t ret = ph_write_register(0x0A, data, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reference pH");
        return ret;
    }
    
    // Запуск калибровки
    uint8_t calib_cmd = 0x01;
    ret = ph_write_register(REG_PH_CALIB, &calib_cmd, 1);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "pH calibration started with reference %.2f", reference_ph);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Ждём завершения калибровки
    }
    
    return ret;
}

bool ph_sensor_is_stable(void) {
    if (!s_initialized) {
        return true; // Считаем стабильным если датчика нет
    }
    
    uint8_t error;
    if (ph_read_register(REG_PH_ERROR, &error, 1) == ESP_OK) {
        return (error & 0x02) == 0; // Бит 1 = stability error
    }
    
    return true;
}

bool ph_sensor_is_connected(void) {
    return s_initialized && !s_mock_mode;
}

bool ph_sensor_is_mock_mode(void) {
    return s_mock_mode;
}

esp_err_t ph_sensor_force_mock_mode(bool enable) {
    if (enable) {
        s_sensor_mode = PH_SENSOR_MODE_MOCK_REACTIVE;  // По умолчанию mock с реакцией
        s_mock_mode = true;
        s_last_value = 6.5f;
        s_mock_ph_base = 6.5f;
        s_mock_initialized = false;  // Сброс для переинициализации
        ESP_LOGI(TAG, "Forced to MOCK reactive mode");
    } else {
        s_sensor_mode = PH_SENSOR_MODE_REAL;
        s_mock_mode = false;
        s_error_count = 0;  // Сброс счетчика ошибок
        ESP_LOGI(TAG, "MOCK mode disabled, attempting real sensor");
    }
    return ESP_OK;
}

esp_err_t ph_sensor_set_mode(ph_sensor_mode_t mode) {
    s_sensor_mode = mode;
    
    switch (mode) {
        case PH_SENSOR_MODE_REAL:
            s_mock_mode = false;
            ESP_LOGI(TAG, "Sensor mode set to REAL");
            break;
        case PH_SENSOR_MODE_MOCK_REACTIVE:
            s_mock_mode = true;
            s_mock_ph_base = 6.5f;
            s_mock_initialized = false;
            ESP_LOGI(TAG, "Sensor mode set to MOCK REACTIVE");
            break;
        case PH_SENSOR_MODE_MOCK_NON_REACTIVE:
            s_mock_mode = true;
            s_mock_ph_base = 6.5f;
            s_mock_initialized = false;
            ESP_LOGI(TAG, "Sensor mode set to MOCK NON-REACTIVE");
            break;
        default:
            ESP_LOGW(TAG, "Unknown sensor mode: %d, using REAL", mode);
            s_sensor_mode = PH_SENSOR_MODE_REAL;
            s_mock_mode = false;
            break;
    }
    
    return ESP_OK;
}

ph_sensor_mode_t ph_sensor_get_mode(void) {
    return s_sensor_mode;
}

esp_err_t ph_sensor_update_mock_for_dosing(uint8_t pump_id, float dose_ml) {
    // Обновляем только в режиме MOCK_REACTIVE
    if (s_sensor_mode != PH_SENSOR_MODE_MOCK_REACTIVE) {
        return ESP_OK;
    }
    
    // Коэффициент эффективности дозирования (pH изменение на мл)
    // pH DOWN (кислота): уменьшает pH, примерно -0.01 pH на 1 мл
    // pH UP (щелочь): увеличивает pH, примерно +0.01 pH на 1 мл
    const float ph_change_per_ml = 0.01f;
    
    float ph_change = 0.0f;
    if (pump_id == 1) {  // PUMP_PH_DOWN
        ph_change = -dose_ml * ph_change_per_ml;  // Кислота снижает pH
    } else if (pump_id == 0) {  // PUMP_PH_UP
        ph_change = dose_ml * ph_change_per_ml;   // Щелочь увеличивает pH
    }
    
    s_mock_ph_base += ph_change;
    
    // Ограничение диапазона pH
    if (s_mock_ph_base < 4.0f) s_mock_ph_base = 4.0f;
    if (s_mock_ph_base > 10.0f) s_mock_ph_base = 10.0f;
    
    // Обновляем s_last_value для следующего чтения
    s_last_value = s_mock_ph_base;
    
    ESP_LOGI(TAG, "MOCK pH updated: pump=%d, dose=%.2f ml, pH_change=%.3f, new_pH=%.2f", 
             pump_id, dose_ml, ph_change, s_mock_ph_base);
    
    return ESP_OK;
}

// Внутренние функции
static esp_err_t ph_read_register(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Запись адреса регистра
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PH_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    
    // Чтение данных
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PH_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    // Детальное логирование ошибок I2C
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C read failed: %s (reg=0x%02X, addr=0x%02X)", 
                 esp_err_to_name(ret), reg, PH_SENSOR_ADDR);
    } else {
        ESP_LOGD(TAG, "I2C read OK: reg=0x%02X, data[0]=0x%02X, data[1]=0x%02X", 
                 reg, data[0], data[1]);
    }
    
    return ret;
}

static esp_err_t ph_write_register(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (PH_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

