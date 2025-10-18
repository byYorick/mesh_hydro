/**
 * @file root_config.h
 * @brief Конфигурация ROOT узла
 */

#ifndef ROOT_CONFIG_H
#define ROOT_CONFIG_H

// === MESH НАСТРОЙКИ ===
#define ROOT_MESH_ID            "HYDRO1"
#define ROOT_MESH_PASSWORD      "hydro_mesh_2025"
#define ROOT_MESH_CHANNEL       0                   // 0 = автовыбор канала
#define ROOT_MAX_CONNECTIONS    10                  // Макс узлов в mesh

// === WIFI ROUTER ===
// По умолчанию используйте значения из sdkconfig если они заданы
#ifndef ROOT_ROUTER_SSID
#define ROOT_ROUTER_SSID        "Yorick"              // Замените на ваш SSID
#endif

#ifndef ROOT_ROUTER_PASSWORD  
#define ROOT_ROUTER_PASSWORD    "pro100parol"         // Замените на ваш пароль
#endif

// === MQTT НАСТРОЙКИ ===
#define ROOT_MQTT_BROKER_URI    "mqtt://192.168.1.100:1883"
#define ROOT_MQTT_USERNAME      "hydro_root"
#define ROOT_MQTT_PASSWORD      "hydro_pass"
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

// Замените на IP вашего ПК
#define ROOT_MQTT_BROKER_URI    "mqtt://192.168.1.100:1883"

// === BUFFER SIZES ===
#define ROOT_MAX_MESH_PACKET_SIZE   1456    // Макс размер mesh пакета
#define ROOT_JSON_BUFFER_SIZE       2048    // Буфер для JSON
#define ROOT_MQTT_BUFFER_SIZE       4096    // Буфер MQTT

// === LOGGING ===
#define ROOT_LOG_LEVEL              ESP_LOG_INFO

#endif // ROOT_CONFIG_H
