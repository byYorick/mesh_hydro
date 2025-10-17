/**
 * @file data_router.h
 * @brief Маршрутизация данных между mesh и MQTT
 */

#ifndef DATA_ROUTER_H
#define DATA_ROUTER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t data_router_init(void);
void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len);
void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len);

#ifdef __cplusplus
}
#endif

#endif // DATA_ROUTER_H

