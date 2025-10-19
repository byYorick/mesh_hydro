/**
 * @file mesh_manager.c
 * @brief Реализация mesh-менеджера
 */

#include "mesh_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include <string.h>

static const char *TAG = "mesh_manager";

static mesh_manager_config_t s_config;
static mesh_recv_cb_t s_recv_cb = NULL;
static bool s_is_mesh_connected = false;
static esp_netif_t *s_netif_sta = NULL;

// Forward declarations
static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
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
    
    // Создание сетевых интерфейсов для mesh (STA и AP)
    esp_netif_t *ap_netif = NULL;
    ESP_ERROR_CHECK(esp_netif_create_default_wifi_mesh_netifs(&s_netif_sta, &ap_netif));

    // Инициализация WiFi
    wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_config));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &ip_event_handler, NULL));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Инициализация mesh
    ESP_ERROR_CHECK(esp_mesh_init());
    ESP_ERROR_CHECK(esp_event_handler_register(MESH_EVENT, ESP_EVENT_ANY_ID, &mesh_event_handler, NULL));
    
    // ВАЖНО: Устанавливаем max_layer ДО esp_mesh_start() (как в официальном примере ESP-IDF)
    // Это ограничивает глубину сети (количество уровней)
    ESP_ERROR_CHECK(esp_mesh_set_max_layer(6));
    ESP_LOGI(TAG, "Max mesh layers set to 6");
    
    // Для ROOT: Устанавливаем vote_percentage=1 (100% голосов для стабильности)
    // Для NODE: Это не влияет, так как ROOT зафиксирован через esp_mesh_fix_root(true)
    ESP_ERROR_CHECK(esp_mesh_set_vote_percentage(1));
    
    // Таймаут ассоциации (как в официальном примере)
    ESP_ERROR_CHECK(esp_mesh_set_ap_assoc_expire(10));

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
    
    // Специфичные настройки для ROOT
    if (s_config.mode == MESH_MODE_ROOT) {
        ESP_ERROR_CHECK(esp_mesh_set_type(MESH_ROOT));
        
        // КРИТИЧНО: Зафиксировать ROOT статус (запретить переключение)
        ESP_ERROR_CHECK(esp_mesh_fix_root(true));
        ESP_LOGI(TAG, "ROOT status fixed (cannot lose ROOT role)");
        
        // ТОЛЬКО ROOT подключается к WiFi роутеру!
        if (s_config.router_ssid == NULL || s_config.router_password == NULL) {
            ESP_LOGE(TAG, "ROOT requires router_ssid and router_password!");
            return ESP_ERR_INVALID_ARG;
        }
        
        cfg.router.ssid_len = strlen(s_config.router_ssid);
        memcpy((uint8_t *)&cfg.router.ssid, s_config.router_ssid, cfg.router.ssid_len);
        memcpy((uint8_t *)&cfg.router.password, s_config.router_password, strlen(s_config.router_password));
        
        // Если указан BSSID - используем его
        if (s_config.router_bssid != NULL) {
            memcpy((uint8_t *)&cfg.router.bssid, s_config.router_bssid, 6);
        }
        
        // Максимум подключений для ROOT
        cfg.mesh_ap.max_connection = s_config.max_connection;
        
        ESP_LOGI(TAG, "ROOT mode: connecting to router '%s' (len=%d), BSSID=%s, max_conn=%d", 
                 s_config.router_ssid, cfg.router.ssid_len,
                 s_config.router_bssid ? "set" : "auto",
                 cfg.mesh_ap.max_connection);
        
        // ✅ ТЕСТ: Попробуем БЕЗ esp_wifi_set_config() (как в официальном примере)
        // Router credentials будут применены через esp_mesh_set_config(&cfg)
        // wifi_config_t wifi_config = {0};
        // strcpy((char *)wifi_config.sta.ssid, s_config.router_ssid);
        // strcpy((char *)wifi_config.sta.password, s_config.router_password);
        // wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        // wifi_config.sta.pmf_cfg.capable = true;
        // wifi_config.sta.pmf_cfg.required = false;
        // ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        
        ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(WIFI_AUTH_WPA2_PSK));
    } else {
        // NODE mode
        // ✅ ГЕНИАЛЬНОЕ РЕШЕНИЕ: Передаем MESH параметры как "router"!
        // NODE будет искать mesh AP "HYDRO1" вместо роутера "Yorick"
        
        // Устанавливаем mesh AP как "роутер" для NODE
        cfg.router.ssid_len = strlen(s_config.mesh_id);
        memcpy((uint8_t *)&cfg.router.ssid, s_config.mesh_id, cfg.router.ssid_len);
        memcpy((uint8_t *)&cfg.router.password, s_config.mesh_password, strlen(s_config.mesh_password));
        
        // BSSID не устанавливаем - NODE найдет любой AP с SSID "HYDRO1"
        
        // Максимум подключений для NODE (для их собственного mesh AP)
        cfg.mesh_ap.max_connection = 6;
        
        ESP_LOGI(TAG, "NODE mode: will search for mesh AP '%s' (password: %s) on channel %d", 
                 s_config.mesh_id, 
                 strlen(s_config.mesh_password) > 0 ? "***" : "OPEN",
                 cfg.channel);
    }

    // Установка mesh пароля
    cfg.mesh_ap.password[0] = '\0';
    if (s_config.mesh_password && strlen(s_config.mesh_password) > 0) {
        strcpy((char *)cfg.mesh_ap.password, s_config.mesh_password);
    }
    
    // ✅ ВАЖНО: Установить authmode для mesh AP ДО esp_mesh_set_config() (как в примере)
    ESP_ERROR_CHECK(esp_mesh_set_ap_authmode(
        (s_config.mesh_password && strlen(s_config.mesh_password) > 0) 
        ? WIFI_AUTH_WPA2_PSK 
        : WIFI_AUTH_OPEN
    ));

    // Логирование Mesh AP конфигурации (для ROOT)
    if (s_config.mode == MESH_MODE_ROOT) {
        ESP_LOGI(TAG, "ROOT Mesh AP configuration:");
        ESP_LOGI(TAG, "  SSID: %s (from mesh_id)", s_config.mesh_id);
        ESP_LOGI(TAG, "  Password: %s", 
                 strlen(s_config.mesh_password) > 0 ? "***" : "OPEN");
        ESP_LOGI(TAG, "  Max connections: %d", cfg.mesh_ap.max_connection);
        ESP_LOGI(TAG, "  Channel: %d (0=auto)", cfg.channel);
    }

    // Применение конфигурации с router credentials
    ESP_ERROR_CHECK(esp_mesh_set_config(&cfg));
    
    // Для NODE тип устанавливается автоматически (MESH_NODE - дефолт)
    // esp_mesh_set_type() вызывается только для ROOT (выше)
    
    // NODE: Никаких дополнительных настроек не требуется!
    // ROOT зафиксирован через esp_mesh_fix_root(true), поэтому NODE не может стать ROOT
    // Self-organizing включен по умолчанию, NODE автоматически найдёт и подключится к mesh AP

    // Запуск mesh
    ESP_ERROR_CHECK(esp_mesh_start());

    // Запуск задачи приема данных (увеличен stack в 4 раза для безопасности: 4096 → 16384)
    xTaskCreate(mesh_recv_task, "mesh_recv", 16384, NULL, 5, NULL);

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
    int flag;
    
    if (dest_addr == NULL) {
        // Отправка на ROOT (TODS - To Distribution System)
        memset(&addr, 0, sizeof(addr));
        flag = MESH_DATA_TODS;  // ← Флаг для отправки к ROOT
    } else {
        // Отправка конкретному узлу (P2P)
        memcpy(addr.addr, dest_addr, 6);
        flag = MESH_DATA_P2P;   // ← Флаг для P2P
    }

    esp_err_t err = esp_mesh_send(&addr, &mesh_data, flag, NULL, 0);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Mesh send failed: %s", esp_err_to_name(err));
    }

    return err;
}

esp_err_t mesh_manager_send_to_root(const uint8_t *data, size_t len) {
    // Проверка: только NODE могут отправлять на ROOT
    if (s_config.mode == MESH_MODE_ROOT) {
        ESP_LOGW(TAG, "ROOT cannot send to itself");
        return ESP_ERR_MESH_ARGUMENT;
    }
    
    // Проверка подключения
    if (!s_is_mesh_connected) {
        ESP_LOGW(TAG, "Mesh not connected, cannot send to root");
        return ESP_ERR_MESH_NOT_START;
    }
    
    // Проверка что узел не является root (не голосованием)
    if (esp_mesh_is_root()) {
        ESP_LOGW(TAG, "Node is currently root (voting), cannot send to parent");
        return ESP_ERR_MESH_NO_PARENT_FOUND;
    }
    
    return mesh_manager_send(NULL, data, len);
}

esp_err_t mesh_manager_broadcast(const uint8_t *data, size_t len) {
    if (!s_is_mesh_connected) {
        ESP_LOGW(TAG, "Mesh not connected, cannot broadcast");
        return ESP_ERR_MESH_NOT_START;
    }

    // Broadcast доступен только для ROOT узла
    if (!mesh_manager_is_root()) {
        ESP_LOGW(TAG, "Broadcast available only for ROOT node");
        return ESP_ERR_MESH_NOT_ALLOWED;
    }

    mesh_data_t mesh_data;
    mesh_data.data = (uint8_t *)data;
    mesh_data.size = len;
    mesh_data.proto = MESH_PROTO_BIN;
    mesh_data.tos = MESH_TOS_P2P;

    // Получение списка всех дочерних узлов
    int total_nodes = esp_mesh_get_total_node_num();
    
    if (total_nodes <= 1) {
        ESP_LOGW(TAG, "No child nodes to broadcast to");
        return ESP_OK;
    }

    // Получение таблицы маршрутизации
    #define MAX_ROUTE_TABLE_SIZE 50
    mesh_addr_t route_table[MAX_ROUTE_TABLE_SIZE];
    int route_table_size = MAX_ROUTE_TABLE_SIZE;
    
    esp_err_t err = esp_mesh_get_routing_table((mesh_addr_t *)&route_table, 
                                                 route_table_size * sizeof(mesh_addr_t), 
                                                 &route_table_size);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get routing table: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Broadcasting to %d nodes", route_table_size);

    // Отправка данных каждому узлу
    int success_count = 0;
    for (int i = 0; i < route_table_size; i++) {
        mesh_addr_t dest_addr;
        memcpy(dest_addr.addr, route_table[i].addr, 6);
        
        err = esp_mesh_send(&dest_addr, &mesh_data, MESH_DATA_P2P, NULL, 0);
        
        if (err == ESP_OK) {
            success_count++;
        } else {
            ESP_LOGW(TAG, "Failed to send to node "MACSTR": %s", 
                     MAC2STR(dest_addr.addr), esp_err_to_name(err));
        }
    }

    ESP_LOGI(TAG, "Broadcast completed: %d/%d nodes", success_count, route_table_size);

    return (success_count > 0) ? ESP_OK : ESP_FAIL;
}

void mesh_manager_register_recv_cb(mesh_recv_cb_t cb) {
    s_recv_cb = cb;
    ESP_LOGI(TAG, "✅ Recv callback registered: %p", (void*)cb);
}

bool mesh_manager_is_root(void) {
    return (s_config.mode == MESH_MODE_ROOT) && esp_mesh_is_root();
}

bool mesh_manager_is_connected(void) {
    return s_is_mesh_connected;
}

int mesh_manager_get_total_nodes(void) {
    return esp_mesh_get_total_node_num();
}

esp_err_t mesh_manager_get_mac(uint8_t *mac) {
    return esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
}

// --- Private functions ---

static void ip_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "========================================");
        ESP_LOGI(TAG, "✓ ROOT GOT IP: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "  Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG, "  Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "========================================");
    }
}

static void mesh_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    mesh_event_id_t event = (mesh_event_id_t)event_id;

    switch (event) {
        case MESH_EVENT_STARTED:
            ESP_LOGI(TAG, "Mesh started");
            break;

        case MESH_EVENT_STOPPED:
            ESP_LOGI(TAG, "Mesh stopped");
            s_is_mesh_connected = false;
            break;

        case MESH_EVENT_PARENT_CONNECTED: {
            mesh_event_connected_t *connected = (mesh_event_connected_t *)event_data;
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "✓ MESH Parent connected!");
            ESP_LOGI(TAG, "  Layer: %d", connected->self_layer);
            ESP_LOGI(TAG, "  Parent BSSID: %02x:%02x:%02x:%02x:%02x:%02x",
                     MAC2STR(connected->connected.bssid));
            ESP_LOGI(TAG, "========================================");
            s_is_mesh_connected = true;
            
            // ROOT зафиксирован ДО подключения к роутеру, поэтому MESH_EVENT_ROOT_FIXED может не прийти
            // Запускаем DHCP клиент здесь для ROOT узла
            if (s_config.mode == MESH_MODE_ROOT && s_netif_sta) {
                ESP_LOGI(TAG, "Starting DHCP client for ROOT node...");
                esp_err_t err = esp_netif_dhcpc_start(s_netif_sta);
                if (err != ESP_OK && err != ESP_ERR_ESP_NETIF_DHCP_ALREADY_STARTED) {
                    ESP_LOGE(TAG, "Failed to start DHCP client: %s", esp_err_to_name(err));
                } else {
                    ESP_LOGI(TAG, "DHCP client started successfully");
                }
            }
            
            // NODE узлы не запускают DHCP - они получают IP через mesh
            break;
        }

        case MESH_EVENT_PARENT_DISCONNECTED:
            ESP_LOGI(TAG, "Parent disconnected");
            s_is_mesh_connected = false;
            break;

        case MESH_EVENT_ROOT_FIXED:
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "✓ ROOT node established and fixed!");
            ESP_LOGI(TAG, "========================================");
            s_is_mesh_connected = true;
            
            // ROOT зафиксирован - запускаем DHCP
            if (s_config.mode == MESH_MODE_ROOT && s_netif_sta) {
                ESP_LOGI(TAG, "Starting DHCP client for fixed ROOT...");
                esp_netif_dhcpc_start(s_netif_sta);
            }
            break;

        case MESH_EVENT_CHILD_CONNECTED: {
            mesh_event_child_connected_t *child = (mesh_event_child_connected_t *)event_data;
            ESP_LOGI(TAG, "Child connected: %02x:%02x:%02x:%02x:%02x:%02x", 
                     MAC2STR(child->mac));
            break;
        }

        case MESH_EVENT_CHILD_DISCONNECTED: {
            mesh_event_child_disconnected_t *child = (mesh_event_child_disconnected_t *)event_data;
            ESP_LOGI(TAG, "Child disconnected: %02x:%02x:%02x:%02x:%02x:%02x", 
                     MAC2STR(child->mac));
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
    const int max_data_size = 1500; // Max mesh packet size
    data.data = malloc(max_data_size);
    
    if (data.data == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for mesh recv");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "mesh_recv_task started, waiting for data...");
    
    while (true) {
        // ВАЖНО: Сбрасываем data.size перед каждым приёмом!
        data.size = max_data_size;
        
        esp_err_t err = esp_mesh_recv(&from, &data, portMAX_DELAY, &flag, NULL, 0);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "✓ Mesh data received: %d bytes from "MACSTR" (flag=%d)", 
                     data.size, MAC2STR(from.addr), flag);
            
            // DEBUG: Вывести первые 100 байт данных
            ESP_LOG_BUFFER_HEXDUMP(TAG, data.data, (data.size > 100 ? 100 : data.size), ESP_LOG_INFO);
            
            if (s_recv_cb != NULL) {
                ESP_LOGI(TAG, "Calling recv_cb...");
                s_recv_cb(from.addr, data.data, data.size);
                ESP_LOGI(TAG, "recv_cb returned");
            } else {
                ESP_LOGW(TAG, "⚠️ No recv callback registered - data dropped!");
            }
        } else {
            ESP_LOGE(TAG, "Mesh recv failed: %s", esp_err_to_name(err));
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    free(data.data);
    vTaskDelete(NULL);
}

esp_err_t mesh_manager_get_routing_table_with_rssi(mesh_node_info_t *nodes, int max_count, int *actual_count) {
    if (nodes == NULL || actual_count == NULL) {
        ESP_LOGE(TAG, "Invalid arguments");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!mesh_manager_is_root()) {
        ESP_LOGW(TAG, "Not a ROOT node, routing table not available");
        return ESP_ERR_NOT_SUPPORTED;
    }
    
    if (!s_is_mesh_connected) {
        ESP_LOGW(TAG, "Mesh not connected");
        *actual_count = 0;
        return ESP_ERR_MESH_NOT_START;
    }
    
    // Получить общее количество узлов (API изменился в v5.5 - теперь возвращает напрямую)
    int total_nodes = esp_mesh_get_total_node_num();
    
    if (total_nodes <= 1) {
        ESP_LOGD(TAG, "No child nodes");
        *actual_count = 0;
        return ESP_OK;
    }
    
    // Выделить временный буфер для таблицы маршрутизации
    int route_table_size = (total_nodes < max_count) ? total_nodes : max_count;
    mesh_addr_t *route_table = malloc(route_table_size * sizeof(mesh_addr_t));
    if (route_table == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for routing table");
        return ESP_ERR_NO_MEM;
    }
    
    // Получить таблицу маршрутизации
    int actual_table_size = route_table_size;
    esp_err_t err = esp_mesh_get_routing_table(route_table, 
                                                 route_table_size * sizeof(mesh_addr_t), 
                                                 &actual_table_size);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get routing table: %s", esp_err_to_name(err));
        free(route_table);
        return err;
    }
    
    // Заполнить массив узлов с информацией
    *actual_count = (actual_table_size < max_count) ? actual_table_size : max_count;
    
    for (int i = 0; i < *actual_count; i++) {
        // Копировать MAC адрес
        memcpy(nodes[i].mac, route_table[i].addr, 6);
        
        // В ESP-IDF v5.5 esp_mesh_get_routing_table_by_mac() удалена
        // Используем базовую информацию без RSSI и layer
        // TODO: Если нужны RSSI/layer - использовать esp_mesh_get_subnet_nodes_list()
        nodes[i].rssi = 0;   // RSSI недоступен через routing table API
        nodes[i].layer = 0;  // Layer недоступен напрямую
    }
    
    free(route_table);
    
    ESP_LOGD(TAG, "Retrieved %d nodes from routing table", *actual_count);
    return ESP_OK;
}

int8_t mesh_manager_get_parent_rssi(void) {
    // Проверка: ROOT узлы не имеют родителя
    if (mesh_manager_is_root()) {
        return 0;
    }
    
    // Проверка подключения
    if (!s_is_mesh_connected) {
        return 0;
    }
    
    // Получить информацию о родительском узле
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    
    if (err == ESP_OK) {
        ESP_LOGD(TAG, "Parent RSSI: %d dBm", ap_info.rssi);
        return ap_info.rssi;
    } else {
        ESP_LOGW(TAG, "Failed to get parent RSSI: %s", esp_err_to_name(err));
        return 0;
    }
}

