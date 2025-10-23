/**
 * @file local_storage.c
 * @brief Реализация локального ring buffer
 */

#include "local_storage.h"
#include "esp_log.h"
#include <string.h>
#include <time.h>

static const char *TAG = "local_storage";

// Ring buffer
static storage_entry_t s_buffer[BUFFER_SIZE];
static int s_write_index = 0;
static int s_count = 0;

esp_err_t local_storage_init(void) {
    memset(s_buffer, 0, sizeof(s_buffer));
    s_write_index = 0;
    s_count = 0;
    
    ESP_LOGI(TAG, "Local Storage initialized (buffer size: %d)", BUFFER_SIZE);
    return ESP_OK;
}

esp_err_t local_storage_add(float ph, float ec, float temp) {
    // Ring buffer: при заполнении затираем старые записи
    s_buffer[s_write_index].timestamp = (uint64_t)time(NULL);
    s_buffer[s_write_index].ph = ph;
    s_buffer[s_write_index].ec = ec;
    s_buffer[s_write_index].temp = temp;
    s_buffer[s_write_index].synced = false;
    
    s_write_index = (s_write_index + 1) % BUFFER_SIZE;
    
    if (s_count < BUFFER_SIZE) {
        s_count++;
    }
    
    ESP_LOGD(TAG, "Added entry: pH=%.2f, EC=%.2f (total: %d)", ph, ec, s_count);
    
    return ESP_OK;
}

int local_storage_get_unsynced_count(void) {
    int unsynced = 0;
    
    for (int i = 0; i < s_count; i++) {
        if (!s_buffer[i].synced) {
            unsynced++;
        }
    }
    
    return unsynced;
}

bool local_storage_get_next_unsynced(storage_entry_t *entry) {
    if (!entry) {
        return false;
    }
    
    // Поиск первой несинхронизированной записи
    for (int i = 0; i < s_count; i++) {
        if (!s_buffer[i].synced && s_buffer[i].timestamp > 0) {
            memcpy(entry, &s_buffer[i], sizeof(storage_entry_t));
            return true;
        }
    }
    
    return false;
}

void local_storage_mark_synced(uint64_t timestamp) {
    for (int i = 0; i < s_count; i++) {
        if (s_buffer[i].timestamp == timestamp) {
            s_buffer[i].synced = true;
            ESP_LOGD(TAG, "Entry marked as synced: ts=%llu", timestamp);
            break;
        }
    }
}

int local_storage_clear_synced(void) {
    int cleared = 0;
    
    // Сдвигаем несинхронизированные записи в начало
    int write_pos = 0;
    for (int read_pos = 0; read_pos < s_count; read_pos++) {
        if (!s_buffer[read_pos].synced) {
            if (write_pos != read_pos) {
                memcpy(&s_buffer[write_pos], &s_buffer[read_pos], sizeof(storage_entry_t));
            }
            write_pos++;
        } else {
            cleared++;
        }
    }
    
    // Очистка хвоста
    for (int i = write_pos; i < s_count; i++) {
        memset(&s_buffer[i], 0, sizeof(storage_entry_t));
    }
    
    s_count = write_pos;
    s_write_index = s_count % BUFFER_SIZE;
    
    if (cleared > 0) {
        ESP_LOGI(TAG, "Cleared %d synced entries (remaining: %d)", cleared, s_count);
    }
    
    return cleared;
}

void local_storage_get_stats(int *total, int *synced, int *unsynced) {
    int synced_count = 0;
    int unsynced_count = 0;
    
    for (int i = 0; i < s_count; i++) {
        if (s_buffer[i].synced) {
            synced_count++;
        } else {
            unsynced_count++;
        }
    }
    
    if (total) *total = s_count;
    if (synced) *synced = synced_count;
    if (unsynced) *unsynced = unsynced_count;
}

