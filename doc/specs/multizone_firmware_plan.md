# План обновления прошивок под мультизонную архитектуру

## 1. Модуль `zone_config`
- Создать `common/mesh_protocol/zone_config.c` и `zone_config.h`.
- API:
  - `esp_err_t zone_config_load(char *mesh_id, size_t mesh_id_len, char *root_id, size_t root_id_len);`
  - `esp_err_t zone_config_save(const char *mesh_id, const char *root_id);`
  - `bool zone_config_validate(const char *str);`
  - `const char *zone_config_get_mesh_id(void);`
  - `const char *zone_config_get_root_id(void);`
- Хранение в NVS namespace `node_config` (ключи `mesh_network_id`, `root_node_id`).
- Использовать кэширование значений после первой загрузки; при пустых значениях возвращать `"UNCONFIGURED"` и логировать `WARNING`.
- Добавить модуль в `common/mesh_protocol/CMakeLists.txt` и экспортировать заголовок.

## 2. Централизация генерации MQTT-топиков
- В `mesh_protocol` добавить функцию `bool mesh_topic_format(char *out, size_t max_len, const char *mesh_id, const char *msg_type, const char *node_id);`.
- Использовать `zone_config_get_mesh_id()` при `mesh_id == NULL`.
- Добавить новые юнит-тесты в `tests/unit_mesh_protocol/test_mqtt_topics.c`.

## 3. Обновления Root Node
- `root_node/components/mqtt_client/mqtt_client_manager.c`:
  - Заменить использование `root_config_get_mqtt_topic_prefix()` на `zone_config_load()`.
  - Формировать подписки на команды/конфиги по новому формату `hydro/{mesh_id}/...`.
  - Использовать `mesh_topic_format()` для публикаций discovery/heartbeat.
- `root_node/components/data_router/data_router.c`:
  - Удалить логику `s_topic_prefix`.
  - На публикациях телеметрии/event/heartbeat/config_response/discovery использовать `mesh_topic_format()`.
  - При приёме MQTT команд парсить топик `hydro/{mesh_id}/{command|config}/{node_id}`, проверять совпадение mesh.

## 4. Обновления Leaf-узлов
- В `node_*/main/app_main.c` добавить буферы `s_mesh_network_id[32]`, `s_root_node_id[32]`.
- При запуске в normal-режиме загружать зону через `zone_config_load()`; fallback на `"UNCONFIGURED"`.
- Во всех `mesh_protocol_create_*` передавать `root_node_id` и `mesh_network_id`.
- В setup payload добавлять поля `mesh_network_id` и `root_node_id`.
- В обработчике `write_config` сохранять значения через `zone_config_save()` и обновлять буферы.

## 5. Общие компоненты
- В `common/node_config/node_config.c` добавить функции-геттеры/сеттеры для `mesh_network_id` и `root_node_id` (thin-wrapper над NVS).
- Обновить публичный заголовок `node_config.h`.
- Убедиться, что существующие ноды вызывают новый API вместо прямой работы с NVS.

## 6. Тесты
- `tests/unit_mesh_protocol/test_zone_config.c`:
  - Проверка сохранения/загрузки.
  - Валидация правильных/неправильных значений.
  - Проверка кэширования и fallback на `"UNCONFIGURED"`.
- `tests/unit_mesh_protocol/test_mqtt_topics.c`:
  - Форматирование heartbeat/telemetry/discovery.
  - Обработка `NULL` параметров и коротких буферов.

## 7. Сборочные скрипты
- Обновить `tools/build_all_nodes.bat`, а также индивидуальные `build.bat`/`flash.bat`, чтобы инициализировать ESP-IDF через `C:\Espressif\idf_cmd_init.bat`.
- Добавить проверку нового модуля в `common/mesh_protocol/CMakeLists.txt` и убедиться, что все проекты ссылаются на обновлённые заголовки.

## 8. Риски и контрольные точки
- Проверить размер MQTT буферов (`topic[192]`) перед миграцией.
- Убедиться, что leaf-ноды подхватывают новые поля перед OTA-обновлением.
- План отката: сохранить прошлые версии `mesh_protocol` и конфигов до объединения.


