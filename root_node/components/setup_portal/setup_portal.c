#include "setup_portal.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "setup_portal";

static httpd_handle_t s_http_server = NULL;
static setup_portal_config_t s_config = {0};
static bool s_running = false;
static bool s_netif_initialized = false;
static esp_netif_t *s_ap_netif = NULL;

static const char *HTML_PAGE = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1'>" \
    "<title>Hydro Setup</title><style>body{font-family:Arial;margin:0;background:#0f172a;color:#e2e8f0;}" \
    ".container{max-width:420px;margin:4rem auto;background:#1f2937;padding:2rem;border-radius:16px;box-shadow:0 22px 45px rgba(15,23,42,0.45);}h1{text-align:center;}" \
    "label{display:block;margin-top:1rem;font-size:0.9rem;color:#94a3b8;text-transform:uppercase;}" \
    "input{width:100%;padding:0.75rem;margin-top:0.5rem;border-radius:10px;border:1px solid #334155;background:#0f172a;color:#f8fafc;}" \
    "button{margin-top:1.5rem;width:100%;padding:0.9rem;border:none;border-radius:12px;background:#38bdf8;color:#0f172a;font-weight:600;font-size:1rem;cursor:pointer;}" \
    "button:disabled{background:#1e40af;color:#94a3b8;cursor:not-allowed;}" \
    ".status{margin-top:1.5rem;line-height:1.6;} .status-success{color:#22c55e;} .status-error{color:#ef4444;}</style></head><body>" \
    "<div class='container'><h1>🌱 Hydro Setup</h1><p>Введите данные вашего WiFi, чтобы ROOT нода подключилась к сети.</p>" \
    "<form id='wifiForm'><label>WiFi SSID<input name='ssid' placeholder='MyHomeWiFi' required></label>" \
    "<label>WiFi Пароль<input name='password' type='password' placeholder='Пароль' required></label>" \
    "<button type='submit' id='submitBtn'>Подключить</button><div class='status' id='statusMsg'></div></form></div>" \
    "<script>(function(){const form=document.getElementById('wifiForm');const statusEl=document.getElementById('statusMsg');const btn=document.getElementById('submitBtn');" \
    "form.addEventListener('submit',function(e){e.preventDefault();btn.disabled=true;statusEl.textContent='Отправка...';" \
    "const payload=JSON.stringify({ssid:form.ssid.value,password:form.password.value});" \
    "fetch('/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:payload}).then(function(resp){" \
    "if(resp.ok){statusEl.innerHTML='<span class=\\'status-success\\'>✓ Данные получены. Устройство перезапустится автоматически.</span>';return null;}" \
    "return resp.json().then(function(body){throw new Error(body.message||resp.statusText||'Ошибка');});}).catch(function(err){" \
    "statusEl.innerHTML='<span class=\\'status-error\\'>'+err.message+'</span>';btn.disabled=false;});});})();</script></body></html>";

static esp_err_t ensure_netif_initialized(void) {
    if (s_netif_initialized) {
        return ESP_OK;
    }

    esp_err_t err = esp_netif_init();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return err;
    }

    s_ap_netif = esp_netif_create_default_wifi_ap();
    if (s_ap_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create default WiFi AP interface");
        return ESP_FAIL;
    }

    s_netif_initialized = true;
    return ESP_OK;
}

static esp_err_t start_softap(const char *ssid, const char *password) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid) - 1);
    wifi_config.ap.ssid_len = strlen(ssid);
    strncpy((char *)wifi_config.ap.password, password, sizeof(wifi_config.ap.password) - 1);
    wifi_config.ap.channel = 6;
    wifi_config.ap.max_connection = 4;
    wifi_config.ap.beacon_interval = 100;
    wifi_config.ap.authmode = (strlen(password) > 0) ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN;
    wifi_config.ap.ssid_hidden = 0;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP запущен: SSID='%s'", ssid);
    return ESP_OK;
}

static esp_err_t stop_softap(void) {
    if (!s_netif_initialized) {
        return ESP_OK;
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    if (s_ap_netif) {
        esp_netif_destroy(s_ap_netif);
        s_ap_netif = NULL;
    }
    s_netif_initialized = false;
    return ESP_OK;
}

static esp_err_t wifi_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, HTML_PAGE, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static esp_err_t wifi_post_handler(httpd_req_t *req) {
    int total = req->content_len;
    if (total <= 0 || total > 512) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Получен POST /wifi/connect (len=%d)", total);

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

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    const cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    const cJSON *password = cJSON_GetObjectItem(root, "password");
    if (!cJSON_IsString(ssid) || !cJSON_IsString(password)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing ssid/password");
        return ESP_FAIL;
    }

    setup_portal_credentials_t creds = {0};
    strncpy(creds.ssid, ssid->valuestring, sizeof(creds.ssid) - 1);
    strncpy(creds.password, password->valuestring, sizeof(creds.password) - 1);
    ESP_LOGI(TAG, "Данные WiFi: SSID='%s', пароль (%d символов)", creds.ssid, (int)strlen(creds.password));
    cJSON_Delete(root);

    if (s_config.on_credentials) {
        s_config.on_credentials(&creds, s_config.user_ctx);
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"success\":true}");
    return ESP_OK;
}

static httpd_handle_t start_http_server(void) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;

    if (httpd_start(&s_http_server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Не удалось запустить HTTP сервер");
        return NULL;
    }

    httpd_uri_t root_get = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = wifi_get_handler,
        .user_ctx = NULL,
    };
    httpd_uri_t wifi_post = {
        .uri = "/wifi/connect",
        .method = HTTP_POST,
        .handler = wifi_post_handler,
        .user_ctx = NULL,
    };

    httpd_register_uri_handler(s_http_server, &root_get);
    httpd_register_uri_handler(s_http_server, &wifi_post);

    ESP_LOGI(TAG, "HTTP Setup портал запущен");
    return s_http_server;
}

esp_err_t setup_portal_start(const setup_portal_config_t *config) {
    if (config == NULL || config->ap_ssid == NULL || config->on_credentials == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (s_running) {
        return ESP_ERR_INVALID_STATE;
    }

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(ensure_netif_initialized());

    memset(&s_config, 0, sizeof(s_config));
    s_config.on_credentials = config->on_credentials;
    s_config.user_ctx = config->user_ctx;
    s_config.ap_ssid = config->ap_ssid;
    s_config.ap_password = config->ap_password;

    ESP_ERROR_CHECK(start_softap(config->ap_ssid, config->ap_password ? config->ap_password : ""));
    if (!start_http_server()) {
        stop_softap();
        return ESP_FAIL;
    }

    s_running = true;
    return ESP_OK;
}

void setup_portal_stop(void) {
    if (!s_running) {
        return;
    }

    if (s_http_server) {
        httpd_stop(s_http_server);
        s_http_server = NULL;
    }

    stop_softap();
    s_running = false;
    ESP_LOGI(TAG, "Setup портал остановлен");
}

bool setup_portal_is_running(void) {
    return s_running;
}
