/**
 * @file node_config.c
 * @brief Реализация NVS конфигурации
 */

#include "node_config.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

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
    if (config == NULL || json_config == NULL || node_type == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (strcmp(node_type, "ph_ec") == 0) {
        ph_ec_node_config_t *cfg = (ph_ec_node_config_t *)config;
        
        // Обновление базовых полей
        cJSON *item;
        if ((item = cJSON_GetObjectItem(json_config, "node_id")) && cJSON_IsString(item)) {
            strncpy(cfg->base.node_id, item->valuestring, sizeof(cfg->base.node_id) - 1);
        }
        if ((item = cJSON_GetObjectItem(json_config, "zone")) && cJSON_IsString(item)) {
            strncpy(cfg->base.zone, item->valuestring, sizeof(cfg->base.zone) - 1);
        }
        
        // Целевые значения
        if ((item = cJSON_GetObjectItem(json_config, "ph_target")) && cJSON_IsNumber(item)) {
            cfg->ph_target = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ec_target")) && cJSON_IsNumber(item)) {
            cfg->ec_target = (float)item->valuedouble;
        }
        
        // Диапазоны
        if ((item = cJSON_GetObjectItem(json_config, "ph_min")) && cJSON_IsNumber(item)) {
            cfg->ph_min = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ph_max")) && cJSON_IsNumber(item)) {
            cfg->ph_max = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ec_min")) && cJSON_IsNumber(item)) {
            cfg->ec_min = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ec_max")) && cJSON_IsNumber(item)) {
            cfg->ec_max = (float)item->valuedouble;
        }
        
        // Аварийные пороги
        if ((item = cJSON_GetObjectItem(json_config, "ph_emergency_low")) && cJSON_IsNumber(item)) {
            cfg->ph_emergency_low = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ph_emergency_high")) && cJSON_IsNumber(item)) {
            cfg->ph_emergency_high = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ec_emergency_high")) && cJSON_IsNumber(item)) {
            cfg->ec_emergency_high = (float)item->valuedouble;
        }
        
        // PID параметры для насосов
        cJSON *pump_pid_array = cJSON_GetObjectItem(json_config, "pump_pid");
        if (pump_pid_array && cJSON_IsArray(pump_pid_array)) {
            int array_size = cJSON_GetArraySize(pump_pid_array);
            int max_pumps = (array_size < 5) ? array_size : 5;
            
            for (int i = 0; i < max_pumps; i++) {
                cJSON *pid_obj = cJSON_GetArrayItem(pump_pid_array, i);
                if (pid_obj) {
                    cJSON *kp, *ki, *kd, *out_min, *out_max, *enabled;
                    if ((kp = cJSON_GetObjectItem(pid_obj, "kp")) && cJSON_IsNumber(kp)) {
                        cfg->pump_pid[i].kp = (float)kp->valuedouble;
                    }
                    if ((ki = cJSON_GetObjectItem(pid_obj, "ki")) && cJSON_IsNumber(ki)) {
                        cfg->pump_pid[i].ki = (float)ki->valuedouble;
                    }
                    if ((kd = cJSON_GetObjectItem(pid_obj, "kd")) && cJSON_IsNumber(kd)) {
                        cfg->pump_pid[i].kd = (float)kd->valuedouble;
                    }
                    if ((out_min = cJSON_GetObjectItem(pid_obj, "output_min")) && cJSON_IsNumber(out_min)) {
                        cfg->pump_pid[i].output_min = (float)out_min->valuedouble;
                    }
                    if ((out_max = cJSON_GetObjectItem(pid_obj, "output_max")) && cJSON_IsNumber(out_max)) {
                        cfg->pump_pid[i].output_max = (float)out_max->valuedouble;
                    }
                    if ((enabled = cJSON_GetObjectItem(pid_obj, "enabled")) && cJSON_IsBool(enabled)) {
                        cfg->pump_pid[i].enabled = cJSON_IsTrue(enabled);
                    }
                }
            }
        }
        
        // Safety параметры
        if ((item = cJSON_GetObjectItem(json_config, "max_pump_time_ms")) && cJSON_IsNumber(item)) {
            cfg->max_pump_time_ms = (uint32_t)item->valueint;
        }
        if ((item = cJSON_GetObjectItem(json_config, "cooldown_ms")) && cJSON_IsNumber(item)) {
            cfg->cooldown_ms = (uint32_t)item->valueint;
        }
        if ((item = cJSON_GetObjectItem(json_config, "max_daily_volume_ml")) && cJSON_IsNumber(item)) {
            cfg->max_daily_volume_ml = (uint32_t)item->valueint;
        }
        
        // Автономия
        if ((item = cJSON_GetObjectItem(json_config, "autonomous_enabled")) && cJSON_IsBool(item)) {
            cfg->autonomous_enabled = cJSON_IsTrue(item);
        }
        if ((item = cJSON_GetObjectItem(json_config, "mesh_timeout_ms")) && cJSON_IsNumber(item)) {
            cfg->mesh_timeout_ms = (uint32_t)item->valueint;
        }
        
        // Калибровка
        if ((item = cJSON_GetObjectItem(json_config, "ph_cal_offset")) && cJSON_IsNumber(item)) {
            cfg->ph_cal_offset = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "ec_cal_offset")) && cJSON_IsNumber(item)) {
            cfg->ec_cal_offset = (float)item->valuedouble;
        }
        
        // Обновление версии и времени
        cfg->base.config_version++;
        cfg->base.last_updated = (uint64_t)time(NULL);
        cfg->base.config_valid = true;
        
        ESP_LOGI(TAG, "pH/EC config updated from JSON (v%d)", cfg->base.config_version);
        
    } else if (strcmp(node_type, "climate") == 0) {
        climate_node_config_t *cfg = (climate_node_config_t *)config;
        
        cJSON *item;
        if ((item = cJSON_GetObjectItem(json_config, "node_id")) && cJSON_IsString(item)) {
            strncpy(cfg->base.node_id, item->valuestring, sizeof(cfg->base.node_id) - 1);
        }
        if ((item = cJSON_GetObjectItem(json_config, "zone")) && cJSON_IsString(item)) {
            strncpy(cfg->base.zone, item->valuestring, sizeof(cfg->base.zone) - 1);
        }
        if ((item = cJSON_GetObjectItem(json_config, "temp_target")) && cJSON_IsNumber(item)) {
            cfg->temp_target = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "humidity_target")) && cJSON_IsNumber(item)) {
            cfg->humidity_target = (float)item->valuedouble;
        }
        if ((item = cJSON_GetObjectItem(json_config, "co2_max")) && cJSON_IsNumber(item)) {
            cfg->co2_max = (uint16_t)item->valueint;
        }
        if ((item = cJSON_GetObjectItem(json_config, "lux_min")) && cJSON_IsNumber(item)) {
            cfg->lux_min = (uint16_t)item->valueint;
        }
        if ((item = cJSON_GetObjectItem(json_config, "read_interval_ms")) && cJSON_IsNumber(item)) {
            cfg->read_interval_ms = (uint32_t)item->valueint;
        }
        
        cfg->base.config_version++;
        cfg->base.last_updated = (uint64_t)time(NULL);
        cfg->base.config_valid = true;
        
        ESP_LOGI(TAG, "Climate config updated from JSON (v%d)", cfg->base.config_version);
        
    } else if (strcmp(node_type, "relay") == 0) {
        relay_node_config_t *cfg = (relay_node_config_t *)config;
        
        cJSON *item;
        if ((item = cJSON_GetObjectItem(json_config, "node_id")) && cJSON_IsString(item)) {
            strncpy(cfg->base.node_id, item->valuestring, sizeof(cfg->base.node_id) - 1);
        }
        
        // Свет
        cJSON *light = cJSON_GetObjectItem(json_config, "light");
        if (light) {
            if ((item = cJSON_GetObjectItem(light, "brightness")) && cJSON_IsNumber(item)) {
                cfg->light.brightness = (uint8_t)item->valueint;
            }
            if ((item = cJSON_GetObjectItem(light, "schedule_enabled")) && cJSON_IsBool(item)) {
                cfg->light.schedule_enabled = cJSON_IsTrue(item);
            }
        }
        
        // Вентиляция
        cJSON *ventilation = cJSON_GetObjectItem(json_config, "ventilation");
        if (ventilation) {
            if ((item = cJSON_GetObjectItem(ventilation, "co2_threshold")) && cJSON_IsNumber(item)) {
                cfg->ventilation.co2_threshold = (uint16_t)item->valueint;
            }
            if ((item = cJSON_GetObjectItem(ventilation, "temp_threshold")) && cJSON_IsNumber(item)) {
                cfg->ventilation.temp_threshold = (float)item->valuedouble;
            }
        }
        
        cfg->base.config_version++;
        cfg->base.last_updated = (uint64_t)time(NULL);
        cfg->base.config_valid = true;
        
        ESP_LOGI(TAG, "Relay config updated from JSON (v%d)", cfg->base.config_version);
        
    } else if (strcmp(node_type, "water") == 0) {
        water_node_config_t *cfg = (water_node_config_t *)config;
        
        cJSON *item;
        if ((item = cJSON_GetObjectItem(json_config, "node_id")) && cJSON_IsString(item)) {
            strncpy(cfg->base.node_id, item->valuestring, sizeof(cfg->base.node_id) - 1);
        }
        if ((item = cJSON_GetObjectItem(json_config, "pump_on_time_ms")) && cJSON_IsNumber(item)) {
            cfg->pump_on_time_ms = (uint32_t)item->valueint;
        }
        if ((item = cJSON_GetObjectItem(json_config, "pump_off_time_ms")) && cJSON_IsNumber(item)) {
            cfg->pump_off_time_ms = (uint32_t)item->valueint;
        }
        if ((item = cJSON_GetObjectItem(json_config, "active_zones")) && cJSON_IsNumber(item)) {
            cfg->active_zones = (uint8_t)item->valueint;
        }
        
        cfg->base.config_version++;
        cfg->base.last_updated = (uint64_t)time(NULL);
        cfg->base.config_valid = true;
        
        ESP_LOGI(TAG, "Water config updated from JSON (v%d)", cfg->base.config_version);
        
    } else {
        ESP_LOGE(TAG, "Unknown node type: %s", node_type);
        return ESP_ERR_INVALID_ARG;
    }

    return ESP_OK;
}

cJSON* node_config_export_to_json(const void *config, const char *node_type) {
    if (config == NULL || node_type == NULL) {
        return NULL;
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return NULL;
    }

    if (strcmp(node_type, "ph_ec") == 0) {
        const ph_ec_node_config_t *cfg = (const ph_ec_node_config_t *)config;
        
        // Базовые поля
        cJSON_AddStringToObject(root, "node_id", cfg->base.node_id);
        cJSON_AddStringToObject(root, "node_type", cfg->base.node_type);
        cJSON_AddStringToObject(root, "zone", cfg->base.zone);
        cJSON_AddNumberToObject(root, "config_version", cfg->base.config_version);
        
        // Целевые значения
        cJSON_AddNumberToObject(root, "ph_target", cfg->ph_target);
        cJSON_AddNumberToObject(root, "ec_target", cfg->ec_target);
        
        // Диапазоны
        cJSON_AddNumberToObject(root, "ph_min", cfg->ph_min);
        cJSON_AddNumberToObject(root, "ph_max", cfg->ph_max);
        cJSON_AddNumberToObject(root, "ec_min", cfg->ec_min);
        cJSON_AddNumberToObject(root, "ec_max", cfg->ec_max);
        
        // Аварийные пороги
        cJSON_AddNumberToObject(root, "ph_emergency_low", cfg->ph_emergency_low);
        cJSON_AddNumberToObject(root, "ph_emergency_high", cfg->ph_emergency_high);
        cJSON_AddNumberToObject(root, "ec_emergency_high", cfg->ec_emergency_high);
        
        // PID параметры для насосов
        cJSON *pump_pid_array = cJSON_CreateArray();
        for (int i = 0; i < 5; i++) {
            cJSON *pid_obj = cJSON_CreateObject();
            cJSON_AddNumberToObject(pid_obj, "kp", cfg->pump_pid[i].kp);
            cJSON_AddNumberToObject(pid_obj, "ki", cfg->pump_pid[i].ki);
            cJSON_AddNumberToObject(pid_obj, "kd", cfg->pump_pid[i].kd);
            cJSON_AddNumberToObject(pid_obj, "output_min", cfg->pump_pid[i].output_min);
            cJSON_AddNumberToObject(pid_obj, "output_max", cfg->pump_pid[i].output_max);
            cJSON_AddBoolToObject(pid_obj, "enabled", cfg->pump_pid[i].enabled);
            cJSON_AddItemToArray(pump_pid_array, pid_obj);
        }
        cJSON_AddItemToObject(root, "pump_pid", pump_pid_array);
        
        // Safety
        cJSON_AddNumberToObject(root, "max_pump_time_ms", cfg->max_pump_time_ms);
        cJSON_AddNumberToObject(root, "cooldown_ms", cfg->cooldown_ms);
        cJSON_AddNumberToObject(root, "max_daily_volume_ml", cfg->max_daily_volume_ml);
        
        // Автономия
        cJSON_AddBoolToObject(root, "autonomous_enabled", cfg->autonomous_enabled);
        cJSON_AddNumberToObject(root, "mesh_timeout_ms", cfg->mesh_timeout_ms);
        
        // Калибровка
        cJSON_AddNumberToObject(root, "ph_cal_offset", cfg->ph_cal_offset);
        cJSON_AddNumberToObject(root, "ec_cal_offset", cfg->ec_cal_offset);
        
        ESP_LOGI(TAG, "pH/EC config exported to JSON");
        
    } else if (strcmp(node_type, "climate") == 0) {
        const climate_node_config_t *cfg = (const climate_node_config_t *)config;
        
        cJSON_AddStringToObject(root, "node_id", cfg->base.node_id);
        cJSON_AddStringToObject(root, "node_type", cfg->base.node_type);
        cJSON_AddStringToObject(root, "zone", cfg->base.zone);
        cJSON_AddNumberToObject(root, "config_version", cfg->base.config_version);
        
        cJSON_AddNumberToObject(root, "temp_target", cfg->temp_target);
        cJSON_AddNumberToObject(root, "humidity_target", cfg->humidity_target);
        cJSON_AddNumberToObject(root, "co2_max", cfg->co2_max);
        cJSON_AddNumberToObject(root, "lux_min", cfg->lux_min);
        cJSON_AddNumberToObject(root, "read_interval_ms", cfg->read_interval_ms);
        
        ESP_LOGI(TAG, "Climate config exported to JSON");
        
    } else if (strcmp(node_type, "relay") == 0) {
        const relay_node_config_t *cfg = (const relay_node_config_t *)config;
        
        cJSON_AddStringToObject(root, "node_id", cfg->base.node_id);
        cJSON_AddStringToObject(root, "node_type", cfg->base.node_type);
        cJSON_AddNumberToObject(root, "config_version", cfg->base.config_version);
        
        // Свет
        cJSON *light = cJSON_CreateObject();
        cJSON_AddNumberToObject(light, "brightness", cfg->light.brightness);
        cJSON_AddStringToObject(light, "schedule_on", cfg->light.schedule_on);
        cJSON_AddStringToObject(light, "schedule_off", cfg->light.schedule_off);
        cJSON_AddBoolToObject(light, "schedule_enabled", cfg->light.schedule_enabled);
        cJSON_AddBoolToObject(light, "lux_control", cfg->light.lux_control);
        cJSON_AddNumberToObject(light, "lux_threshold", cfg->light.lux_threshold);
        cJSON_AddItemToObject(root, "light", light);
        
        // Вентиляция
        cJSON *ventilation = cJSON_CreateObject();
        cJSON_AddNumberToObject(ventilation, "co2_threshold", cfg->ventilation.co2_threshold);
        cJSON_AddNumberToObject(ventilation, "temp_threshold", cfg->ventilation.temp_threshold);
        cJSON_AddNumberToObject(ventilation, "humidity_threshold", cfg->ventilation.humidity_threshold);
        cJSON_AddItemToObject(root, "ventilation", ventilation);
        
        // Форточки
        cJSON *windows = cJSON_CreateObject();
        cJSON_AddNumberToObject(windows, "open_time_ms", cfg->windows.open_time_ms);
        cJSON_AddNumberToObject(windows, "close_time_ms", cfg->windows.close_time_ms);
        cJSON_AddItemToObject(root, "windows", windows);
        
        ESP_LOGI(TAG, "Relay config exported to JSON");
        
    } else if (strcmp(node_type, "water") == 0) {
        const water_node_config_t *cfg = (const water_node_config_t *)config;
        
        cJSON_AddStringToObject(root, "node_id", cfg->base.node_id);
        cJSON_AddStringToObject(root, "node_type", cfg->base.node_type);
        cJSON_AddNumberToObject(root, "config_version", cfg->base.config_version);
        
        cJSON_AddNumberToObject(root, "pump_on_time_ms", cfg->pump_on_time_ms);
        cJSON_AddNumberToObject(root, "pump_off_time_ms", cfg->pump_off_time_ms);
        cJSON_AddNumberToObject(root, "active_zones", cfg->active_zones);
        cJSON_AddNumberToObject(root, "max_pump_time_ms", cfg->max_pump_time_ms);
        cJSON_AddBoolToObject(root, "water_level_check", cfg->water_level_check);
        
        ESP_LOGI(TAG, "Water config exported to JSON");
        
    } else {
        ESP_LOGE(TAG, "Unknown node type: %s", node_type);
        cJSON_Delete(root);
        return NULL;
    }

    return root;
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

