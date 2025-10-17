/**
 * @file data_router.c
 * @brief Реализация маршрутизатора данных
 */

#include "data_router.h"
#include "mesh_protocol.h"
#include "mesh_manager.h"
#include "mqtt_client_manager.h"
#include "node_registry.h"
#include "esp_log.h"

static const char *TAG = "data_router";

esp_err_t data_router_init(void) {
    // Регистрация callback для mesh
    mesh_manager_register_recv_cb(data_router_handle_mesh_data);
    
    // Подписка на MQTT топики команд
    mqtt_client_manager_subscribe("hydro/command/#", data_router_handle_mqtt_data);
    mqtt_client_manager_subscribe("hydro/config/#", data_router_handle_mqtt_data);
    
    ESP_LOGI(TAG, "Data router initialized");
    return ESP_OK;
}

void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len) {
    ESP_LOGI(TAG, "Mesh data received (%d bytes)", len);
    
    // Парсинг JSON
    mesh_message_t msg;
    if (!mesh_protocol_parse((char*)data, &msg)) {
        ESP_LOGE(TAG, "Failed to parse mesh message");
        return;
    }
    
    // Обновление реестра
    node_registry_update_last_seen(msg.node_id);
    
    // Маршрутизация в MQTT
    switch (msg.type) {
        case MESH_MSG_TELEMETRY:
            mqtt_client_manager_publish("hydro/telemetry", (char*)data);
            break;
            
        case MESH_MSG_EVENT:
            mqtt_client_manager_publish("hydro/event", (char*)data);
            break;
            
        case MESH_MSG_HEARTBEAT:
            // Heartbeat не отправляем в MQTT (только для внутреннего учета)
            break;
            
        default:
            ESP_LOGW(TAG, "Unknown message type: %d", msg.type);
            break;
    }
    
    mesh_protocol_free_message(&msg);
}

void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len) {
    ESP_LOGI(TAG, "MQTT data received: %s", topic);
    
    // TODO: Парсинг команды и отправка на NODE
    // Пример: hydro/command/ph_ec_001 -> отправить на node_id=ph_ec_001
}

