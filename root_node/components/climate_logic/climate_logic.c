/**
 * @file climate_logic.c
 * @brief Реализация резервной логики климата
 */

#include "climate_logic.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "climate_logic";

esp_err_t climate_logic_init(void) {
    xTaskCreate(climate_logic_task, "climate_logic", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Climate logic initialized");
    return ESP_OK;
}

void climate_logic_task(void *arg) {
    while (1) {
        // TODO: Реализовать fallback логику
        // Если Climate node offline > 30 сек:
        //   - Открывать форточки на 30 мин каждые 3 часа
        //   - Включать вентилятор на 15 мин каждый час
        
        vTaskDelay(pdMS_TO_TICKS(60000));  // Каждую минуту
    }
}

