/**
 * @file mqtt_client_manager.c
 * @brief Реализация MQTT клиента
 */

#include "mqtt_client_manager.h"
#include "mqtt_client.h"
#include "root_config.h"
#include "zone_config.h"
#include "mesh_protocol.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "mesh_config.h"
#include <string.h>
#include <stdio.h>

// Внешнее объявление для spi_flash функции
extern uint32_t spi_flash_get_chip_size(void);

static const char *TAG = "mqtt_manager";

// Удалено s_topic_prefix - используем zone_config и mesh_topic_format
static char s_mesh_network_id[32] = {0};
static char s_root_node_id[32] = {0};

// MQTT конфигурация берётся из mesh_config.h
// MQTT_BROKER_URI уже определён в mesh_config.h: "mqtt://192.168.1.100:1883"
// Mosquitto настроен с allow_anonymous, поэтому username/password не нужны
#define MQTT_USERNAME           NULL
#define MQTT_PASSWORD           NULL

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static mqtt_recv_callback_t s_recv_cb = NULL;
static bool s_is_connected = false;
static char s_mqtt_uri[256] = {0};
static char s_mqtt_host[128] = {0};
static uint16_t s_mqtt_port = MQTT_BROKER_PORT;

// Forward declaration
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data);

static void refresh_zone_context(void) {
    memset(s_mesh_network_id, 0, sizeof(s_mesh_network_id));
    memset(s_root_node_id, 0, sizeof(s_root_node_id));

    // Загружаем zone конфигурацию из NVS через zone_config
    esp_err_t err = zone_config_load(s_mesh_network_id, sizeof(s_mesh_network_id),
                                      s_root_node_id, sizeof(s_root_node_id));
    
    if (err != ESP_OK || strcmp(s_mesh_network_id, "UNCONFIGURED") == 0) {
        ESP_LOGW(TAG, "Zone not configured, using defaults");
        strncpy(s_mesh_network_id, "UNCONFIGURED", sizeof(s_mesh_network_id) - 1);
        strncpy(s_root_node_id, "root_setup", sizeof(s_root_node_id) - 1);
    }

    ESP_LOGI(TAG, "Zone context: mesh_id=%s, root_node_id=%s",
             s_mesh_network_id, s_root_node_id);
}

esp_err_t mqtt_client_manager_init(void) {
    ESP_LOGI(TAG, "Initializing MQTT client...");

    memset(s_mqtt_host, 0, sizeof(s_mqtt_host));
    if (root_config_get_mqtt_endpoint(s_mqtt_host, sizeof(s_mqtt_host), &s_mqtt_port) != ESP_OK || s_mqtt_host[0] == '\0') {
        strncpy(s_mqtt_host, MQTT_BROKER_HOST, sizeof(s_mqtt_host) - 1);
        s_mqtt_port = MQTT_BROKER_PORT;
    }
    int uri_len = snprintf(s_mqtt_uri, sizeof(s_mqtt_uri), "mqtt://%s:%u", s_mqtt_host, s_mqtt_port);
    if (uri_len < 0 || uri_len >= sizeof(s_mqtt_uri)) {
        ESP_LOGE(TAG, "MQTT URI is too long, host='%s'", s_mqtt_host);
        return ESP_ERR_INVALID_SIZE;
    }

    refresh_zone_context();

    ESP_LOGI(TAG, "MQTT broker endpoint: %s:%u", s_mqtt_host, s_mqtt_port);

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = s_mqtt_uri,
        // .credentials не указываем для anonymous доступа
        .session = {
            .keepalive = 60,
            .disable_clean_session = 0,
        },
        .network = {
            .reconnect_timeout_ms = 10000,
            .timeout_ms = 10000,
        },
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (s_mqtt_client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }

    // Регистрация обработчика событий
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID,
                                                    mqtt_event_handler, NULL));

    ESP_LOGI(TAG, "MQTT client initialized");
    return ESP_OK;
}

esp_err_t mqtt_client_manager_start(void) {
    if (!s_mqtt_client) {
        ESP_LOGE(TAG, "MQTT client not initialized");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Starting MQTT client...");
    ESP_LOGI(TAG, "Connecting to broker: %s:%u", s_mqtt_host, s_mqtt_port);

    esp_err_t err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t mqtt_client_manager_stop(void) {
    if (!s_mqtt_client) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Stopping MQTT client...");
    return esp_mqtt_client_stop(s_mqtt_client);
}

esp_err_t mqtt_client_manager_publish(const char *topic, const char *data) {
    if (!s_mqtt_client || !topic || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    if (!s_is_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish to %s", topic);
        return ESP_FAIL;
    }

    // ВАЖНО: strlen() для null-terminated строк (JSON payload)
    int data_len = strlen(data);
    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, data, data_len, 0, 0);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to %s", topic);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "✅ MQTT Published: %s (msg_id=%d, len=%d)", topic, msg_id, data_len);
    return ESP_OK;
}

void mqtt_client_manager_register_recv_cb(mqtt_recv_callback_t cb) {
    s_recv_cb = cb;
}

bool mqtt_client_manager_is_connected(void) {
    return s_is_connected;
}

esp_err_t mqtt_client_manager_reconnect(void) {
    if (!s_mqtt_client) {
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Reconnecting to MQTT broker...");
    return esp_mqtt_client_reconnect(s_mqtt_client);
}

// Обработчик событий MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to broker");
            s_is_connected = true;

            refresh_zone_context();

            const char *mesh_id = zone_config_get_mesh_id();
            if (!zone_config_validate(mesh_id)) {
                ESP_LOGE(TAG, "Cannot subscribe: invalid mesh_id '%s'", mesh_id);
                break;
            }

            // Подписка на команды: hydro/{mesh_id}/command/#
            char command_topic[192];
            snprintf(command_topic, sizeof(command_topic), "hydro/%s/command/#", mesh_id);
            esp_mqtt_client_subscribe(s_mqtt_client, command_topic, 1);
            ESP_LOGI(TAG, "Subscribed to %s", command_topic);

            // Подписка на конфигурацию: hydro/{mesh_id}/config/#
            char config_topic[192];
            snprintf(config_topic, sizeof(config_topic), "hydro/%s/config/#", mesh_id);
            esp_mqtt_client_subscribe(s_mqtt_client, config_topic, 1);
            ESP_LOGI(TAG, "Subscribed to %s", config_topic);
            
            // Отправка discovery сообщения
            mqtt_client_manager_send_discovery();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected from broker");
            s_is_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT subscribed, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT unsubscribed, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "MQTT published, msg_id=%d", event->msg_id);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT data received: %.*s", event->topic_len, event->topic);
            
            // Вызов callback если зарегистрирован
            if (s_recv_cb) {
                // Создание null-terminated строк
                char topic[128] = {0};
                char data[1024] = {0};

                int topic_len = (event->topic_len < sizeof(topic) - 1) ? 
                                event->topic_len : sizeof(topic) - 1;
                int data_len = (event->data_len < sizeof(data) - 1) ? 
                              event->data_len : sizeof(data) - 1;

                memcpy(topic, event->topic, topic_len);
                memcpy(data, event->data, data_len);

                s_recv_cb(topic, data, data_len);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "TCP transport error");
            }
            break;

        default:
            ESP_LOGD(TAG, "MQTT event: %d", event_id);
            break;
    }
}

void mqtt_client_manager_send_discovery(void) {
    if (!s_mqtt_client || !s_is_connected) {
        ESP_LOGW(TAG, "Cannot send discovery - MQTT not connected");
        return;
    }

    refresh_zone_context();

    root_config_t root_cfg = {0};
    if (root_config_get(&root_cfg) != ESP_OK) {
        memset(&root_cfg, 0, sizeof(root_cfg));
    }

    const char *mesh_id_cached = zone_config_get_mesh_id();
    const char *root_id_cached = zone_config_get_root_id();

    const char *mesh_id = zone_config_validate(mesh_id_cached)
                              ? mesh_id_cached
                              : (root_cfg.mesh_network_id[0] ? root_cfg.mesh_network_id : s_mesh_network_id);
    const char *node_id = zone_config_validate(root_id_cached)
                              ? root_id_cached
                              : (root_cfg.root_node_id[0] ? root_cfg.root_node_id : s_root_node_id);

    if (!mesh_id || !zone_config_validate(mesh_id) ||
        !node_id || node_id[0] == '\0') {
        ESP_LOGE(TAG, "Discovery publish aborted: mesh_id='%s', node_id='%s'",
                 mesh_id ? mesh_id : "(null)",
                 node_id ? node_id : "(null)");
        return;
    }

    // Сбор информации о системе
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    wifi_ap_record_t ap_info;
    int8_t rssi = -100;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    }
    
    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t min_heap = esp_get_minimum_free_heap_size();
    uint32_t flash_size = spi_flash_get_chip_size();
    uint32_t flash_used = 2 * 1024 * 1024;  // Примерная оценка
    
    char discovery_msg[768];
    snprintf(discovery_msg, sizeof(discovery_msg),
            "{\"type\":\"discovery\","
            "\"node_id\":\"%s\","
            "\"root_node_id\":\"%s\","
            "\"mesh_network_id\":\"%s\","
            "\"node_type\":\"root\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","
            "\"firmware\":\"2.0.0\","
            "\"hardware\":\"ESP32-S3\","
            "\"heap_free\":%lu,"
            "\"heap_min\":%lu,"
            "\"flash_total\":%lu,"
            "\"flash_used\":%lu,"
            "\"wifi_rssi\":%d,"
            "\"timestamp\":%lu,"
            "\"capabilities\":[\"mesh_coordinator\",\"mqtt_bridge\",\"data_router\"]}",
            node_id,
            node_id,
            mesh_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            (unsigned long)free_heap, (unsigned long)min_heap,
            (unsigned long)flash_size, (unsigned long)flash_used,
            rssi,
            (unsigned long)(esp_timer_get_time() / 1000000));

    // Используем mesh_topic_format для формирования топика
    char topic[192];
    if (!mesh_topic_format(topic, sizeof(topic), NULL, "discovery", node_id)) {
        ESP_LOGE(TAG, "Failed to format discovery topic");
        return;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, 
                                        discovery_msg, strlen(discovery_msg), 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published discovery message to %s (msg_id=%d, len=%d)", topic, msg_id, strlen(discovery_msg));
    } else {
        ESP_LOGE(TAG, "Failed to publish discovery message");
    }
}

void mqtt_client_manager_send_heartbeat(void) {
    if (!s_mqtt_client || !s_is_connected) {
        ESP_LOGW(TAG, "Cannot send heartbeat - MQTT not connected");
        return;
    }

    refresh_zone_context();

    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);

    wifi_ap_record_t ap_info;
    int8_t rssi = -100;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        rssi = ap_info.rssi;
    }

    uint32_t free_heap = esp_get_free_heap_size();
    uint32_t uptime_ms = esp_timer_get_time() / 1000;

    root_config_t root_cfg = {0};
    if (root_config_get(&root_cfg) != ESP_OK) {
        memset(&root_cfg, 0, sizeof(root_cfg));
    }

    const char *mesh_id_cached = zone_config_get_mesh_id();
    const char *root_id_cached = zone_config_get_root_id();

    const char *node_id = zone_config_validate(root_id_cached)
                              ? root_id_cached
                              : (root_cfg.root_node_id[0] ? root_cfg.root_node_id : s_root_node_id);
    const char *mesh_id = zone_config_validate(mesh_id_cached)
                              ? mesh_id_cached
                              : (root_cfg.mesh_network_id[0] ? root_cfg.mesh_network_id : s_mesh_network_id);

    if (!mesh_id || !zone_config_validate(mesh_id) ||
        !node_id || node_id[0] == '\0') {
        ESP_LOGE(TAG, "Heartbeat publish aborted: mesh_id='%s', node_id='%s'",
                 mesh_id ? mesh_id : "(null)",
                 node_id ? node_id : "(null)");
        return;
    }

    char heartbeat_msg[512];
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
             "{\"type\":\"heartbeat\","
             "\"node_id\":\"%s\","
             "\"root_node_id\":\"%s\","
             "\"mesh_network_id\":\"%s\","
             "\"node_type\":\"root\","
             "\"status\":\"online\","
             "\"uptime_ms\":%lu,"
             "\"heap_free\":%lu,"
             "\"wifi_rssi\":%d,"
             "\"timestamp\":%lu}",
             node_id,
             node_id,
             mesh_id,
             (unsigned long)uptime_ms,
             (unsigned long)free_heap,
             rssi,
             (unsigned long)(esp_timer_get_time() / 1000000));

    // Используем mesh_topic_format для формирования топика
    size_t payload_len = strlen(heartbeat_msg);
    char topic[192];
    if (!mesh_topic_format(topic, sizeof(topic), NULL, "heartbeat", node_id)) {
        ESP_LOGE(TAG, "Failed to format heartbeat topic");
        return;
    }

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic,
                                         heartbeat_msg, payload_len, 1, 0);
    if (msg_id >= 0) {
        ESP_LOGI(TAG, "Published heartbeat message to %s (msg_id=%d, len=%d)", topic, msg_id, (int)payload_len);
    } else {
        ESP_LOGE(TAG, "Failed to publish heartbeat message");
    }
}

