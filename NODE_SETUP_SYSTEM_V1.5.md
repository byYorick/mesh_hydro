# Упрощенная архитектура: Система добавления узлов V1.5

## Философия версии 1.5

**Цель:** Баланс между простотой реализации и надежностью работы

### Что упростили из V2:
- ❌ BLE Provisioning → ✅ WiFi AP режим (проще)
- ❌ Токены с истечением → ✅ Простой PIN из MAC адреса
- ❌ Сложный State Machine (8 состояний) → ✅ Упрощенный (4 состояния)
- ❌ Setup logs в БД → ✅ Простое логирование в metadata
- ❌ QR коды → ✅ Web интерфейс на ESP32

### Что сохранили из V2:
- ✅ Уникальные mesh ID (не фиксированные)
- ✅ LED индикация
- ✅ Timeout с retry
- ✅ Валидация данных
- ✅ Пошаговый wizard

### Что оставили из V1:
- ✅ Простая структура кода
- ✅ HTTP Discovery
- ✅ Минимум зависимостей

---

## 1. Упрощенная безопасность

### 1.1 PIN из MAC адреса (вместо токенов)

**Файл: `common/node_config/node_config.h`**

```c
// Простое решение: PIN = последние 6 символов MAC
void node_config_generate_pin(char *pin_out, size_t size);
bool node_config_validate_pin(const char *pin);
```

**Файл: `common/node_config/node_config.c`**

```c
void node_config_generate_pin(char *pin_out, size_t size) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    
    // PIN = последние 6 символов MAC без двоеточий
    // Например: MAC = AA:BB:CC:DD:EE:FF → PIN = DDEEFF
    snprintf(pin_out, size, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    
    ESP_LOGI(TAG, "Generated PIN: %s", pin_out);
}

bool node_config_validate_pin(const char *pin) {
    char expected_pin[7];
    node_config_generate_pin(expected_pin, sizeof(expected_pin));
    
    return strcmp(expected_pin, pin) == 0;
}

// Уникальный mesh ID из MAC
void node_config_generate_temp_mesh_id(char *mesh_id_out, size_t size) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    
    snprintf(mesh_id_out, size, "HYDRO_%02X%02X%02X", 
             mac[3], mac[4], mac[5]);
}
```

**Преимущества:**
- ✅ Простая реализация
- ✅ Уникальный для каждого устройства
- ✅ Пользователь видит PIN на устройстве (Serial/LED/Display)
- ✅ Нет истечения токена

**Недостатки:**
- ⚠️ Предсказуемый (но для домашней гидропоники достаточно)
- ⚠️ Нельзя изменить без перепрошивки (но не нужно)

---

### 1.2 Упрощенный State Machine

Вместо 8 состояний V2 → **4 состояния**

```c
typedef enum {
    NODE_STATE_SETUP,      // Режим настройки
    NODE_STATE_WAITING,    // Ожидание конфигурации
    NODE_STATE_READY,      // Готов к работе
    NODE_STATE_ERROR       // Ошибка
} node_state_t;
```

**Файл: `common/node_config/node_state.h`**

```c
#ifndef NODE_STATE_H
#define NODE_STATE_H

typedef enum {
    NODE_STATE_SETUP,      // Первый запуск, настройка
    NODE_STATE_WAITING,    // Ожидание конфигурации
    NODE_STATE_READY,      // Настроен и готов
    NODE_STATE_ERROR       // Ошибка
} node_state_t;

typedef struct {
    node_state_t state;
    uint32_t state_start_time;
    uint32_t timeout_sec;
    int retry_count;
    void (*on_timeout)(void);
} node_state_ctx_t;

void node_state_init(node_state_ctx_t *ctx);
void node_state_set(node_state_ctx_t *ctx, node_state_t new_state, uint32_t timeout_sec);
bool node_state_check_timeout(node_state_ctx_t *ctx);
const char* node_state_name(node_state_t state);

#endif
```

**Файл: `common/node_config/node_state.c`**

```c
#include "node_state.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "node_state";

const char* node_state_name(node_state_t state) {
    switch (state) {
        case NODE_STATE_SETUP:   return "SETUP";
        case NODE_STATE_WAITING: return "WAITING";
        case NODE_STATE_READY:   return "READY";
        case NODE_STATE_ERROR:   return "ERROR";
        default: return "UNKNOWN";
    }
}

void node_state_init(node_state_ctx_t *ctx) {
    ctx->state = NODE_STATE_SETUP;
    ctx->state_start_time = 0;
    ctx->timeout_sec = 0;
    ctx->retry_count = 0;
    ctx->on_timeout = NULL;
}

void node_state_set(node_state_ctx_t *ctx, node_state_t new_state, uint32_t timeout_sec) {
    ESP_LOGI(TAG, "State: %s -> %s (timeout: %lu sec)",
             node_state_name(ctx->state),
             node_state_name(new_state),
             timeout_sec);
    
    ctx->state = new_state;
    ctx->state_start_time = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    ctx->timeout_sec = timeout_sec;
}

bool node_state_check_timeout(node_state_ctx_t *ctx) {
    if (ctx->timeout_sec == 0) return false;
    
    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t elapsed = now - ctx->state_start_time;
    
    if (elapsed >= ctx->timeout_sec) {
        ESP_LOGW(TAG, "Timeout in state %s after %lu sec",
                 node_state_name(ctx->state), elapsed);
        
        if (ctx->on_timeout) {
            ctx->on_timeout();
        }
        
        return true;
    }
    
    return false;
}
```

---

## 2. WiFi Provisioning через AP режим

### 2.1 Вместо BLE → WiFi Access Point

**ROOT нода в режиме setup:**
1. Создает WiFi точку доступа: `HYDRO_SETUP_DDEEFF`
2. Пароль AP: `hydro2025` (простой фиксированный)
3. Запускает web сервер на `192.168.4.1`
4. Пользователь подключается и вводит WiFi credentials через браузер

**Файл: `root_node/components/wifi_setup/wifi_setup.h`**

```c
#ifndef WIFI_SETUP_H
#define WIFI_SETUP_H

#include "esp_err.h"

typedef struct {
    char ssid[32];
    char password[64];
    void (*on_configured)(const char *ssid, const char *password);
} wifi_setup_config_t;

// Запустить AP режим с web сервером
esp_err_t wifi_setup_start_ap(const char *ap_ssid, const char *ap_password);

// Остановить AP
esp_err_t wifi_setup_stop_ap(void);

// Установить callback при получении credentials
void wifi_setup_set_callback(void (*callback)(const char*, const char*));

#endif
```

**Файл: `root_node/components/wifi_setup/wifi_setup.c`**

```c
#include "wifi_setup.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "cJSON.h"

static const char *TAG = "wifi_setup";
static httpd_handle_t s_server = NULL;
static void (*s_on_configured_cb)(const char*, const char*) = NULL;

// HTML страница для настройки WiFi
static const char *setup_html = 
"<!DOCTYPE html>"
"<html>"
"<head>"
"  <meta charset='UTF-8'>"
"  <meta name='viewport' content='width=device-width, initial-scale=1'>"
"  <title>Hydro Setup</title>"
"  <style>"
"    body { font-family: Arial; margin: 20px; background: #f0f0f0; }"
"    .container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; }"
"    h1 { color: #2c3e50; }"
"    input { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }"
"    button { width: 100%; padding: 12px; background: #27ae60; color: white; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; }"
"    button:hover { background: #229954; }"
"    .info { background: #e8f4f8; padding: 10px; border-radius: 4px; margin-bottom: 20px; }"
"  </style>"
"</head>"
"<body>"
"  <div class='container'>"
"    <h1>🌱 Hydro Setup</h1>"
"    <div class='info'>"
"      <strong>ROOT нода в режиме настройки</strong><br>"
"      Введите данные вашего WiFi роутера"
"    </div>"
"    <form id='wifiForm'>"
"      <input type='text' name='ssid' placeholder='WiFi SSID' required>"
"      <input type='password' name='password' placeholder='WiFi пароль' required>"
"      <button type='submit'>Подключить</button>"
"    </form>"
"    <div id='status'></div>"
"  </div>"
"  <script>"
"    document.getElementById('wifiForm').onsubmit = async (e) => {"
"      e.preventDefault();"
"      const formData = new FormData(e.target);"
"      const data = { ssid: formData.get('ssid'), password: formData.get('password') };"
"      "
"      document.getElementById('status').innerHTML = '<p style=\"color:blue\">Подключение...</p>';"
"      "
"      try {"
"        const response = await fetch('/wifi/connect', {"
"          method: 'POST',"
"          headers: { 'Content-Type': 'application/json' },"
"          body: JSON.stringify(data)"
"        });"
"        "
"        if (response.ok) {"
"          document.getElementById('status').innerHTML = '<p style=\"color:green\">✓ Подключено! Закройте это окно.</p>';"
"        } else {"
"          document.getElementById('status').innerHTML = '<p style=\"color:red\">✗ Ошибка подключения</p>';"
"        }"
"      } catch (err) {"
"        document.getElementById('status').innerHTML = '<p style=\"color:red\">✗ ' + err + '</p>';"
"      }"
"    }"
"  </script>"
"</body>"
"</html>";

// Handler для главной страницы
static esp_err_t root_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, setup_html, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

// Handler для POST /wifi/connect
static esp_err_t wifi_connect_handler(httpd_req_t *req) {
    char content[200];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    // Парсинг JSON
    cJSON *json = cJSON_Parse(content);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    cJSON *ssid_obj = cJSON_GetObjectItem(json, "ssid");
    cJSON *pass_obj = cJSON_GetObjectItem(json, "password");
    
    if (!ssid_obj || !pass_obj) {
        cJSON_Delete(json);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    
    const char *ssid = ssid_obj->valuestring;
    const char *password = pass_obj->valuestring;
    
    ESP_LOGI(TAG, "Received WiFi credentials: SSID=%s", ssid);
    
    // Вызвать callback
    if (s_on_configured_cb) {
        s_on_configured_cb(ssid, password);
    }
    
    cJSON_Delete(json);
    
    // Отправить OK
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true}");
    
    return ESP_OK;
}

esp_err_t wifi_setup_start_ap(const char *ap_ssid, const char *ap_password) {
    ESP_LOGI(TAG, "Starting AP: %s", ap_ssid);
    
    // Настройка WiFi AP
    wifi_config_t wifi_config = {
        .ap = {
            .ssid_len = strlen(ap_ssid),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    strcpy((char*)wifi_config.ap.ssid, ap_ssid);
    strcpy((char*)wifi_config.ap.password, ap_password);
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "AP started. Connect to: %s / %s", ap_ssid, ap_password);
    ESP_LOGI(TAG, "Open browser: http://192.168.4.1");
    
    // Запуск HTTP сервера
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    
    ESP_ERROR_CHECK(httpd_start(&s_server, &config));
    
    // Регистрация handlers
    httpd_uri_t root_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = root_handler,
    };
    httpd_register_uri_handler(s_server, &root_uri);
    
    httpd_uri_t connect_uri = {
        .uri = "/wifi/connect",
        .method = HTTP_POST,
        .handler = wifi_connect_handler,
    };
    httpd_register_uri_handler(s_server, &connect_uri);
    
    return ESP_OK;
}

esp_err_t wifi_setup_stop_ap(void) {
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
    }
    esp_wifi_stop();
    return ESP_OK;
}

void wifi_setup_set_callback(void (*callback)(const char*, const char*)) {
    s_on_configured_cb = callback;
}
```

---

## 3. ROOT Node - Упрощенная логика

**Файл: `root_node/main/app_main.c`**

```c
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "node_config.h"
#include "node_state.h"
#include "wifi_setup.h"
#include "mesh_manager.h"

static const char *TAG = "root_main";

static node_state_ctx_t s_state;
static char s_pin[7];
static char s_temp_mesh_id[32];
static char s_wifi_ssid[32];
static char s_wifi_password[64];
static bool s_wifi_configured = false;

// LED индикация
static TaskHandle_t s_led_task = NULL;
typedef enum {
    LED_OFF,
    LED_SLOW,    // 1 Hz - WiFi setup
    LED_FAST,    // 4 Hz - ожидание конфигурации
    LED_SOLID,   // Постоянно - настроен
    LED_ERROR    // SOS
} led_mode_t;

static led_mode_t s_led_mode = LED_OFF;

static void led_task(void *arg) {
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    
    while (1) {
        switch (s_led_mode) {
            case LED_OFF:
                gpio_set_level(GPIO_NUM_2, 0);
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
            
            case LED_SLOW:
                gpio_set_level(GPIO_NUM_2, 1);
                vTaskDelay(pdMS_TO_TICKS(500));
                gpio_set_level(GPIO_NUM_2, 0);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;
            
            case LED_FAST:
                gpio_set_level(GPIO_NUM_2, 1);
                vTaskDelay(pdMS_TO_TICKS(125));
                gpio_set_level(GPIO_NUM_2, 0);
                vTaskDelay(pdMS_TO_TICKS(125));
                break;
            
            case LED_SOLID:
                gpio_set_level(GPIO_NUM_2, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;
            
            case LED_ERROR:
                // SOS
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
                vTaskDelay(pdMS_TO_TICKS(1000));
                break;
        }
    }
}

// Callback при получении WiFi credentials
static void on_wifi_configured(const char *ssid, const char *password) {
    ESP_LOGI(TAG, "WiFi configured: %s", ssid);
    
    strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
    strncpy(s_wifi_password, password, sizeof(s_wifi_password) - 1);
    s_wifi_configured = true;
    
    // Остановить AP
    wifi_setup_stop_ap();
    
    // Подключиться к WiFi роутеру
    // TODO: реализовать подключение
    
    // Изменить состояние
    node_state_set(&s_state, NODE_STATE_WAITING, 600); // 10 минут
    s_led_mode = LED_FAST;
}

// Discovery task
static void discovery_task(void *arg) {
    const char *server_ip = "192.168.1.100"; // TODO: получить через mDNS
    int retry = 0;
    
    while (s_state.state == NODE_STATE_WAITING && retry < 20) {
        // Отправить discovery через HTTP
        char url[128];
        snprintf(url, sizeof(url), "http://%s/api/discovery/root", server_ip);
        
        // Создать JSON с PIN для безопасности
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "mac_address", "..."); // TODO
        cJSON_AddStringToObject(json, "node_type", "root");
        cJSON_AddStringToObject(json, "pin", s_pin);
        cJSON_AddStringToObject(json, "temp_mesh_id", s_temp_mesh_id);
        
        char *json_str = cJSON_PrintUnformatted(json);
        cJSON_Delete(json);
        
        // HTTP POST
        // TODO: реализовать отправку
        
        free(json_str);
        
        vTaskDelay(pdMS_TO_TICKS(30000)); // Каждые 30 секунд
        retry++;
    }
    
    if (retry >= 20) {
        ESP_LOGE(TAG, "Discovery timeout");
        node_state_set(&s_state, NODE_STATE_ERROR, 0);
        s_led_mode = LED_ERROR;
    }
    
    vTaskDelete(NULL);
}

// Мониторинг состояний
static void monitor_task(void *arg) {
    while (1) {
        // Проверка timeout
        if (node_state_check_timeout(&s_state)) {
            ESP_LOGW(TAG, "State timeout");
            
            if (s_state.retry_count < 3) {
                s_state.retry_count++;
                ESP_LOGI(TAG, "Retry %d/3", s_state.retry_count);
                
                // Вернуться в SETUP
                node_state_set(&s_state, NODE_STATE_SETUP, 0);
            } else {
                // Ошибка
                node_state_set(&s_state, NODE_STATE_ERROR, 0);
                s_led_mode = LED_ERROR;
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "ROOT Node v1.5 starting...");
    
    // NVS init
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    
    // Инициализация состояния
    node_state_init(&s_state);
    
    // Генерация PIN и mesh ID
    node_config_generate_pin(s_pin, sizeof(s_pin));
    node_config_generate_temp_mesh_id(s_temp_mesh_id, sizeof(s_temp_mesh_id));
    
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "PIN: %s", s_pin);
    ESP_LOGI(TAG, "Temp Mesh ID: %s", s_temp_mesh_id);
    ESP_LOGI(TAG, "=================================");
    
    // Запуск LED
    xTaskCreate(led_task, "led", 2048, NULL, 5, &s_led_task);
    
    // Проверка конфигурации
    bool is_configured = node_config_is_configured();
    
    if (!is_configured) {
        ESP_LOGW(TAG, "=== SETUP MODE ===");
        
        node_state_set(&s_state, NODE_STATE_SETUP, 600); // 10 минут на WiFi setup
        s_led_mode = LED_SLOW;
        
        // Запустить WiFi AP для настройки
        char ap_ssid[32];
        snprintf(ap_ssid, sizeof(ap_ssid), "HYDRO_SETUP_%s", s_pin);
        
        wifi_setup_set_callback(on_wifi_configured);
        wifi_setup_start_ap(ap_ssid, "hydro2025");
        
        ESP_LOGI(TAG, "Connect to WiFi: %s / hydro2025", ap_ssid);
        ESP_LOGI(TAG, "Open browser: http://192.168.4.1");
        
        // Ждать WiFi конфигурации
        while (!s_wifi_configured && s_state.state == NODE_STATE_SETUP) {
            if (node_state_check_timeout(&s_state)) {
                ESP_LOGE(TAG, "WiFi setup timeout");
                node_state_set(&s_state, NODE_STATE_ERROR, 0);
                s_led_mode = LED_ERROR;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        
        if (s_wifi_configured) {
            // Запустить discovery
            xTaskCreate(discovery_task, "discovery", 8192, NULL, 5, NULL);
            
            // Запустить монитор
            xTaskCreate(monitor_task, "monitor", 4096, NULL, 5, NULL);
        }
        
    } else {
        ESP_LOGI(TAG, "=== CONFIGURED - NORMAL MODE ===");
        
        node_state_set(&s_state, NODE_STATE_READY, 0);
        s_led_mode = LED_SOLID;
        
        // TODO: загрузить конфигурацию, запустить MQTT, mesh
    }
}
```

---

## 4. Backend - Упрощенная таблица

### 4.1 Без токенов с истечением, с простым PIN

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
            
            // Упрощенная безопасность
            $table->string('pin', 6)->nullable(); // DDEEFF из MAC
            $table->string('temp_mesh_id', 32)->nullable();
            
            // Простое состояние (без токена и истечения)
            $table->enum('status', [
                'discovered',
                'configuring',
                'confirmed',
                'failed'
            ])->default('discovered');
            
            // Метаданные
            $table->jsonb('metadata');
            
            // Временные метки
            $table->timestamp('discovered_at');
            $table->timestamp('last_heartbeat_at')->nullable();
            $table->timestamp('configured_at')->nullable();
            $table->timestamps();
            
            $table->index('mac_address');
            $table->index('pin');
            $table->index('status');
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('new_nodes');
    }
};
```

### 4.2 Упрощенная модель

**Файл: `server/backend/app/Models/NewNode.php`**

```php
<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class NewNode extends Model
{
    protected $fillable = [
        'mac_address',
        'node_type',
        'is_root',
        'pin',
        'temp_mesh_id',
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
    ];
    
    public function isOnline(): bool
    {
        return $this->last_heartbeat_at && 
               $this->last_heartbeat_at->diffInSeconds(now()) < 30;
    }
    
    public function configure(array $config): bool
    {
        // Проверка PIN для безопасности
        if (!isset($config['pin']) || $config['pin'] !== $this->pin) {
            \Log::warning("Invalid PIN for node {$this->mac_address}");
            return false;
        }
        
        $this->update(['status' => 'configuring']);
        
        try {
            if ($this->is_root) {
                $this->sendConfigViaHttp($config);
            } else {
                app(\App\Services\MqttService::class)->sendCommand(
                    $this->mac_address,
                    'write_config',
                    $config
                );
            }
            
            return true;
        } catch (\Exception $e) {
            \Log::error("Config failed for {$this->mac_address}: " . $e->getMessage());
            $this->update(['status' => 'failed']);
            return false;
        }
    }
    
    protected function sendConfigViaHttp(array $config): void
    {
        if (!isset($this->metadata['ip_address'])) {
            throw new \Exception('ROOT IP address not available');
        }
        
        $url = "http://{$this->metadata['ip_address']}/api/config";
        
        // HTTP POST (простая версия без retry)
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
            throw new \Exception("HTTP {$httpCode}");
        }
    }
    
    public function confirmConfiguration(string $nodeId): Node
    {
        $this->update([
            'status' => 'confirmed',
            'configured_at' => now(),
        ]);
        
        return $this->moveToNodes($nodeId);
    }
    
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
        
        $this->delete();
        
        broadcast(new \App\Events\NodeConfigured($node));
        
        return $node;
    }
}
```

---

## 5. Frontend - Упрощенный wizard

**Файл: `server/frontend/src/components/setup/SimpleSetupWizard.vue`**

```vue
<template>
  <v-card>
    <v-card-title>Настройка {{ nodeType }}</v-card-title>
    
    <v-card-text>
      <v-stepper v-model="step" alt-labels>
        <v-stepper-header>
          <v-stepper-item value="1" title="Подключение" />
          <v-divider />
          <v-stepper-item value="2" title="Параметры" />
          <v-divider />
          <v-stepper-item value="3" title="Готово" />
        </v-stepper-header>
        
        <v-stepper-window>
          <!-- Шаг 1: Инструкции -->
          <v-stepper-window-item value="1">
            <div class="pa-4">
              <template v-if="isRoot">
                <h3>Подключение ROOT ноды</h3>
                
                <v-alert type="info" class="my-4">
                  <div><strong>1.</strong> Подключите питание к ROOT ноде</div>
                  <div><strong>2.</strong> LED начнет медленно мигать</div>
                  <div><strong>3.</strong> Подключитесь к WiFi сети:</div>
                </v-alert>
                
                <v-card class="my-4" color="primary" variant="outlined">
                  <v-card-text>
                    <div class="text-h6">📶 WiFi: HYDRO_SETUP_{{ node.pin }}</div>
                    <div class="text-subtitle-1">🔒 Пароль: hydro2025</div>
                  </v-card-text>
                </v-card>
                
                <v-alert type="warning" class="my-4">
                  <div><strong>4.</strong> Откройте браузер: <code>http://192.168.4.1</code></div>
                  <div><strong>5.</strong> Введите данные вашего WiFi роутера</div>
                  <div><strong>6.</strong> После подключения LED будет быстро мигать</div>
                </v-alert>
                
                <v-progress-linear 
                  v-if="!node.is_online"
                  indeterminate 
                  color="primary"
                  class="my-4"
                />
                
                <v-alert 
                  v-if="node.is_online" 
                  type="success"
                >
                  ✓ ROOT нода подключена!
                </v-alert>
              </template>
              
              <template v-else>
                <h3>Подключение ноды {{ nodeType }}</h3>
                
                <v-alert type="info" class="my-4">
                  <div><strong>1.</strong> Убедитесь что ROOT нода настроена</div>
                  <div><strong>2.</strong> Подключите питание к ноде</div>
                  <div><strong>3.</strong> LED будет быстро мигать</div>
                  <div><strong>4.</strong> Нода автоматически подключится к mesh сети</div>
                </v-alert>
                
                <v-progress-linear 
                  v-if="!node.is_online"
                  indeterminate 
                  color="primary"
                  class="my-4"
                />
                
                <v-alert 
                  v-if="node.is_online" 
                  type="success"
                >
                  ✓ Нода подключена к mesh сети!
                </v-alert>
              </template>
            </div>
          </v-stepper-window-item>
          
          <!-- Шаг 2: Форма -->
          <v-stepper-window-item value="2">
            <div class="pa-4">
              <v-form ref="form" v-model="formValid">
                <!-- PIN для безопасности -->
                <v-text-field
                  v-model="config.pin"
                  label="PIN (с наклейки на устройстве)"
                  :placeholder="node.pin"
                  :rules="[v => v === node.pin || 'Неверный PIN']"
                  hint="6 символов из MAC адреса"
                  required
                />
                
                <v-text-field
                  v-model="config.node_id"
                  label="ID узла"
                  :placeholder="suggestedNodeId"
                  :rules="[rules.required, rules.nodeId]"
                  required
                />
                
                <v-text-field
                  v-model="config.zone"
                  label="Зона"
                  placeholder="Zone 1"
                  :rules="[rules.required]"
                  required
                />
                
                <v-text-field
                  v-if="isRoot"
                  v-model="config.mesh_id"
                  label="ID Mesh сети"
                  :placeholder="suggestedMeshId"
                  :rules="[rules.required]"
                  required
                />
                
                <v-select
                  v-else
                  v-model="config.mesh_id"
                  :items="availableMeshes"
                  label="Mesh сеть"
                  required
                />
                
                <template v-if="isRoot">
                  <v-divider class="my-4" />
                  
                  <v-text-field
                    v-model="config.mqtt_broker_host"
                    label="MQTT Broker"
                    placeholder="192.168.1.100"
                    :rules="[rules.required]"
                    required
                  />
                  
                  <v-text-field
                    v-model.number="config.mqtt_broker_port"
                    label="MQTT Port"
                    type="number"
                    placeholder="1883"
                    :rules="[rules.required]"
                    required
                  />
                </template>
              </v-form>
            </div>
          </v-stepper-window-item>
          
          <!-- Шаг 3: Подтверждение -->
          <v-stepper-window-item value="3">
            <div class="pa-4">
              <v-alert type="success" class="mb-4">
                Конфигурация отправлена!
              </v-alert>
              
              <p>Устройство перезагрузится через 3 секунды.</p>
              <p>После перезагрузки LED будет гореть постоянно.</p>
              
              <v-progress-circular
                indeterminate
                color="primary"
                class="mt-4"
              />
            </div>
          </v-stepper-window-item>
        </v-stepper-window>
      </v-stepper>
    </v-card-text>
    
    <v-card-actions>
      <v-btn v-if="step > 1 && step < 3" @click="step--">
        Назад
      </v-btn>
      <v-spacer />
      <v-btn 
        v-if="step === 1"
        color="primary"
        :disabled="!node.is_online"
        @click="step++"
      >
        Далее
      </v-btn>
      <v-btn 
        v-if="step === 2"
        color="success"
        :disabled="!formValid"
        :loading="configuring"
        @click="applyConfig"
      >
        Применить
      </v-btn>
      <v-btn 
        v-if="step === 3"
        @click="$emit('close')"
      >
        Закрыть
      </v-btn>
    </v-card-actions>
  </v-card>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

interface Props {
  node: any
  availableMeshes?: string[]
}

const props = withDefaults(defineProps<Props>(), {
  availableMeshes: () => []
})

const emit = defineEmits(['configure', 'close'])

const step = ref(1)
const formValid = ref(false)
const configuring = ref(false)

const config = ref({
  pin: '',
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
  const pin = props.node.pin
  return `${type}_${pin}`
})

const suggestedMeshId = computed(() => {
  return `HYDRO1_ZONE1`
})

const rules = {
  required: (v: any) => !!v || 'Обязательное поле',
  nodeId: (v: string) => /^[a-z0-9_]+$/.test(v) || 'Только a-z, 0-9, _',
}

async function applyConfig() {
  configuring.value = true
  try {
    await emit('configure', config.value)
    step.value = 3
  } catch (error) {
    console.error('Configuration failed:', error)
  } finally {
    configuring.value = false
  }
}
</script>
```

---

## 6. Сравнение версий

| Критерий | V1 | V1.5 (этот документ) | V2 |
|----------|----|-----------------------|----|
| **Безопасность** | ⭐☆☆ Фиксированные пароли | ⭐⭐⭐ PIN из MAC | ⭐⭐⭐⭐⭐ Токены с истечением |
| **WiFi Setup** | ❌ Hardcoded | ⭐⭐⭐⭐ WiFi AP + браузер | ⭐⭐⭐⭐⭐ BLE Provisioning |
| **Состояния** | ⭐☆☆ 2 состояния | ⭐⭐⭐ 4 состояния | ⭐⭐⭐⭐⭐ 8 состояний |
| **LED индикация** | ❌ Нет | ⭐⭐⭐⭐ 4 режима | ⭐⭐⭐⭐⭐ 4 режима + SOS |
| **Timeout** | ❌ Бесконечно | ⭐⭐⭐⭐ 10 мин + retry | ⭐⭐⭐⭐⭐ Настраиваемый |
| **Валидация** | ⭐☆☆ Минимальная | ⭐⭐⭐⭐ PIN + базовая | ⭐⭐⭐⭐⭐ Полная |
| **UX** | ⭐⭐☆ Простая форма | ⭐⭐⭐⭐ Wizard с инструкциями | ⭐⭐⭐⭐⭐ + QR коды |
| **Диагностика** | ❌ Нет | ⭐⭐☆ В metadata | ⭐⭐⭐⭐⭐ Setup logs |
| **Сложность** | ⭐☆☆ Очень простая | ⭐⭐⭐ Средняя | ⭐⭐⭐⭐⭐ Высокая |
| **Время реализации** | 36 часов | **45 часов** | 60 часов |

---

## 7. Преимущества V1.5

### ✅ Что лучше чем V1:
- Уникальный PIN и mesh ID (не фиксированные)
- WiFi настраивается пользователем (не hardcoded)
- Timeout с автоматическим retry
- LED индикация для визуальной обратной связи
- Валидация с проверкой PIN
- Простой state machine

### ✅ Что проще чем V2:
- Нет BLE (сложная интеграция)
- Нет токенов с истечением (достаточно PIN)
- Упрощенный state machine (4 вместо 8 состояний)
- Нет отдельной таблицы setup_logs
- Нет QR кодов (можно добавить позже)

### ✅ Компромисс между:
- **Простотой** - не требует мобильного приложения, только браузер
- **Безопасностью** - PIN уникальный для каждого устройства
- **UX** - web интерфейс на ESP32 + пошаговый wizard
- **Надежностью** - timeout, retry, валидация

---

## 8. Workflow пользователя

### ROOT нода:
1. ✅ Подключить питание
2. ✅ LED медленно мигает
3. ✅ Найти WiFi "HYDRO_SETUP_DDEEFF"
4. ✅ Подключиться (пароль: hydro2025)
5. ✅ Открыть http://192.168.4.1 в браузере
6. ✅ Ввести данные своего WiFi роутера
7. ✅ Дождаться подключения (LED быстро мигает)
8. ✅ В веб-интерфейсе ввести PIN (DDEEFF) и параметры
9. ✅ Нажать "Применить"
10. ✅ LED горит постоянно - готово!

### Обычная нода:
1. ✅ Подключить питание
2. ✅ LED быстро мигает
3. ✅ Автоматически подключается к mesh
4. ✅ В веб-интерфейсе ввести PIN и параметры
5. ✅ Нажать "Применить"
6. ✅ LED горит постоянно - готово!

**Время setup:** 3-5 минут на устройство

---

## 9. TODO List

### 🔴 Критичные (неделя 1)
- [ ] **Firmware: node_state.c** - упрощенный state machine
- [ ] **Firmware: PIN generation** - из MAC адреса
- [ ] **Firmware: WiFi AP mode** - с web сервером
- [ ] **Firmware: LED task** - 4 режима индикации
- [ ] **Backend: migration** - таблица с PIN
- [ ] **Backend: NewNode model** - с проверкой PIN

### 🟡 Важные (неделя 2)
- [ ] **Firmware: HTTP server** - endpoint /api/config
- [ ] **Firmware: timeout monitoring** - с retry
- [ ] **Frontend: SimpleSetupWizard** - 3 шага
- [ ] **Frontend: store** - управление new_nodes
- [ ] **Testing: ROOT setup** - полный цикл
- [ ] **Testing: Node setup** - полный цикл

### 🟢 Желательные (неделя 3)
- [ ] **Firmware: mDNS** - автоопределение сервера
- [ ] **Frontend: auto-refresh** - websocket события
- [ ] **Documentation** - инструкции для пользователей
- [ ] **Print labels** - наклейки с PIN

---

## 10. Рекомендация

**V1.5 оптимальна для вашего проекта потому что:**

1. ✅ **Не требует мобильного приложения** - только браузер
2. ✅ **Безопаснее V1** - уникальный PIN на каждом устройстве
3. ✅ **Проще V2** - нет BLE, токенов, сложных state machine
4. ✅ **Быстрая реализация** - 45 часов вместо 60
5. ✅ **Хороший UX** - web интерфейс + wizard с инструкциями
6. ✅ **Легко масштабируется** - можно позже добавить QR коды, setup logs

**Готов начать реализацию V1.5!** 🚀

---

**Статус:** Готово к реализации  
**Дата:** 2025-01-07  
**Версия:** 1.5  
**Рекомендуется для:** Домашних систем и малого бизнеса

