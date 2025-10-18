/**
 * @file ccs811_driver.c
 * @brief Реализация драйвера CCS811
 */

#include "ccs811_driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ccs811";

static i2c_port_t s_i2c_port = I2C_NUM_0;

// Регистры CCS811
#define CCS811_REG_STATUS       0x00
#define CCS811_REG_MEAS_MODE    0x01
#define CCS811_REG_ALG_RESULT   0x02
#define CCS811_REG_ENV_DATA     0x05
#define CCS811_REG_APP_START    0xF4
#define CCS811_REG_SW_RESET     0xFF

// Команда для запуска приложения
#define CCS811_BOOTLOADER_APP_START  0xF4

esp_err_t ccs811_init(i2c_port_t i2c_port) {
    s_i2c_port = i2c_port;
    
    ESP_LOGI(TAG, "CCS811 initializing on I2C port %d, addr 0x%02X", i2c_port, CCS811_I2C_ADDR);

    // Запуск приложения CCS811
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CCS811_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, CCS811_REG_APP_START, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start app");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    // Установка режима измерений (Mode 1: каждую секунду)
    uint8_t mode_data[2] = {CCS811_REG_MEAS_MODE, 0x10};
    
    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CCS811_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, mode_data, 2, true);
    i2c_master_stop(cmd);
    
    ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set measure mode");
        return ret;
    }

    ESP_LOGI(TAG, "CCS811 initialized successfully");
    return ESP_OK;
}

esp_err_t ccs811_read(uint16_t *co2, uint16_t *tvoc) {
    if (!co2) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[8];
    
    // Чтение результатов из ALG_RESULT_DATA
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CCS811_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, CCS811_REG_ALG_RESULT, true);
    i2c_master_start(cmd);  // Repeated start
    i2c_master_write_byte(cmd, (CCS811_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, data, 8, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ALG_RESULT");
        return ret;
    }

    // Парсинг данных
    *co2 = (data[0] << 8) | data[1];
    
    if (tvoc) {
        *tvoc = (data[2] << 8) | data[3];
    }

    ESP_LOGD(TAG, "CCS811: CO2=%d ppm, TVOC=%d ppb", *co2, tvoc ? *tvoc : 0);

    return ESP_OK;
}

esp_err_t ccs811_set_environment(float temperature, float humidity) {
    // Конвертация в формат CCS811
    // Temp: (value + 25) * 512
    // Hum: value * 512
    uint16_t temp_conv = (uint16_t)((temperature + 25.0f) * 512.0f);
    uint16_t hum_conv = (uint16_t)(humidity * 512.0f);

    uint8_t data[5] = {
        CCS811_REG_ENV_DATA,
        (hum_conv >> 8) & 0xFF,
        hum_conv & 0xFF,
        (temp_conv >> 8) & 0xFF,
        temp_conv & 0xFF
    };

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CCS811_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, 5, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(s_i2c_port, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);

    ESP_LOGD(TAG, "Environment set: %.1f°C, %.0f%%", temperature, humidity);

    return ret;
}

