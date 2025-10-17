/**
 * @file mqtt_client_manager.c
 * @brief Реализация MQTT клиента
 */

#include "mqtt_client_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"

static const char *TAG = "mqtt_manager";

static esp_mqtt_client_handle_t s_client = NULL;
static mqtt_data_cb_t s_data_cb = NULL;
static bool s_is_connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            s_is_connected = true;
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            s_is_connected = false;
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT data: topic=%.*s", event->topic_len, event->topic);
            if (s_data_cb != NULL) {
                s_data_cb(event->topic, event->data, event->data_len);
            }
            break;
            
        default:
            break;
    }
}

esp_err_t mqtt_client_manager_init(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = CONFIG_MQTT_BROKER_URI,
    };
    
    s_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(s_client);
    
    ESP_LOGI(TAG, "MQTT client initialized");
    return ESP_OK;
}

esp_err_t mqtt_client_manager_publish(const char *topic, const char *data) {
    if (!s_is_connected) {
        ESP_LOGW(TAG, "MQTT not connected, cannot publish");
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_publish(s_client, topic, data, 0, 1, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "MQTT publish failed");
        return ESP_FAIL;
    }
    
    return ESP_OK;
}

esp_err_t mqtt_client_manager_subscribe(const char *topic, mqtt_data_cb_t callback) {
    s_data_cb = callback;
    
    int msg_id = esp_mqtt_client_subscribe(s_client, topic, 0);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "MQTT subscribe failed");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Subscribed to %s", topic);
    return ESP_OK;
}

bool mqtt_client_manager_is_connected(void) {
    return s_is_connected;
}

