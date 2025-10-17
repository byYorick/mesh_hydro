/**
 * @file app_main.c
 * @brief NODE Display - TFT экран с LVGL
 */

#include <stdio.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "mesh_manager.h"

static const char *TAG = "display_main";

void app_main(void)
{
    ESP_LOGI(TAG, "=== NODE Display Starting ===");
    ESP_ERROR_CHECK(nvs_flash_init());

    // TODO: Инициализация LVGL, LCD, Encoder из hydro1.0
    
    ESP_LOGI(TAG, "=== NODE Display Running ===");
}

