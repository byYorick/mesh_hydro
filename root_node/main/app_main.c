/**
 * @file app_main.c
 * @brief ROOT NODE - Точка входа координатора mesh-сети
 * 
 * ESP32-S3 #1 - Mesh ROOT + MQTT Bridge
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_http_server.h"
#include "esp_chip_info.h"
#include "esp_app_desc.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "driver/gpio.h"

// Common компоненты
#include "mesh_manager.h"
#include "mesh_protocol.h"
#include "node_config.h"
#include "mesh_config.h"
#include "zone_config.h"
#include "cJSON.h"
#include "root_defaults.h"

// ROOT компоненты
#include "node_registry.h"
#include "mqtt_client_manager.h"
#include "data_router.h"
#include "climate_logic.h"
#include "root_config.h"
#include "setup_portal.h"
#include "oled_display.h"

static const char *TAG = "root_main";

#define SETUP_SERVER_URL "http://" MQTT_BROKER_HOST ":8000/api/discovery/root"
#define SETUP_HEARTBEAT_INTERVAL_MS 10000

static SemaphoreHandle_t s_credentials_sem = NULL;
static SemaphoreHandle_t s_config_sem = NULL;
static setup_portal_credentials_t s_credentials = {0};
static TaskHandle_t s_heartbeat_task_handle = NULL;
static httpd_handle_t s_config_http_server = NULL;
static bool s_setup_active = false;
static esp_netif_ip_info_t s_setup_ip_info = {0};
static char s_current_pin[7] = {0};
static char s_temp_mesh_id[32] = {0};
static char s_mesh_id_buffer[32] = {0};
static char s_router_ssid_buffer[33] = {0};
static char s_router_pass_buffer[65] = {0};
static char s_setup_ap_ssid[32] = {0};

#define ROOT_OLED_I2C_PORT  I2C_NUM_1
#define ROOT_OLED_SDA_PIN   GPIO_NUM_8
#define ROOT_OLED_SCL_PIN   GPIO_NUM_9

#define ROOT_SETUP_BUTTON_PIN      GPIO_NUM_45
#define ROOT_BUTTON_ACTIVE_LEVEL   0
#define ROOT_BUTTON_POLL_MS        50
#define ROOT_BUTTON_LONG_PRESS_MS  3000

static bool s_root_display_ready = false;
static int s_last_nodes_online = 0;
static int8_t s_last_wifi_rssi = 0;
static bool s_last_router_connected = false;
static bool s_last_mqtt_connected = false;
static TaskHandle_t s_button_task_handle = NULL;
static bool s_setup_request_pending = false;

#define NODE_PAIRING_SOFTAP_PASS MESH_NETWORK_PASSWORD
#define NODE_PAIRING_SOFTAP_CHANNEL 6
#define NODE_PAIRING_SOFTAP_GRACE_MS 10000
#define NODE_PAIRING_SSID_PREFIX "ROOT_PAIR_"
#define NODE_PAIRING_DISCOVERY_TIMEOUT_MS 120000
#define NODE_PAIRING_CONFIRM_TIMEOUT_MS 60000

typedef struct {
    char generated_pin[7];
    char mesh_tag_hex[13];
    uint8_t mesh_id_bytes[6];
    char ap_ssid[33];
    char node_pin[8];
    char node_type[16];
    char node_mac_str[18];
    char assigned_node_id[32];
    char router_ssid[33];
    char router_password[65];
    uint8_t node_mac[6];
    uint8_t src_mac[6];
    bool discovery_received;
    bool confirmation_received;
} node_pairing_context_t;

static node_pairing_context_t s_pairing_ctx = {0};
static SemaphoreHandle_t s_pairing_discovery_sem = NULL;
static SemaphoreHandle_t s_pairing_confirmation_sem = NULL;

static void run_setup_mode(void);
static void run_normal_mode(void);
static void run_node_pairing_mode(void);
static void setup_portal_credentials_cb(const setup_portal_credentials_t *credentials, void *user_ctx);
static esp_err_t start_setup_mesh(const char *mesh_id, const char *ssid, const char *password, esp_netif_ip_info_t *ip_info_out);
static esp_err_t send_setup_message(const char *type, const char *pin, const char *temp_mesh_id, const esp_netif_ip_info_t *ip_info);
static void setup_heartbeat_task(void *arg);
static esp_err_t start_config_http_server(void);
static void stop_config_http_server(void);
static esp_err_t config_post_handler(httpd_req_t *req);
static void schedule_restart_task(void *arg);
static void send_config_confirmation_task(void *arg);
static void root_display_init(const root_config_t *cfg);
static void root_display_update(int8_t wifi_rssi, int online_nodes, const char *mesh_id,
                                bool router_connected, bool mqtt_connected);
static void root_display_show_heartbeat(void);
static bool root_get_router_status(int8_t *rssi_out);
static void root_display_show_setup(const char *ap_ssid, const char *pin, const char *ssid_hint, const char *status);
static void root_display_show_setup_request(void);
static void root_display_show_setup_error(const char *message);
static void root_display_show_pairing_status(const char *ap_ssid, const char *pin, const char *status);
static void root_display_configure_normal(const char *mesh_id);
static void root_button_init(void);
static void root_button_task(void *arg);
static void handle_setup_button_trigger(void);
static void node_pairing_mesh_recv_cb(const uint8_t *src_addr, const uint8_t *data, size_t len);
static bool node_pairing_assign_node_id(const char *node_type, const uint8_t mac[6], char *out_id, size_t out_len);
static esp_err_t node_pairing_send_write_config(const root_config_t *root_cfg);
static void root_log_config_state(const char *stage);

static void root_display_init(const root_config_t *cfg) {
    if (s_root_display_ready) {
        return;
    }

    oled_display_config_t disp_cfg = {
        .i2c_port = ROOT_OLED_I2C_PORT,
        .sda_pin = ROOT_OLED_SDA_PIN,
        .scl_pin = ROOT_OLED_SCL_PIN,
        .clk_speed_hz = 400000,
        .i2c_address = 0x3C,
        .width = 128,
        .height = 64,
        .line_count = 4,
    };

    if (oled_display_init(&disp_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "OLED init failed");
        return;
    }

    oled_display_task_config_t task_cfg = {
        .stack_size = 4096,
        .priority = 4,
        .queue_depth = 6,
        .heartbeat_timeout_ticks = pdMS_TO_TICKS(2000),
    };

    if (oled_display_start_task(&task_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "OLED task start failed");
        return;
    }

    s_root_display_ready = true;
    root_display_configure_normal((cfg && cfg->mesh_network_id[0]) ? cfg->mesh_network_id : NULL);
}

static const char* get_node_short_name(const node_info_t *node);
static const char* get_msg_type_icon(node_msg_type_t msg_type);

static void root_display_update(int8_t wifi_rssi, int online_nodes, const char *mesh_id,
                                bool router_connected, bool mqtt_connected) {
    if (!s_root_display_ready) {
        return;
    }

    char wifi_buf[12];
    if (router_connected && wifi_rssi <= 0) {
        snprintf(wifi_buf, sizeof(wifi_buf), "%d", wifi_rssi);
    } else if (router_connected) {
        snprintf(wifi_buf, sizeof(wifi_buf), "%d", wifi_rssi);
    } else {
        strlcpy(wifi_buf, "--", sizeof(wifi_buf));
    }

    char nodes_buf[12];
    snprintf(nodes_buf, sizeof(nodes_buf), "%d", online_nodes);

    char router_buf[4];
    strlcpy(router_buf, router_connected ? "OK" : "NO", sizeof(router_buf));

    char mqtt_buf[4];
    strlcpy(mqtt_buf, mqtt_connected ? "OK" : "NO", sizeof(mqtt_buf));

    const char *mesh_value = (mesh_id && mesh_id[0]) ? mesh_id : "UNSET";

    // Получаем список всех онлайн нод
    node_info_t nodes[MAX_NODES];
    int node_count = node_registry_get_all(nodes);
    
    // Текущее время для проверки сброса иконок (0.5 сек = 500 мс)
    uint64_t now_ms = esp_timer_get_time() / 1000;
    const uint64_t MSG_ICON_TIMEOUT_MS = 500;  // 0.5 секунды
    
    // Формируем строки для нод (максимум 5 строк: 3-7)
    char node_lines[5][21] = {0};  // 21 символ для строки на OLED 128x64
    for (int i = 0; i < 5 && i < node_count; i++) {
        const char *short_name = get_node_short_name(&nodes[i]);
        
        // Проверяем, прошло ли больше 0.5 сек с последнего сообщения
        node_msg_type_t display_msg_type = NODE_MSG_NONE;
        if (nodes[i].last_msg_type != NODE_MSG_NONE) {
            // Всегда показываем иконку, если есть тип сообщения
            // Проверяем время только для сброса
            bool show_icon = true;
            if (nodes[i].last_msg_time_ms > 0) {
                uint64_t elapsed = now_ms - nodes[i].last_msg_time_ms;
                // Проверяем на переполнение (если now_ms < last_msg_time_ms, значит произошло переполнение)
                if (now_ms >= nodes[i].last_msg_time_ms && elapsed > MSG_ICON_TIMEOUT_MS) {
                    show_icon = false;  // Прошло больше 0.5 сек - скрываем
                }
            }
            if (show_icon) {
                display_msg_type = nodes[i].last_msg_type;
            }
            ESP_LOGD(TAG, "Node %s: last_msg_type=%d, last_msg_time_ms=%llu, now_ms=%llu, show_icon=%d, display_msg_type=%d",
                     short_name, nodes[i].last_msg_type, nodes[i].last_msg_time_ms, now_ms, show_icon, display_msg_type);
        }
        
        const char *icon = get_msg_type_icon(display_msg_type);
        // Формат: "ph H" или "climate T" (короткое имя + иконка)
        snprintf(node_lines[i], sizeof(node_lines[i]), "%.12s %s", short_name, icon);
    }

    oled_display_kv_t values[] = {
        {.key = "mesh", .value = mesh_value},
        {.key = "wifi", .value = wifi_buf},
        {.key = "nodes", .value = nodes_buf},
        {.key = "router", .value = router_buf},
        {.key = "mqtt", .value = mqtt_buf},
        {.key = "node3", .value = node_count > 0 ? node_lines[0] : ""},
        {.key = "node4", .value = node_count > 1 ? node_lines[1] : ""},
        {.key = "node5", .value = node_count > 2 ? node_lines[2] : ""},
        {.key = "node6", .value = node_count > 3 ? node_lines[3] : ""},
        {.key = "node7", .value = node_count > 4 ? node_lines[4] : ""},
    };
    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);

    s_last_nodes_online = online_nodes;
    s_last_wifi_rssi = wifi_rssi;
    s_last_router_connected = router_connected;
    s_last_mqtt_connected = mqtt_connected;
}

static void root_display_show_heartbeat(void) {
    if (!s_root_display_ready) {
        return;
    }
    oled_display_show_heartbeat(true);
}

static bool root_get_router_status(int8_t *rssi_out) {
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK) {
        if (rssi_out) {
            *rssi_out = ap_info.rssi;
        }
        return true;
    }
    if (rssi_out) {
        *rssi_out = 0;
    }
    return false;
}

static void root_display_show_setup(const char *ap_ssid, const char *pin, const char *ssid_hint, const char *status) {
    if (!s_root_display_ready) {
        return;
    }

    oled_display_set_template(0, "SETUP {status}");
    oled_display_set_template(1, "AP  {ap}");
    oled_display_set_template(2, "PIN {pin}");
    oled_display_set_template(3, "TIP {hint}");

    oled_display_kv_t values[] = {
        {.key = "status", .value = (status && status[0]) ? status : "READY"},
        {.key = "ap", .value = ap_ssid ? ap_ssid : "ROOT_SETUP"},
        {.key = "pin", .value = pin ? pin : "-----"},
        {.key = "hint", .value = (ssid_hint && ssid_hint[0]) ? ssid_hint : "use portal"},
    };
    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);
}

static void root_display_show_setup_request(void)
{
    if (!s_root_display_ready) {
        return;
    }

    oled_display_set_template(0, "SETUP REQUEST");
    oled_display_set_template(1, "Hold>3s done");
    oled_display_set_template(2, "Rebooting...");
    oled_display_set_template(3, "{status}");

    oled_display_kv_t values[] = {
        {.key = "status", .value = "Saving flag"},
    };
    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);
}

static void root_display_show_setup_error(const char *message)
{
    if (!s_root_display_ready) {
        return;
    }

    oled_display_set_template(0, "SETUP ERROR");
    oled_display_set_template(1, "{msg}");
    oled_display_set_template(2, "Check logs");
    oled_display_set_template(3, "Retry later");

    const char *text = (message && message[0]) ? message : "NVS FAIL";
    oled_display_kv_t values[] = {
        {.key = "msg", .value = text},
    };
    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);
}

static void root_display_show_pairing_status(const char *ap_ssid, const char *pin, const char *status)
{
    if (!s_root_display_ready) {
        root_display_init(NULL);
    }

    oled_display_set_template(0, "NODE PAIR MODE");
    oled_display_set_template(1, "AP  {ap}");
    oled_display_set_template(2, "PIN {pin}");
    oled_display_set_template(3, "{status}");

    oled_display_kv_t values[] = {
        {.key = "ap", .value = (ap_ssid && ap_ssid[0]) ? ap_ssid : "ROOT_SETUP"},
        {.key = "pin", .value = (pin && pin[0]) ? pin : "------"},
        {.key = "status", .value = (status && status[0]) ? status : "Waiting..."},
    };
    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);
}

static void node_pairing_generate_mesh_identity(void)
{
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    memcpy(s_pairing_ctx.mesh_id_bytes, mac, 3);

    uint32_t rnd = esp_random();
    uint32_t pin_value = (uint32_t)strtoul(s_pairing_ctx.generated_pin, NULL, 10);

    s_pairing_ctx.mesh_id_bytes[3] = (uint8_t)((rnd & 0xFF) ^ mac[3] ^ (pin_value & 0xFF));
    s_pairing_ctx.mesh_id_bytes[4] = (uint8_t)(((rnd >> 8) & 0xFF) ^ mac[4] ^ ((pin_value >> 3) & 0xFF));
    s_pairing_ctx.mesh_id_bytes[5] = (uint8_t)(((rnd >> 16) & 0xFF) ^ mac[5] ^ ((pin_value >> 6) & 0xFF));

    for (size_t i = 0; i < sizeof(s_pairing_ctx.mesh_id_bytes); ++i) {
        if (s_pairing_ctx.mesh_id_bytes[i] == 0) {
            s_pairing_ctx.mesh_id_bytes[i] = (uint8_t)(mac[i % 6] ?: 1);
        }
    }

    snprintf(s_pairing_ctx.mesh_tag_hex, sizeof(s_pairing_ctx.mesh_tag_hex),
             "%02X%02X%02X%02X%02X%02X",
             s_pairing_ctx.mesh_id_bytes[0], s_pairing_ctx.mesh_id_bytes[1],
             s_pairing_ctx.mesh_id_bytes[2], s_pairing_ctx.mesh_id_bytes[3],
             s_pairing_ctx.mesh_id_bytes[4], s_pairing_ctx.mesh_id_bytes[5]);
    ESP_LOGI(TAG, "Pairing mesh tag: %s", s_pairing_ctx.mesh_tag_hex);
}

static esp_err_t node_pairing_prepare_softap(const char *ssid, const char *password, uint8_t channel)
{
    esp_err_t err = mesh_manager_configure_softap(ssid, password, channel, false, 4);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Pairing SoftAP configured: SSID='%s' channel=%d", ssid, channel);
    }
    return err;
}

static void root_log_config_state(const char *stage)
{
    if (!stage) {
        stage = "UNSPEC";
    }

    memset(s_pairing_ctx.router_ssid, 0, sizeof(s_pairing_ctx.router_ssid));
    memset(s_pairing_ctx.router_password, 0, sizeof(s_pairing_ctx.router_password));
    if (node_config_get_router_credentials(s_pairing_ctx.router_ssid, sizeof(s_pairing_ctx.router_ssid),
                                           s_pairing_ctx.router_password, sizeof(s_pairing_ctx.router_password)) != ESP_OK) {
        strncpy(s_pairing_ctx.router_ssid, MESH_ROUTER_SSID, sizeof(s_pairing_ctx.router_ssid) - 1);
        strncpy(s_pairing_ctx.router_password, MESH_ROUTER_PASSWORD, sizeof(s_pairing_ctx.router_password) - 1);
    }

    root_config_t root_cfg = {0};
    esp_err_t root_err = root_config_get(&root_cfg);
    if (root_err == ESP_OK) {
        ESP_LOGI(TAG,
                 "[CFG:%s] root_config: root_id=%s mesh_id=%s zone=%s(%d) mqtt=%s:%u topic=%s configured=%s",
                 stage,
                 root_cfg.root_node_id,
                 root_cfg.mesh_network_id,
                 root_cfg.zone_name[0] ? root_cfg.zone_name : "-",
                 root_cfg.zone_number,
                 root_cfg.mqtt_host[0] ? root_cfg.mqtt_host : MQTT_BROKER_HOST,
                 root_cfg.mqtt_port ? root_cfg.mqtt_port : MQTT_BROKER_PORT,
                 root_cfg.mqtt_topic_prefix[0] ? root_cfg.mqtt_topic_prefix : "-",
                 root_cfg.is_configured ? "yes" : "no");
    } else {
        ESP_LOGW(TAG, "[CFG:%s] root_config_get failed: %s", stage, esp_err_to_name(root_err));
    }

    char zone_mesh[ZONE_CONFIG_MAX_LEN] = {0};
    char zone_root[ZONE_CONFIG_MAX_LEN] = {0};
    esp_err_t zone_err = zone_config_load(zone_mesh, sizeof(zone_mesh),
                                          zone_root, sizeof(zone_root));

    bool zone_valid = (zone_err == ESP_OK) &&
                      zone_config_validate(zone_mesh) &&
                      zone_config_validate(zone_root);

    if (!zone_valid &&
        root_err == ESP_OK &&
        root_cfg.is_configured &&
        root_cfg.mesh_network_id[0] != '\0' &&
        root_cfg.root_node_id[0] != '\0') {
        ESP_LOGW(TAG,
                 "[CFG:%s] zone_config missing, restoring from root_config (mesh=%s root=%s)",
                 stage,
                 root_cfg.mesh_network_id,
                 root_cfg.root_node_id);
        if (zone_config_save(root_cfg.mesh_network_id, root_cfg.root_node_id) == ESP_OK) {
            strncpy(zone_mesh, root_cfg.mesh_network_id, sizeof(zone_mesh) - 1);
            strncpy(zone_root, root_cfg.root_node_id, sizeof(zone_root) - 1);
            zone_mesh[sizeof(zone_mesh) - 1] = '\0';
            zone_root[sizeof(zone_root) - 1] = '\0';
            zone_err = ESP_OK;
            zone_valid = true;
        } else {
            ESP_LOGE(TAG, "[CFG:%s] failed to restore zone_config", stage);
        }
    }

    const char *zone_mesh_cached = zone_config_get_mesh_id();
    const char *zone_root_cached = zone_config_get_root_id();
    ESP_LOGI(TAG,
             "[CFG:%s] zone_config: load=%s mesh_id=%s root_id=%s (cached mesh=%s root=%s)",
             stage,
             esp_err_to_name(zone_err),
             zone_valid ? zone_mesh : "-",
             zone_valid ? zone_root : "-",
             zone_mesh_cached ? zone_mesh_cached : "(null)",
             zone_root_cached ? zone_root_cached : "(null)");

    char router_ssid[33] = {0};
    char router_pass[65] = {0};
    esp_err_t router_err = node_config_get_router_credentials(router_ssid, sizeof(router_ssid),
                                                              router_pass, sizeof(router_pass));
    ESP_LOGI(TAG,
             "[CFG:%s] node_config: configured=%s router_ssid=%s (len=%d) router_get=%s",
             stage,
             node_config_is_configured() ? "yes" : "no",
             router_err == ESP_OK ? router_ssid : "<none>",
             router_err == ESP_OK ? (int)strlen(router_pass) : -1,
             esp_err_to_name(router_err));
}

static const char* get_node_short_name(const node_info_t *node) {
    if (!node) {
        return "?";
    }
    // Используем node_type если есть, иначе берем часть node_id до первого подчеркивания
    if (node->node_type[0] != '\0') {
        return node->node_type;
    }
    // Ищем первое подчеркивание в node_id
    const char *underscore = strchr(node->node_id, '_');
    if (underscore) {
        static char short_name[16];
        size_t len = underscore - node->node_id;
        if (len > 0 && len < sizeof(short_name)) {
            strncpy(short_name, node->node_id, len);
            short_name[len] = '\0';
            return short_name;
        }
    }
    return node->node_id;
}

static const char* get_msg_type_icon(node_msg_type_t msg_type) {
    switch (msg_type) {
        case NODE_MSG_HEARTBEAT:
            return "H";
        case NODE_MSG_TELEMETRY:
            return "T";
        case NODE_MSG_COMMAND:
            return "C";
        case NODE_MSG_EVENT:
            return "E";
        default:
            return "-";
    }
}

static void root_display_configure_normal(const char *mesh_id) {
    if (!s_root_display_ready) {
        return;
    }

    oled_display_set_template(0, "Mesh {mesh}");
    oled_display_set_template(1, "W:{wifi} R:{router} M:{mqtt}");
    oled_display_set_template(2, "Nodes {nodes}");
    oled_display_set_template(3, "{node3}");
    oled_display_set_template(4, "{node4}");
    oled_display_set_template(5, "{node5}");
    oled_display_set_template(6, "{node6}");
    oled_display_set_template(7, "{node7}");

    oled_display_kv_t values[] = {
        {.key = "mesh", .value = (mesh_id && mesh_id[0]) ? mesh_id : "UNSET"},
        {.key = "wifi", .value = "--"},
        {.key = "nodes", .value = "0"},
        {.key = "router", .value = "WAIT"},
        {.key = "mqtt", .value = "WAIT"},
        {.key = "node3", .value = ""},
        {.key = "node4", .value = ""},
        {.key = "node5", .value = ""},
        {.key = "node6", .value = ""},
        {.key = "node7", .value = ""},
    };
    oled_display_queue_render(values, sizeof(values) / sizeof(values[0]), 0);
}

static bool node_pairing_parse_mac(const char *mac_str, uint8_t out_mac[6])
{
    if (!mac_str || strlen(mac_str) < 17 || !out_mac) {
        return false;
    }

    unsigned int values[6] = {0};
    if (sscanf(mac_str, "%02x:%02x:%02x:%02x:%02x:%02x",
               &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
        return false;
    }

    for (int i = 0; i < 6; ++i) {
        out_mac[i] = (uint8_t)values[i];
    }
    return true;
}

static bool node_pairing_assign_node_id(const char *node_type, const uint8_t mac[6], char *out_id, size_t out_len)
{
    if (!node_type || node_type[0] == '\0' || !mac || !out_id || out_len < 8) {
        return false;
    }

    const char *prefix = node_type;
    char sanitized_prefix[12] = {0};
    size_t len = strlen(node_type);
    size_t idx = 0;
    for (size_t i = 0; i < len && idx < sizeof(sanitized_prefix) - 1; ++i) {
        char c = node_type[i];
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') {
            sanitized_prefix[idx++] = c;
        } else if (c >= 'A' && c <= 'Z') {
            sanitized_prefix[idx++] = (char)(c - 'A' + 'a');
        }
    }
    sanitized_prefix[idx] = '\0';
    if (sanitized_prefix[0] == '\0') {
        strncpy(sanitized_prefix, "node", sizeof(sanitized_prefix) - 1);
    }
    prefix = sanitized_prefix;

    int written = snprintf(out_id, out_len, "%s_%02X%02X%02X",
                           prefix, mac[3], mac[4], mac[5]);
    if (written < 0 || (size_t)written >= out_len) {
        return false;
    }
    return true;
}

static esp_err_t node_pairing_send_write_config(const root_config_t *root_cfg)
{
    if (!root_cfg) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!s_pairing_ctx.discovery_received || s_pairing_ctx.node_pin[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }

    cJSON *params = cJSON_CreateObject();
    if (!params) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(params, "pin", s_pairing_ctx.node_pin);
    cJSON_AddStringToObject(params, "node_id", s_pairing_ctx.assigned_node_id);
    cJSON_AddStringToObject(params, "mesh_network_id", root_cfg->mesh_network_id);
    cJSON_AddStringToObject(params, "mesh_id", root_cfg->mesh_network_id);
    cJSON_AddStringToObject(params, "root_node_id", root_cfg->root_node_id);
    cJSON_AddStringToObject(params, "pairing_mesh_tag", s_pairing_ctx.mesh_tag_hex);

    if (root_cfg->zone_name[0]) {
        cJSON_AddStringToObject(params, "zone", root_cfg->zone_name);
    }
    if (root_cfg->zone_location[0]) {
        cJSON_AddStringToObject(params, "zone_location", root_cfg->zone_location);
    }
    if (root_cfg->zone_number > 0) {
        cJSON_AddNumberToObject(params, "zone_number", root_cfg->zone_number);
    }
    if (s_pairing_ctx.router_ssid[0] && s_pairing_ctx.router_password[0]) {
        cJSON_AddStringToObject(params, "wifi_ssid", s_pairing_ctx.router_ssid);
        cJSON_AddStringToObject(params, "wifi_password", s_pairing_ctx.router_password);
    }

    char command_buf[512];
    if (!mesh_protocol_create_command(
            s_pairing_ctx.assigned_node_id,
            root_cfg->root_node_id,
            root_cfg->mesh_network_id,
            "write_config",
            params,
            command_buf,
            sizeof(command_buf))) {
        ESP_LOGE(TAG, "Pairing: failed to create write_config payload");
        cJSON_Delete(params);
        return ESP_FAIL;
    }
    cJSON_Delete(params);

    ESP_LOGI(TAG, "Pairing: sending write_config to %s", s_pairing_ctx.assigned_node_id);
    const uint8_t *dest_mac = s_pairing_ctx.src_mac[0] ? s_pairing_ctx.src_mac : s_pairing_ctx.node_mac;
    esp_err_t err = mesh_manager_send(dest_mac, (const uint8_t *)command_buf, strlen(command_buf));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pairing: mesh_manager_send failed: %s", esp_err_to_name(err));
    }
    return err;
}

static void node_pairing_mesh_recv_cb(const uint8_t *src_addr, const uint8_t *data, size_t len)
{
    ESP_LOGI(TAG, "🔵 [ROOT_PAIRING] Received mesh data:");
    if (src_addr) {
        ESP_LOGI(TAG, "   From: " MACSTR, MAC2STR(src_addr));
    } else {
        ESP_LOGI(TAG, "   From: NULL");
    }
    ESP_LOGI(TAG, "   Length: %d bytes", len);
    
    if (!data || len == 0) {
        ESP_LOGW(TAG, "   ⚠️ Empty data, ignoring");
        return;
    }

    char *json_copy = calloc(1, len + 1);
    if (!json_copy) {
        ESP_LOGE(TAG, "   ❌ Not enough memory for packet copy");
        return;
    }
    memcpy(json_copy, data, len);
    json_copy[len] = '\0';
    
    ESP_LOGI(TAG, "   Data: %s", json_copy);

    mesh_message_t msg = {0};
    if (!mesh_protocol_parse(json_copy, &msg)) {
        ESP_LOGE(TAG, "Pairing: failed to parse mesh message");
        free(json_copy);
        return;
    }

    cJSON *root = cJSON_Parse(json_copy);
    const char *pin_str = NULL;
    const char *node_type = NULL;
    const char *mac_str = NULL;
    const char *status_str = NULL;
    const char *pairing_tag = NULL;

    if (root) {
        cJSON *pin_item = cJSON_GetObjectItem(root, "pin");
        if (cJSON_IsString(pin_item)) {
            pin_str = pin_item->valuestring;
        }
        cJSON *type_item = cJSON_GetObjectItem(root, "node_type");
        if (cJSON_IsString(type_item)) {
            node_type = type_item->valuestring;
        }
        cJSON *mac_item = cJSON_GetObjectItem(root, "mac_address");
        if (cJSON_IsString(mac_item)) {
            mac_str = mac_item->valuestring;
        }
        cJSON *status_item = cJSON_GetObjectItem(root, "status");
        if (cJSON_IsString(status_item)) {
            status_str = status_item->valuestring;
        }
        cJSON *tag_item = cJSON_GetObjectItem(root, "pairing_mesh_tag");
        if (cJSON_IsString(tag_item)) {
            pairing_tag = tag_item->valuestring;
        }
    }

    switch (msg.type) {
        case MESH_MSG_DISCOVERY:
        case MESH_MSG_HEARTBEAT:
            if (!pin_str || !node_type || !mac_str) {
                ESP_LOGW(TAG, "Pairing: discovery message missing fields");
                break;
            }

            if (!s_pairing_ctx.discovery_received) {
                strncpy(s_pairing_ctx.node_pin, pin_str, sizeof(s_pairing_ctx.node_pin) - 1);
                strncpy(s_pairing_ctx.node_type, node_type, sizeof(s_pairing_ctx.node_type) - 1);
                strncpy(s_pairing_ctx.node_mac_str, mac_str, sizeof(s_pairing_ctx.node_mac_str) - 1);
                if (!node_pairing_parse_mac(mac_str, s_pairing_ctx.node_mac)) {
                    ESP_LOGW(TAG, "Pairing: failed to parse mac %s", mac_str);
                    break;
                }
                if (src_addr) {
                    memcpy(s_pairing_ctx.src_mac, src_addr, sizeof(s_pairing_ctx.src_mac));
                } else {
                    memcpy(s_pairing_ctx.src_mac, s_pairing_ctx.node_mac, sizeof(s_pairing_ctx.src_mac));
                }
                if (pairing_tag && strcmp(pairing_tag, s_pairing_ctx.mesh_tag_hex) != 0) {
                    ESP_LOGW(TAG, "Pairing: node tag mismatch (node=%s root=%s)", pairing_tag, s_pairing_ctx.mesh_tag_hex);
                }
                s_pairing_ctx.discovery_received = true;
                ESP_LOGI(TAG, "Pairing: discovery from %s (pin=%s, type=%s)",
                         s_pairing_ctx.node_mac_str, s_pairing_ctx.node_pin, s_pairing_ctx.node_type);
                root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Node found");
                if (s_pairing_discovery_sem) {
                    xSemaphoreGive(s_pairing_discovery_sem);
                }
            } else if (status_str) {
                root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, status_str);
            }
            break;

        case MESH_MSG_CONFIG_CONFIRMATION:
            if (!pin_str || strcmp(pin_str, s_pairing_ctx.node_pin) != 0) {
                ESP_LOGW(TAG, "Pairing: config confirmation pin mismatch");
                break;
            }
            s_pairing_ctx.confirmation_received = true;
            ESP_LOGI(TAG, "Pairing: config confirmation received for %s", msg.node_id);
            if (msg.node_id[0] != '\0') {
                strncpy(s_pairing_ctx.assigned_node_id, msg.node_id, sizeof(s_pairing_ctx.assigned_node_id) - 1);
            }
            if (s_pairing_confirmation_sem) {
                xSemaphoreGive(s_pairing_confirmation_sem);
            }
            break;

        default:
            break;
    }

    if (root) {
        cJSON_Delete(root);
    }
    mesh_protocol_free_message(&msg);
    free(json_copy);
}

static void root_button_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << ROOT_SETUP_BUTTON_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    int level = gpio_get_level(ROOT_SETUP_BUTTON_PIN);
    ESP_LOGI(TAG, "Setup button configured on GPIO %d (initial level=%d)", (int)ROOT_SETUP_BUTTON_PIN, level);
}

static void handle_setup_button_trigger(void)
{
    if (s_setup_request_pending) {
        return;
    }
    s_setup_request_pending = true;
    ESP_LOGW(TAG, "Долгое нажатие кнопки: запрос режима добавления ноды");

    root_display_show_setup_request();

    esp_err_t err = root_config_request_node_pairing();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось записать флаг pairing в NVS: %s", esp_err_to_name(err));
        root_display_show_setup_error("NVS ERROR");
        s_setup_request_pending = false;
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

static void root_button_task(void *arg)
{
    (void)arg;
    uint32_t press_ms = 0;
    const TickType_t delay_ticks = pdMS_TO_TICKS(ROOT_BUTTON_POLL_MS);
    bool last_pressed = gpio_get_level(ROOT_SETUP_BUTTON_PIN) == ROOT_BUTTON_ACTIVE_LEVEL;
    ESP_LOGI(TAG, "Setup button task started, initial state: %s",
             last_pressed ? "PRESSED" : "released");
    while (1) {
        bool pressed = gpio_get_level(ROOT_SETUP_BUTTON_PIN) == ROOT_BUTTON_ACTIVE_LEVEL;
        if (pressed != last_pressed) {
            ESP_LOGI(TAG, "Setup button %s", pressed ? "PRESSED" : "released");
            last_pressed = pressed;
        }
        if (pressed) {
            if (press_ms < ROOT_BUTTON_LONG_PRESS_MS) {
                press_ms += ROOT_BUTTON_POLL_MS;
            }
            if (press_ms >= ROOT_BUTTON_LONG_PRESS_MS) {
                handle_setup_button_trigger();
            }
        } else {
            press_ms = 0;
        }
        vTaskDelay(delay_ticks);
    }
}

/**
 * @brief Задача мониторинга системы
 * 
 * Периодически логирует статус системы и проверяет таймауты узлов
 */
static void root_monitoring_task(void *arg) {
    uint32_t last_log_ms = 0;
    
    // Регистрация в watchdog
    esp_task_wdt_add(NULL);
    
    ESP_LOGI(TAG, "Monitoring task started");
    
    while (1) {
        // Сброс watchdog
        esp_task_wdt_reset();
        
        uint32_t now_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
        
        // Проверка таймаутов узлов
        node_registry_check_timeouts();

        int registry_nodes = node_registry_get_count();
        int8_t wifi_rssi = 0;
        bool router_connected = root_get_router_status(&wifi_rssi);
        bool mqtt_online = mqtt_client_manager_is_connected();
        root_display_update(wifi_rssi, registry_nodes, s_mesh_id_buffer, router_connected, mqtt_online);
        
        // Логирование статуса каждые 30 секунд
        if (now_ms - last_log_ms > ROOT_MONITORING_INTERVAL_MS) {
            uint32_t free_heap = esp_get_free_heap_size();
            int mesh_nodes = mesh_manager_get_total_nodes();
            bool fallback_active = climate_logic_is_fallback_active();
            
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "=== ROOT NODE STATUS ===");
            ESP_LOGI(TAG, "Free heap: %d bytes", free_heap);
            ESP_LOGI(TAG, "Mesh nodes: %d (total), %d (online)", mesh_nodes, registry_nodes);
            ESP_LOGI(TAG, "MQTT: %s", mqtt_online ? "ONLINE" : "OFFLINE");
            ESP_LOGI(TAG, "Climate fallback: %s", fallback_active ? "ACTIVE" : "INACTIVE");
            ESP_LOGI(TAG, "========================================");
            
            // Отправка discovery сообщения (для регистрации на сервере)
            // Отправляем heartbeat (discovery уже был при запуске)
            mqtt_client_manager_send_heartbeat();
            root_display_show_heartbeat();
            
            // Предупреждение при низкой памяти
            if (free_heap < 50000) {
                ESP_LOGW(TAG, "LOW HEAP MEMORY!");
            }
            
            last_log_ms = now_ms;
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));  // Обновление дисплея каждые 0.5 сек для корректного сброса иконок
    }
}

/**
 * @brief Главная функция приложения
 */
void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(root_config_init());

    if (root_config_take_node_pairing_request()) {
        ESP_LOGW(TAG, "Обнаружен запрос добавления ноды — запускаем pairing режим");
        run_node_pairing_mode();
        return;
    }

    if (root_config_take_setup_request()) {
        ESP_LOGW(TAG, "Обнаружен запрос setup режима — запускаем настройку");
        run_setup_mode();
        return;
    }

    bool zone_configured = root_config_is_zone_configured();
    bool node_ready = node_config_is_configured();

    if (zone_configured && !node_ready) {
        node_config_mark_configured(true);
        node_ready = true;
    }

    if (!zone_configured || !node_ready) {
        run_setup_mode();
        return;
    }

    run_normal_mode();
}

static void run_node_pairing_mode(void)
{
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "=== NODE PAIRING MODE ACTIVATED ===");
    ESP_LOGW(TAG, "========================================");
    root_log_config_state("PAIRING-START");

    memset(&s_pairing_ctx, 0, sizeof(s_pairing_ctx));

    if (s_pairing_discovery_sem) {
        vSemaphoreDelete(s_pairing_discovery_sem);
        s_pairing_discovery_sem = NULL;
    }
    if (s_pairing_confirmation_sem) {
        vSemaphoreDelete(s_pairing_confirmation_sem);
        s_pairing_confirmation_sem = NULL;
    }

    node_config_generate_setup_pin(s_pairing_ctx.generated_pin, sizeof(s_pairing_ctx.generated_pin));
    node_pairing_generate_mesh_identity();
    snprintf(s_pairing_ctx.ap_ssid, sizeof(s_pairing_ctx.ap_ssid),
             NODE_PAIRING_SSID_PREFIX "%s_%s", s_pairing_ctx.mesh_tag_hex, s_pairing_ctx.generated_pin);

    root_display_init(NULL);
    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Init mesh...");

    root_config_t root_cfg = {0};
    if (root_config_get(&root_cfg) != ESP_OK) {
        memset(&root_cfg, 0, sizeof(root_cfg));
    }
    root_log_config_state("PAIRING-ROOTCFG");

    // В pairing режиме ROOT работает только как SoftAP, без подключения к роутеру
    // Это предотвращает конфликты каналов и ошибки аутентификации
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = NODE_PAIRING_SOFTAP_CHANNEL,
        .max_connection = ROOT_MAX_MESH_CONNECTIONS,
        .router_ssid = NULL,  // Отключаем подключение к роутеру в pairing режиме
        .router_password = NULL,
        .router_bssid = NULL,
    };
    memcpy(mesh_config.mesh_id, s_pairing_ctx.mesh_id_bytes, sizeof(mesh_config.mesh_id));
    mesh_config.mesh_id_str = s_pairing_ctx.ap_ssid;

    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Preparing Wi-Fi...");
    mesh_manager_register_recv_cb(node_pairing_mesh_recv_cb);
    esp_err_t err = mesh_manager_init(&mesh_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pairing: mesh_manager_init failed: %s", esp_err_to_name(err));
        root_display_show_setup_error("MESH INIT");
        goto pairing_cleanup;
    }

    if (node_pairing_prepare_softap(s_pairing_ctx.ap_ssid, NODE_PAIRING_SOFTAP_PASS, NODE_PAIRING_SOFTAP_CHANNEL) != ESP_OK) {
        root_display_show_setup_error("AP CONFIG");
        goto pairing_cleanup;
    }

    ESP_LOGI(TAG, "Pairing: waiting %d ms for node discovery before mesh start", NODE_PAIRING_SOFTAP_GRACE_MS);
    char status_buf[32];
    snprintf(status_buf, sizeof(status_buf), "SoftAP %.*s", 6, s_pairing_ctx.mesh_tag_hex);
    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, status_buf);
    vTaskDelay(pdMS_TO_TICKS(NODE_PAIRING_SOFTAP_GRACE_MS));

    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Starting mesh...");
    err = mesh_manager_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Pairing: mesh_manager_start failed: %s", esp_err_to_name(err));
        root_display_show_setup_error("MESH START");
        goto pairing_cleanup;
    }

    s_pairing_discovery_sem = xSemaphoreCreateBinary();
    s_pairing_confirmation_sem = xSemaphoreCreateBinary();
    if (!s_pairing_discovery_sem || !s_pairing_confirmation_sem) {
        root_display_show_setup_error("NO SEMAPHORE");
        goto pairing_cleanup;
    }

    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Waiting node...");
    if (xSemaphoreTake(s_pairing_discovery_sem, pdMS_TO_TICKS(NODE_PAIRING_DISCOVERY_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Pairing: discovery timeout");
        root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Timeout waiting node");
        goto pairing_cleanup;
    }

    if (!node_pairing_assign_node_id(s_pairing_ctx.node_type, s_pairing_ctx.node_mac,
                                     s_pairing_ctx.assigned_node_id, sizeof(s_pairing_ctx.assigned_node_id))) {
        ESP_LOGE(TAG, "Pairing: failed to assign node id");
        root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "ID error");
        goto pairing_cleanup;
    }

    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Configuring node...");
    if (node_pairing_send_write_config(&root_cfg) != ESP_OK) {
        root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Config send fail");
        root_config_request_node_pairing();
        goto pairing_cleanup;
    }

    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Waiting confirm...");
    if (xSemaphoreTake(s_pairing_confirmation_sem, pdMS_TO_TICKS(NODE_PAIRING_CONFIRM_TIMEOUT_MS)) != pdTRUE) {
        ESP_LOGW(TAG, "Pairing: confirmation timeout");
        root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Confirm timeout");
        goto pairing_cleanup;
    }

    root_display_show_pairing_status(s_pairing_ctx.ap_ssid, s_pairing_ctx.generated_pin, "Success! Restart...");
    ESP_LOGI(TAG, "Pairing: node %s configured successfully", s_pairing_ctx.assigned_node_id);
    root_log_config_state("PAIRING-END");
    vTaskDelay(pdMS_TO_TICKS(2000));

pairing_cleanup:
    if (s_pairing_discovery_sem) {
        vSemaphoreDelete(s_pairing_discovery_sem);
        s_pairing_discovery_sem = NULL;
    }
    if (s_pairing_confirmation_sem) {
        vSemaphoreDelete(s_pairing_confirmation_sem);
        s_pairing_confirmation_sem = NULL;
    }
    mesh_manager_stop();
    mesh_manager_register_recv_cb(NULL);

    ESP_LOGI(TAG, "Pairing mode finished, restarting to normal mode");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void run_setup_mode(void) {
    esp_log_level_set(TAG, ESP_LOG_DEBUG);
    ESP_LOGW(TAG, "========================================");
    ESP_LOGW(TAG, "=== ROOT NODE SETUP MODE ACTIVATED ===");
    ESP_LOGW(TAG, "========================================");
    root_log_config_state("SETUP-START");

    memset(&s_credentials, 0, sizeof(s_credentials));
    memset(&s_setup_ip_info, 0, sizeof(s_setup_ip_info));

    node_config_generate_setup_pin(s_current_pin, sizeof(s_current_pin));
    node_config_generate_temp_mesh_id(s_current_pin, s_temp_mesh_id, sizeof(s_temp_mesh_id));

    memset(s_setup_ap_ssid, 0, sizeof(s_setup_ap_ssid));
    snprintf(s_setup_ap_ssid, sizeof(s_setup_ap_ssid), "ROOT_SETUP_%s", s_current_pin);
    const char *ap_password = "hydro2025";

    root_display_init(NULL);
    root_display_show_setup(s_setup_ap_ssid, s_current_pin, "Open 192.168.4.1", "AP READY");

    s_credentials_sem = xSemaphoreCreateBinary();
    if (!s_credentials_sem) {
        ESP_LOGE(TAG, "Не удалось создать семафор для setup портала");
        return;
    }

    setup_portal_config_t portal_cfg = {
        .ap_ssid = s_setup_ap_ssid,
        .ap_password = ap_password,
        .on_credentials = setup_portal_credentials_cb,
        .user_ctx = NULL,
    };

    esp_err_t err = setup_portal_start(&portal_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска setup портала: %s", esp_err_to_name(err));
        vSemaphoreDelete(s_credentials_sem);
        s_credentials_sem = NULL;
        return;
    }

    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "🌐 Данные для подключения:");
    ESP_LOGI(TAG, "  WiFi SSID:    %s", s_setup_ap_ssid);
    ESP_LOGI(TAG, "  WiFi Pass:    %s", ap_password);
    ESP_LOGI(TAG, "  PIN (наклейка на устройстве): %s", s_current_pin);
    ESP_LOGI(TAG, "  Откройте в браузере: http://192.168.4.1");
    ESP_LOGI(TAG, "========================================");

    if (xSemaphoreTake(s_credentials_sem, portMAX_DELAY) != pdTRUE) {
        ESP_LOGE(TAG, "Ожидание WiFi данных прервано");
    }

    setup_portal_stop();
    vSemaphoreDelete(s_credentials_sem);
    s_credentials_sem = NULL;

    if (s_credentials.ssid[0] == '\0') {
        ESP_LOGE(TAG, "WiFi данные не получены. Повторите настройку.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    ESP_LOGI(TAG, "WiFi данные сохранены. SSID='%s'", s_credentials.ssid);
    root_log_config_state("SETUP-CREDS");

    s_config_sem = xSemaphoreCreateBinary();
    if (!s_config_sem) {
        ESP_LOGE(TAG, "Не удалось создать семафор ожидания конфигурации");
    }

    esp_err_t mesh_err = start_setup_mesh(s_temp_mesh_id, s_credentials.ssid, s_credentials.password, &s_setup_ip_info);
    if (mesh_err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось запустить временную mesh сеть: %s", esp_err_to_name(mesh_err));
        root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "MESH FAIL");
    }

    if (start_config_http_server() != ESP_OK) {
        ESP_LOGE(TAG, "Ошибка запуска HTTP сервера конфигурации");
        root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "HTTP FAIL");
    }

    if (mesh_err == ESP_OK) {
        root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "MESH READY");
        if (send_setup_message("discovery", s_current_pin, s_temp_mesh_id, &s_setup_ip_info) == ESP_OK) {
            ESP_LOGI(TAG, "Discovery запрос отправлен. Ожидаем конфигурацию от сервера...");
            root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "DISCOVERY OK");
        } else {
            ESP_LOGE(TAG, "Не удалось отправить discovery запрос на сервер");
            root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "DISCOVERY ERR");
        }

        s_setup_active = true;
        if (s_heartbeat_task_handle == NULL) {
            if (xTaskCreate(setup_heartbeat_task, "setup_heartbeat", 4096, NULL, 4, &s_heartbeat_task_handle) != pdPASS) {
                ESP_LOGE(TAG, "Не удалось создать задачу heartbeat");
                s_heartbeat_task_handle = NULL;
                s_setup_active = false;
            }
        }
    }

    if (s_config_sem) {
        ESP_LOGI(TAG, "Ожидаем конфигурацию (POST /api/config)...");
        root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "WAIT CONFIG");
        xSemaphoreTake(s_config_sem, portMAX_DELAY);
        vSemaphoreDelete(s_config_sem);
        s_config_sem = NULL;
    } else {
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }

    s_setup_active = false;
    if (s_heartbeat_task_handle) {
        while (s_heartbeat_task_handle != NULL) {
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    stop_config_http_server();

    ESP_LOGI(TAG, "Конфигурация получена. Перезагрузка для перехода в рабочий режим...");
    root_display_show_setup(s_setup_ap_ssid, s_current_pin, s_credentials.ssid, "CONFIG OK");
    root_log_config_state("SETUP-DONE");
    if (xTaskCreate(schedule_restart_task, "setup_restart", 2048, NULL, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Не удалось создать задачу перезапуска. Выполняем esp_restart немедленно.");
        esp_restart();
    }

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void run_normal_mode(void) {
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== HYDRO MESH ROOT NODE Starting ===");
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "ESP32-S3 #1 - Mesh Coordinator + MQTT Bridge");
    ESP_LOGI(TAG, "Build date: %s %s", __DATE__, __TIME__);
    
    root_config_t root_cfg = {0};
    if (root_config_get(&root_cfg) != ESP_OK) {
        ESP_LOGW(TAG, "Не удалось загрузить root_config из NVS, используем дефолтные значения");
        root_config_auto_configure(1);
        root_config_get(&root_cfg);
    }

    root_display_init(&root_cfg);
    root_display_configure_normal(s_mesh_id_buffer);

    const char *configured_mqtt_host = root_cfg.mqtt_host[0] ? root_cfg.mqtt_host : MQTT_BROKER_HOST;
    uint16_t configured_mqtt_port = root_cfg.mqtt_port ? root_cfg.mqtt_port : MQTT_BROKER_PORT;
    root_log_config_state("RUN-NORMAL");

    root_button_init();
    if (s_button_task_handle == NULL) {
        if (xTaskCreate(root_button_task, "root_btn", 3072, NULL, 5, &s_button_task_handle) != pdPASS) {
            ESP_LOGW(TAG, "Не удалось создать задачу кнопки setup");
        }
    }

    strncpy(s_mesh_id_buffer, root_cfg.mesh_network_id, sizeof(s_mesh_id_buffer) - 1);
    if (s_mesh_id_buffer[0] == '\0') {
        strncpy(s_mesh_id_buffer, MESH_NETWORK_ID, sizeof(s_mesh_id_buffer) - 1);
    }

    if (node_config_get_router_credentials(s_router_ssid_buffer, sizeof(s_router_ssid_buffer),
                                           s_router_pass_buffer, sizeof(s_router_pass_buffer)) != ESP_OK) {
        ESP_LOGW(TAG, "Router credentials отсутствуют в NVS, используем mesh_config значения");
        strncpy(s_router_ssid_buffer, MESH_ROUTER_SSID, sizeof(s_router_ssid_buffer) - 1);
        strncpy(s_router_pass_buffer, MESH_ROUTER_PASSWORD, sizeof(s_router_pass_buffer) - 1);
    }

    ESP_LOGI(TAG, "[Step 1/6] Initializing Node Registry...");
    ESP_ERROR_CHECK(node_registry_init());
    
    ESP_LOGI(TAG, "[Step 2/6] Initializing Mesh (ROOT mode)...");
    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = ROOT_MAX_MESH_CONNECTIONS,
        .router_ssid = s_router_ssid_buffer,
        .router_password = s_router_pass_buffer,
        .router_bssid = NULL
    };
    mesh_manager_string_to_mesh_id(s_mesh_id_buffer, mesh_config.mesh_id);
    mesh_config.mesh_id_str = s_mesh_id_buffer;
    ESP_ERROR_CHECK(mesh_manager_init(&mesh_config));
    ESP_LOGI(TAG, "Mesh ID: %s, Channel: %d", s_mesh_id_buffer, MESH_NETWORK_CHANNEL);
    
    ESP_LOGI(TAG, "[Step 3/6] Starting Mesh network...");
    ESP_ERROR_CHECK(mesh_manager_start());
    ESP_LOGI(TAG, "Mesh network started");
    
    ESP_LOGI(TAG, "Waiting for IP address from DHCP...");
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info;
    
    int retry_count = 0;
    const int max_retries = 60;
    bool ip_obtained = false;
    
    while (retry_count < max_retries) {
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                ESP_LOGI(TAG, "✓ IP address obtained: " IPSTR, IP2STR(&ip_info.ip));
                ESP_LOGI(TAG, "✓ Netmask: " IPSTR, IP2STR(&ip_info.netmask));
                ESP_LOGI(TAG, "✓ Gateway: " IPSTR, IP2STR(&ip_info.gw));
                ip_obtained = true;
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
        retry_count++;
        if (retry_count % 10 == 0) {
            ESP_LOGW(TAG, "Still waiting for IP... (%d/%d)", retry_count, max_retries);
        }
    }
    
    if (!ip_obtained) {
        ESP_LOGE(TAG, "❌ CRITICAL: Failed to obtain IP address from DHCP!");
        ESP_LOGW(TAG, "Continuing startup, but MQTT will not work...");
    }
    
    ESP_LOGI(TAG, "[Step 4/6] Initializing MQTT Client...");
    if (ip_obtained) {
        ESP_ERROR_CHECK(mqtt_client_manager_init());
        ESP_ERROR_CHECK(mqtt_client_manager_start());
        ESP_LOGI(TAG, "MQTT Client started (connecting to broker...)");
    } else {
        ESP_LOGE(TAG, "Skipping MQTT initialization - no IP address!");
    }
    
    ESP_LOGI(TAG, "[Step 5/6] Initializing Data Router...");
    ESP_ERROR_CHECK(data_router_init());
    ESP_LOGI(TAG, "Data Router initialized");
    
    ESP_LOGI(TAG, "[Step 6/6] Initializing Climate Fallback Logic...");
    ESP_ERROR_CHECK(climate_logic_init());
    ESP_LOGI(TAG, "Climate Fallback Logic initialized");
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "=== ROOT NODE Running ===");
    ESP_LOGI(TAG, "Mesh ID: %s", s_mesh_id_buffer);
    ESP_LOGI(TAG, "MQTT Broker: %s:%u", configured_mqtt_host, configured_mqtt_port);
    ESP_LOGI(TAG, "========================================");

    int8_t initial_rssi = 0;
    bool router_connected = root_get_router_status(&initial_rssi);
    bool mqtt_online = mqtt_client_manager_is_connected();
    root_display_update(initial_rssi, node_registry_get_count(), s_mesh_id_buffer, router_connected, mqtt_online);
    
    if (xTaskCreate(send_config_confirmation_task, "cfg_confirm", 4096, NULL, 4, NULL) != pdPASS) {
        ESP_LOGW(TAG, "Не удалось создать задачу отправки подтверждения конфигурации");
    }

    xTaskCreate(root_monitoring_task, "root_monitor", 8192, NULL, 5, NULL);
    ESP_LOGI(TAG, "All systems operational. ROOT node ready.");
}

static void setup_portal_credentials_cb(const setup_portal_credentials_t *credentials, void *user_ctx) {
    if (!credentials) {
        return;
    }

    ESP_LOGI(TAG, "Получены WiFi данные: SSID='%s'", credentials->ssid);
    memcpy(&s_credentials, credentials, sizeof(s_credentials));

    esp_err_t err = node_config_set_router_credentials(credentials->ssid, credentials->password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить WiFi credentials: %s", esp_err_to_name(err));
    }

    root_display_show_setup(s_setup_ap_ssid, s_current_pin, credentials->ssid, "WIFI STORED");

    node_config_mark_configured(false);

    if (s_credentials_sem) {
        xSemaphoreGive(s_credentials_sem);
    }
}

static esp_err_t start_setup_mesh(const char *mesh_id, const char *ssid, const char *password, esp_netif_ip_info_t *ip_info_out) {
    if (!mesh_id || mesh_id[0] == '\0' || !ssid || ssid[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }

    mesh_manager_config_t mesh_config = {
        .mode = MESH_MODE_ROOT,
        .mesh_password = MESH_NETWORK_PASSWORD,
        .channel = MESH_NETWORK_CHANNEL,
        .max_connection = ROOT_MAX_MESH_CONNECTIONS,
        .router_ssid = ssid,
        .router_password = password,
        .router_bssid = NULL,
    };
    mesh_manager_string_to_mesh_id(mesh_id, mesh_config.mesh_id);
    mesh_config.mesh_id_str = mesh_id;

    ESP_LOGI(TAG, "Запуск временной mesh сети: mesh_id='%s'", mesh_id);

    esp_err_t err = mesh_manager_init(&mesh_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mesh_manager_init error: %s", esp_err_to_name(err));
        return err;
    }

    err = mesh_manager_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mesh_manager_start error: %s", esp_err_to_name(err));
        return err;
    }

    ESP_LOGI(TAG, "Mesh network (setup) started, ожидаем IP...");

    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t ip_info = {0};

    const int max_retries = 60;
    for (int attempt = 0; attempt < max_retries; ++attempt) {
        if (netif && esp_netif_get_ip_info(netif, &ip_info) == ESP_OK && ip_info.ip.addr != 0) {
            ESP_LOGI(TAG, "Setup mesh получил IP: " IPSTR, IP2STR(&ip_info.ip));
            if (ip_info_out) {
                *ip_info_out = ip_info;
            }
            return ESP_OK;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGE(TAG, "Не удалось получить IP адрес для setup mesh");
    return ESP_FAIL;
}

static esp_err_t send_setup_message(const char *type, const char *pin, const char *temp_mesh_id, const esp_netif_ip_info_t *ip_info) {
    if (!type) {
        return ESP_ERR_INVALID_ARG;
    }

    char mac_str[18] = {0};
    uint8_t mac[6] = {0};
    esp_efuse_mac_get_default(mac);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    const esp_app_desc_t *app_desc = esp_app_get_description();
    esp_chip_info_t chip_info = {0};
    esp_chip_info(&chip_info);

    const char *chip_model_str = "ESP32";
#if defined(ESP_CHIP_MODEL_ESP32S2)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32S2) {
        chip_model_str = "ESP32-S2";
    } else if (chip_info.model == ESP_CHIP_MODEL_ESP32S3) {
        chip_model_str = "ESP32-S3";
    } else if (chip_info.model == ESP_CHIP_MODEL_ESP32C3) {
        chip_model_str = "ESP32-C3";
    }
#elif defined(ESP_CHIP_MODEL_ESP32S3)
    if (chip_info.model == ESP_CHIP_MODEL_ESP32S3) {
        chip_model_str = "ESP32-S3";
    }
#endif

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        return ESP_ERR_NO_MEM;
    }

    cJSON_AddStringToObject(root, "type", type);
    cJSON_AddStringToObject(root, "node_type", "root");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    if (pin) {
        cJSON_AddStringToObject(root, "pin", pin);
    }
    if (temp_mesh_id) {
        cJSON_AddStringToObject(root, "temp_mesh_id", temp_mesh_id);
    }

    if (strcmp(type, "discovery") == 0) {
        cJSON_AddStringToObject(root, "firmware_version", app_desc ? app_desc->version : "unknown");
        cJSON_AddStringToObject(root, "chip_model", chip_model_str);
    } else if (strcmp(type, "heartbeat") == 0) {
        cJSON_AddNumberToObject(root, "uptime_ms", esp_timer_get_time() / 1000);
        cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
    }

    if (ip_info) {
        char ip_buf[16];
        snprintf(ip_buf, sizeof(ip_buf), IPSTR, IP2STR(&ip_info->ip));
        cJSON_AddStringToObject(root, "ip_address", ip_buf);
    }

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    if (!payload) {
        return ESP_ERR_NO_MEM;
    }

    esp_http_client_config_t config = {
        .url = SETUP_SERVER_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(payload);
        return ESP_FAIL;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, strlen(payload));

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "Setup message '%s' отправлен. HTTP статус: %d", type, status);
    } else {
        ESP_LOGE(TAG, "Ошибка HTTP при отправке '%s': %s", type, esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    free(payload);
    return err;
}

static void setup_heartbeat_task(void *arg) {
    ESP_LOGI(TAG, "Setup heartbeat task started");
    while (s_setup_active) {
        vTaskDelay(pdMS_TO_TICKS(SETUP_HEARTBEAT_INTERVAL_MS));
        if (!s_setup_active) {
            break;
        }
        if (send_setup_message("heartbeat", s_current_pin, s_temp_mesh_id, &s_setup_ip_info) != ESP_OK) {
            ESP_LOGW(TAG, "Не удалось отправить heartbeat в setup режиме");
        }
    }
    ESP_LOGI(TAG, "Setup heartbeat task finished");
    s_heartbeat_task_handle = NULL;
    vTaskDelete(NULL);
}

static esp_err_t config_post_handler(httpd_req_t *req) {
    int total = req->content_len;
    if (total <= 0 || total > 2048) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Получен POST /api/config (len=%d)", total);

    char *buf = calloc(1, total + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No memory");
        return ESP_FAIL;
    }

    int received = 0;
    while (received < total) {
        int r = httpd_req_recv(req, buf + received, total - received);
        if (r <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Receive failed");
            return ESP_FAIL;
        }
        received += r;
    }

    ESP_LOGD(TAG, "Сырые данные конфигурации: %s", buf);

    ESP_LOGI(TAG, "Setup config payload: %s", buf);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *pin = cJSON_GetObjectItem(root, "pin");
    if (!cJSON_IsString(pin) || strncmp(pin->valuestring, s_current_pin, sizeof(s_current_pin) - 1) != 0) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid PIN");
        return ESP_FAIL;
    }

    const cJSON *node_id = cJSON_GetObjectItem(root, "node_id");
    const cJSON *mesh_id = cJSON_GetObjectItem(root, "mesh_id");
    const cJSON *root_node_id = cJSON_GetObjectItem(root, "root_node_id");
    const cJSON *mqtt_host = cJSON_GetObjectItem(root, "mqtt_broker_host");
    const cJSON *mqtt_port = cJSON_GetObjectItem(root, "mqtt_broker_port");
    const cJSON *wifi_ssid = cJSON_GetObjectItem(root, "wifi_ssid");
    const cJSON *wifi_password = cJSON_GetObjectItem(root, "wifi_password");
    const cJSON *zone = cJSON_GetObjectItem(root, "zone");
    const cJSON *zone_number = cJSON_GetObjectItem(root, "zone_number");
    const cJSON *zone_location = cJSON_GetObjectItem(root, "zone_location");
    const cJSON *mqtt_topic_prefix = cJSON_GetObjectItem(root, "mqtt_topic_prefix");

    if (!cJSON_IsString(node_id) || !node_id->valuestring || node_id->valuestring[0] == '\0' ||
        !cJSON_IsString(mesh_id) || !mesh_id->valuestring || mesh_id->valuestring[0] == '\0' ||
        !cJSON_IsString(wifi_ssid) || !wifi_ssid->valuestring || !cJSON_IsString(wifi_password) || !wifi_password->valuestring) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing required fields");
        return ESP_FAIL;
    }

    const char *root_node_id_str = node_id->valuestring;
    if (cJSON_IsString(root_node_id) && root_node_id->valuestring && root_node_id->valuestring[0] != '\0') {
        root_node_id_str = root_node_id->valuestring;
    }

    const char *mqtt_host_str = MQTT_BROKER_HOST;
    if (cJSON_IsString(mqtt_host) && mqtt_host->valuestring && mqtt_host->valuestring[0] != '\0') {
        mqtt_host_str = mqtt_host->valuestring;
    }

    uint16_t mqtt_port_value = MQTT_BROKER_PORT;
    if (cJSON_IsNumber(mqtt_port) && mqtt_port->valuedouble > 0) {
        mqtt_port_value = (uint16_t)mqtt_port->valuedouble;
    }

    esp_err_t err = node_config_set_router_credentials(wifi_ssid->valuestring, wifi_password->valuestring);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить WiFi данные: %s", esp_err_to_name(err));
    }

    if ((err = node_config_set_root_node_id(root_node_id_str)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist root_node_id: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to store root id");
        goto cleanup;
    }
    if ((err = node_config_set_mesh_network_id(mesh_id->valuestring)) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist mesh_id: %s", esp_err_to_name(err));
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "failed to store mesh id");
        goto cleanup;
    }

    ESP_LOGI(TAG,
             "Конфигурация: node_id='%s', mesh_id='%s', root_node_id='%s', MQTT='%s:%u', zone='%s', zone_number=%d",
             node_id->valuestring,
             mesh_id->valuestring,
             root_node_id_str,
             mqtt_host_str,
             mqtt_port_value,
             (cJSON_IsString(zone) && zone->valuestring) ? zone->valuestring : "-",
             cJSON_IsNumber(zone_number) ? (int)zone_number->valuedouble : -1);
    if (cJSON_IsString(zone_location) && zone_location->valuestring) {
        ESP_LOGI(TAG, "Расположение зоны: %s", zone_location->valuestring);
    }
    ESP_LOGI(TAG, "WiFi: SSID='%s', пароль (%d символов)", wifi_ssid->valuestring,
             wifi_password->valuestring ? (int)strlen(wifi_password->valuestring) : 0);

    root_config_t cfg = {0};
    if (root_config_get(&cfg) != ESP_OK) {
        memset(&cfg, 0, sizeof(cfg));
    }

    strncpy(cfg.root_node_id, root_node_id_str, sizeof(cfg.root_node_id) - 1);
    strncpy(cfg.mesh_network_id, mesh_id->valuestring, sizeof(cfg.mesh_network_id) - 1);
    strncpy(cfg.mqtt_host, mqtt_host_str, sizeof(cfg.mqtt_host) - 1);
    cfg.mqtt_port = mqtt_port_value;
    if (cJSON_IsString(zone) && zone->valuestring) {
        strncpy(cfg.zone_name, zone->valuestring, sizeof(cfg.zone_name) - 1);
    }
    if (cJSON_IsNumber(zone_number)) {
        cfg.zone_number = (uint8_t)zone_number->valuedouble;
    }
    if (cJSON_IsString(zone_location) && zone_location->valuestring) {
        strncpy(cfg.zone_location, zone_location->valuestring, sizeof(cfg.zone_location) - 1);
    }
    if (cJSON_IsString(mqtt_topic_prefix) && mqtt_topic_prefix->valuestring) {
        strncpy(cfg.mqtt_topic_prefix, mqtt_topic_prefix->valuestring, sizeof(cfg.mqtt_topic_prefix) - 1);
    } else if (cfg.zone_number > 0) {
        snprintf(cfg.mqtt_topic_prefix, sizeof(cfg.mqtt_topic_prefix), "hydro/zone%u/", cfg.zone_number);
    }
    cfg.is_configured = true;
    cfg.configured_at = esp_timer_get_time() / 1000000ULL;

    err = root_config_save(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось сохранить root_config: %s", esp_err_to_name(err));
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to persist config");
        return ESP_FAIL;
    }

    err = zone_config_save(mesh_id->valuestring, root_node_id_str);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to persist zone_config: %s", esp_err_to_name(err));
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to store zone config");
        return ESP_FAIL;
    }

    node_config_mark_configured(true);

    root_display_show_setup(s_setup_ap_ssid, s_current_pin, "Config saved", "DONE");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true}");

    cJSON_Delete(root);

    if (s_config_sem) {
        xSemaphoreGive(s_config_sem);
    }

    root_log_config_state("HTTP-CONFIG");

    return ESP_OK;
cleanup:
    cJSON_Delete(root);
    return err;
}

static esp_err_t start_config_http_server(void) {
    if (s_config_http_server) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    esp_err_t err = httpd_start(&s_config_http_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "httpd_start error: %s", esp_err_to_name(err));
        s_config_http_server = NULL;
        return err;
    }

    httpd_uri_t config_uri = {
        .uri = "/api/config",
        .method = HTTP_POST,
        .handler = config_post_handler,
        .user_ctx = NULL,
    };

    httpd_register_uri_handler(s_config_http_server, &config_uri);
    ESP_LOGI(TAG, "HTTP сервер конфигурации запущен на порту %d", config.server_port);
    return ESP_OK;
}

static void stop_config_http_server(void) {
    if (s_config_http_server) {
        httpd_stop(s_config_http_server);
        s_config_http_server = NULL;
        ESP_LOGI(TAG, "HTTP сервер конфигурации остановлен");
    }
}

static void schedule_restart_task(void *arg) {
    ESP_LOGI(TAG, "Перезагрузка через 2 секунды...");
    vTaskDelay(pdMS_TO_TICKS(2000));
    ESP_LOGI(TAG, "Перезагрузка устройства");
    esp_restart();
}

static void send_config_confirmation_task(void *arg) {
    root_config_t cfg = {0};
    if (root_config_get(&cfg) != ESP_OK || !cfg.is_configured || cfg.root_node_id[0] == '\0') {
        vTaskDelete(NULL);
        return;
    }

    char mac_str[18] = {0};
    uint8_t mac[6] = {0};
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    char assigned_root_id[32] = {0};
    if (node_config_get_root_node_id(assigned_root_id) != ESP_OK || assigned_root_id[0] == '\0') {
        strncpy(assigned_root_id, cfg.root_node_id, sizeof(assigned_root_id) - 1);
    }

    const int max_attempts = 60;
    int attempts = 0;
    while (!mqtt_client_manager_is_connected() && attempts < max_attempts) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        attempts++;
    }

    if (!mqtt_client_manager_is_connected()) {
        ESP_LOGW(TAG, "Не удалось подключиться к MQTT для отправки подтверждения конфигурации");
        vTaskDelete(NULL);
        return;
    }

    char topic_prefix[128] = {0};
    if (root_config_get_mqtt_topic_prefix(topic_prefix) != ESP_OK || topic_prefix[0] == '\0') {
        strcpy(topic_prefix, "hydro/setup/");
    }
    size_t prefix_len = strlen(topic_prefix);
    if (prefix_len > 0 && topic_prefix[prefix_len - 1] != '/') {
        if (prefix_len + 1 < sizeof(topic_prefix)) {
            topic_prefix[prefix_len] = '/';
            topic_prefix[prefix_len + 1] = '\0';
        } else {
            topic_prefix[prefix_len - 1] = '/';
            topic_prefix[prefix_len] = '\0';
        }
        prefix_len = strlen(topic_prefix);
    }

    cJSON *root = cJSON_CreateObject();
    if (!root) {
        vTaskDelete(NULL);
        return;
    }

    cJSON_AddStringToObject(root, "type", "config_confirmation");
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    cJSON_AddStringToObject(root, "node_id", cfg.root_node_id);
    cJSON_AddStringToObject(root, "mesh_id", cfg.mesh_network_id);
    cJSON_AddStringToObject(root, "status", "success");
    if (cfg.zone_name[0]) {
        cJSON_AddStringToObject(root, "zone", cfg.zone_name);
    }
    cJSON_AddStringToObject(root, "root_node_id", assigned_root_id);
    cJSON_AddStringToObject(root, "mqtt_topic_prefix", topic_prefix);

    char *payload = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!payload) {
        vTaskDelete(NULL);
        return;
    }

    char discovery_topic[160];
    snprintf(discovery_topic, sizeof(discovery_topic), "%sdiscovery", topic_prefix);

    if (mqtt_client_manager_publish(discovery_topic, payload) == ESP_OK) {
        ESP_LOGI(TAG, "Отправлено подтверждение конфигурации для %s", cfg.root_node_id);
    } else {
        ESP_LOGE(TAG, "Не удалось отправить config_confirmation для %s", cfg.root_node_id);
    }

    free(payload);
    vTaskDelete(NULL);
}
