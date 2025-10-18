/**
 * @file data_router.c
 * @brief Реализация маршрутизатора данных
 */

#include "data_router.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "esp_log.h"
#include "esp_mac.h"
#include <string.h>

static const char *TAG = "data_router";

// MQTT топики
#define MQTT_TOPIC_TELEMETRY    "hydro/telemetry"
#define MQTT_TOPIC_EVENT        "hydro/event"
#define MQTT_TOPIC_HEARTBEAT    "hydro/heartbeat"

esp_err_t data_router_init(void) {
    ESP_LOGI(TAG, "Data Router initialized");
    
    // Регистрация callbacks
    mesh_manager_register_recv_cb(data_router_handle_mesh_data);
    mqtt_client_manager_register_recv_cb(data_router_handle_mqtt_data);
    
    return ESP_OK;
}

void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    ESP_LOGD(TAG, "Mesh data received: %d bytes from "MACSTR, len, MAC2STR(src_addr));

    // Парсинг JSON
    mesh_message_t msg;
    if (!mesh_protocol_parse((const char *)data, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        return;
    }

    // Обновление реестра узлов (отметка последнего контакта)
    node_registry_update_last_seen(msg.node_id, src_addr);

    // Маршрутизация в зависимости от типа сообщения
    switch (msg.type) {
        case MESH_MSG_TELEMETRY:
            ESP_LOGI(TAG, "Telemetry from %s → MQTT", msg.node_id);
            
            // Обновление данных в реестре
            node_registry_update_data(msg.node_id, msg.data);
            
            // Отправка в MQTT
            if (mqtt_client_manager_is_connected()) {
                mqtt_client_manager_publish(MQTT_TOPIC_TELEMETRY, (const char *)data);
            } else {
                ESP_LOGW(TAG, "MQTT offline, telemetry dropped");
                // TODO: буферизация для отправки позже
            }
            break;

        case MESH_MSG_EVENT:
            ESP_LOGI(TAG, "Event from %s → MQTT", msg.node_id);
            
            if (mqtt_client_manager_is_connected()) {
                mqtt_client_manager_publish(MQTT_TOPIC_EVENT, (const char *)data);

                // Проверка критичности события
                cJSON *level = cJSON_GetObjectItem(msg.data, "level");
                if (level && cJSON_IsString(level)) {
                    const char *level_str = level->valuestring;
                    if (strcmp(level_str, "critical") == 0 || strcmp(level_str, "emergency") == 0) {
                        ESP_LOGW(TAG, "CRITICAL event from %s!", msg.node_id);
                        // TODO: дополнительные действия (SMS, Telegram)
                    }
                }
            }
            break;

        case MESH_MSG_HEARTBEAT:
            ESP_LOGD(TAG, "Heartbeat from %s", msg.node_id);
            
            // Heartbeat обновляет только реестр (уже сделано выше)
            // Опционально отправлять в MQTT для мониторинга
            if (mqtt_client_manager_is_connected()) {
                mqtt_client_manager_publish(MQTT_TOPIC_HEARTBEAT, (const char *)data);
            }
            break;

        case MESH_MSG_REQUEST:
            ESP_LOGI(TAG, "Request from %s (Display)", msg.node_id);
            
            // Запрос от Display узла - собрать данные всех узлов
            cJSON *request_type = cJSON_GetObjectItem(msg.data, "request");
            if (request_type && cJSON_IsString(request_type)) {
                const char *req = request_type->valuestring;
                
                if (strcmp(req, "all_nodes_data") == 0) {
                    // Экспорт всех узлов в JSON
                    cJSON *nodes_data = node_registry_export_all_to_json();
                    
                    if (nodes_data) {
                        // Создание response сообщения
                        char response_buf[2048];
                        if (mesh_protocol_create_response(msg.node_id, nodes_data,
                                                          response_buf, sizeof(response_buf))) {
                            // Отправка обратно Display узлу
                            mesh_manager_send(src_addr, (uint8_t *)response_buf, strlen(response_buf));
                            ESP_LOGI(TAG, "Sent response to Display");
                        }
                        
                        cJSON_Delete(nodes_data);
                    }
                }
            }
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
}

void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len) {
    ESP_LOGI(TAG, "MQTT data received: %s (%d bytes)", topic, data_len);

    // Парсинг топика: hydro/command/{node_id} или hydro/config/{node_id}
    char node_id[32] = {0};
    bool is_command = (strstr(topic, "/command/") != NULL);
    bool is_config = (strstr(topic, "/config/") != NULL);

    if (is_command || is_config) {
        // Извлечение node_id из топика
        const char *slash = strrchr(topic, '/');
        if (slash && strlen(slash + 1) > 0) {
            strncpy(node_id, slash + 1, sizeof(node_id) - 1);

            // Поиск узла в реестре
            node_info_t *node = node_registry_get(node_id);
            if (node && node->online) {
                ESP_LOGI(TAG, "Forwarding %s to %s", 
                         is_command ? "command" : "config", node_id);

                // Отправка через mesh
                esp_err_t err = mesh_manager_send(node->mac_addr, 
                                                  (const uint8_t *)data, data_len);
                
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to send to node: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "Node %s offline or not found, message dropped", node_id);
            }
        } else {
            ESP_LOGW(TAG, "Invalid topic format: %s", topic);
        }
    } else {
        ESP_LOGW(TAG, "Unknown MQTT topic: %s", topic);
    }
}

