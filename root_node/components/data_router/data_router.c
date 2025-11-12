/**
 * @file data_router.c
 * @brief Реализация маршрутизатора данных
 */

#include "data_router.h"
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "root_config.h"
#include "zone_config.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_heap_caps.h"
#include <string.h>

static const char *TAG = "data_router";

// Ограничения
#define MAX_MESH_DATA_SIZE     2048

// Удалено s_topic_prefix - используем zone_config и mesh_topic_format

esp_err_t data_router_init(void) {
    ESP_LOGI(TAG, "Data Router initialized");
    
    // Регистрация callbacks
    mesh_manager_register_recv_cb(data_router_handle_mesh_data);
    mqtt_client_manager_register_recv_cb(data_router_handle_mqtt_data);
    
    return ESP_OK;
}

void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    ESP_LOGI(TAG, "📥 Mesh data received: %d bytes from "MACSTR, len, MAC2STR(src_addr));

    if (data == NULL) {
        ESP_LOGE(TAG, "Received NULL data pointer");
        return;
    }

    if (len == 0U) {
        ESP_LOGW(TAG, "Received empty payload, dropping");
        return;
    }

    if (len > MAX_MESH_DATA_SIZE) {
        ESP_LOGE(TAG, "Payload too large: %u bytes (max %u). Dropping message.", (unsigned)len, (unsigned)MAX_MESH_DATA_SIZE);
        return;
    }
    
    // ВАЖНО: Создаём NULL-terminated копию для безопасного парсинга и публикации
    char *data_copy = malloc(len + 1);
    if (data_copy == NULL) {
        size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
        ESP_LOGE(TAG, "Failed to allocate memory for data copy (%u bytes). Free heap: %u", (unsigned)(len + 1), (unsigned)free_heap);
        // TODO: Метрика/событие для отслеживания нехватки памяти
        return;
    }
    memcpy(data_copy, data, len);
    data_copy[len] = '\0';  // ← Добавляем '\0' для strlen()
    
    // DEBUG: Показать первые 100 символов JSON
    char preview[101] = {0};
    size_t preview_len = (len > 100) ? 100 : len;
    memcpy(preview, data_copy, preview_len);
    ESP_LOGI(TAG, "   Data: %s%s", preview, (len > 100) ? "..." : "");

    const char *json_error_pos = NULL;
    if (!mesh_protocol_validate_structure(data_copy, &json_error_pos)) {
        size_t error_offset = json_error_pos ? (size_t)(json_error_pos - data_copy) : len;
        ESP_LOGE(TAG, "Invalid JSON structure (offset %u)", (unsigned)error_offset);

        if (json_error_pos != NULL && error_offset < len) {
            char context[33] = {0};
            size_t remaining = len - error_offset;
            size_t copy_len = remaining > 32 ? 32 : remaining;
            for (size_t i = 0; i < copy_len; ++i) {
                char c = json_error_pos[i];
                context[i] = (c >= 32 && c <= 126) ? c : '?';
            }
            ESP_LOGE(TAG, "   Near: \"%s\"%s", context, (remaining > copy_len) ? "..." : "");
        }

        free(data_copy);
        return;
    }

    // Парсинг JSON (используем data_copy с '\0')
    mesh_message_t msg = {0};
    if (!mesh_protocol_parse(data_copy, &msg)) {
        ESP_LOGE(TAG, "❌ Failed to parse mesh message!");
        ESP_LOGE(TAG, "   Raw data: %s", data_copy);
        free(data_copy);
        return;
    }
    
    ESP_LOGI(TAG, "✅ Message parsed: type=%d, node_id=%s", msg.type, msg.node_id);

    const char *our_mesh = zone_config_get_mesh_id();
    if (!zone_config_validate(our_mesh)) {
        ESP_LOGE(TAG, "Router mesh_id is not configured, dropping message");
        mesh_protocol_free_message(&msg);
        free(data_copy);
        return;
    }

    if (msg.mesh_network_id[0] != '\0' &&
        strcmp(msg.mesh_network_id, our_mesh) != 0) {
        ESP_LOGW(TAG,
                 "Message for different mesh ignored: msg.mesh_id=%s, ours=%s",
                 msg.mesh_network_id,
                 our_mesh);
        mesh_protocol_free_message(&msg);
        free(data_copy);
        return;
    }

    // Обновление реестра узлов (отметка последнего контакта)
    node_registry_update_last_seen(msg.node_id, src_addr);

    // Маршрутизация в зависимости от типа сообщения
    switch (msg.type) {
        case MESH_MSG_TELEMETRY:
            ESP_LOGI(TAG, "📊 Telemetry from %s → MQTT", msg.node_id);
            
            // Обновление данных в реестре
            node_registry_update_data(msg.node_id, msg.data);
            
            // Отправка в MQTT с использованием mesh_topic_format
            if (mqtt_client_manager_is_connected()) {
                char topic[192];
                if (!mesh_topic_format(topic, sizeof(topic), NULL,
                                      "telemetry", msg.node_id)) {
                    ESP_LOGE(TAG, "Failed to format telemetry topic");
                    break;
                }
                
                esp_err_t err = mqtt_client_manager_publish(topic, data_copy);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "   ✓ Telemetry published to %s", topic);
                } else {
                    ESP_LOGW(TAG, "   ✗ Failed to publish telemetry: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "MQTT offline, telemetry dropped");
                // TODO: буферизация для отправки позже
            }
            break;

        case MESH_MSG_EVENT:
            ESP_LOGI(TAG, "🔔 Event from %s → MQTT", msg.node_id);
            
            if (mqtt_client_manager_is_connected()) {
                char topic[192];
                if (!mesh_topic_format(topic, sizeof(topic), NULL,
                                      "event", msg.node_id)) {
                    ESP_LOGE(TAG, "Failed to format event topic");
                    break;
                }
                
                esp_err_t err = mqtt_client_manager_publish(topic, data_copy);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "   ✓ Event published to %s", topic);
                } else {
                    ESP_LOGW(TAG, "   ✗ Failed to publish event: %s", esp_err_to_name(err));
                }

                // Проверка критичности события
                cJSON *level = cJSON_GetObjectItem(msg.data, "level");
                if (level && cJSON_IsString(level)) {
                    const char *level_str = level->valuestring;
                    if (strcmp(level_str, "critical") == 0 || strcmp(level_str, "emergency") == 0) {
                        ESP_LOGW(TAG, "⚠️ CRITICAL event from %s!", msg.node_id);
                        // TODO: дополнительные действия (SMS, Telegram)
                    }
                }
            }
            break;

        case MESH_MSG_HEARTBEAT:
            ESP_LOGI(TAG, "💓 Heartbeat from %s → MQTT", msg.node_id);
            
            // Heartbeat обновляет только реестр (уже сделано выше)
            // Отправка в MQTT с использованием mesh_topic_format
            if (mqtt_client_manager_is_connected()) {
                char topic[192];
                if (!mesh_topic_format(topic, sizeof(topic), NULL,
                                      "heartbeat", msg.node_id)) {
                    ESP_LOGE(TAG, "Failed to format heartbeat topic");
                    break;
                }
                
                esp_err_t err = mqtt_client_manager_publish(topic, data_copy);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "   ✓ Heartbeat published to %s (len=%d)", topic, len);
                } else {
                    ESP_LOGW(TAG, "   ✗ Failed to publish heartbeat: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "   ✗ MQTT offline, heartbeat dropped");
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
                        if (mesh_protocol_create_response(msg.node_id, msg.root_node_id, msg.mesh_network_id, nodes_data,
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

        case MESH_MSG_RESPONSE:
            ESP_LOGI(TAG, "📋 Response from %s → MQTT", msg.node_id);
            
            // Это может быть config_response от pH/EC ноды
            // Публикуем в MQTT для backend
            if (mqtt_client_manager_is_connected()) {
                char topic[192];
                if (!mesh_topic_format(topic, sizeof(topic), NULL,
                                      "config_response", msg.node_id)) {
                    ESP_LOGE(TAG, "Failed to format config_response topic");
                    break;
                }
                
                esp_err_t err = mqtt_client_manager_publish(topic, data_copy);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "   ✓ Config response published to %s", topic);
                } else {
                    ESP_LOGW(TAG, "   ✗ Failed to publish config response: %s", esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "MQTT offline, config response dropped");
            }
            break;

        case MESH_MSG_DISCOVERY:
        case MESH_MSG_CONFIG_CONFIRMATION:
            if (mqtt_client_manager_is_connected()) {
                const char *type_str = (msg.type == MESH_MSG_DISCOVERY) ? "discovery" : "config_confirmation";
                char topic[192];
                if (!mesh_topic_format(topic, sizeof(topic), NULL,
                                      "discovery", msg.node_id)) {
                    ESP_LOGE(TAG, "Failed to format discovery topic");
                    break;
                }
                esp_err_t err = mqtt_client_manager_publish(topic, data_copy);
                if (err == ESP_OK) {
                    ESP_LOGI(TAG, "🔍 %s forwarded to MQTT (%s)", type_str, topic);
                } else {
                    ESP_LOGW(TAG, "✗ Failed to publish %s: %s", type_str, esp_err_to_name(err));
                }
            } else {
                ESP_LOGW(TAG, "MQTT offline, discovery message dropped");
            }
            break;

        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }

    mesh_protocol_free_message(&msg);
    free(data_copy);
}

void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len) {
    ESP_LOGI(TAG, "MQTT data received: %s (%d bytes)", topic, data_len);

    // Ожидаем формат: hydro/{mesh_id}/{command|config}/{node_id}
    char topic_copy[256];
    strncpy(topic_copy, topic, sizeof(topic_copy) - 1);
    topic_copy[sizeof(topic_copy) - 1] = '\0';
    
    char *saveptr;
    char *part1 = strtok_r(topic_copy, "/", &saveptr);  // "hydro"
    char *mesh_id = strtok_r(NULL, "/", &saveptr);      // mesh_id
    char *action = strtok_r(NULL, "/", &saveptr);       // "command" or "config"
    char *node_id = strtok_r(NULL, "/", &saveptr);      // node_id
    
    if (!part1 || strcmp(part1, "hydro") != 0 || !mesh_id || !action || !node_id) {
        ESP_LOGW(TAG, "Invalid topic format: %s (expected hydro/{mesh_id}/{command|config}/{node_id})", topic);
        return;
    }
    
    // Проверка что mesh_id совпадает с нашим
    const char *our_mesh = zone_config_get_mesh_id();
    if (!zone_config_validate(our_mesh)) {
        ESP_LOGE(TAG, "Router mesh_id not configured, cannot process MQTT data");
        return;
    }

    if (strcmp(mesh_id, our_mesh) != 0) {
        ESP_LOGW(TAG, "Command for different mesh: %s (ours: %s) - ignoring", mesh_id, our_mesh);
        return;
    }
    
    // Проверка типа действия
    bool is_command = strcmp(action, "command") == 0;
    bool is_config = strcmp(action, "config") == 0;
    
    if (!is_command && !is_config) {
        ESP_LOGW(TAG, "Unknown action: %s (expected command or config)", action);
        return;
    }

    // Поиск узла в реестре
    node_info_t *node = node_registry_get(node_id);
    if (node && node->online) {
        ESP_LOGI(TAG, "Forwarding %s to %s in zone %s",
                 is_command ? "command" : "config", node_id, mesh_id);

        esp_err_t err = mesh_manager_send(node->mac_addr,
                                          (const uint8_t *)data, data_len);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to send to node: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "✓ %s forwarded to %s", is_command ? "Command" : "Config", node_id);
        }
    } else {
        ESP_LOGW(TAG, "Node %s offline or not found, message dropped", node_id);
    }
}

