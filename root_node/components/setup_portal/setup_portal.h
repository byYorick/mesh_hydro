#ifndef SETUP_PORTAL_H
#define SETUP_PORTAL_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char ssid[33];
    char password[65];
} setup_portal_credentials_t;

typedef void (*setup_portal_credentials_cb_t)(const setup_portal_credentials_t *credentials, void *user_ctx);

typedef struct {
    const char *ap_ssid;
    const char *ap_password;
    setup_portal_credentials_cb_t on_credentials;
    void *user_ctx;
} setup_portal_config_t;

esp_err_t setup_portal_start(const setup_portal_config_t *config);
void setup_portal_stop(void);
bool setup_portal_is_running(void);

#ifdef __cplusplus
}
#endif

#endif // SETUP_PORTAL_H
