/**
 * @file node_config.h
 * @brief NVS конфигурация для всех типов узлов
 */

#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include "esp_err.h"
#include "cJSON.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Базовая конфигурация (общая для всех узлов)
typedef struct {
    char node_id[32];             ///< Уникальный ID узла
    char node_type[16];           ///< Тип узла (ph_ec, climate, etc)
    char zone[32];                ///< Зона/помещение
    bool config_valid;            ///< Флаг валидности конфигурации
    uint32_t config_version;      ///< Версия конфигурации
    uint64_t last_updated;        ///< Время последнего обновления
} base_config_t;

// PID параметры для насоса
typedef struct {
    float kp, ki, kd;
    float output_min, output_max;
    bool enabled;
} pump_pid_t;

// Калибровка насоса (мл/сек)
typedef struct {
    float ml_per_second;       ///< Миллилитров в секунду
    uint32_t calibration_time_ms;  ///< Время калибровки (мс)
    float calibration_volume_ml;   ///< Объем при калибровке (мл)
    uint64_t last_calibrated;  ///< Timestamp последней калибровки
    bool is_calibrated;        ///< Флаг калибровки
} pump_calibration_t;

// Конфигурация pH/EC узла
typedef struct {
    base_config_t base;
    
    // Целевые значения
    float ph_target;
    float ec_target;
    
    // Рабочие диапазоны
    float ph_min, ph_max;
    float ec_min, ec_max;
    
    // Аварийные пороги
    float ph_emergency_low, ph_emergency_high;
    float ec_emergency_high;
    
    // PID параметры (5 насосов)
    pump_pid_t pump_pid[5];
    
    // Safety
    uint32_t max_pump_time_ms;
    uint32_t cooldown_ms;
    uint32_t max_daily_volume_ml;
    
    // Автономный режим
    bool autonomous_enabled;
    uint32_t mesh_timeout_ms;
    
    // Калибровка
    float ph_cal_offset;
    float ec_cal_offset;
} ph_ec_node_config_t;

// Конфигурация pH узла (только pH, 2 насоса)
typedef struct {
    base_config_t base;
    
    // Целевое значение
    float ph_target;
    
    // Рабочие диапазоны
    float ph_min, ph_max;
    
    // PID параметры (2 насоса: UP, DOWN)
    pump_pid_t pump_pid[2];
    
    // Калибровка насосов (2 насоса: UP, DOWN)
    pump_calibration_t pump_calibration[2];
    
    // Safety
    uint32_t max_pump_time_ms;
    uint32_t cooldown_ms;
    
    // Автономный режим
    bool autonomous_enabled;
    uint32_t mesh_timeout_ms;
    
    // Калибровка датчика pH
    float ph_cal_offset;
} ph_node_config_t;

// Конфигурация EC узла (только EC, 3 насоса)
typedef struct {
    base_config_t base;
    
    // Целевое значение
    float ec_target;
    
    // Рабочие диапазоны
    float ec_min, ec_max;
    
    // PID параметры (3 насоса: A, B, C)
    pump_pid_t pump_pid[3];
    
    // Калибровка насосов (3 насоса: A, B, C)
    pump_calibration_t pump_calibration[3];
    
    // Safety
    uint32_t max_pump_time_ms;
    uint32_t cooldown_ms;
    
    // Автономный режим
    bool autonomous_enabled;
    uint32_t mesh_timeout_ms;
    
    // Калибровка датчика EC
    float ec_cal_offset;
} ec_node_config_t;

// Конфигурация Climate узла
typedef struct {
    base_config_t base;
    
    float temp_target;
    float humidity_target;
    uint16_t co2_max;
    uint16_t lux_min;
    uint32_t read_interval_ms;
} climate_node_config_t;

// Конфигурация Relay узла
typedef struct {
    base_config_t base;
    
    // Свет
    struct {
        uint8_t brightness;        // 0-100%
        char schedule_on[6];       // "08:00"
        char schedule_off[6];      // "22:00"
        bool schedule_enabled;
        bool lux_control;          // Управление по освещенности
        uint16_t lux_threshold;    // Порог включения
    } light;
    
    // Вентиляция
    struct {
        uint16_t co2_threshold;    // 800 ppm
        float temp_threshold;      // 28.0°C
        float humidity_threshold;  // 75.0%
    } ventilation;
    
    // Форточки
    struct {
        uint32_t open_time_ms;     // Время полного открытия
        uint32_t close_time_ms;    // Время полного закрытия
    } windows;
} relay_node_config_t;

// Конфигурация Water узла
typedef struct {
    base_config_t base;
    
    // Настройки насосов и клапанов
    uint32_t pump_on_time_ms;
    uint32_t pump_off_time_ms;
    uint8_t active_zones;          // Битовая маска активных зон
    
    // Safety
    uint32_t max_pump_time_ms;
    bool water_level_check;
} water_node_config_t;

/**
 * @brief Инициализация NVS
 * 
 * @return ESP_OK при успехе
 */
esp_err_t node_config_init(void);

/**
 * @brief Загрузка конфигурации из NVS
 * 
 * @param config Указатель на структуру конфигурации
 * @param size Размер структуры
 * @param namespace Имя namespace NVS
 * @return ESP_OK при успехе
 */
esp_err_t node_config_load(void *config, size_t size, const char *namespace);

/**
 * @brief Сохранение конфигурации в NVS
 * 
 * @param config Указатель на структуру конфигурации
 * @param size Размер структуры
 * @param namespace Имя namespace NVS
 * @return ESP_OK при успехе
 */
esp_err_t node_config_save(const void *config, size_t size, const char *namespace);

/**
 * @brief Сброс конфигурации к значениям по умолчанию
 * 
 * @param config Указатель на структуру конфигурации
 * @param node_type Тип узла ("ph_ec", "climate", etc)
 * @return ESP_OK при успехе
 */
esp_err_t node_config_reset_to_default(void *config, const char *node_type);

/**
 * @brief Обновление конфигурации из JSON
 * 
 * @param config Указатель на структуру конфигурации
 * @param json_config cJSON объект с новой конфигурацией
 * @param node_type Тип узла
 * @return ESP_OK при успехе
 */
esp_err_t node_config_update_from_json(void *config, cJSON *json_config, const char *node_type);

/**
 * @brief Экспорт конфигурации в JSON
 * 
 * @param config Указатель на структуру конфигурации
 * @param node_type Тип узла
 * @return cJSON объект (нужно освободить через cJSON_Delete)
 */
cJSON* node_config_export_to_json(const void *config, const char *node_type);

/**
 * @brief Очистка всех конфигураций в NVS (factory reset)
 * 
 * @return ESP_OK при успехе
 */
esp_err_t node_config_erase_all(void);

#ifdef __cplusplus
}
#endif

#endif // NODE_CONFIG_H

