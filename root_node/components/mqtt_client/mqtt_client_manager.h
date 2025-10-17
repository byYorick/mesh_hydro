/**
 * @file mqtt_client_manager.h
 * @brief MQTT клиент для ROOT узла
 */

#ifndef MQTT_CLIENT_MANAGER_H
#define MQTT_CLIENT_MANAGER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mqtt_data_cb_t)(const char *topic, const char *data, int data_len);

esp_err_t mqtt_client_manager_init(void);
esp_err_t mqtt_client_manager_publish(const char *topic, const char *data);
esp_err_t mqtt_client_manager_subscribe(const char *topic, mqtt_data_cb_t callback);
bool mqtt_client_manager_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_CLIENT_MANAGER_H

