/**
 * @file data_router.h
 * @brief Маршрутизатор данных между mesh и MQTT
 * 
 * Обрабатывает данные от NODE узлов и перенаправляет их в MQTT.
 * Обрабатывает команды от MQTT и перенаправляет их в mesh.
 */

#ifndef DATA_ROUTER_H
#define DATA_ROUTER_H

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Инициализация маршрутизатора данных
 * 
 * @return ESP_OK при успехе
 */
esp_err_t data_router_init(void);

/**
 * @brief Обработка данных от NODE через mesh
 * 
 * Вызывается из mesh_manager callback
 * 
 * @param src_addr MAC адрес отправителя
 * @param data Данные (JSON строка)
 * @param len Длина данных
 */
void data_router_handle_mesh_data(const uint8_t *src_addr, const uint8_t *data, size_t len);

/**
 * @brief Обработка команд от MQTT
 * 
 * Вызывается из mqtt_client_manager callback
 * 
 * @param topic MQTT топик
 * @param data Данные (JSON строка)
 * @param data_len Длина данных
 */
void data_router_handle_mqtt_data(const char *topic, const char *data, int data_len);

#ifdef __cplusplus
}
#endif

#endif // DATA_ROUTER_H

