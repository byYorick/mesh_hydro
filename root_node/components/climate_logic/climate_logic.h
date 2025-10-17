/**
 * @file climate_logic.h
 * @brief Резервная логика климата (если Climate node offline)
 */

#ifndef CLIMATE_LOGIC_H
#define CLIMATE_LOGIC_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t climate_logic_init(void);
void climate_logic_task(void *arg);

#ifdef __cplusplus
}
#endif

#endif // CLIMATE_LOGIC_H

