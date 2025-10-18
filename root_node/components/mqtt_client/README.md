# MQTT Client Manager

MQTT клиент для связи ROOT узла с сервером.

## Возможности

- Подключение к MQTT broker
- Публикация телеметрии и событий
- Подписка на команды и конфигурации
- Автоматическое переподключение
- Callback для обработки входящих сообщений

## API

```c
// Инициализация и запуск
mqtt_client_manager_init();
mqtt_client_manager_start();

// Регистрация callback
mqtt_client_manager_register_recv_cb(on_mqtt_message);

// Публикация
mqtt_client_manager_publish("hydro/telemetry", json_str);

// Проверка соединения
if (mqtt_client_manager_is_connected()) {
    // MQTT онлайн
}
```

## Конфигурация

Настройки в `root_config.h`:
- Broker URI
- Username/Password
- Топики для подписки

## Топики

### Публикация (ROOT → MQTT):
- `hydro/telemetry` - телеметрия от узлов
- `hydro/event` - события от узлов
- `hydro/heartbeat` - heartbeat узлов

### Подписка (MQTT → ROOT):
- `hydro/command/#` - команды от сервера
- `hydro/config/#` - конфигурации от сервера

