/**
 * @file app_main.c
 * @brief NODE Climate - датчики климата
 */

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "climate_main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== NODE Climate Starting ===");
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // TODO: SHT3x, CCS811, Lux sensors
    
    ESP_LOGI(TAG, "=== NODE Climate Running ===");
}

