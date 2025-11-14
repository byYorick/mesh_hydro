# MQTT Subsystem Refactor – Architectural Notes

## 1. Current State Audit

- Core class `App\Services\MqttService` выполняет **все** обязанности: подключение к брокеру, подписка, обработка телеметрии/heartbeat/discovery/config/error, публикация команд и уведомления, авто-добавление узлов, кэширование статуса MQTT, депрекейт политика.
- Консольная команда `App\Console\Commands\MqttListenerCommand` напрямую использует методы `MqttService::subscribe` и `MqttService::handle*`.
- В коде присутствуют прямые обращения к моделям (`Node`, `Telemetry`, `Event`, `NewNode`, `Command`, `NodeError`) и вспомогательные действия (`NotificationThrottleService`), что затрудняет тестирование и нарушает SRP.
- Типы узлов (включая устаревший `ph_ec`) определяются через большое количество `if/elseif` блоков; для каждого нового типа приходится менять множество файлов.
- Setup-процесс (auto-discovery, confirmation) тесно связан с основной обработкой сообщений.
- Кеш статуса MQTT хранится через `Cache::store('file')->put(...)`, TTL = 10 минут, но при отсутствии сообщений статус падает в «offline».

## 2. Границы Подсистемы

### Входящие сообщения
| Топик | Формат | Назначение |
|-------|--------|------------|
| `hydro/{mesh_id}/telemetry/#` | JSON, содержит `node_id` и `data` | Сохранение телеметрии, обновление узла |
| `hydro/{mesh_id}/heartbeat/#` | JSON, `node_id`, системная инфо | Обновление статуса узла, auto-discovery |
| `hydro/{mesh_id}/event/#` | JSON, `node_id`, `message`, `level` | Сохранение событий, уведомления |
| `hydro/{mesh_id}/config_response/#` | JSON, `node_id`, `config` | Фиксация конфигураций узлов |
| `hydro/{mesh_id}/error/#` | JSON, `node_id`, `error_code` | Логирование ошибок узлов |
| `hydro/{mesh_id}/discovery` | JSON, данные нового узла | Setup-режим, автоконфигурация |
| Setup-топики (`hydro/setup/...`) | JSON | Первичный поиск/обмен пином |

### Исходящие действия
- Публикация команд/конфигов `hydro/{mesh_id}/command/{node_id}` / `config/{node_id}`.
- Сохранение в БД (узлы, телеметрия, события, ошибки, новые узлы).
- Генерация уведомлений (event broadcast, NotificationThrottle).
- Кеширование статуса соединения MQTT (`Cache`, `mqtt.last_successful_connection`).

## 3. Целевая Архитектура

### Слои
1. **Messaging Port** – абстракция клиента MQTT (подключение, подписка, публикация).
2. **Message Router** – маршрутизация входящих сообщений на обработчики.
3. **Handlers** – небольшие SRP-классы: `TelemetryHandler`, `HeartbeatHandler`, `DiscoveryHandler`, `ConfigResponseHandler`, `ErrorHandler`.
4. **Domain Services / Repositories** – операции с узлами, телеметрией, событиями, setup-данными.
5. **Policies** – правила депрекейтов, валидация зон, нормализация типов узлов.
6. **Application Facade** – тонкий `MqttService`, который «склеивает» порт, роутер и издателей команд.

### Основные интерфейсы
- `App\Contracts\Messaging\MqttClientInterface` – методы `connect`, `disconnect`, `publish`, `subscribe`, `loop`.
- `App\Services\Mqtt\Contracts\MqttMessageHandlerInterface` – контракт обработчиков.
- `App\Services\Mqtt\Contracts\CommandPublisherInterface` – отправка команд/конфигураций.
- Репозитории: `NodeRepositoryInterface`, `TelemetryRepositoryInterface`, `EventRepositoryInterface`, `NewNodeRepositoryInterface`.

### DTO / Value Objects
- `MqttMessageContext` – единый контейнер для `topic`, `payload`, `receivedAt`.
- `NodeIdentifier` (`nodeId`, `meshId`), `TelemetryPayload`, `HeartbeatPayload`, `DiscoveryPayload`.
- `NodeType` enum (включает `deprecated_ph_ec`).

## 4. План Реализации (высокоуровневый)
1. **Инфраструктура** – реализовать `MqttClientInterface` адаптером `PhpMqttClient`, обновить DI.
2. **Доменные сервисы** – вынести операции с узлами/телеметрией/событиями в отдельные классы, добавить политики депрекейтов.
3. **Обработчики** – перенести логику из `MqttService::handle*` в специализированные классы, покрыть тестами.
4. **Роутер** – конфигурируемый реестр обработчиков, подписки из `config/mqtt.php`.
5. **Команда Listener** – использовать новый роутер и порт, убрать бизнес-логику из консольной команды.
6. **Документация** – обновить README/ARCHITECTURE, описать новую структуру, правила расширения и миграции.

## 5. Примечания по деплоям
- Подготовить фича-флаг/ENV для переключения между старым и новым MQTT слоем.
- Обеспечить миграцию/очистку кешей (`mqtt.last_successful_connection`).
- Выделить тесты для основных цепочек (telemetry, heartbeat, discovery) и нагрузочные скрипты.


