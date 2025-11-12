# Интеграционное тестирование мультизонной цепочки

Документ описывает ручные сценарии, покрывающие критические этапы мультизонной цепочки ― регистрацию нового узла (discovery) и доставку команды от backend к узлу. Тесты ориентированы на стенд с развернутыми сервисами Mesh Hydro (MQTT брокер, backend Laravel, WebSocket) и набором прошивок, собранных на актуальной ветке.

## Предусловия

- Установлена ESP-IDF `esp-idf-1dcc643656a1439837fdf6ab63363005`.
- Все прошивки собраны командой:

```cmd
tools\build_all_nodes.bat
```

- Запущены MQTT брокер (по умолчанию `localhost:1883`), backend (`php artisan mqtt:listen`) и WebSocket сервер.
- Определена тестовая зона, например `zone_lab_1`, и привязанный к ней root node (`root_demo`).

## Тест 1 — Discovery нового узла

**Цель:** убедиться, что backend корректно принимает JSON discovery в формате `hydro/{mesh}/discovery`, создаёт запись в БД и уведомляет UI через WebSocket.

1. Запустить утилиту публикации тестовых сообщений:

```bash
python tools/mqtt_tester.py ^
  --mode discovery ^
  --mesh-id zone_lab_1 ^
  --node-id climate_demo_001 ^
  --node-type climate ^
  --root-id root_demo ^
  --firmware 2.0.0 ^
  --hardware ESP32
```

2. Проверить логи backend (`storage/logs/laravel.log`) — должен появиться блок `MQTT discovery` с корректной зоной `zone_lab_1`.
3. Убедиться, что в таблице `nodes` появилась запись:
   - `node_id = climate_demo_001`
   - `zone = zone_lab_1`
   - `root_node_id = root_demo`
   - `online = false` (до прихода heartbeat).
4. На фронтенде (узел `Nodes` → выбранная зона) должна отобразиться карточка нового узла со статусом `Offline`.

**Ожидаемый результат:** discovery принят, запись создана, UI получил событие `NodeDiscovered`.

## Тест 2 — Отправка команды узлу

**Цель:** подтвердить корректность формирования топика `hydro/{mesh}/command/{node_id}` и валидации зоны в backend.

1. Убедиться, что узел из теста 1 помечен как `online` (можно отправить heartbeat скриптом):

```bash
python tools/mqtt_tester.py ^
  --mode heartbeat ^
  --mesh-id zone_lab_1 ^
  --node-id climate_demo_001 ^
  --root-id root_demo ^
  --node-type climate
```

2. Отправить команду:

```bash
python tools/mqtt_tester.py ^
  --mode command ^
  --mesh-id zone_lab_1 ^
  --node-id climate_demo_001 ^
  --root-id root_demo ^
  --command-name request_status ^
  --command-payload "{\"includeTelemetry\": true}"
```

3. Проверить логи backend — сообщение должно быть опубликовано без ошибок, с топиком `hydro/zone_lab_1/command/climate_demo_001`.
4. На стороне узла (серийный порт) ожидается получение команды и ответ `hydro/zone_lab_1/response/climate_demo_001`. Backend должен зафиксировать событие в таблице `commands` со статусом `sent`.

**Ожидаемый результат:** команда успешно опубликована, зона валидирована, ответ принят (при наличии прошивки).

## Автоматизация и наблюдения

- Утилита `tools/mqtt_tester.py` обновлена для генерации мультизонных сообщений (режимы `discovery`, `heartbeat`, `telemetry`, `command`).
- Скрипт `tools/build_all_nodes.bat` собирает все прошивки в соответствии с актуальной версией ESP-IDF и требуемыми MCU.
- При ручном выполнении сценариев проверено:
  - backend отбрасывает сообщения с некорректной зоной (возникает запись уровня `ERROR`).
  - Маршрутизация тем `hydro/{zone}/...` соответствует спецификации.

> **Примечание:** тесты требуют запущенного оборудования/эмуляторов. В среде CI можно заглушить MQTT обмен, используя контейнер с Mosquitto и PHPUnit/Vitest моками для подтверждения обработки сообщений.


