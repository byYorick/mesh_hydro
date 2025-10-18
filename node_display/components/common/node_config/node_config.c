/**
 * @file node_config.c
 * @brief Реализация NVS конфигурации
 */

#include "node_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "node_config";

esp_err_t node_config_init(void) {
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs to be erased");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS initialized");
    } else {
        ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

esp_err_t node_config_load(void *config, size_t size, const char *namespace) {
    if (config == NULL || namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READONLY, &handle);
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed for namespace '%s': %s", namespace, esp_err_to_name(err));
        return err;
    }

    size_t required_size = size;
    err = nvs_get_blob(handle, "config", config, &required_size);
    
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Config not found in namespace '%s': %s", namespace, esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Config loaded from namespace '%s' (%d bytes)", namespace, required_size);
    }

    nvs_close(handle);
    return err;
}

esp_err_t node_config_save(const void *config, size_t size, const char *namespace) {
    if (config == NULL || namespace == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(namespace, NVS_READWRITE, &handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed for namespace '%s': %s", namespace, esp_err_to_name(err));
        return err;
    }

    err = nvs_set_blob(handle, "config", config, size);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS set_blob failed: %s", esp_err_to_name(err));
        nvs_close(handle);
        return err;
    }

    err = nvs_commit(handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS commit failed: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Config saved to namespace '%s' (%d bytes)", namespace, size);
    }

    nvs_close(handle);
    return err;
}

esp_err_t node_config_reset_to_default(void *config, const char *node_type) {
    if (config == NULL || node_type == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(node_type, "ph_ec") == 0) {
        ph_ec_node_config_t *cfg = (ph_ec_node_config_t *)config;
        memset(cfg, 0, sizeof(ph_ec_node_config_t));
        
        // Базовая конфигурация
        strcpy(cfg->base.node_type, "ph_ec");
        cfg->base.config_valid = true;
        cfg->base.config_version = 1;
        
        // Целевые значения
        cfg->ph_target = 6.8;
        cfg->ec_target = 2.0;
        
        // Диапазоны
        cfg->ph_min = 6.0;
        cfg->ph_max = 7.5;
        cfg->ec_min = 1.2;
        cfg->ec_max = 2.5;
        
        // Аварийные пороги
        cfg->ph_emergency_low = 5.0;
        cfg->ph_emergency_high = 8.0;
        cfg->ec_emergency_high = 3.0;
        
        // PID параметры (примерные значения)
        for (int i = 0; i < 5; i++) {
            cfg->pump_pid[i].kp = 1.0;
            cfg->pump_pid[i].ki = 0.5;
            cfg->pump_pid[i].kd = 0.1;
            cfg->pump_pid[i].output_min = 0.0;
            cfg->pump_pid[i].output_max = 1.0;
            cfg->pump_pid[i].enabled = true;
        }
        
        // Safety
        cfg->max_pump_time_ms = 10000;  // 10 сек макс
        cfg->cooldown_ms = 60000;       // 1 мин между включениями
        cfg->max_daily_volume_ml = 1000; // 1 литр в день
        
        // Автономия
        cfg->autonomous_enabled = true;
        cfg->mesh_timeout_ms = 30000;   // 30 сек
        
        ESP_LOGI(TAG, "Reset to default: pH/EC config");
        
    } else if (strcmp(node_type, "climate") == 0) {
        climate_node_config_t *cfg = (climate_node_config_t *)config;
        memset(cfg, 0, sizeof(climate_node_config_t));
        
        strcpy(cfg->base.node_type, "climate");
        cfg->base.config_valid = true;
        cfg->base.config_version = 1;
        
        cfg->temp_target = 24.0;
        cfg->humidity_target = 65.0;
        cfg->co2_max = 800;
        cfg->lux_min = 500;
        cfg->read_interval_ms = 10000;  // 10 сек
        
        ESP_LOGI(TAG, "Reset to default: Climate config");
        
    } else if (strcmp(node_type, "relay") == 0) {
        relay_node_config_t *cfg = (relay_node_config_t *)config;
        memset(cfg, 0, sizeof(relay_node_config_t));
        
        strcpy(cfg->base.node_type, "relay");
        cfg->base.config_valid = true;
        cfg->base.config_version = 1;
        
        // Свет
        cfg->light.brightness = 80;
        strcpy(cfg->light.schedule_on, "08:00");
        strcpy(cfg->light.schedule_off, "22:00");
        cfg->light.schedule_enabled = true;
        cfg->light.lux_control = true;
        cfg->light.lux_threshold = 500;
        
        // Вентиляция
        cfg->ventilation.co2_threshold = 800;
        cfg->ventilation.temp_threshold = 28.0;
        cfg->ventilation.humidity_threshold = 75.0;
        
        // Форточки
        cfg->windows.open_time_ms = 30000;   // 30 сек
        cfg->windows.close_time_ms = 30000;  // 30 сек
        
        ESP_LOGI(TAG, "Reset to default: Relay config");
        
    } else if (strcmp(node_type, "water") == 0) {
        water_node_config_t *cfg = (water_node_config_t *)config;
        memset(cfg, 0, sizeof(water_node_config_t));
        
        strcpy(cfg->base.node_type, "water");
        cfg->base.config_valid = true;
        cfg->base.config_version = 1;
        
        cfg->pump_on_time_ms = 5000;     // 5 сек
        cfg->pump_off_time_ms = 300000;  // 5 мин
        cfg->active_zones = 0x07;        // 3 зоны активны
        cfg->max_pump_time_ms = 60000;   // 60 сек макс
        cfg->water_level_check = true;
        
        ESP_LOGI(TAG, "Reset to default: Water config");
        
    } else {
        ESP_LOGE(TAG, "Unknown node type: %s", node_type);
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

esp_err_t node_config_update_from_json(void *config, cJSON *json_config, const char *node_type) {
    // TODO: Реализовать парсинг JSON и обновление конфигурации
    ESP_LOGW(TAG, "update_from_json not fully implemented yet");
    return ESP_ERR_NOT_SUPPORTED;
}

cJSON* node_config_export_to_json(const void *config, const char *node_type) {
    // TODO: Реализовать экспорт конфигурации в JSON
    ESP_LOGW(TAG, "export_to_json not fully implemented yet");
    return NULL;
}

esp_err_t node_config_erase_all(void) {
    esp_err_t err = nvs_flash_erase();
    
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS erased (factory reset)");
        err = nvs_flash_init();
    } else {
        ESP_LOGE(TAG, "NVS erase failed: %s", esp_err_to_name(err));
    }
    
    return err;
}

