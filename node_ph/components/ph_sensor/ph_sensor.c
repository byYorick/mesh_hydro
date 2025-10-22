/**
 * @file ph_sensor.c
 * @brief Trema pH sensor driver implementation
 */

#include "ph_sensor.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ph_sensor";

static i2c_port_t s_i2c_port = I2C_NUM_0;
static bool s_initialized = false;
static float s_last_value = 7.0f;
static bool s_mock_mode = false;  // Mock режим для тестирования
static uint32_t s_error_count = 0;  // Счетчик ошибок чтения
static uint32_t s_last_error_time = 0;  // Время последней ошибки

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
        ESP_LOGW(TAG, "pH sensor not found at 0x%02X - MOCK MODE enabled", PH_SENSOR_ADDR);
        ESP_LOGW(TAG, "Mock pH will vary 6.0-7.5 for testing");
        s_initialized = true;  // Включаем для Mock режима
        s_mock_mode = true;
        s_last_value = 6.5f;   // Устанавливаем начальное значение
        ESP_LOGI(TAG, "MOCK mode initialized with pH=%.2f", s_last_value);
        return ESP_OK;         // Возвращаем OK для Mock режима
    }
    
    s_initialized = true;
    s_mock_mode = false;
    ESP_LOGI(TAG, "pH sensor initialized (REAL MODE)");
    return ESP_OK;
}

esp_err_t ph_sensor_read(float *ph) {
    if (ph == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Если датчик не инициализирован - MOCK MODE с рандомными значениями
    if (!s_initialized || s_mock_mode) {
        // Генерация реалистичных значений pH (6.0-7.5 с плавными изменениями)
        static float mock_ph = 6.5f;
        static int mock_direction = 1;
        static bool mock_initialized = false;
        
        // Инициализация MOCK режима
        if (!mock_initialized) {
            mock_ph = 6.5f;
            mock_direction = 1;
            mock_initialized = true;
            ESP_LOGI(TAG, "MOCK pH generator initialized at %.2f", mock_ph);
        }
        
        // Плавное изменение ±0.01 каждое чтение
        mock_ph += (mock_direction ? 0.01f : -0.01f);
        
        // Разворачиваемся на границах
        if (mock_ph >= 7.5f) mock_direction = 0;
        if (mock_ph <= 6.0f) mock_direction = 1;
        
        // Добавляем небольшой шум (более безопасный)
        int32_t noise_raw = esp_random() % 21;  // 0-20
        float noise = (noise_raw - 10) / 1000.0f;  // ±0.01
        float final_ph = mock_ph + noise;
        
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
        s_mock_mode = true;
        s_last_value = 6.5f;
        ESP_LOGI(TAG, "Forced to MOCK mode");
    } else {
        s_mock_mode = false;
        s_error_count = 0;  // Сброс счетчика ошибок
        ESP_LOGI(TAG, "MOCK mode disabled, attempting real sensor");
    }
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

