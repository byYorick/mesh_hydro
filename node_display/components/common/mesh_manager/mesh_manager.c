/**
 * @file mesh_manager.c
 * @brief Реализация mesh-менеджера
 */

#include "mesh_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "mesh_manager";

static mesh_manager_config_t s_config;
static mesh_recv_cb_t s_recv_cb = NULL;
static bool s_is_mesh_connected = false;

// Forward declarations
static void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static void mesh_recv_task(void *arg);

esp_err_t mesh_manager_init(const mesh_manager_config_t *config) {
    if (config == NULL) {
        ESP_LOGE(TAG, "Config is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&s_config, config, sizeof(mesh_manager_config_t));

    // Инициализация NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Инициализация TCP/IP
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Инициализация WiFi
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));

    // Инициализация mesh
    ESP_ERROR_CHECK(esp_mesh_init());

    // Регистрация обработчика событий
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));

    ESP_LOGI(TAG, "Mesh manager initialized (mode: %s)", 
             config->mode == MESH_MODE_ROOT ? "ROOT" : "NODE");

    return ESP_OK;
}

esp_err_t mesh_manager_start(void) {
    mesh_cfg_t cfg = MESH_INIT_CONFIG_DEFAULT();
    
    // Установка mesh ID
    memcpy((uint8_t *)&cfg.mesh_id, s_config.mesh_id, 6);
    
    // Установка канала
    cfg.channel = s_config.channel;
    
    // Установка типа узла
    if (s_config.mode == MESH_MODE_ROOT) {
        cfg.mesh_ap.max_connection = s_config.max_connection;
        ESP_ERROR_CHECK(esp_mesh_set_type(MESH_ROOT));
        
        // Настройка подключения к роутеру
        wifi_config_t wifi_config = {0};
        strcpy((char *)wifi_config.sta.ssid, s_config.router_ssid);
        strcpy((char *)wifi_config.sta.password, s_config.router_password);
        ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK));
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        
        ESP_LOGI(TAG, "ROOT mode: connecting to router %s", s_config.router_ssid);
    } else {
        ESP_ERROR_CHECK(esp_mesh_set_type(MESH_NODE));
        ESP_LOGI(TAG, "NODE mode");
    }

    // Установка mesh пароля
    cfg.mesh_ap.password[0] = '\0';
    if (s_config.mesh_password && strlen(s_config.mesh_password) > 0) {
        strcpy((char *)cfg.mesh_ap.password, s_config.mesh_password);
    }

    // Применение конфигурации
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));

    // Запуск mesh
    ESP_ERROR_CHECK(esp_mesh_start());

    // Запуск задачи приема данных
    xTaskCreate(mesh_recv_task, "mesh_recv", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Mesh started");

    return ESP_OK;
}

esp_err_t mesh_manager_stop(void) {
    ESP_ERROR_CHECK(esp_mesh_stop());
    s_is_mesh_connected = false;
    ESP_LOGI(TAG, "Mesh stopped");
    return ESP_OK;
}

esp_err_t mesh_manager_send(const uint8_t *dest_addr, const uint8_t *data, size_t len) {
    if (!s_is_mesh_connected) {
        ESP_LOGW(TAG, "Mesh not connected, cannot send");
        return ESP_ERR_MESH_NOT_START;
    }

    mesh_data_t mesh_data;
    mesh_data.data = (uint8_t *)data;
    mesh_data.size = len;
    mesh_data.proto = MESH_PROTO_BIN;
    mesh_data.tos = MESH_TOS_P2P;

    mesh_addr_t addr;
    if (dest_addr == NULL) {
        // Отправка на ROOT
        addr.addr[0] = 0;
        addr.addr[1] = 0;
        addr.addr[2] = 0;
        addr.addr[3] = 0;
        addr.addr[4] = 0;
        addr.addr[5] = 0;
    } else {
        memcpy(addr.addr, dest_addr, 6);
    }

    int flag = 0;
    esp_err_t err = esp_mesh_send(&addr, &mesh_data, flag, NULL, 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mesh send failed: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t mesh_manager_send_to_root(const uint8_t *data, size_t len) {
    return mesh_manager_send(NULL, data, len);
}

esp_err_t mesh_manager_broadcast(const uint8_t *data, size_t len) {
    // TODO: Реализовать broadcast всем узлам
    ESP_LOGW(TAG, "Broadcast not implemented yet");
    return ESP_ERR_NOT_SUPPORTED;
}

void mesh_manager_register_recv_cb(mesh_recv_cb_t cb) {
    s_recv_cb = cb;
}

bool mesh_manager_is_root(void) {
    return (s_config.mode == MESH_MODE_ROOT) && esp_mesh_is_root();
}

bool mesh_manager_is_connected(void) {
    return s_is_mesh_connected;
}

int mesh_manager_get_total_nodes(void) {
    int total = 0;
    esp_mesh_get_total_node_num(&total);
    return total;
}

esp_err_t mesh_manager_get_mac(uint8_t *mac) {
    return esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
}

// --- Private functions ---

static void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    mesh_event_t event = (mesh_event_t)event_id;

    switch (event) {
        case MESH_EVENT_STARTED:
            ESP_LOGI(TAG, "Mesh started");
            break;

        case MESH_EVENT_STOPPED:
            ESP_LOGI(TAG, "Mesh stopped");
            s_is_mesh_connected = false;
            break;

        case MESH_EVENT_PARENT_CONNECTED:
            ESP_LOGI(TAG, "Parent connected");
            s_is_mesh_connected = true;
            break;

        case MESH_EVENT_PARENT_DISCONNECTED:
            ESP_LOGI(TAG, "Parent disconnected");
            s_is_mesh_connected = false;
            break;

        case MESH_EVENT_ROOT_GOT_IP:
            ESP_LOGI(TAG, "ROOT got IP address");
            s_is_mesh_connected = true;
            break;

        case MESH_EVENT_CHILD_CONNECTED: {
            mesh_event_child_connected_t *child = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(TAG, "Child connected: "MACSTR, MAC2STR(child->mac));
            break;
        }

        case MESH_EVENT_CHILD_DISCONNECTED: {
            mesh_event_child_disconnected_t *child = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(TAG, "Child disconnected: "MACSTR, MAC2STR(child->mac));
            break;
        }

        default:
            break;
    }
}

static void mesh_recv_task(void *arg) {
    mesh_addr_t from;
    mesh_data_t data;
    int flag = 0;
    data.data = malloc(1500); // Max mesh packet size
    data.size = 1500;

    while (true) {
        esp_err_t err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        
        if (err == ESP_OK) {
            if (s_recv_cb != NULL) {
                s_recv_cb(from.addr, data.data, data.size);
            }
        } else {
            ESP_LOGE(TAG, "Mesh recv failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    free(data.data);
    vTaskDelete(NULL);
}

