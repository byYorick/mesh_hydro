/**
 * @file ec_sensor.c
 * @brief Trema EC sensor driver implementation
 */

#include "ec_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ec_sensor";

static i2c_port_t s_i2c_port = I2C_NUM_0;
static bool s_initialized = false;
static float s_last_value = 2.0f; // Дефолтное значение EC

// Регистры Trema EC
#define REG_EC_VALUE    0x22
#define REG_TDS_VALUE   0x24
#define REG_EC_CALIB    0x10
#define REG_EC_TEMP     0x0C

// Внутренние функции
static esp_err_t ec_read_register(uint8_t reg, uint8_t *data, size_t len);
static esp_err_t ec_write_register(uint8_t reg, uint8_t *data, size_t len);

esp_err_t ec_sensor_init(i2c_port_t i2c_port) {
    s_i2c_port = i2c_port;
    
    // Проверка подключения
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (EC_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "EC sensor not found at 0x%02X - will use default values", EC_SENSOR_ADDR);
        s_initialized = false;
        return ESP_ERR_NOT_FOUND;
    }
    
    s_initialized = true;
    ESP_LOGI(TAG, "EC sensor initialized");
    return ESP_OK;
}

esp_err_t ec_sensor_read(float *ec) {
    if (ec == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Если датчик не инициализирован - возвращаем дефолтное значение
    if (!s_initialized) {
        *ec = 2.0f;
        return ESP_OK;
    }
    
    uint8_t data[2];
    esp_err_t ret = ec_read_register(REG_EC_VALUE, data, 2);
    
    if (ret == ESP_OK) {
        // Преобразование: EC = (data[0] << 8 | data[1]) / 100.0
        uint16_t raw = (data[0] << 8) | data[1];
        s_last_value = raw / 100.0f;
        *ec = s_last_value;
        
        ESP_LOGD(TAG, "EC read: %.2f mS/cm", *ec);
    } else {
        // При ошибке возвращаем последнее значение
        *ec = s_last_value;
        ESP_LOGW(TAG, "EC read failed, using last value: %.2f", *ec);
    }
    
    return ret;
}

esp_err_t ec_sensor_calibrate(uint16_t reference_tds) {
    if (!s_initialized) {
        ESP_LOGW(TAG, "Cannot calibrate - sensor not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (reference_tds > 10000) {
        ESP_LOGE(TAG, "Invalid reference TDS: %d ppm", reference_tds);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Запись эталонного значения TDS
    uint8_t data[2] = {(reference_tds >> 8) & 0xFF, reference_tds & 0xFF};
    
    esp_err_t ret = ec_write_register(0x0A, data, 2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reference TDS");
        return ret;
    }
    
    // Запуск калибровки
    uint8_t calib_cmd = 0x01;
    ret = ec_write_register(REG_EC_CALIB, &calib_cmd, 1);
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "EC calibration started with reference %d ppm", reference_tds);
        vTaskDelay(pdMS_TO_TICKS(2000)); // Ждём завершения калибровки
    }
    
    return ret;
}

esp_err_t ec_sensor_set_temperature(float temperature) {
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (temperature < 0.0f || temperature > 63.75f) {
        ESP_LOGE(TAG, "Invalid temperature: %.2f °C", temperature);
        return ESP_ERR_INVALID_ARG;
    }
    
    // Температура передаётся с шагом 0.25°C
    uint8_t temp_raw = (uint8_t)(temperature * 4.0f);
    
    esp_err_t ret = ec_write_register(REG_EC_TEMP, &temp_raw, 1);
    
    if (ret == ESP_OK) {
        ESP_LOGD(TAG, "Temperature set: %.2f °C", temperature);
    }
    
    return ret;
}

bool ec_sensor_is_connected(void) {
    return s_initialized;
}

// Внутренние функции
static esp_err_t ec_read_register(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Запись адреса регистра
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (EC_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    
    // Чтение данных
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (EC_SENSOR_ADDR << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

static esp_err_t ec_write_register(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (EC_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    return ret;
}

