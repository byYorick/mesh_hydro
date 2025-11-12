# Улучшенная архитектура: Система добавления узлов V2

## Обзор изменений

### Ключевые улучшения:
1. ✅ **Многоуровневая безопасность** - токены, шифрование, аутентификация
2. ✅ **Конечные автоматы состояний** - четкие переходы между режимами
3. ✅ **Обработка всех edge cases** - timeout'ы, retry, rollback
4. ✅ **WiFi Provisioning** - безопасная настройка WiFi через Bluetooth
5. ✅ **Диагностика и мониторинг** - полное логирование всех событий
6. ✅ **UX улучшения** - LED индикация, QR коды, пошаговые инструкции

---

## 1. Безопасная архитектура

### 1.1 Генерация уникальных идентификаторов

**Проблема оригинала:** Фиксированный mesh ID "HYDRO_NEW" для всех ROOT нод

**Решение:**

```c
// common/node_config/node_config.h
typedef struct {
    char setup_token[33];      // 32 символа + \0
    char temp_mesh_id[33];     // HYDRO_SETUP_XXXX
    uint32_t token_timestamp;  // Для истечения токена
} setup_credentials_t;

esp_err_t node_config_generate_setup_credentials(setup_credentials_t *creds);
bool node_config_validate_setup_token(const char *token);
```

**Реализация:**

```c
// common/node_config/node_config.c
#include "esp_random.h"
#include "mbedtls/base64.h"

esp_err_t node_config_generate_setup_credentials(setup_credentials_t *creds) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    
    // Генерация уникального mesh ID из MAC
    snprintf(creds->temp_mesh_id, sizeof(creds->temp_mesh_id),
             "HYDRO_SETUP_%02X%02X%02X", mac[3], mac[4], mac[5]);
    
    // Генерация случайного токена (128 бит)
    uint8_t random_bytes[16];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    
    // Конвертация в base64
    size_t out_len;
    mbedtls_base64_encode((uint8_t*)creds->setup_token, 
                          sizeof(creds->setup_token),
                          &out_len, 
                          random_bytes, 
                          sizeof(random_bytes));
    
    creds->setup_token[32] = '\0'; // Обрезать до 32 символов
    creds->token_timestamp = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    
    // Сохранить в NVS для валидации
    nvs_handle_t handle;
    esp_err_t err = nvs_open("setup_ns", NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;
    
    nvs_set_str(handle, "setup_token", creds->setup_token);
    nvs_set_u32(handle, "token_time", creds->token_timestamp);
    nvs_commit(handle);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "Setup credentials generated:");
    ESP_LOGI(TAG, "  Mesh ID: %s", creds->temp_mesh_id);
    ESP_LOGI(TAG, "  Token: %s", creds->setup_token);
    
    return ESP_OK;
}

bool node_config_validate_setup_token(const char *token) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("setup_ns", NVS_READONLY, &handle);
    if (err != ESP_OK) return false;
    
    char stored_token[33];
    size_t len = sizeof(stored_token);
    err = nvs_get_str(handle, "setup_token", stored_token, &len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return false;
    }
    
    // Проверка истечения токена (24 часа)
    uint32_t token_time;
    err = nvs_get_u32(handle, "token_time", &token_time);
    nvs_close(handle);
    
    if (err == ESP_OK) {
        uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000000ULL);
        if (current_time - token_time > 86400) { // 24 часа
            ESP_LOGW(TAG, "Setup token expired");
            return false;
        }
    }
    
    return strcmp(stored_token, token) == 0;
}
```

### 1.2 Конечный автомат состояний (State Machine)

**Файл: `common/node_config/node_state_machine.h`**

```c
#ifndef NODE_STATE_MACHINE_H
#define NODE_STATE_MACHINE_H

typedef enum {
    NODE_STATE_UNINITIALIZED,     // Первый запуск
    NODE_STATE_SETUP_WAITING,     // Ожидание настройки
    NODE_STATE_SETUP_CONFIGURING, // Получение конфигурации
    NODE_STATE_SETUP_VALIDATING,  // Проверка конфигурации
    NODE_STATE_SETUP_COMPLETE,    // Настройка завершена
    NODE_STATE_OPERATIONAL,       // Нормальная работа
    NODE_STATE_ERROR,             // Ошибка
    NODE_STATE_RECOVERY,          // Восстановление
} node_state_t;

typedef enum {
    STATE_EVENT_INITIALIZED,      // NVS инициализирован
    STATE_EVENT_DISCOVERY_SENT,   // Discovery отправлен
    STATE_EVENT_CONFIG_RECEIVED,  // Конфигурация получена
    STATE_EVENT_CONFIG_VALIDATED, // Конфигурация проверена
    STATE_EVENT_CONFIG_SAVED,     // Конфигурация сохранена
    STATE_EVENT_TIMEOUT,          // Timeout истек
    STATE_EVENT_ERROR,            // Ошибка
    STATE_EVENT_RESET,            // Сброс настроек
} state_event_t;

typedef struct {
    node_state_t current_state;
    node_state_t previous_state;
    uint32_t state_enter_time;
    uint32_t timeout_sec;
    void (*on_state_change)(node_state_t from, node_state_t to);
} state_machine_t;

esp_err_t state_machine_init(state_machine_t *sm);
esp_err_t state_machine_transition(state_machine_t *sm, state_event_t event);
const char* state_machine_get_state_name(node_state_t state);
bool state_machine_check_timeout(state_machine_t *sm);

#endif // NODE_STATE_MACHINE_H
```

**Файл: `common/node_config/node_state_machine.c`**

```c
#include "node_state_machine.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "state_machine";

// Матрица переходов состояний
static const struct {
    node_state_t from;
    state_event_t event;
    node_state_t to;
    uint32_t timeout_sec;
} transitions[] = {
    // Начальные переходы
    {NODE_STATE_UNINITIALIZED, STATE_EVENT_INITIALIZED, NODE_STATE_SETUP_WAITING, 600},
    
    // Процесс настройки
    {NODE_STATE_SETUP_WAITING, STATE_EVENT_CONFIG_RECEIVED, NODE_STATE_SETUP_CONFIGURING, 30},
    {NODE_STATE_SETUP_CONFIGURING, STATE_EVENT_CONFIG_VALIDATED, NODE_STATE_SETUP_VALIDATING, 10},
    {NODE_STATE_SETUP_VALIDATING, STATE_EVENT_CONFIG_SAVED, NODE_STATE_SETUP_COMPLETE, 5},
    {NODE_STATE_SETUP_COMPLETE, STATE_EVENT_INITIALIZED, NODE_STATE_OPERATIONAL, 0},
    
    // Timeout'ы
    {NODE_STATE_SETUP_WAITING, STATE_EVENT_TIMEOUT, NODE_STATE_ERROR, 0},
    {NODE_STATE_SETUP_CONFIGURING, STATE_EVENT_TIMEOUT, NODE_STATE_ERROR, 0},
    
    // Обработка ошибок
    {NODE_STATE_ERROR, STATE_EVENT_RESET, NODE_STATE_RECOVERY, 10},
    {NODE_STATE_RECOVERY, STATE_EVENT_INITIALIZED, NODE_STATE_SETUP_WAITING, 600},
    
    // Любое состояние → ошибка
    {NODE_STATE_SETUP_WAITING, STATE_EVENT_ERROR, NODE_STATE_ERROR, 0},
    {NODE_STATE_SETUP_CONFIGURING, STATE_EVENT_ERROR, NODE_STATE_ERROR, 0},
    {NODE_STATE_SETUP_VALIDATING, STATE_EVENT_ERROR, NODE_STATE_ERROR, 0},
};

const char* state_machine_get_state_name(node_state_t state) {
    switch (state) {
        case NODE_STATE_UNINITIALIZED: return "UNINITIALIZED";
        case NODE_STATE_SETUP_WAITING: return "SETUP_WAITING";
        case NODE_STATE_SETUP_CONFIGURING: return "SETUP_CONFIGURING";
        case NODE_STATE_SETUP_VALIDATING: return "SETUP_VALIDATING";
        case NODE_STATE_SETUP_COMPLETE: return "SETUP_COMPLETE";
        case NODE_STATE_OPERATIONAL: return "OPERATIONAL";
        case NODE_STATE_ERROR: return "ERROR";
        case NODE_STATE_RECOVERY: return "RECOVERY";
        default: return "UNKNOWN";
    }
}

esp_err_t state_machine_init(state_machine_t *sm) {
    sm->current_state = NODE_STATE_UNINITIALIZED;
    sm->previous_state = NODE_STATE_UNINITIALIZED;
    sm->state_enter_time = 0;
    sm->timeout_sec = 0;
    sm->on_state_change = NULL;
    return ESP_OK;
}

esp_err_t state_machine_transition(state_machine_t *sm, state_event_t event) {
    // Найти соответствующий переход
    node_state_t new_state = sm->current_state;
    uint32_t new_timeout = 0;
    bool found = false;
    
    for (int i = 0; i < sizeof(transitions) / sizeof(transitions[0]); i++) {
        if (transitions[i].from == sm->current_state && 
            transitions[i].event == event) {
            new_state = transitions[i].to;
            new_timeout = transitions[i].timeout_sec;
            found = true;
            break;
        }
    }
    
    if (!found) {
        ESP_LOGW(TAG, "No transition found from %s on event %d",
                 state_machine_get_state_name(sm->current_state), event);
        return ESP_ERR_NOT_FOUND;
    }
    
    // Выполнить переход
    ESP_LOGI(TAG, "State transition: %s -> %s (timeout: %lu sec)",
             state_machine_get_state_name(sm->current_state),
             state_machine_get_state_name(new_state),
             new_timeout);
    
    sm->previous_state = sm->current_state;
    sm->current_state = new_state;
    sm->state_enter_time = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    sm->timeout_sec = new_timeout;
    
    // Callback
    if (sm->on_state_change) {
        sm->on_state_change(sm->previous_state, sm->current_state);
    }
    
    return ESP_OK;
}

bool state_machine_check_timeout(state_machine_t *sm) {
    if (sm->timeout_sec == 0) return false;
    
    uint32_t current_time = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t elapsed = current_time - sm->state_enter_time;
    
    if (elapsed >= sm->timeout_sec) {
        ESP_LOGW(TAG, "State timeout in %s after %lu seconds",
                 state_machine_get_state_name(sm->current_state), elapsed);
        return true;
    }
    
    return false;
}
```

### 1.3 WiFi Provisioning через Bluetooth

**Проблема:** Hardcoded WiFi credentials в коде

**Решение:** Использовать ESP Unified Provisioning

**Файл: `root_node/components/wifi_provisioning/wifi_prov.h`**

```c
#ifndef WIFI_PROV_H
#define WIFI_PROV_H

#include "esp_err.h"

typedef void (*wifi_prov_callback_t)(void);

typedef struct {
    const char *service_name;     // BLE имя сервиса
    const char *pop;              // Proof of Possession (пароль)
    wifi_prov_callback_t on_success;
    wifi_prov_callback_t on_fail;
} wifi_prov_config_t;

esp_err_t wifi_prov_start(const wifi_prov_config_t *config);
esp_err_t wifi_prov_stop(void);
bool wifi_prov_is_provisioned(void);

#endif // WIFI_PROV_H
```

**Файл: `root_node/components/wifi_provisioning/wifi_prov.c`**

```c
#include "wifi_prov.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_ble.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_prov";

static void prov_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    wifi_prov_config_t *config = (wifi_prov_config_t *)arg;
    
    switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
            
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received WiFi credentials: SSID=%s", wifi_cfg->ssid);
            break;
        }
        
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            if (config && config->on_success) {
                config->on_success();
            }
            break;
            
        case WIFI_PROV_CRED_FAIL:
            ESP_LOGE(TAG, "Provisioning failed");
            if (config && config->on_fail) {
                config->on_fail();
            }
            break;
            
        case WIFI_PROV_END:
            ESP_LOGI(TAG, "Provisioning ended");
            wifi_prov_mgr_deinit();
            break;
            
        default:
            break;
    }
}

bool wifi_prov_is_provisioned(void) {
    bool provisioned = false;
    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    return provisioned;
}

esp_err_t wifi_prov_start(const wifi_prov_config_t *config) {
    ESP_LOGI(TAG, "Starting WiFi provisioning via BLE");
    
    // Инициализация provisioning manager
    wifi_prov_mgr_config_t prov_config = {
        .scheme = wifi_prov_scheme_ble,
        .scheme_event_handler = WIFI_PROV_SCHEME_BLE_EVENT_HANDLER_FREE_BTDM
    };
    
    ESP_ERROR_CHECK(wifi_prov_mgr_init(prov_config));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID,
                                                prov_event_handler, (void*)config));
    
    // Генерация service name из MAC
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char service_name[32];
    snprintf(service_name, sizeof(service_name), "HYDRO_%02X%02X%02X",
             mac[3], mac[4], mac[5]);
    
    // Генерация PoP (Proof of Possession) из токена
    const char *pop = config->pop ? config->pop : "hydro2025";
    
    ESP_LOGI(TAG, "BLE Service Name: %s", service_name);
    ESP_LOGI(TAG, "PoP (password): %s", pop);
    
    // Запуск provisioning
    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(
        WIFI_PROV_SECURITY_1, pop, service_name, NULL));
    
    return ESP_OK;
}

esp_err_t wifi_prov_stop(void) {
    wifi_prov_mgr_deinit();
    return ESP_OK;
}
```

---

## 2. ROOT Node - Улучшенная логика

### 2.1 Главная логика с State Machine

**Файл: `root_node/main/app_main.c`**

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "node_config.h"
#include "node_state_machine.h"
#include "wifi_prov.h"
#include "mesh_manager.h"
#include "esp_http_client.h"
#include "cJSON.h"

static const char *TAG = "root_main";

// Глобальные переменные
static state_machine_t s_state_machine;
static setup_credentials_t s_setup_creds;
static root_node_config_t s_config;
static TaskHandle_t s_heartbeat_task = NULL;
static TaskHandle_t s_discovery_task = NULL;

// LED индикация
typedef enum {
    LED_PATTERN_OFF,
    LED_PATTERN_SLOW_BLINK,     // 1 Hz - ожидание WiFi
    LED_PATTERN_FAST_BLINK,     // 4 Hz - ожидание настройки
    LED_PATTERN_SOLID,          // Постоянно - настроен
    LED_PATTERN_ERROR,          // SOS паттерн
} led_pattern_t;

void led_set_pattern(led_pattern_t pattern);

// ==================== LED ИНДИКАЦИЯ ====================

static void led_task(void *pvParameters) {
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    
    while (1) {
        led_pattern_t pattern = *(led_pattern_t*)pvParameters;
        
        switch (pattern) {
            case LED_PATTERN_OFF:
                gpio_set_level(GPIO_NUM_2, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
                
            case LED_PATTERN_SLOW_BLINK:
                gpio_set_level(GPIO_NUM_2, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(GPIO_NUM_2, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
                
            case LED_PATTERN_FAST_BLINK:
                gpio_set_level(GPIO_NUM_2, 1);
                vTaskDelay(pdMS_TO_TICKS(125));
                gpio_set_level(GPIO_NUM_2, 0);
                vTaskDelay(pdMS_TO_TICKS(125));
                break;
                
            case LED_PATTERN_SOLID:
                gpio_set_level(GPIO_NUM_2, 1);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
                
            case LED_PATTERN_ERROR:
                // SOS: ... --- ...
                for (int i = 0; i < 3; i++) {
                    gpio_set_level(GPIO_NUM_2, 1);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    gpio_set_level(GPIO_NUM_2, 0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                for (int i = 0; i < 3; i++) {
                    gpio_set_level(GPIO_NUM_2, 1);
                    vTaskDelay(pdMS_TO_TICKS(300));
                    gpio_set_level(GPIO_NUM_2, 0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                vTaskDelay(pdMS_TO_TICKS(200));
                for (int i = 0; i < 3; i++) {
                    gpio_set_level(GPIO_NUM_2, 1);
                    vTaskDelay(pdMS_TO_TICKS(100));
                    gpio_set_level(GPIO_NUM_2, 0);
                    vTaskDelay(pdMS_TO_TICKS(100));
                }
                vTaskDelay(pdMS_TO_TICKS(2000));
                break;
        }
    }
}

static led_pattern_t s_led_pattern = LED_PATTERN_OFF;

void led_set_pattern(led_pattern_t pattern) {
    s_led_pattern = pattern;
}

// ==================== HTTP DISCOVERY ====================

static esp_err_t send_discovery_http(const char *server_url, const setup_credentials_t *creds) {
    ESP_LOGI(TAG, "Sending discovery to %s", server_url);
    
    // Получить MAC адрес
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Создать JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mac_address", mac_str);
    cJSON_AddStringToObject(root, "node_type", "root");
    cJSON_AddStringToObject(root, "chip_model", CONFIG_IDF_TARGET);
    cJSON_AddStringToObject(root, "firmware_version", "2.0.0");
    cJSON_AddNumberToObject(root, "free_heap", esp_get_free_heap_size());
    cJSON_AddStringToObject(root, "temp_mesh_id", creds->temp_mesh_id);
    cJSON_AddStringToObject(root, "setup_token", creds->setup_token);
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    // HTTP POST
    esp_http_client_config_t config = {
        .url = server_url,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_str, strlen(json_str));
    
    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    
    esp_http_client_cleanup(client);
    free(json_str);
    
    if (err == ESP_OK && status_code == 200) {
        ESP_LOGI(TAG, "Discovery sent successfully");
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Discovery failed: err=%d, status=%d", err, status_code);
        return ESP_FAIL;
    }
}

// ==================== DISCOVERY TASK С RETRY ====================

static void discovery_task(void *pvParameters) {
    const char *server_url = (const char *)pvParameters;
    int retry_count = 0;
    const int max_retries = 20; // 10 минут с интервалом 30 сек
    
    while (retry_count < max_retries) {
        if (s_state_machine.current_state != NODE_STATE_SETUP_WAITING) {
            ESP_LOGI(TAG, "Discovery task stopping - state changed");
            break;
        }
        
        esp_err_t err = send_discovery_http(server_url, &s_setup_creds);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Discovery successful, waiting for configuration...");
            // Продолжать отправлять heartbeat
            vTaskDelay(pdMS_TO_TICKS(30000)); // Каждые 30 секунд
        } else {
            retry_count++;
            ESP_LOGW(TAG, "Discovery retry %d/%d", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }
    
    if (retry_count >= max_retries) {
        ESP_LOGE(TAG, "Discovery failed after all retries");
        state_machine_transition(&s_state_machine, STATE_EVENT_ERROR);
    }
    
    s_discovery_task = NULL;
    vTaskDelete(NULL);
}

// ==================== ОБРАБОТКА КОНФИГУРАЦИИ ====================

static esp_err_t validate_config(cJSON *config_json) {
    // Проверка токена
    cJSON *token_obj = cJSON_GetObjectItem(config_json, "setup_token");
    if (!token_obj || !cJSON_IsString(token_obj)) {
        ESP_LOGE(TAG, "Missing setup_token");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!node_config_validate_setup_token(token_obj->valuestring)) {
        ESP_LOGE(TAG, "Invalid setup_token");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Проверка обязательных полей
    const char *required_fields[] = {
        "node_id", "mesh_id", "zone", "mqtt_broker_host", "mqtt_broker_port"
    };
    
    for (int i = 0; i < sizeof(required_fields) / sizeof(required_fields[0]); i++) {
        if (!cJSON_HasObjectItem(config_json, required_fields[i])) {
            ESP_LOGE(TAG, "Missing required field: %s", required_fields[i]);
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // Проверка длины строк
    cJSON *node_id = cJSON_GetObjectItem(config_json, "node_id");
    if (strlen(node_id->valuestring) > 31 || strlen(node_id->valuestring) == 0) {
        ESP_LOGE(TAG, "node_id length invalid");
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

static esp_err_t apply_config(cJSON *config_json) {
    ESP_LOGI(TAG, "Applying configuration...");
    
    // Сначала проверить
    state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_RECEIVED);
    
    esp_err_t err = validate_config(config_json);
    if (err != ESP_OK) {
        state_machine_transition(&s_state_machine, STATE_EVENT_ERROR);
        return err;
    }
    
    state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_VALIDATED);
    
    // Извлечь данные
    strncpy(s_config.base.node_id,
            cJSON_GetObjectItem(config_json, "node_id")->valuestring,
            sizeof(s_config.base.node_id) - 1);
    
    strncpy(s_config.base.zone,
            cJSON_GetObjectItem(config_json, "zone")->valuestring,
            sizeof(s_config.base.zone) - 1);
    
    node_config_set_mesh_network_id(
        cJSON_GetObjectItem(config_json, "mesh_id")->valuestring);
    
    strncpy(s_config.mqtt_broker_host,
            cJSON_GetObjectItem(config_json, "mqtt_broker_host")->valuestring,
            sizeof(s_config.mqtt_broker_host) - 1);
    
    s_config.mqtt_broker_port =
        cJSON_GetObjectItem(config_json, "mqtt_broker_port")->valueint;
    
    // Сохранить в NVS
    err = node_config_save(&s_config, sizeof(s_config), "root_ns");
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save config to NVS");
        state_machine_transition(&s_state_machine, STATE_EVENT_ERROR);
        return err;
    }
    
    state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_SAVED);
    
    ESP_LOGI(TAG, "Configuration saved successfully");
    ESP_LOGI(TAG, "  node_id: %s", s_config.base.node_id);
    ESP_LOGI(TAG, "  zone: %s", s_config.base.zone);
    ESP_LOGI(TAG, "  mqtt: %s:%d", s_config.mqtt_broker_host, s_config.mqtt_broker_port);
    
    return ESP_OK;
}

static void handle_write_config(cJSON *params) {
    esp_err_t err = apply_config(params);
    
    // Отправить подтверждение
    cJSON *response = cJSON_CreateObject();
    cJSON_AddStringToObject(response, "type", "config_confirmation");
    
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    cJSON_AddStringToObject(response, "mac_address", mac_str);
    
    if (err == ESP_OK) {
        cJSON_AddStringToObject(response, "status", "success");
        cJSON_AddStringToObject(response, "node_id", s_config.base.node_id);
    } else {
        cJSON_AddStringToObject(response, "status", "failed");
        cJSON_AddStringToObject(response, "error", esp_err_to_name(err));
    }
    
    char *json_str = cJSON_PrintUnformatted(response);
    
    // Отправить через HTTP
    char url[128];
    snprintf(url, sizeof(url), "http://%s/api/config-confirmation",
             DEFAULT_SERVER_IP); // Нужно получить из WiFi provisioning
    
    // TODO: отправка подтверждения
    
    free(json_str);
    cJSON_Delete(response);
    
    if (err == ESP_OK) {
        // Перезагрузка через 3 секунды
        ESP_LOGI(TAG, "Rebooting in 3 seconds...");
        led_set_pattern(LED_PATTERN_SOLID);
        vTaskDelay(pdMS_TO_TICKS(3000));
        esp_restart();
    }
}

// ==================== CALLBACK STATE MACHINE ====================

static void on_state_change(node_state_t from, node_state_t to) {
    ESP_LOGI(TAG, "State changed: %s -> %s",
             state_machine_get_state_name(from),
             state_machine_get_state_name(to));
    
    switch (to) {
        case NODE_STATE_SETUP_WAITING:
            led_set_pattern(LED_PATTERN_FAST_BLINK);
            break;
            
        case NODE_STATE_SETUP_CONFIGURING:
            led_set_pattern(LED_PATTERN_FAST_BLINK);
            break;
            
        case NODE_STATE_OPERATIONAL:
            led_set_pattern(LED_PATTERN_SOLID);
            break;
            
        case NODE_STATE_ERROR:
            led_set_pattern(LED_PATTERN_ERROR);
            break;
            
        default:
            break;
    }
}

// ==================== MAIN ====================

void app_main(void) {
    ESP_LOGI(TAG, "ROOT Node starting (v2.0)...");
    
    // Инициализация NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Инициализация state machine
    state_machine_init(&s_state_machine);
    s_state_machine.on_state_change = on_state_change;
    
    // Запуск LED индикации
    xTaskCreate(led_task, "led", 2048, &s_led_pattern, 5, NULL);
    
    // Проверка режима
    bool is_configured = node_config_is_configured();
    
    if (!is_configured) {
        ESP_LOGW(TAG, "=== ROOT NODE IN SETUP MODE ===");
        
        led_set_pattern(LED_PATTERN_SLOW_BLINK);
        state_machine_transition(&s_state_machine, STATE_EVENT_INITIALIZED);
        
        // Генерация setup credentials
        node_config_generate_setup_credentials(&s_setup_creds);
        
        // WiFi Provisioning через BLE
        wifi_prov_config_t prov_config = {
            .service_name = NULL, // Автогенерация
            .pop = s_setup_creds.setup_token, // Использовать токен как пароль
            .on_success = NULL,
            .on_fail = NULL,
        };
        
        wifi_prov_start(&prov_config);
        
        // Ждать WiFi подключения
        // TODO: добавить обработчик события WiFi connected
        
        // Запустить discovery task
        char server_url[128];
        snprintf(server_url, sizeof(server_url),
                 "http://%s/api/discovery/root", DEFAULT_SERVER_IP);
        
        xTaskCreate(discovery_task, "discovery", 8192, server_url, 5, &s_discovery_task);
        
        // Мониторинг timeout
        while (1) {
            if (state_machine_check_timeout(&s_state_machine)) {
                state_machine_transition(&s_state_machine, STATE_EVENT_TIMEOUT);
            }
            
            if (s_state_machine.current_state == NODE_STATE_ERROR) {
                ESP_LOGE(TAG, "Setup failed, restarting...");
                vTaskDelay(pdMS_TO_TICKS(5000));
                esp_restart();
            }
            
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        
    } else {
        ESP_LOGI(TAG, "=== ROOT NODE CONFIGURED - NORMAL OPERATION ===");
        
        state_machine_transition(&s_state_machine, STATE_EVENT_INITIALIZED);
        state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_VALIDATED);
        state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_SAVED);
        
        // Загрузить конфигурацию
        err = node_config_load(&s_config, sizeof(s_config), "root_ns");
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to load config, erasing...");
            node_config_erase("root_ns");
            esp_restart();
        }
        
        ESP_LOGI(TAG, "Loaded config: node_id=%s, zone=%s",
                 s_config.base.node_id, s_config.base.zone);
        
        state_machine_transition(&s_state_machine, STATE_EVENT_INITIALIZED);
        
        // Нормальная работа
        // TODO: запуск MQTT, mesh, etc.
    }
}
```

---

## 3. Backend - Улучшенная архитектура

### 3.1 Расширенная таблица с состояниями

**Файл: `server/backend/database/migrations/2025_01_XX_create_new_nodes_table.php`**

```php
<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::create('new_nodes', function (Blueprint $table) {
            $table->id();
            $table->string('mac_address', 17)->unique();
            $table->string('node_type');
            $table->boolean('is_root')->default(false);
            
            // Setup credentials
            $table->string('setup_token', 32)->nullable();
            $table->string('temp_mesh_id', 32)->nullable();
            $table->timestamp('token_expires_at')->nullable();
            
            // Состояние
            $table->enum('status', [
                'discovered',
                'configuring',
                'waiting_confirmation',
                'confirmed',
                'failed',
                'timeout'
            ])->default('discovered');
            
            // Метаданные
            $table->jsonb('metadata');
            
            // Временные метки
            $table->timestamp('discovered_at');
            $table->timestamp('last_heartbeat_at')->nullable();
            $table->timestamp('configured_at')->nullable();
            $table->timestamps();
            
            // Индексы
            $table->index('mac_address');
            $table->index('node_type');
            $table->index('is_root');
            $table->index('status');
            $table->index('last_heartbeat_at');
            $table->index('setup_token');
        });
        
        // Таблица логов setup процесса
        Schema::create('setup_logs', function (Blueprint $table) {
            $table->id();
            $table->string('mac_address', 17);
            $table->enum('event', [
                'discovered',
                'heartbeat',
                'config_sent',
                'config_confirmed',
                'config_failed',
                'moved_to_nodes',
                'deleted',
                'timeout'
            ]);
            $table->jsonb('details')->nullable();
            $table->timestamp('created_at');
            
            $table->index('mac_address');
            $table->index('event');
            $table->index('created_at');
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('setup_logs');
        Schema::dropIfExists('new_nodes');
    }
};
```

### 3.2 Улучшенная модель NewNode

**Файл: `server/backend/app/Models/NewNode.php`**

```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use App\Services\MqttService;
use App\Events\NewNodeDiscovered;
use App\Events\NodeConfigured;
use Illuminate\Support\Facades\Log;

class NewNode extends Model
{
    protected $fillable = [
        'mac_address',
        'node_type',
        'is_root',
        'setup_token',
        'temp_mesh_id',
        'token_expires_at',
        'status',
        'metadata',
        'discovered_at',
        'last_heartbeat_at',
        'configured_at',
    ];
    
    protected $casts = [
        'metadata' => 'array',
        'is_root' => 'boolean',
        'discovered_at' => 'datetime',
        'last_heartbeat_at' => 'datetime',
        'configured_at' => 'datetime',
        'token_expires_at' => 'datetime',
    ];
    
    /**
     * Проверка online статуса (heartbeat < 30 сек назад)
     */
    public function isOnline(): bool
    {
        if (!$this->last_heartbeat_at) {
            return false;
        }
        return $this->last_heartbeat_at->diffInSeconds(now()) < 30;
    }
    
    /**
     * Проверка валидности токена
     */
    public function isTokenValid(): bool
    {
        if (!$this->token_expires_at) {
            return false;
        }
        return now()->lt($this->token_expires_at);
    }
    
    /**
     * Отправка конфигурации на ноду
     */
    public function configure(array $config): bool
    {
        // Проверка валидности токена
        if (!$this->isTokenValid()) {
            $this->logEvent('config_failed', ['reason' => 'token_expired']);
            return false;
        }
        
        // Проверка online статуса
        if (!$this->isOnline()) {
            $this->logEvent('config_failed', ['reason' => 'node_offline']);
            return false;
        }
        
        // Добавить setup_token в конфигурацию для валидации
        $config['setup_token'] = $this->setup_token;
        
        // Обновить статус
        $this->update(['status' => 'configuring']);
        $this->logEvent('config_sent', $config);
        
        try {
            if ($this->is_root) {
                // Для ROOT отправить через HTTP
                $this->sendConfigViaHttp($config);
            } else {
                // Для обычных нод через MQTT
                app(MqttService::class)->sendCommand(
                    $this->mac_address,
                    'write_config',
                    $config
                );
            }
            
            $this->update(['status' => 'waiting_confirmation']);
            return true;
            
        } catch (\Exception $e) {
            Log::error("Failed to send config to {$this->mac_address}: " . $e->getMessage());
            $this->update(['status' => 'failed']);
            $this->logEvent('config_failed', ['error' => $e->getMessage()]);
            return false;
        }
    }
    
    /**
     * Отправка конфигурации ROOT через HTTP
     */
    protected function sendConfigViaHttp(array $config): void
    {
        // TODO: Реализовать HTTP отправку
        // В режиме "setup" ROOT слушает на HTTP endpoint
        // Нужно сохранить IP адрес ROOT при discovery
        
        if (!isset($this->metadata['ip_address'])) {
            throw new \Exception('ROOT IP address not available');
        }
        
        $url = "http://{$this->metadata['ip_address']}/api/config";
        
        // HTTP POST
        $ch = curl_init($url);
        curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, json_encode($config));
        curl_setopt($ch, CURLOPT_HTTPHEADER, ['Content-Type: application/json']);
        curl_setopt($ch, CURLOPT_TIMEOUT, 5);
        
        $response = curl_exec($ch);
        $httpCode = curl_getinfo($ch, CURLINFO_HTTP_CODE);
        curl_close($ch);
        
        if ($httpCode !== 200) {
            throw new \Exception("HTTP request failed with code {$httpCode}");
        }
    }
    
    /**
     * Подтверждение конфигурации от ноды
     */
    public function confirmConfiguration(string $nodeId): Node
    {
        $this->update([
            'status' => 'confirmed',
            'configured_at' => now(),
        ]);
        
        $this->logEvent('config_confirmed', ['node_id' => $nodeId]);
        
        // Переместить в основную таблицу nodes
        $node = $this->moveToNodes($nodeId);
        
        // Удалить setup_token из NVS (уже не нужен)
        // TODO: отправить команду на очистку
        
        return $node;
    }
    
    /**
     * Перемещение в основную таблицу nodes
     */
    public function moveToNodes(string $nodeId): Node
    {
        $node = Node::create([
            'node_id' => $nodeId,
            'node_type' => $this->node_type,
            'mac_address' => $this->mac_address,
            'online' => $this->isOnline(),
            'config' => [],
            'metadata' => $this->metadata,
        ]);
        
        $this->logEvent('moved_to_nodes', ['node_id' => $nodeId]);
        
        // Удалить из new_nodes
        $this->delete();
        
        // Событие websocket
        broadcast(new NodeConfigured($node));
        
        return $node;
    }
    
    /**
     * Логирование события
     */
    public function logEvent(string $event, ?array $details = null): void
    {
        \DB::table('setup_logs')->insert([
            'mac_address' => $this->mac_address,
            'event' => $event,
            'details' => $details ? json_encode($details) : null,
            'created_at' => now(),
        ]);
    }
    
    /**
     * Обновление heartbeat
     */
    public function updateHeartbeat(): void
    {
        $this->update(['last_heartbeat_at' => now()]);
        $this->logEvent('heartbeat');
    }
    
    /**
     * Проверка timeout (10 минут без heartbeat)
     */
    public function checkTimeout(): bool
    {
        if (!$this->last_heartbeat_at) {
            return false;
        }
        
        $timeoutMinutes = 10;
        if ($this->last_heartbeat_at->diffInMinutes(now()) > $timeoutMinutes) {
            $this->update(['status' => 'timeout']);
            $this->logEvent('timeout');
            return true;
        }
        
        return false;
    }
}
```

---

## 4. Frontend - Улучшенный UX

### 4.1 Store с реактивными обновлениями

**Файл: `server/frontend/src/stores/newNodes.ts`**

```typescript
import { defineStore } from 'pinia'
import api from '@/services/api'
import { useWebSocket } from '@/composables/useWebSocket'

export interface NewNode {
  id: number
  mac_address: string
  node_type: string
  is_root: boolean
  setup_token: string
  temp_mesh_id: string
  status: 'discovered' | 'configuring' | 'waiting_confirmation' | 'confirmed' | 'failed' | 'timeout'
  metadata: Record<string, any>
  discovered_at: string
  last_heartbeat_at?: string
  configured_at?: string
  is_online?: boolean
  token_expires_at?: string
}

export const useNewNodesStore = defineStore('newNodes', {
  state: () => ({
    newNodes: [] as NewNode[],
    loading: false,
    error: null as string | null,
  }),
  
  actions: {
    async init() {
      await this.fetchNewNodes()
      this.subscribeToWebSocket()
    },
    
    async fetchNewNodes() {
      this.loading = true
      this.error = null
      try {
        const response = await api.get('/new-nodes')
        this.newNodes = response.data
      } catch (error: any) {
        this.error = error.message
        console.error('Failed to fetch new nodes:', error)
      } finally {
        this.loading = false
      }
    },
    
    async configureNode(mac: string, config: any) {
      try {
        await api.post(`/new-nodes/${mac}/configure`, config)
        
        // Обновить локальный статус
        const node = this.newNodes.find(n => n.mac_address === mac)
        if (node) {
          node.status = 'configuring'
        }
        
        return { success: true }
      } catch (error: any) {
        return { success: false, error: error.message }
      }
    },
    
    async deleteNewNode(mac: string) {
      try {
        await api.delete(`/new-nodes/${mac}`)
        await this.fetchNewNodes()
      } catch (error) {
        console.error('Failed to delete node:', error)
      }
    },
    
    async getSetupInstructions(mac: string) {
      try {
        const response = await api.get(`/new-nodes/${mac}/instructions`)
        return response.data
      } catch (error) {
        console.error('Failed to get instructions:', error)
        return null
      }
    },
    
    subscribeToWebSocket() {
      const ws = useWebSocket()
      
      // Новая нода обнаружена
      ws.on('new-node-discovered', (node: NewNode) => {
        const existing = this.newNodes.find(n => n.mac_address === node.mac_address)
        if (existing) {
          Object.assign(existing, node)
        } else {
          this.newNodes.push(node)
        }
      })
      
      // Heartbeat обновление
      ws.on('node-heartbeat', ({ mac_address, last_heartbeat_at }: any) => {
        const node = this.newNodes.find(n => n.mac_address === mac_address)
        if (node) {
          node.last_heartbeat_at = last_heartbeat_at
          node.is_online = true
        }
      })
      
      // Конфигурация подтверждена
      ws.on('node-configured', ({ mac_address }: any) => {
        // Удалить из списка new_nodes
        const index = this.newNodes.findIndex(n => n.mac_address === mac_address)
        if (index !== -1) {
          this.newNodes.splice(index, 1)
        }
      })
      
      // Статус изменен
      ws.on('node-status-changed', ({ mac_address, status }: any) => {
        const node = this.newNodes.find(n => n.mac_address === mac_address)
        if (node) {
          node.status = status
        }
      })
    },
  },
  
  getters: {
    rootNodes: (state) => state.newNodes.filter(n => n.is_root),
    regularNodes: (state) => state.newNodes.filter(n => !n.is_root),
    onlineNodes: (state) => state.newNodes.filter(n => n.is_online),
    
    pendingConfigCount: (state) => 
      state.newNodes.filter(n => n.status === 'discovered').length,
    
    configuringCount: (state) => 
      state.newNodes.filter(n => 
        n.status === 'configuring' || n.status === 'waiting_confirmation'
      ).length,
    
    failedNodes: (state) => 
      state.newNodes.filter(n => n.status === 'failed' || n.status === 'timeout'),
    
    hasConfiguredRoot(state) {
      // TODO: проверить через nodes store
      return false
    },
    
    getNodeByMac: (state) => (mac: string) => 
      state.newNodes.find(n => n.mac_address === mac),
  }
})
```

### 4.2 Компонент инструкций по подключению

**Файл: `server/frontend/src/components/setup/SetupInstructions.vue`**

```vue
<template>
  <v-card class="setup-instructions">
    <v-card-title>
      <v-icon left>mdi-information-outline</v-icon>
      Инструкция по подключению {{ nodeType }}
    </v-card-title>
    
    <v-card-text>
      <v-stepper v-model="step" non-linear>
        <v-stepper-header>
          <v-stepper-item 
            :complete="step > 1" 
            value="1" 
            title="Подготовка"
          />
          <v-divider />
          <v-stepper-item 
            :complete="step > 2" 
            value="2" 
            :title="isRoot ? 'WiFi через BLE' : 'Mesh подключение'"
          />
          <v-divider />
          <v-stepper-item 
            :complete="step > 3" 
            value="3" 
            title="Настройка"
          />
          <v-divider />
          <v-stepper-item 
            value="4" 
            title="Подтверждение"
          />
        </v-stepper-header>
        
        <v-stepper-window>
          <!-- Шаг 1: Подготовка -->
          <v-stepper-window-item value="1">
            <v-card flat>
              <v-card-text>
                <h3 class="mb-4">1. Подготовка устройства</h3>
                
                <v-alert type="info" class="mb-4">
                  <strong>MAC адрес:</strong> {{ node.mac_address }}
                </v-alert>
                
                <ol class="setup-steps">
                  <li>Подключите питание к устройству</li>
                  <li>Дождитесь запуска (LED начнет мигать)</li>
                  <li v-if="isRoot">
                    LED мигает <strong>медленно (1 раз в секунду)</strong> - 
                    устройство ожидает настройки WiFi
                  </li>
                  <li v-else>
                    LED мигает <strong>быстро (4 раза в секунду)</strong> - 
                    устройство подключается к mesh сети
                  </li>
                </ol>
                
                <div v-if="node.metadata?.chip_model" class="mt-4">
                  <v-chip size="small" class="mr-2">
                    <v-icon left size="small">mdi-chip</v-icon>
                    {{ node.metadata.chip_model }}
                  </v-chip>
                  <v-chip size="small" class="mr-2">
                    <v-icon left size="small">mdi-memory</v-icon>
                    {{ formatBytes(node.metadata.free_heap) }}
                  </v-chip>
                  <v-chip size="small">
                    <v-icon left size="small">mdi-code-tags</v-icon>
                    v{{ node.metadata.firmware_version }}
                  </v-chip>
                </div>
              </v-card-text>
            </v-card>
          </v-stepper-window-item>
          
          <!-- Шаг 2: WiFi / Mesh -->
          <v-stepper-window-item value="2">
            <v-card flat>
              <v-card-text>
                <template v-if="isRoot">
                  <h3 class="mb-4">2. Настройка WiFi через Bluetooth</h3>
                  
                  <v-alert type="warning" class="mb-4">
                    Для настройки ROOT ноды нужно мобильное приложение
                  </v-alert>
                  
                  <div class="qr-code-section mb-4">
                    <h4>Вариант 1: QR код</h4>
                    <p>Отсканируйте QR код в приложении ESP BLE Provisioning:</p>
                    <div class="qr-placeholder">
                      <!-- TODO: сгенерировать QR код с данными provisioning -->
                      <v-img 
                        :src="qrCodeUrl" 
                        max-width="200" 
                        class="mx-auto"
                      />
                    </div>
                  </div>
                  
                  <v-divider class="my-4" />
                  
                  <div class="manual-section">
                    <h4>Вариант 2: Ручная настройка</h4>
                    <ol>
                      <li>Скачайте приложение "ESP BLE Provisioning"</li>
                      <li>Откройте приложение и найдите устройство:
                        <v-chip class="ml-2" color="primary">
                          {{ node.temp_mesh_id || 'HYDRO_SETUP_XXX' }}
                        </v-chip>
                      </li>
                      <li>Введите пароль (PoP):
                        <v-text-field
                          :model-value="node.setup_token"
                          readonly
                          density="compact"
                          class="mt-2"
                        >
                          <template v-slot:append>
                            <v-btn 
                              icon="mdi-content-copy" 
                              size="small"
                              @click="copyToClipboard(node.setup_token)"
                            />
                          </template>
                        </v-text-field>
                      </li>
                      <li>Выберите вашу WiFi сеть и введите пароль</li>
                      <li>Дождитесь подтверждения подключения</li>
                    </ol>
                  </div>
                  
                  <v-alert type="info" class="mt-4">
                    После успешного подключения LED перейдет в режим 
                    <strong>быстрого мигания</strong>
                  </v-alert>
                </template>
                
                <template v-else>
                  <h3 class="mb-4">2. Подключение к Mesh сети</h3>
                  
                  <v-alert type="success" class="mb-4">
                    Устройство автоматически подключается к настроенной ROOT ноде
                  </v-alert>
                  
                  <p>Mesh сеть: <strong>{{ rootMeshId || 'Автоопределение' }}</strong></p>
                  
                  <v-progress-linear 
                    v-if="!node.is_online"
                    indeterminate 
                    color="primary"
                    class="mt-4"
                  />
                  
                  <v-alert 
                    v-if="node.is_online" 
                    type="success" 
                    class="mt-4"
                  >
                    <v-icon left>mdi-check-circle</v-icon>
                    Устройство подключено к mesh сети
                  </v-alert>
                </template>
              </v-card-text>
            </v-card>
          </v-stepper-window-item>
          
          <!-- Шаг 3: Настройка -->
          <v-stepper-window-item value="3">
            <v-card flat>
              <v-card-text>
                <h3 class="mb-4">3. Параметры узла</h3>
                
                <v-form ref="form" v-model="formValid">
                  <v-text-field
                    v-model="config.node_id"
                    label="ID узла"
                    :placeholder="suggestedNodeId"
                    :rules="[rules.required, rules.nodeId]"
                    hint="Уникальный идентификатор, например: root_001, ph_001"
                  />
                  
                  <v-text-field
                    v-model="config.zone"
                    label="Зона"
                    placeholder="Zone 1"
                    :rules="[rules.required]"
                    hint="Название зоны выращивания"
                  />
                  
                  <v-text-field
                    v-if="isRoot"
                    v-model="config.mesh_id"
                    label="ID Mesh сети"
                    :placeholder="suggestedMeshId"
                    :rules="[rules.required]"
                    hint="Идентификатор mesh сети, например: HYDRO1_ZONE1"
                  />
                  
                  <v-select
                    v-else
                    v-model="config.mesh_id"
                    :items="availableMeshNetworks"
                    label="Mesh сеть"
                    :rules="[rules.required]"
                    hint="Выберите существующую mesh сеть ROOT ноды"
                  />
                  
                  <template v-if="isRoot">
                    <v-divider class="my-4" />
                    <h4 class="mb-2">MQTT настройки</h4>
                    
                    <v-text-field
                      v-model="config.mqtt_broker_host"
                      label="MQTT Broker Host"
                      placeholder="192.168.1.100"
                      :rules="[rules.required, rules.ip]"
                    />
                    
                    <v-text-field
                      v-model.number="config.mqtt_broker_port"
                      label="MQTT Broker Port"
                      type="number"
                      placeholder="1883"
                      :rules="[rules.required, rules.port]"
                    />
                  </template>
                </v-form>
              </v-card-text>
            </v-card>
          </v-stepper-window-item>
          
          <!-- Шаг 4: Подтверждение -->
          <v-stepper-window-item value="4">
            <v-card flat>
              <v-card-text>
                <h3 class="mb-4">4. Проверка и применение</h3>
                
                <v-alert type="info" class="mb-4">
                  Проверьте правильность введенных данных
                </v-alert>
                
                <v-list>
                  <v-list-item>
                    <template v-slot:prepend>
                      <v-icon>mdi-identifier</v-icon>
                    </template>
                    <v-list-item-title>ID узла</v-list-item-title>
                    <v-list-item-subtitle>{{ config.node_id }}</v-list-item-subtitle>
                  </v-list-item>
                  
                  <v-list-item>
                    <template v-slot:prepend>
                      <v-icon>mdi-map-marker</v-icon>
                    </template>
                    <v-list-item-title>Зона</v-list-item-title>
                    <v-list-item-subtitle>{{ config.zone }}</v-list-item-subtitle>
                  </v-list-item>
                  
                  <v-list-item>
                    <template v-slot:prepend>
                      <v-icon>mdi-network</v-icon>
                    </template>
                    <v-list-item-title>Mesh сеть</v-list-item-title>
                    <v-list-item-subtitle>{{ config.mesh_id }}</v-list-item-subtitle>
                  </v-list-item>
                  
                  <template v-if="isRoot">
                    <v-list-item>
                      <template v-slot:prepend>
                        <v-icon>mdi-server-network</v-icon>
                      </template>
                      <v-list-item-title>MQTT Broker</v-list-item-title>
                      <v-list-item-subtitle>
                        {{ config.mqtt_broker_host }}:{{ config.mqtt_broker_port }}
                      </v-list-item-subtitle>
                    </v-list-item>
                  </template>
                </v-list>
                
                <v-alert type="warning" class="mt-4">
                  После применения настроек устройство перезагрузится через 3 секунды
                </v-alert>
              </v-card-text>
            </v-card>
          </v-stepper-window-item>
        </v-stepper-window>
      </v-stepper>
    </v-card-text>
    
    <v-card-actions>
      <v-btn 
        v-if="step > 1" 
        @click="step--"
      >
        Назад
      </v-btn>
      <v-spacer />
      <v-btn 
        v-if="step < 4"
        color="primary" 
        :disabled="!canProceed"
        @click="step++"
      >
        Далее
      </v-btn>
      <v-btn 
        v-else
        color="success" 
        :loading="configuring"
        :disabled="!formValid"
        @click="applyConfiguration"
      >
        Применить
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import type { NewNode } from '@/stores/newNodes'

interface Props {
  node: NewNode
  availableMeshNetworks?: string[]
}

const props = withDefaults(defineProps<Props>(), {
  availableMeshNetworks: () => []
})

const emit = defineEmits<{
  configure: [config: any]
}>()

const step = ref(1)
const formValid = ref(false)
const configuring = ref(false)

const config = ref({
  node_id: '',
  zone: '',
  mesh_id: '',
  mqtt_broker_host: '192.168.1.100',
  mqtt_broker_port: 1883,
})

const isRoot = computed(() => props.node.is_root)
const nodeType = computed(() => props.node.node_type.toUpperCase())

const suggestedNodeId = computed(() => {
  const type = props.node.node_type
  const mac = props.node.mac_address.split(':').slice(-2).join('')
  return `${type}_${mac}`
})

const suggestedMeshId = computed(() => {
  return `HYDRO1_ZONE1`
})

const rootMeshId = computed(() => {
  // TODO: получить из существующих ROOT нод
  return props.availableMeshNetworks[0]
})

const qrCodeUrl = computed(() => {
  // TODO: генерация QR кода для provisioning
  return 'data:image/svg+xml;base64,PHN2ZyB3aWR0aD0iMjAwIiBoZWlnaHQ9IjIwMCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cmVjdCB3aWR0aD0iMjAwIiBoZWlnaHQ9IjIwMCIgZmlsbD0iI2ZmZiIvPjx0ZXh0IHg9IjUwJSIgeT0iNTAlIiBkb21pbmFudC1iYXNlbGluZT0ibWlkZGxlIiB0ZXh0LWFuY2hvcj0ibWlkZGxlIj5RUiBDb2RlPC90ZXh0Pjwvc3ZnPg=='
})

const canProceed = computed(() => {
  if (step.value === 1) return true
  if (step.value === 2 && !isRoot.value) return props.node.is_online
  if (step.value === 2 && isRoot.value) return true // TODO: проверить WiFi подключение
  if (step.value === 3) return formValid.value
  return true
})

const rules = {
  required: (v: string) => !!v || 'Обязательное поле',
  nodeId: (v: string) => /^[a-z0-9_]+$/.test(v) || 'Только латиница, цифры и подчеркивание',
  ip: (v: string) => /^(\d{1,3}\.){3}\d{1,3}$/.test(v) || 'Неверный формат IP',
  port: (v: number) => (v > 0 && v < 65536) || 'Порт должен быть от 1 до 65535',
}

function formatBytes(bytes: number): string {
  if (bytes < 1024) return bytes + ' B'
  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB'
  return (bytes / 1024 / 1024).toFixed(1) + ' MB'
}

function copyToClipboard(text: string) {
  navigator.clipboard.writeText(text)
  // TODO: показать snackbar с подтверждением
}

async function applyConfiguration() {
  configuring.value = true
  try {
    emit('configure', config.value)
  } finally {
    configuring.value = false
  }
}
</script>

<style scoped>
.setup-steps {
  line-height: 2;
}

.qr-code-section {
  text-align: center;
}

.qr-placeholder {
  padding: 20px;
  border: 2px dashed #ccc;
  border-radius: 8px;
  display: inline-block;
}
</style>
```

---

## 5. Ключевые отличия от оригинального плана

| Аспект | Оригинал | Улучшенная версия |
|--------|----------|-------------------|
| **Безопасность** | Фиксированные пароли | Уникальные токены с истечением |
| **Mesh ID** | "HYDRO_NEW" для всех | Уникальный per устройство |
| **WiFi** | Hardcoded credentials | BLE Provisioning |
| **Состояния** | Бинарное (new/configured) | State Machine с 8 состояниями |
| **Timeout** | Бесконечное ожидание | 10 минут с автоматическим retry |
| **Валидация** | Минимальная | Полная с проверкой токенов |
| **Логирование** | Нет | Таблица setup_logs |
| **UX** | Простые формы | Пошаговый wizard с инструкциями |
| **LED** | Не указано | 4 режима индикации |
| **Error handling** | Минимальная | Rollback + recovery режим |

---

## 6. TODO List с приоритетами

### 🔴 Критичные (фаза 1)

- [ ] **Firmware: State Machine** - реализация `node_state_machine.c`
- [ ] **Firmware: Setup credentials** - генерация токенов и уникальных ID
- [ ] **Firmware: WiFi Provisioning** - интеграция BLE provisioning для ROOT
- [ ] **Backend: Расширенная таблица** - migration с полями status, setup_token
- [ ] **Backend: Улучшенная модель** - NewNode с методами валидации
- [ ] **Backend: Endpoint защита** - проверка токенов при configure

### 🟡 Важные (фаза 2)

- [ ] **Firmware: LED индикация** - 4 режима (off, slow, fast, error)
- [ ] **Firmware: HTTP server** - endpoint для получения конфигурации ROOT
- [ ] **Firmware: Timeout monitoring** - автоматический retry и recovery
- [ ] **Backend: Setup logs** - таблица для аудита всех событий
- [ ] **Frontend: Wizard компонент** - пошаговые инструкции
- [ ] **Frontend: WebSocket события** - реал-тайм обновления статусов

### 🟢 Желательные (фаза 3)

- [ ] **Firmware: QR код** - генерация для BLE provisioning
- [ ] **Backend: Batch configuration** - настройка нескольких нод сразу
- [ ] **Frontend: QR сканер** - для быстрой настройки
- [ ] **Frontend: Диагностика** - просмотр setup_logs
- [ ] **Документация** - инструкции для пользователей

---

## 7. Оценка времени реализации

| Компонент | Сложность | Время (часы) |
|-----------|-----------|--------------|
| Firmware: State Machine | Средняя | 6 |
| Firmware: Setup credentials | Низкая | 3 |
| Firmware: WiFi Provisioning | Высокая | 10 |
| Firmware: LED индикация | Низкая | 2 |
| Backend: Migration + Model | Средняя | 4 |
| Backend: Controller + MQTT | Средняя | 6 |
| Backend: Setup logs | Низкая | 2 |
| Frontend: Store | Низкая | 3 |
| Frontend: Wizard компонент | Высокая | 12 |
| Frontend: WebSocket | Средняя | 4 |
| Тестирование | - | 8 |
| **ИТОГО** | - | **~60 часов** |

---

## 8. Риски и митигация

| Риск | Вероятность | Влияние | Митигация |
|------|-------------|---------|-----------|
| BLE Provisioning не работает | Средняя | Высокое | Fallback на WiFi AP режим |
| Токены истекают слишком быстро | Низкая | Среднее | Увеличить timeout до 24ч |
| Race conditions в Backend | Средняя | Высокое | Использовать database locks |
| Mesh сеть недоступна | Средняя | Высокое | Auto-retry с экспоненциальным backoff |
| Пользователь теряет setup_token | Низкая | Среднее | Кнопка "Reset" для генерации нового |

---

**Статус:** Готово к реализации  
**Дата:** 2025-01-07  
**Версия:** 2.0

**Рекомендация:** Начать с критичных компонентов (фаза 1), затем итеративно добавлять функциональность.

