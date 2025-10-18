/**
 * @file lux_sensor.c
 * @brief Реализация драйвера Trema Lux
 */

#include "lux_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "lux_sensor";

static i2c_port_t s_i2c_port = I2C_NUM_0;

// Регистры Trema Lux
#define LUX_REG_CONTROL     0x00
#define LUX_REG_TIMING      0x01
#define LUX_REG_DATA_LOW    0x0C
#define LUX_REG_DATA_HIGH   0x0D

esp_err_t lux_sensor_init(i2c_port_t i2c_port) {
    s_i2c_port = i2c_port;
    
    ESP_LOGI(TAG, "Lux sensor initializing on I2C port %d, addr 0x%02X", 
             i2c_port, LUX_SENSOR_I2C_ADDR);

    // Включение датчика (power on)
    uint8_t control_data[2] = {LUX_REG_CONTROL, 0x03};  // Power ON
    
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LUX_SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, control_data, 2, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on sensor");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    // Установка времени интеграции (402 ms, high gain)
    uint8_t timing_data[2] = {LUX_REG_TIMING, 0x02};
    
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LUX_SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, timing_data, 2, true);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set timing");
        return ret;
    }

    ESP_LOGI(TAG, "Lux sensor initialized successfully");
    return ESP_OK;
}

esp_err_t lux_sensor_read(uint16_t *lux) {
    if (!lux) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data_low, data_high;

    // Чтение младшего байта
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LUX_SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, LUX_REG_DATA_LOW, true);
    i2c_master_start(cmd);  // Repeated start
    i2c_master_write_byte(cmd, (LUX_SENSOR_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data_low, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read DATA_LOW");
        return ret;
    }

    // Чтение старшего байта
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (LUX_SENSOR_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, LUX_REG_DATA_HIGH, true);
    i2c_master_start(cmd);  // Repeated start
    i2c_master_write_byte(cmd, (LUX_SENSOR_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, &data_high, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read DATA_HIGH");
        return ret;
    }

    // Комбинирование байтов
    uint16_t raw = (data_high << 8) | data_low;
    
    // Конвертация в lux (упрощенная формула)
    *lux = raw;  // Для Trema можно использовать прямое значение

    ESP_LOGD(TAG, "Lux: %d", *lux);

    return ESP_OK;
}

