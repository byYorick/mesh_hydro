/**
 * @file local_storage.h
 * @brief Локальный ring buffer для хранения данных при offline
 * 
 * Сохраняет телеметрию локально когда нет связи с ROOT.
 * При восстановлении связи синхронизирует данные.
 */

#ifndef LOCAL_STORAGE_H
#define LOCAL_STORAGE_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BUFFER_SIZE 1000  // Максимум 1000 записей

/**
 * @brief Запись данных в буфере
 */
typedef struct {
    uint64_t timestamp;     ///< Unix timestamp
    float ph;               ///< Значение pH
    float ec;               ///< Значение EC
    float temp;             ///< Температура
    bool synced;            ///< Синхронизировано с ROOT
} storage_entry_t;

/**
 * @brief Инициализация локального хранилища
 * 
 * @return ESP_OK при успехе
 */
esp_err_t local_storage_init(void);

/**
 * @brief Добавление записи в буфер
 * 
 * @param ph Значение pH
 * @param ec Значение EC
 * @param temp Температура
 * @return ESP_OK при успехе
 */
esp_err_t local_storage_add(float ph, float ec, float temp);

/**
 * @brief Получение количества несинхронизированных записей
 * 
 * @return Количество записей
 */
int local_storage_get_unsynced_count(void);

/**
 * @brief Получение следующей несинхронизированной записи
 * 
 * @param entry Указатель на структуру для заполнения
 * @return true если запись получена, false если буфер пуст
 */
bool local_storage_get_next_unsynced(storage_entry_t *entry);

/**
 * @brief Отметка записи как синхронизированной
 * 
 * @param timestamp Timestamp записи для отметки
 */
void local_storage_mark_synced(uint64_t timestamp);

/**
 * @brief Очистка всех синхронизированных записей
 * 
 * @return Количество удаленных записей
 */
int local_storage_clear_synced(void);

/**
 * @brief Получение статистики буфера
 * 
 * @param total Общее количество записей
 * @param synced Количество синхронизированных
 * @param unsynced Количество несинхронизированных
 */
void local_storage_get_stats(int *total, int *synced, int *unsynced);

#ifdef __cplusplus
}
#endif

#endif // LOCAL_STORAGE_H

