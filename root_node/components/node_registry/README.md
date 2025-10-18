# Node Registry

Реестр всех узлов mesh-сети для ROOT узла.

## Возможности

- Отслеживание статуса всех подключенных узлов
- Хранение MAC адресов и последних данных
- Автоматическая проверка таймаутов (30 сек → offline)
- Экспорт данных в JSON (для Display узла)

## API

```c
// Инициализация
node_registry_init();

// Обновление при получении данных
node_registry_update_last_seen("ph_ec_001", mac_addr);
node_registry_update_data("ph_ec_001", json_data);

// Проверка таймаутов
node_registry_check_timeouts();

// Получение узла
node_info_t *node = node_registry_get("climate_001");
if (node && node->online) {
    // Узел онлайн
}

// Экспорт всех узлов (для Display)
cJSON *all_nodes = node_registry_export_all_to_json();
```

## Использование

См. `data_router.c` для примеров интеграции.

