/**
 * @file app_main.c
 * @brief NODE Relay - управление климатом
 */

#include "esp_log.h"
#include "nvs_flash.h"

static const char *TAG = "relay_main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== NODE Relay Starting ===");
    ESP_ERROR_CHECK(nvs_flash_init());
    
    // TODO: Актуаторы, вентиляция, PWM свет
    
    ESP_LOGI(TAG, "=== NODE Relay Running ===");
}

