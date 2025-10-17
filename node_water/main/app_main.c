/**
 * @file app_main.c
 * @brief NODE Water - управление водой
 */

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "water_main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== NODE Water Starting ===");
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // TODO: Насосы, клапаны, датчик уровня
    
    ESP_LOGI(TAG, "=== NODE Water Running ===");
}

