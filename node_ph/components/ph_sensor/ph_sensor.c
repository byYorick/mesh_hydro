/**
 * @file ph_sensor.c
 * @brief Trema pH sensor driver implementation
 */

#include "ph_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ph_sensor";

static i2c_port_t s_i2c_port = I2C_NUM_0;
static bool s_initialized = false;
static float s_last_value = 7.0f;

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
        ESP_LOGW(TAG, "pH sensor not found at 0x%02X - will use default values", PH_SENSOR_ADDR);
        s_initialized = false;
        return ESP_ERR_NOT_FOUND;
    }
    
    s_initialized = true;
    ESP_LOGI(TAG, "pH sensor initialized");
    return ESP_OK;
}

esp_err_t ph_sensor_read(float *ph) {
    if (ph == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Если датчик не инициализирован - возвращаем дефолтное значение
    if (!s_initialized) {
        *ph = 7.0f;
        return ESP_OK;
    }
    
    uint8_t data[2];
    esp_err_t ret = ph_read_register(REG_PH_VALUE, data, 2);
    
    if (ret == ESP_OK) {
        // Преобразование: pH = (data[0] << 8 | data[1]) / 100.0
        uint16_t raw = (data[0] << 8) | data[1];
        s_last_value = raw / 100.0f;
        *ph = s_last_value;
        
        ESP_LOGD(TAG, "pH read: %.2f", *ph);
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
    return s_initialized;
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

