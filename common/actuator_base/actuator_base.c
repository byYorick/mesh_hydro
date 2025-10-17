/**
 * @file actuator_base.c
 * @brief Реализация базовых функций исполнителей
 */

#include "actuator_base.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "actuator_base";

typedef struct {
    char actuator_id[32];
    uint32_t total_time_ms;
} actuator_stats_entry_t;

#define MAX_STATS_ENTRIES 16
static actuator_stats_entry_t s_stats[MAX_STATS_ENTRIES];
static int s_stats_count = 0;

bool actuator_safety_check(const char *actuator_id, uint32_t duration_ms, uint32_t max_duration_ms) {
    if (duration_ms > max_duration_ms) {
        ESP_LOGW(TAG, "Actuator %s: duration %d ms exceeds max %d ms", 
                 actuator_id, duration_ms, max_duration_ms);
        return false;
    }
    return true;
}

void actuator_log_usage(const char *actuator_id, uint32_t duration_ms) {
    // Поиск существующей записи
    for (int i = 0; i < s_stats_count; i++) {
        if (strcmp(s_stats[i].actuator_id, actuator_id) == 0) {
            s_stats[i].total_time_ms += duration_ms;
            return;
        }
    }
    
    // Добавление новой записи
    if (s_stats_count < MAX_STATS_ENTRIES) {
        strncpy(s_stats[s_stats_count].actuator_id, actuator_id, sizeof(s_stats[0].actuator_id) - 1);
        s_stats[s_stats_count].total_time_ms = duration_ms;
        s_stats_count++;
    }
}

uint32_t actuator_get_total_time(const char *actuator_id) {
    for (int i = 0; i < s_stats_count; i++) {
        if (strcmp(s_stats[i].actuator_id, actuator_id) == 0) {
            return s_stats[i].total_time_ms;
        }
    }
    return 0;
}

