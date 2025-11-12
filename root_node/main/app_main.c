/**
 * @file app_main.c
 * @brief ROOT NODE - Точка входа координатора mesh-сети
 * 
 * ESP32-S3 #1 - Mesh ROOT + MQTT Bridge
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
#include "esp_timer.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"
#include "cJSON.h"
#include "root_defaults.h"

// ROOT компоненты
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "data_router.h"
#include "climate_logic.h"
#include "root_config.h"
#include "setup_portal.h"

static const char *TAG = "root_main";

#define SETUP_SERVER_URL "http://" MQTT_BROKER_HOST ":8000/api/discovery/root"
#define SETUP_HEARTBEAT_INTERVAL_MS 10000

static SemaphoreHandle_t s_credentials_sem = NULL;
static SemaphoreHandle_t s_config_sem = NULL;
static setup_portal_credentials_t s_credentials = {0};
static TaskHandle_t s_heartbeat_task_handle = NULL;
static httpd_handle_t s_config_http_server = NULL;
static bool s_setup_active = false;
static esp_netif_ip_info_t s_setup_ip_info = {0};
static char s_current_pin[7] = {0};
static char s_temp_mesh_id[32] = {0};
static char s_mesh_id_buffer[32] = {0};
static char s_router_ssid_buffer[33] = {0};
static char s_router_pass_buffer[65] = {0};

static void run_setup_mode(void);
static void run_normal_mode(void);
static void setup_portal_credentials_cb(const setup_portal_credentials_t *credentials, void *user_ctx);
static esp_err_t start_setup_mesh(const char *mesh_id, const char *ssid, const char *password, esp_netif_ip_info_t *ip_info_out);
static esp_err_t send_setup_message(const char *type, const char *pin, const char *temp_mesh_id, const esp_netif_ip_info_t *ip_info);
static void setup_heartbeat_task(void *arg);
static esp_err_t start_config_http_server(void);
static void stop_config_http_server(void);
static esp_err_t config_post_handler(httpd_req_t *req);
static void schedule_restart_task(void *arg);
static void send_config_confirmation_task(void *arg);

/**
 * @brief Задача мониторинга системы
 * 
 * Периодически логирует статус системы и проверяет таймауты узлов
 */
static void root_monitoring_task(void *arg) {
    uint32_t last_log_ms = 0;
    
    // Регистрация в watchdog
    esp_task_wdt_add(NULL);
    
    ESP_LOGI(TAG, "Monitoring task started");
    
    while (1) {
        // Сброс watchdog
        esp_task_wdt_reset();
        
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Проверка таймаутов узлов
        node_registry_check_timeouts();
        
        // Логирование статуса каждые 30 секунд
        if (now_ms - last_log_ms > ROOT_MONITORING_INTERVAL_MS) {
            uint32_t free_heap = esp_get_free_heap_size();
            int mesh_nodes = mesh_manager_get_total_nodes();
            int registry_nodes = node_registry_get_count();
            bool mqtt_online = mqtt_client_manager_is_connected();
            bool fallback_active = climate_logic_is_fallback_active();
            
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "=== ROOT NODE STATUS ===");
            ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
            ESP_LOGI(TAG, "Mesh nodes: %d (total), %d (online)", mesh_nodes, registry_nodes);
            ESP_LOGI(TAG, "MQTT: %s", mqtt_online ? "ONLINE" : "OFFLINE");
            ESP_LOGI(TAG, "Climate fallback: %s", fallback_active ? "ACTIVE" : "INACTIVE");
            ESP_LOGI(TAG, "========================================");
            
            // Отправка discovery сообщения (для регистрации на сервере)
            // Отправляем heartbeat (discovery уже был при запуске)
            mqtt_client_manager_send_heartbeat();
            
            // Предупреждение при низкой памяти
            if (free_heap < 50000) {
                ESP_LOGW(TAG, "LOW HEAP MEMORY!");
            }
            
            last_log_ms = now_ms;
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));  // Проверка каждые 5 сек
    }
}

/**
 * @brief Главная функция приложения
 */
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(root_config_init());

    bool zone_configured = root_config_is_zone_configured();
    bool node_ready = node_config_is_configured();

    if (zone_configured && !node_ready) {
        node_config_mark_configured(true);
        node_ready = true;
    }

    if (!zone_configured || !node_ready) {
        run_setup_mode();
        return;
    }

    run_normal_mode();
}

static void run_setup_mode(void) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "=== ROOT NODE SETUP MODE ACTIVATED ===");
    ESP_LOGW(TAG, "========================================");

    memset(&s_credentials, 0, sizeof(s_credentials));
    memset(&s_setup_ip_info, 0, sizeof(s_setup_ip_info));

    node_config_generate_setup_pin(s_current_pin, sizeof(s_current_pin));
    node_config_generate_temp_mesh_id(s_current_pin, s_temp_mesh_id, sizeof(s_temp_mesh_id));

    char ap_ssid[32] = {0};
    snprintf(ap_ssid, sizeof(ap_ssid), "HYDRO_SETUP_%s", s_current_pin);
    const char *ap_password = "hydro2025";

    s_credentials_sem = xSemaphoreCreateBinary();
    if (!s_credentials_sem) {
        ESP_LOGE(TAG, "Не удалось создать семафор для setup портала");
        return;
    }

    setup_portal_config_t portal_cfg = {
        .ap_ssid = ap_ssid,
        .ap_password = ap_password,
        .on_credentials = setup_portal_credentials_cb,
        .user_ctx = NULL,
    };

    esp_err_t err = setup_portal_start(&portal_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска setup портала: %s", esp_err_to_name(err));
        vSemaphoreDelete(s_credentials_sem);
        s_credentials_sem = NULL;
        return;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "🌐 Данные для подключения:");
    ESP_LOGI(TAG, "  WiFi SSID:    %s", ap_ssid);
    ESP_LOGI(TAG, "  WiFi Pass:    %s", ap_password);
    ESP_LOGI(TAG, "  PIN (наклейка на устройстве): %s", s_current_pin);
    ESP_LOGI(TAG, "  Откройте в браузере: http://192.168.4.1");
    ESP_LOGI(TAG, "========================================");

    if (xSemaphoreTake(s_credentials_sem, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Ожидание WiFi данных прервано");
    }

    setup_portal_stop();
    vSemaphoreDelete(s_credentials_sem);
    s_credentials_sem = NULL;

    if (s_credentials.ssid[0] == '\0') {
        ESP_LOGE(TAG, "WiFi данные не получены. Повторите настройку.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    ESP_LOGI(TAG, "WiFi данные сохранены. SSID='%s'", s_credentials.ssid);

    s_config_sem = xSemaphoreCreateBinary();
    if (!s_config_sem) {
        ESP_LOGE(TAG, "Не удалось создать семафор ожидания конфигурации");
    }

    esp_err_t mesh_err = start_setup_mesh(s_temp_mesh_id, s_credentials.ssid, s_credentials.password, &s_setup_ip_info);
    if (mesh_err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось запустить временную mesh сеть: %s", esp_err_to_name(mesh_err));
    }

    if (start_config_http_server() != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска HTTP сервера конфигурации");
    }

    if (mesh_err == ESP_OK) {
        if (send_setup_message("discovery", s_current_pin, s_temp_mesh_id, &s_setup_ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "Discovery запрос отправлен. Ожидаем конфигурацию от сервера...");
        } else {
            ESP_LOGE(TAG, "Не удалось отправить discovery запрос на сервер");
        }

        s_setup_active = true;
        if (s_heartbeat_task_handle == NULL) {
            if (xTaskCreate(setup_heartbeat_task, "setup_heartbeat", 4096, NULL, 4, &s_heartbeat_task_handle) != pdPASS) {
                ESP_LOGE(TAG, "Не удалось создать задачу heartbeat");
                s_heartbeat_task_handle = NULL;
                s_setup_active = false;
            }
        }
    }

    if (s_config_sem) {
        ESP_LOGI(TAG, "Ожидаем конфигурацию (POST /api/config)...");
        xSemaphoreTake(s_config_sem, portMAX_DELAY);
        vSemaphoreDelete(s_config_sem);
        s_config_sem = NULL;
    } else {
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    s_setup_active = false;
    if (s_heartbeat_task_handle) {
        while (s_heartbeat_task_handle != NULL) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    stop_config_http_server();

    ESP_LOGI(TAG, "Конфигурация получена. Перезагрузка для перехода в рабочий режим...");
    if (xTaskCreate(schedule_restart_task, "setup_restart", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Не удалось создать задачу перезапуска. Выполняем esp_restart немедленно.");
        esp_restart();
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void run_normal_mode(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== HYDRO MESH ROOT NODE Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 #1 - Mesh Coordinator + MQTT Bridge");
    ESP_LOGI(TAG, "Build date: %s %s", __DATE__, __TIME__);
    
    root_config_t root_cfg = {0};
    if (root_config_get(&root_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "Не удалось загрузить root_config из NVS, используем дефолтные значения");
        root_config_auto_configure(1);
        root_config_get(&root_cfg);
    }

    const char *configured_mqtt_host = root_cfg.mqtt_host[0] ? root_cfg.mqtt_host : MQTT_BROKER_HOST;
    uint16_t configured_mqtt_port = root_cfg.mqtt_port ? root_cfg.mqtt_port : MQTT_BROKER_PORT;

    strncpy(s_mesh_id_buffer, root_cfg.mesh_network_id, sizeof(s_mesh_id_buffer) - 1);
    if (s_mesh_id_buffer[0] == '\0') {
        strncpy(s_mesh_id_buffer, MESH_NETWORK_ID, sizeof(s_mesh_id_buffer) - 1);
    }

    if (node_config_get_router_credentials(s_router_ssid_buffer, sizeof(s_router_ssid_buffer),
                                           s_router_pass_buffer, sizeof(s_router_pass_buffer)) != ESP_OK) {
        ESP_LOGW(TAG, "Router credentials отсутствуют в NVS, используем mesh_config значения");
        strncpy(s_router_ssid_buffer, MESH_ROUTER_SSID, sizeof(s_router_ssid_buffer) - 1);
        strncpy(s_router_pass_buffer, MESH_ROUTER_PASSWORD, sizeof(s_router_pass_buffer) - 1);
    }

    ESP_LOGI(TAG, "[Step 1/6] Initializing Node Registry...");
    ESP_ERROR_CHECK(node_registry_init());
    
    ESP_LOGI(TAG, "[Step 2/6] Initializing Mesh (ROOT mode)...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_id = s_mesh_id_buffer,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = ROOT_MAX_MESH_CONNECTIONS,
        .router_ssid = s_router_ssid_buffer,
        .router_password = s_router_pass_buffer,
        .router_bssid = NULL
    };
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    ESP_LOGI(TAG, "Mesh ID: %s, Channel: %d", s_mesh_id_buffer, MESH_NETWORK_CHANNEL);
    
    ESP_LOGI(TAG, "[Step 3/6] Starting Mesh network...");
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_LOGI(TAG, "Mesh network started");
    
    ESP_LOGI(TAG, "Waiting for IP address from DHCP...");
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    
    int retry_count = 0;
    const int max_retries = 60;
    bool ip_obtained = false;
    
    while (retry_count < max_retries) {
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                ESP_LOGI(TAG, "✓ IP address obtained: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG, "✓ Netmask: " IPSTR, IP2STR(&ip_info.netmask));
                ESP_LOGI(TAG, "✓ Gateway: " IPSTR, IP2STR(&ip_info.gw));
                ip_obtained = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        retry_count++;
        if (retry_count % 10 == 0) {
            ESP_LOGW(TAG, "Still waiting for IP... (%d/%d)", retry_count, max_retries);
        }
    }
    
    if (!ip_obtained) {
        ESP_LOGE(TAG, "❌ CRITICAL: Failed to obtain IP address from DHCP!");
        ESP_LOGW(TAG, "Continuing startup, but MQTT will not work...");
    }
    
    ESP_LOGI(TAG, "[Step 4/6] Initializing MQTT Client...");
    if (ip_obtained) {
        ESP_ERROR_CHECK(mqtt_client_manager_init());
        ESP_ERROR_CHECK(mqtt_client_manager_start());
        ESP_LOGI(TAG, "MQTT Client started (connecting to broker...)");
    } else {
        ESP_LOGE(TAG, "Skipping MQTT initialization - no IP address!");
    }
    
    ESP_LOGI(TAG, "[Step 5/6] Initializing Data Router...");
    ESP_ERROR_CHECK(data_router_init());
    ESP_LOGI(TAG, "Data Router initialized");
    
    ESP_LOGI(TAG, "[Step 6/6] Initializing Climate Fallback Logic...");
    ESP_ERROR_CHECK(climate_logic_init());
    ESP_LOGI(TAG, "Climate Fallback Logic initialized");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== ROOT NODE Running ===");
    ESP_LOGI(TAG, "Mesh ID: %s", s_mesh_id_buffer);
    ESP_LOGI(TAG, "MQTT Broker: %s:%u", configured_mqtt_host, configured_mqtt_port);
    ESP_LOGI(TAG, "========================================");
    
    if (xTaskCreate(send_config_confirmation_task, "cfg_confirm", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGW(TAG, "Не удалось создать задачу отправки подтверждения конфигурации");
    }

    xTaskCreate(root_monitoring_task, "root_monitor", 8192, NULL, 5, NULL);
    ESP_LOGI(TAG, "All systems operational. ROOT node ready.");
}

static void setup_portal_credentials_cb(const setup_portal_credentials_t *credentials, void *user_ctx) {
    if (!credentials) {
        return;
    }

    ESP_LOGI(TAG, "Получены WiFi данные: SSID='%s'", credentials->ssid);
    memcpy(&s_credentials, credentials, sizeof(s_credentials));

    esp_err_t err = node_config_set_router_credentials(credentials->ssid, credentials->password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить WiFi credentials: %s", esp_err_to_name(err));
    }

    node_config_mark_configured(false);

    if (s_credentials_sem) {
        xSemaphoreGive(s_credentials_sem);
    }
}

static esp_err_t start_setup_mesh(const char *mesh_id, const char *ssid, const char *password, esp_netif_ip_info_t *ip_info_out) {
    if (!mesh_id || mesh_id[0] == '\0' || !ssid || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_id = mesh_id,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = ROOT_MAX_MESH_CONNECTIONS,
        .router_ssid = ssid,
        .router_password = password,
        .router_bssid = NULL,
    };

    ESP_LOGI(TAG, "Запуск временной mesh сети: mesh_id='%s'", mesh_id);

    esp_err_t err = mesh_manager_init(&mesh_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mesh_manager_init error: %s", esp_err_to_name(err));
        return err;
    }

    err = mesh_manager_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mesh_manager_start error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Mesh network (setup) started, ожидаем IP...");

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info = {0};

    const int max_retries = 60;
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            ESP_LOGI(TAG, "Setup mesh получил IP: " IPSTR, IP2STR(&ip_info.ip));
            if (ip_info_out) {
                *ip_info_out = ip_info;
            }
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGE(TAG, "Не удалось получить IP адрес для setup mesh");
    return ESP_FAIL;
}

static esp_err_t send_setup_message(const char *type, const char *pin, const char *temp_mesh_id, const esp_netif_ip_info_t *ip_info) {
    if (!type) {
        return ESP_ERR_INVALID_ARG;
    }

    char mac_str[18] = {0};
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    const esp_app_desc_t *app_desc = esp_app_get_description();
    esp_chip_info_t chip_info = {0};
    esp_chip_info(&chip_info);

    const char *chip_model_str = "ESP32";
#if defined(ESP_CHIP_MODEL_ESP32S2)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32S2) {
        chip_model_str = "ESP32-S2";
    } else if (chip_info.model == ESP_CHIP_MODEL_ESP32S3) {
        chip_model_str = "ESP32-S3";
    } else if (chip_info.model == ESP_CHIP_MODEL_ESP32C3) {
        chip_model_str = "ESP32-C3";
    }
#elif defined(ESP_CHIP_MODEL_ESP32S3)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32S3) {
        chip_model_str = "ESP32-S3";
    }
#endif

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddStringToObject(root, "node_type", "root");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    if (pin) {
        cJSON_AddStringToObject(root, "pin", pin);
    }
    if (temp_mesh_id) {
        cJSON_AddStringToObject(root, "temp_mesh_id", temp_mesh_id);
    }

    if (strcmp(type, "discovery") == 0) {
        cJSON_AddStringToObject(root, "firmware_version", app_desc ? app_desc->version : "unknown");
        cJSON_AddStringToObject(root, "chip_model", chip_model_str);
    } else if (strcmp(type, "heartbeat") == 0) {
        cJSON_AddNumberToObject(root, "uptime_ms", esp_timer_get_time() / 1000);
        cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
    }

    if (ip_info) {
        char ip_buf[16];
        snprintf(ip_buf, sizeof(ip_buf), IPSTR, IP2STR(&ip_info->ip));
        cJSON_AddStringToObject(root, "ip_address", ip_buf);
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!payload) {
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_config_t config = {
        .url = SETUP_SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(payload);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "Setup message '%s' отправлен. HTTP статус: %d", type, status);
    } else {
        ESP_LOGE(TAG, "Ошибка HTTP при отправке '%s': %s", type, esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(payload);
    return err;
}

static void setup_heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Setup heartbeat task started");
    while (s_setup_active) {
        vTaskDelay(pdMS_TO_TICKS(SETUP_HEARTBEAT_INTERVAL_MS));
        if (!s_setup_active) {
            break;
        }
        if (send_setup_message("heartbeat", s_current_pin, s_temp_mesh_id, &s_setup_ip_info) != ESP_OK) {
            ESP_LOGW(TAG, "Не удалось отправить heartbeat в setup режиме");
        }
    }
    ESP_LOGI(TAG, "Setup heartbeat task finished");
    s_heartbeat_task_handle = NULL;
    vTaskDelete(NULL);
}

static esp_err_t config_post_handler(httpd_req_t *req) {
    int total = req->content_len;
    if (total <= 0 || total > 2048) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Получен POST /api/config (len=%d)", total);

    char *buf = calloc(1, total + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
        return ESP_FAIL;
    }

    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, buf + received, total - received);
        if (r <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            return ESP_FAIL;
        }
        received += r;
    }

    ESP_LOGD(TAG, "Сырые данные конфигурации: %s", buf);

    ESP_LOGI(TAG, "Setup config payload: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *pin = cJSON_GetObjectItem(root, "pin");
    if (!cJSON_IsString(pin) || strncmp(pin->valuestring, s_current_pin, sizeof(s_current_pin) - 1) != 0) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid PIN");
        return ESP_FAIL;
    }

    const cJSON *node_id = cJSON_GetObjectItem(root, "node_id");
    const cJSON *mesh_id = cJSON_GetObjectItem(root, "mesh_id");
    const cJSON *root_node_id = cJSON_GetObjectItem(root, "root_node_id");
    const cJSON *mqtt_host = cJSON_GetObjectItem(root, "mqtt_broker_host");
    const cJSON *mqtt_port = cJSON_GetObjectItem(root, "mqtt_broker_port");
    const cJSON *wifi_ssid = cJSON_GetObjectItem(root, "wifi_ssid");
    const cJSON *wifi_password = cJSON_GetObjectItem(root, "wifi_password");
    const cJSON *zone = cJSON_GetObjectItem(root, "zone");
    const cJSON *zone_number = cJSON_GetObjectItem(root, "zone_number");
    const cJSON *zone_location = cJSON_GetObjectItem(root, "zone_location");
    const cJSON *mqtt_topic_prefix = cJSON_GetObjectItem(root, "mqtt_topic_prefix");

    if (!cJSON_IsString(node_id) || !node_id->valuestring || node_id->valuestring[0] == '\0' ||
        !cJSON_IsString(mesh_id) || !mesh_id->valuestring || mesh_id->valuestring[0] == '\0' ||
        !cJSON_IsString(wifi_ssid) || !wifi_ssid->valuestring || !cJSON_IsString(wifi_password) || !wifi_password->valuestring) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields");
        return ESP_FAIL;
    }

    const char *root_node_id_str = node_id->valuestring;
    if (cJSON_IsString(root_node_id) && root_node_id->valuestring && root_node_id->valuestring[0] != '\0') {
        root_node_id_str = root_node_id->valuestring;
    }

    const char *mqtt_host_str = MQTT_BROKER_HOST;
    if (cJSON_IsString(mqtt_host) && mqtt_host->valuestring && mqtt_host->valuestring[0] != '\0') {
        mqtt_host_str = mqtt_host->valuestring;
    }

    uint16_t mqtt_port_value = MQTT_BROKER_PORT;
    if (cJSON_IsNumber(mqtt_port) && mqtt_port->valuedouble > 0) {
        mqtt_port_value = (uint16_t)mqtt_port->valuedouble;
    }

    esp_err_t err = node_config_set_router_credentials(wifi_ssid->valuestring, wifi_password->valuestring);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить WiFi данные: %s", esp_err_to_name(err));
    }

    if ((err = node_config_set_root_node_id(root_node_id_str)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist root_node_id: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to store root id");
        goto cleanup;
    }
    if ((err = node_config_set_mesh_network_id(mesh_id->valuestring)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist mesh_id: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to store mesh id");
        goto cleanup;
    }

    ESP_LOGI(TAG,
             "Конфигурация: node_id='%s', mesh_id='%s', root_node_id='%s', MQTT='%s:%u', zone='%s', zone_number=%d",
             node_id->valuestring,
             mesh_id->valuestring,
             root_node_id_str,
             mqtt_host_str,
             mqtt_port_value,
             (cJSON_IsString(zone) && zone->valuestring) ? zone->valuestring : "-",
             cJSON_IsNumber(zone_number) ? (int)zone_number->valuedouble : -1);
    if (cJSON_IsString(zone_location) && zone_location->valuestring) {
        ESP_LOGI(TAG, "Расположение зоны: %s", zone_location->valuestring);
    }
    ESP_LOGI(TAG, "WiFi: SSID='%s', пароль (%d символов)", wifi_ssid->valuestring,
             wifi_password->valuestring ? (int)strlen(wifi_password->valuestring) : 0);

    root_config_t cfg = {0};
    if (root_config_get(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }

    strncpy(cfg.root_node_id, root_node_id_str, sizeof(cfg.root_node_id) - 1);
    strncpy(cfg.mesh_network_id, mesh_id->valuestring, sizeof(cfg.mesh_network_id) - 1);
    strncpy(cfg.mqtt_host, mqtt_host_str, sizeof(cfg.mqtt_host) - 1);
    cfg.mqtt_port = mqtt_port_value;
    if (cJSON_IsString(zone) && zone->valuestring) {
        strncpy(cfg.zone_name, zone->valuestring, sizeof(cfg.zone_name) - 1);
    }
    if (cJSON_IsNumber(zone_number)) {
        cfg.zone_number = (uint8_t)zone_number->valuedouble;
    }
    if (cJSON_IsString(zone_location) && zone_location->valuestring) {
        strncpy(cfg.zone_location, zone_location->valuestring, sizeof(cfg.zone_location) - 1);
    }
    if (cJSON_IsString(mqtt_topic_prefix) && mqtt_topic_prefix->valuestring) {
        strncpy(cfg.mqtt_topic_prefix, mqtt_topic_prefix->valuestring, sizeof(cfg.mqtt_topic_prefix) - 1);
    } else if (cfg.zone_number > 0) {
        snprintf(cfg.mqtt_topic_prefix, sizeof(cfg.mqtt_topic_prefix), "hydro/zone%u/", cfg.zone_number);
    }
    cfg.is_configured = true;
    cfg.configured_at = esp_timer_get_time() / 1000000ULL;

    err = root_config_save(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить root_config: %s", esp_err_to_name(err));
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist config");
        return ESP_FAIL;
    }

    node_config_mark_configured(true);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true}");

    cJSON_Delete(root);

    if (s_config_sem) {
        xSemaphoreGive(s_config_sem);
    }

    return ESP_OK;
cleanup:
    cJSON_Delete(root);
    return err;
}

static esp_err_t start_config_http_server(void) {
    if (s_config_http_server) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_config_http_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start error: %s", esp_err_to_name(err));
        s_config_http_server = NULL;
        return err;
    }

    httpd_uri_t config_uri = {
        .uri = "/api/config",
        .method = HTTP_POST,
        .handler = config_post_handler,
        .user_ctx = NULL,
    };

    httpd_register_uri_handler(s_config_http_server, &config_uri);
    ESP_LOGI(TAG, "HTTP сервер конфигурации запущен на порту %d", config.server_port);
    return ESP_OK;
}

static void stop_config_http_server(void) {
    if (s_config_http_server) {
        httpd_stop(s_config_http_server);
        s_config_http_server = NULL;
        ESP_LOGI(TAG, "HTTP сервер конфигурации остановлен");
    }
}

static void schedule_restart_task(void *arg) {
    ESP_LOGI(TAG, "Перезагрузка через 2 секунды...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Перезагрузка устройства");
    esp_restart();
}

static void send_config_confirmation_task(void *arg) {
    root_config_t cfg = {0};
    if (root_config_get(&cfg) != ESP_OK || !cfg.is_configured || cfg.root_node_id[0] == '\0') {
        vTaskDelete(NULL);
        return;
    }

    char mac_str[18] = {0};
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    char assigned_root_id[32] = {0};
    if (node_config_get_root_node_id(assigned_root_id) != ESP_OK || assigned_root_id[0] == '\0') {
        strncpy(assigned_root_id, cfg.root_node_id, sizeof(assigned_root_id) - 1);
    }

    const int max_attempts = 60;
    int attempts = 0;
    while (!mqtt_client_manager_is_connected() && attempts < max_attempts) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        attempts++;
    }

    if (!mqtt_client_manager_is_connected()) {
        ESP_LOGW(TAG, "Не удалось подключиться к MQTT для отправки подтверждения конфигурации");
        vTaskDelete(NULL);
        return;
    }

    char topic_prefix[128] = {0};
    if (root_config_get_mqtt_topic_prefix(topic_prefix) != ESP_OK || topic_prefix[0] == '\0') {
        strcpy(topic_prefix, "hydro/setup/");
    }
    size_t prefix_len = strlen(topic_prefix);
    if (prefix_len > 0 && topic_prefix[prefix_len - 1] != '/') {
        if (prefix_len + 1 < sizeof(topic_prefix)) {
            topic_prefix[prefix_len] = '/';
            topic_prefix[prefix_len + 1] = '\0';
        } else {
            topic_prefix[prefix_len - 1] = '/';
            topic_prefix[prefix_len] = '\0';
        }
        prefix_len = strlen(topic_prefix);
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        vTaskDelete(NULL);
        return;
    }

    cJSON_AddStringToObject(root, "type", "config_confirmation");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    cJSON_AddStringToObject(root, "node_id", cfg.root_node_id);
    cJSON_AddStringToObject(root, "mesh_id", cfg.mesh_network_id);
    cJSON_AddStringToObject(root, "status", "success");
    if (cfg.zone_name[0]) {
        cJSON_AddStringToObject(root, "zone", cfg.zone_name);
    }
    cJSON_AddStringToObject(root, "root_node_id", assigned_root_id);
    cJSON_AddStringToObject(root, "mqtt_topic_prefix", topic_prefix);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        vTaskDelete(NULL);
        return;
    }

    char discovery_topic[160];
    snprintf(discovery_topic, sizeof(discovery_topic), "%sdiscovery", topic_prefix);

    if (mqtt_client_manager_publish(discovery_topic, payload) == ESP_OK) {
        ESP_LOGI(TAG, "Отправлено подтверждение конфигурации для %s", cfg.root_node_id);
    } else {
        ESP_LOGE(TAG, "Не удалось отправить config_confirmation для %s", cfg.root_node_id);
    }

    free(payload);
    vTaskDelete(NULL);
}
