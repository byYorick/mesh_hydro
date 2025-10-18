/**
 * @file mqtt_client_manager.c
 * @brief Реализация MQTT клиента
 */

#include "mqtt_client_manager.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "mqtt_manager";

// MQTT топики
#define MQTT_TOPIC_TELEMETRY    "hydro/telemetry"
#define MQTT_TOPIC_EVENT        "hydro/event"
#define MQTT_TOPIC_HEARTBEAT    "hydro/heartbeat"
#define MQTT_TOPIC_COMMAND      "hydro/command/#"
#define MQTT_TOPIC_CONFIG       "hydro/config/#"

// MQTT конфигурация (TODO: переместить в sdkconfig)
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"
#define MQTT_USERNAME           "hydro_root"
#define MQTT_PASSWORD           "hydro_pass"

static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static mqtt_recv_callback_t s_recv_cb = NULL;
static bool s_is_connected = false;

// Forward declaration
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data);

esp_err_t mqtt_client_manager_init(void) {
    ESP_LOGI(TAG, "Initializing MQTT client...");

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = MQTT_BROKER_URI,
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
        },
        .session = {
            .keepalive = 60,
            .disable_clean_session = 0,
        },
        .network = {
            .reconnect_timeout_ms = 10000,
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
    ESP_LOGI(TAG, "Connecting to broker: %s", MQTT_BROKER_URI);

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

    int msg_id = esp_mqtt_client_publish(s_mqtt_client, topic, data, strlen(data), 0, 0);
    
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to %s", topic);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Published to %s (msg_id=%d)", topic, msg_id);
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

            // Подписка на топики команд
            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_COMMAND, 1);
            ESP_LOGI(TAG, "Subscribed to %s", MQTT_TOPIC_COMMAND);

            esp_mqtt_client_subscribe(s_mqtt_client, MQTT_TOPIC_CONFIG, 1);
            ESP_LOGI(TAG, "Subscribed to %s", MQTT_TOPIC_CONFIG);
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

