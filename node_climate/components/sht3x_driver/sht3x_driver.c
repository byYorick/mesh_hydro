/**
 * @file sht3x_driver.c
 * @brief Реализация драйвера SHT3x
 */

#include "sht3x_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "sht3x";

static i2c_port_t s_i2c_port = I2C_NUM_0;

// Команды SHT3x
#define SHT3X_CMD_MEASURE_HIGH      0x2400  // High repeatability
#define SHT3X_CMD_SOFT_RESET        0x30A2

esp_err_t sht3x_init(i2c_port_t i2c_port) {
    s_i2c_port = i2c_port;
    
    ESP_LOGI(TAG, "SHT3x initializing on I2C port %d, addr 0x%02X", i2c_port, SHT3X_I2C_ADDR);
    
    // Программный сброс
    esp_err_t ret = sht3x_soft_reset();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SHT3x reset failed");
        return ret;
    }
    
    vTaskDelay(pdMS_TO_TICKS(50));  // Ожидание сброса
    
    ESP_LOGI(TAG, "SHT3x initialized successfully");
    return ESP_OK;
}

esp_err_t sht3x_read(float *temperature, float *humidity) {
    if (!temperature || !humidity) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t cmd[2] = {
        (SHT3X_CMD_MEASURE_HIGH >> 8) & 0xFF,
        SHT3X_CMD_MEASURE_HIGH & 0xFF
    };

    // Отправка команды измерения
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (SHT3X_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd_handle, cmd, 2, true);
    i2c_master_stop(cmd_handle);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send measure command");
        return ret;
    }

    // Ожидание завершения измерения
    vTaskDelay(pdMS_TO_TICKS(50));

    // Чтение данных (6 байт: 2 temp + 1 CRC + 2 hum + 1 CRC)
    uint8_t data[6];
    cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (SHT3X_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd_handle, data, 6, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd_handle);
    
    ret = i2c_master_cmd_begin(s_i2c_port, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read data");
        return ret;
    }

    // Расчет температуры
    uint16_t temp_raw = (data[0] << 8) | data[1];
    *temperature = -45.0f + (175.0f * temp_raw / 65535.0f);

    // Расчет влажности
    uint16_t hum_raw = (data[3] << 8) | data[4];
    *humidity = 100.0f * hum_raw / 65535.0f;

    ESP_LOGD(TAG, "SHT3x: %.2f°C, %.1f%%", *temperature, *humidity);

    return ESP_OK;
}

esp_err_t sht3x_soft_reset(void) {
    uint8_t cmd[2] = {
        (SHT3X_CMD_SOFT_RESET >> 8) & 0xFF,
        SHT3X_CMD_SOFT_RESET & 0xFF
    };

    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (SHT3X_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd_handle, cmd, 2, true);
    i2c_master_stop(cmd_handle);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);

    return ret;
}

