/**
 * @file root_config.h
 * @brief Конфигурация ROOT узла
 * 
 * ⚠️ ВАЖНО: Общие настройки (WiFi, Mesh ID, пароли) теперь в common/mesh_config.h
 */

#ifndef ROOT_CONFIG_H
#define ROOT_CONFIG_H

// Импорт общей конфигурации
#include "mesh_config.h"

// === MESH НАСТРОЙКИ (из общего конфига) ===
#define ROOT_MESH_ID            MESH_NETWORK_ID
#define ROOT_MESH_PASSWORD      MESH_NETWORK_PASSWORD
#define ROOT_MESH_CHANNEL       MESH_NETWORK_CHANNEL
#define ROOT_MAX_CONNECTIONS    ROOT_MAX_MESH_CONNECTIONS

// === WIFI ROUTER (из общего конфига) ===
#define ROOT_ROUTER_SSID        MESH_ROUTER_SSID
#define ROOT_ROUTER_PASSWORD    MESH_ROUTER_PASSWORD

// === MQTT НАСТРОЙКИ ===
#define ROOT_MQTT_BROKER_URI    MQTT_BROKER_URI
#define ROOT_MQTT_USERNAME      ""                      // Пустой = анонимный доступ
#define ROOT_MQTT_PASSWORD      ""                      // Mosquitto настроен на anonymous
#define ROOT_MQTT_KEEPALIVE_SEC 60

// === MQTT ТОПИКИ ===
#define ROOT_MQTT_TOPIC_TELEMETRY   "hydro/telemetry"
#define ROOT_MQTT_TOPIC_EVENT       "hydro/event"
#define ROOT_MQTT_TOPIC_HEARTBEAT   "hydro/heartbeat"
#define ROOT_MQTT_TOPIC_COMMAND     "hydro/command/#"
#define ROOT_MQTT_TOPIC_CONFIG      "hydro/config/#"

// === TIMEOUTS ===
#define ROOT_HEARTBEAT_INTERVAL_MS      10000       // 10 сек
#define ROOT_NODE_TIMEOUT_MS            30000       // 30 сек
#define ROOT_MONITORING_INTERVAL_MS     30000       // 30 сек
#define ROOT_CLIMATE_FALLBACK_CHECK_MS  60000       // 60 сек

// === BUFFER SIZES ===
#define ROOT_MAX_MESH_PACKET_SIZE   1456    // Макс размер mesh пакета
#define ROOT_JSON_BUFFER_SIZE       2048    // Буфер для JSON
#define ROOT_MQTT_BUFFER_SIZE       4096    // Буфер MQTT

// === LOGGING ===
#define ROOT_LOG_LEVEL              ESP_LOG_INFO

#endif // ROOT_CONFIG_H
