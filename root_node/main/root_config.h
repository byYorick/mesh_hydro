/**
 * @file root_config.h
 * @brief Конфигурация ROOT узла
 */

#ifndef ROOT_CONFIG_H
#define ROOT_CONFIG_H

// Mesh настройки
#define ROOT_MESH_ID "HYDRO1"
#define ROOT_MESH_PASSWORD "hydro_mesh_pass"
#define ROOT_MESH_CHANNEL 1
#define ROOT_MAX_CONNECTIONS 10

// MQTT настройки
#define ROOT_MQTT_BROKER_URI "mqtt://192.168.1.100:1883"
#define ROOT_MQTT_USERNAME "hydro_system"
#define ROOT_MQTT_PASSWORD "hydro_mqtt_pass"

// MQTT топики
#define MQTT_TOPIC_TELEMETRY "hydro/telemetry"
#define MQTT_TOPIC_COMMAND "hydro/command"
#define MQTT_TOPIC_CONFIG "hydro/config"
#define MQTT_TOPIC_EVENT "hydro/event"
#define MQTT_TOPIC_STATUS "hydro/status"

// Node Registry
#define MAX_NODES 32

// Heartbeat
#define HEARTBEAT_INTERVAL_MS 10000  // 10 секунд
#define NODE_TIMEOUT_MS 30000        // 30 секунд без heartbeat = offline

#endif // ROOT_CONFIG_H

