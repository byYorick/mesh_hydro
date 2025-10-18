/**
 * @file sensor_manager.c
 * @brief Реализация менеджера датчиков (ЗАГЛУШКА)
 * 
 * TODO: Портировать реальную реализацию из hydro1.0/components/sensor_manager/
 */

#include "sensor_manager.h"
#include "esp_log.h"

static const char *TAG = "sensor_manager";

esp_err_t sensor_manager_init(void) {
    ESP_LOGI(TAG, "Sensor Manager initialized (STUB - TODO: port from hydro1.0)");
    
    // TODO: Инициализация I2C
    // TODO: Инициализация Trema pH датчика (0x0A)
    // TODO: Инициализация Trema EC датчика (0x08)
    // TODO: Загрузка калибровки из NVS
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_ph(float *ph) {
    if (!ph) return ESP_ERR_INVALID_ARG;
    
    // TODO: Реальное чтение с Trema pH датчика
    *ph = 6.5;  // Заглушка
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_ec(float *ec) {
    if (!ec) return ESP_ERR_INVALID_ARG;
    
    // TODO: Реальное чтение с Trema EC датчика
    *ec = 1.8;  // Заглушка
    
    return ESP_OK;
}

esp_err_t sensor_manager_read_temp(float *temp) {
    if (!temp) return ESP_ERR_INVALID_ARG;
    
    // TODO: Чтение температуры (встроен в pH или EC датчик)
    *temp = 24.0;  // Заглушка
    
    return ESP_OK;
}

esp_err_t sensor_manager_calibrate_ph(float calibration_value) {
    ESP_LOGI(TAG, "pH calibration: %.2f", calibration_value);
    
    // TODO: Калибровка pH датчика
    // TODO: Сохранение в NVS
    
    return ESP_OK;
}

esp_err_t sensor_manager_calibrate_ec(float calibration_value) {
    ESP_LOGI(TAG, "EC calibration: %.2f", calibration_value);
    
    // TODO: Калибровка EC датчика
    // TODO: Сохранение в NVS
    
    return ESP_OK;
}

