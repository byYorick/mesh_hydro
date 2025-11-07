/**
 * @file mesh_config.h
 * @brief Централизованная конфигурация для всей Mesh Hydro системы
 * 
 * ⚠️ ВАЖНО: Это ЕДИНСТВЕННОЕ место где нужно менять общие настройки!
 * 
 * После изменения этого файла нужно ПЕРЕСОБРАТЬ ВСЕ узлы:
 * - root_node
 * - node_climate
 * - node_ph_ec
 * - node_relay
 * - node_water
 */

#ifndef MESH_CONFIG_H
#define MESH_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * WIFI ROUTER - ОБЩИЕ ДЛЯ ВСЕХ УЗЛОВ
 ******************************************************************************/

/**
 * @brief SSID вашего WiFi роутера
 * 
 * Все узлы (ROOT и NODE) должны знать credentials роутера для mesh сети
 */
#define MESH_ROUTER_SSID        "Yorick"

/**
 * @brief Пароль вашего WiFi роутера
 */
#define MESH_ROUTER_PASSWORD    "pro100parol"

/*******************************************************************************
 * ⭐ ЗОНИРОВАНИЕ - MESH NETWORK (критично!)
 ******************************************************************************/

/**
 * @brief ⭐ ID mesh сети (УНИКАЛЬНЫЙ для каждой зоны!)
 * 
 * ВАЖНО ДЛЯ ЗОНИРОВАНИЯ:
 * - Каждый Root Node должен иметь УНИКАЛЬНЫЙ MESH_NETWORK_ID
 * - Все узлы одной зоны используют ОДИНАКОВЫЙ mesh_id своего Root Node
 * - Формат: "HYDRO1_ZONE{N}" где N = номер зоны
 * 
 * Примеры:
 * - Зона 1 (Root #1): "HYDRO1_ZONE1"
 * - Зона 2 (Root #2): "HYDRO1_ZONE2"
 * - Зона 3 (Root #3): "HYDRO1_ZONE3"
 * 
 * ⚠️ Если несколько Root Nodes используют одинаковый MESH_NETWORK_ID,
 *    они объединятся в ОДНУ mesh сеть! Это НЕ то что нужно для зонирования.
 * 
 * 📝 Настройка:
 * - Через menuconfig: Hydro Mesh Config -> Mesh Network ID
 * - Через NVS: key="mesh_id", value="HYDRO1_ZONE1"
 * - Или измените дефолтное значение ниже для каждой зоны
 */
#ifndef MESH_NETWORK_ID
#define MESH_NETWORK_ID         "HYDRO1_ZONE1"  // ⭐ По умолчанию Зона 1
#endif

/**
 * @brief Пароль mesh сети
 * 
 * Используется для защиты mesh AP (WPA2-PSK)
 * Может быть одинаковым для всех зон (или разным для доп. безопасности)
 */
#define MESH_NETWORK_PASSWORD   "hydro_mesh_2025"

/**
 * @brief WiFi канал для mesh сети
 * 
 * 0 = автовыбор канала
 * 1-13 = фиксированный канал (РЕКОМЕНДУЕТСЯ для стабильности!)
 * 
 * ⚠️ ВАЖНО: Канал ДОЛЖЕН совпадать с каналом роутера для ROOT узла!
 * Ваш роутер "Yorick" работает на channel 7 (из логов mesh)
 */
#define MESH_NETWORK_CHANNEL    7

/*******************************************************************************
 * MQTT BROKER - ДЛЯ ROOT УЗЛА
 ******************************************************************************/

/**
 * @brief IP адрес или hostname MQTT брокера
 * 
 * Обычно это IP вашего компьютера где запущен Docker
 * Узнать можно: ipconfig (Windows) или ifconfig (Linux/Mac)
 */
#define MQTT_BROKER_HOST        "192.168.1.100"

/**
 * @brief Порт MQTT брокера
 */
#define MQTT_BROKER_PORT        1883

/**
 * @brief Полный URI MQTT брокера
 */
#define MQTT_BROKER_URI         "mqtt://192.168.1.100:1883"

/*******************************************************************************
 * TIMEOUTS И ИНТЕРВАЛЫ - ОБЩИЕ
 ******************************************************************************/

/**
 * @brief Таймаут для определения офлайн узла (мс)
 * 
 * ⚠️ ВАЖНО: Должен совпадать с backend таймаутом (30 сек)
 * Формула: timeout >= heartbeat_interval * 3 для надежности
 */
#define MESH_NODE_TIMEOUT_MS    30000          // 30 секунд (3x heartbeat интервал)

/**
 * @brief Интервал отправки telemetry (мс)
 */
#define TELEMETRY_INTERVAL_MS   30000          // 30 секунд

/**
 * @brief Интервал отправки heartbeat (мс)
 * 
 * ⚠️ ВАЖНО: Все узлы должны использовать одинаковый интервал (10 сек)
 * Backend timeout = 30 сек (3x heartbeat для надежности)
 */
#define HEARTBEAT_INTERVAL_MS   10000          // 10 секунд (стандарт для всех узлов)

/**
 * @brief Интервал мониторинга системы (мс)
 */
#define MONITORING_INTERVAL_MS  10000          // 10 секунд

/*******************************************************************************
 * ⭐ ROOT NODE SPECIFIC - НАСТРОЙКИ ТОЛЬКО ДЛЯ ROOT (ЗОНИРОВАНИЕ)
 ******************************************************************************/

/**
 * @brief ⭐ ID Root Node (уникальный идентификатор зоны)
 * 
 * ВАЖНО ДЛЯ ЗОНИРОВАНИЯ:
 * - Каждый Root Node должен иметь УНИКАЛЬНЫЙ ROOT_NODE_ID
 * - Формат: "root_00{N}" где N = номер зоны
 * - Этот ID используется в БД для связи зоны с Root Node
 * 
 * Примеры:
 * - Root #1: "root_001"
 * - Root #2: "root_002"
 * - Root #3: "root_003"
 * 
 * 📝 Настройка:
 * - Через menuconfig: Hydro Mesh Config -> Root Node ID
 * - Через NVS: key="root_id", value="root_001"
 * - Или измените дефолтное значение ниже для каждого Root Node
 */
#ifndef ROOT_NODE_ID
#define ROOT_NODE_ID            "root_001"  // ⭐ По умолчанию Root #1
#endif

/**
 * @brief ⭐ MQTT Topic Prefix для зоны
 * 
 * Формат: "hydro/zone{N}/" где N = номер зоны
 * Используется для изоляции MQTT топиков между зонами
 * 
 * Примеры:
 * - Зона 1: "hydro/zone1/telemetry/ph_001"
 * - Зона 2: "hydro/zone2/telemetry/ph_002"
 * - Зона 3: "hydro/zone3/telemetry/ph_003"
 */
#ifndef MQTT_TOPIC_PREFIX
#define MQTT_TOPIC_PREFIX       "hydro/zone1/"  // ⭐ По умолчанию зона 1
#endif

/**
 * @brief Максимальное количество подключений к ROOT узлу
 * 
 * Рекомендуется: 6-10 для стабильной работы
 */
#define ROOT_MAX_MESH_CONNECTIONS  6

/**
 * @brief Интервал проверки fallback логики климата (мс)
 */
#define ROOT_CLIMATE_FALLBACK_CHECK_MS  60000  // 60 секунд

/*******************************************************************************
 * BUFFER SIZES - РАЗМЕРЫ БУФЕРОВ
 ******************************************************************************/

/**
 * @brief Максимальный размер mesh пакета
 */
#define MESH_MAX_PACKET_SIZE    1456

/**
 * @brief Размер буфера для JSON сообщений
 */
#define JSON_BUFFER_SIZE        2048

/**
 * @brief Размер буфера для MQTT сообщений
 */
#define MQTT_BUFFER_SIZE        4096

/*******************************************************************************
 * ВАЖНЫЕ ПРИМЕЧАНИЯ
 ******************************************************************************/

/*
 * ⚠️ После изменения этого файла:
 * 
 * 1. Пересоберите ВСЕ узлы:
 *    cd root_node && idf.py build
 *    cd node_climate && idf.py build
 *    cd node_ph_ec && idf.py build
 * 
 * 2. Прошейте все узлы заново
 * 
 * 3. Проверьте что они подключаются к одной mesh сети
 * 
 * 📝 Рекомендации:
 * - Используйте сильные пароли в продакшене
 * - Измените MQTT_BROKER_HOST на IP вашего компьютера
 * - Для нескольких систем используйте разные MESH_NETWORK_ID
 * 
 * ⭐ ДЛЯ ЗОНИРОВАНИЯ:
 * - Каждый Root Node = своя зона = уникальный MESH_NETWORK_ID
 * - Пример для Зоны 1:
 *   ROOT_NODE_ID = "root_001"
 *   MESH_NETWORK_ID = "HYDRO1_ZONE1"
 *   MQTT_TOPIC_PREFIX = "hydro/zone1/"
 * 
 * - Пример для Зоны 2:
 *   ROOT_NODE_ID = "root_002"
 *   MESH_NETWORK_ID = "HYDRO1_ZONE2"
 *   MQTT_TOPIC_PREFIX = "hydro/zone2/"
 * 
 * - Узлы Зоны 1 используют MESH_NETWORK_ID = "HYDRO1_ZONE1"
 * - Узлы Зоны 2 используют MESH_NETWORK_ID = "HYDRO1_ZONE2"
 * - И так далее...
 * 
 * 📝 Настройка через menuconfig:
 *    idf.py menuconfig
 *    -> Hydro Mesh Configuration
 *       -> Root Node ID (для Root Node)
 *       -> Mesh Network ID (для всех узлов зоны)
 *       -> MQTT Topic Prefix (для Root Node)
 * 
 * 📝 Настройка через NVS (динамическая):
 *    - root_id: "root_001"
 *    - mesh_id: "HYDRO1_ZONE1"
 *    - mqtt_prefix: "hydro/zone1/"
 * 
 * ⚠️ КРИТИЧНО: Не используйте одинаковый MESH_NETWORK_ID для разных зон!
 */

#ifdef __cplusplus
}
#endif

#endif // MESH_CONFIG_H

