# Руководство по режиму настройки узлов (Node Setup System)

Документ описывает новую систему добавления узлов в Mesh Hydro и заменяет прежние инструкции по ручной прошивке параметров. Используйте его для ввода в эксплуатацию чистых устройств и повторной привязки узлов.

---

## 1. Требования

### Оборудование
- Минимум один ROOT‑узел на ESP32‑S3.
- Обычные узлы (Climate, Relay, Water, pH, EC, Display и др.) на соответствующих платформах ESP32/ESP32‑C3.
- Wi‑Fi роутер с доступом в локальную сеть.
- ПК с Windows 10 (подходит текущая среда) и предоставляемым проектом.

### ПО и окружение
- ESP‑IDF v5.5 (установлен в `C:\Espressif\`).
- Docker Desktop (для backend/frontend окружения).
- Git (поставляется вместе с ESP‑IDF).

---

## 2. Подготовка проекта

### 2.1 Обновление зависимостей
```powershell
cd C:\esp\hydro\mesh\mesh_hydro
git pull
```

### 2.2 Запуск dev‑окружения
```powershell
cd server
docker-compose -f docker-compose.dev.yml build --no-cache
docker-compose -f docker-compose.dev.yml up -d
```
Службы, которые должны подняться:
- `hydro_postgres_dev`
- `hydro_mosquitto_dev`
- `hydro_backend_dev`
- `hydro_frontend_dev`
- `hydro_mqtt_dev`
- `hydro_reverb_dev`

Проверить статус можно через Docker Desktop или командой:
```powershell
docker-compose -f docker-compose.dev.yml ps
```

---

## 3. Настройка ROOT‑узла

1. **Прошивка**  
   ```powershell
   cd C:\esp\hydro\mesh\mesh_hydro\root_node
   cmd.exe /c "call ""C:\Espressif\idf_cmd_init.bat"" esp-idf-1dcc643656a1439837fdf6ab63363005 && idf.py -p COMX flash monitor"
   ```
   Замените `COMX` на порт вашего устройства. При первом запуске устройство войдёт в *setup mode*.

2. **Подключение к порталу**  
   ROOT поднимает точку доступа `HYDRO_SETUP_<PIN>` (PIN — последние 6 символов MAC). Подключитесь к ней телефоном или ноутбуком, перейдите по адресу `http://192.168.4.1` и введите SSID/пароль домашнего роутера.

3. **Проверка обнаружения**  
   После получения IP и запуска временной mesh ROOT отправит HTTP‑discovery на backend. Откройте веб‑приложение `http://localhost:4173` (или порт, который выводит фронтенд) и перейдите в раздел `Настройка узлов` (`/setup`). ROOT появится в списке «Новые узлы».

4. **Мастер конфигурации ROOT**  
   - Нажмите «Настроить» на карточке ROOT.
   - Заполните поля:
     - `Node ID` — уникальный идентификатор (например, `root_main`).
     - `Mesh Network` — оставьте автосгенерированное значение или задайте вручную. Это значение станет частью MQTT-префикса `hydro/<mesh_id>/…`.
     - `Zone` — номер/название зоны.
     - `MQTT broker host` и `port` — адрес брокера (по умолчанию `hydro_mosquitto_dev:1883` внутри Docker, либо IP хоста, если устройства в той же сети).
     - `Wi-Fi SSID` / `Wi-Fi Password` — параметры роутера, который ввели в портале. Убедитесь, что значения совпадают.
   - Подтвердите. Backend отправит HTTP POST `/api/config` на ROOT. На устройстве появится лог вида `Конфигурация сохранена, перезагрузка`.
   - После перезагрузки ROOT перейдёт в нормальный режим, поднимет постоянную mesh и начнёт публиковать heartbeat в MQTT топик `hydro/<mesh_id>/heartbeat/<root_node_id>` (например, `hydro/HYDRO_ZONE1/heartbeat/root_main`).

---

## 4. Настройка обычных узлов

1. **Прошивка**  
   Повторите процедуру прошивки, аналогичную ROOT, в каталоге соответствующего узла (`node_climate`, `node_ec`, `node_water` и т.д.).

2. **Автоскан mesh**  
   После старта устройство войдёт в setup‑режим, автоматически найдёт доступную mesh (витую ROOT) и начнёт отправлять:
   - `discovery` (через mesh → ROOT → MQTT)
   - `heartbeat` каждые 10 секунд

3. **Мастер конфигурации**  
   - В интерфейсе `/setup` появится карточка новой ноды.
   - Нажмите «Настроить» → шаги мастера:
     1. Подтверждение типа узла и PIN.
     2. Ввод параметров: `Node ID`, выбор зоны, привязка к `Mesh Network` конкретного ROOT (выпадающий список), дополнительные настройки в зависимости от типа (например, каллибровочные значения — пока опционально).
     3. Подтверждение.
   - После отправки backend сформирует команду `write_config` и отправит её через `hydro/<mesh_id>/command/<node_id>` (MQTT). ROOT пробрасывает команду в mesh.
   - Узел сохранит конфигурацию (`node_config_set_*`), отправит `config_confirmation`, и backend перенесёт запись из `new_nodes` в `nodes`.

4. **Проверка**  
   - В `/setup` карточка исчезнет, запись появится на основной панели мониторинга.
   - В логах устройства — сообщения: `Configuration saved`, затем перезагрузка.

---

## 5. Диагностика и очистка

### 5.1 Проверка очередей
```powershell
docker logs hydro_mqtt_dev --tail 50
docker logs hydro_backend_dev --tail 100
```

### 5.2 Повторная привязка
Если нужно перепривязать узел:
1. Сбросьте NVS (`idf.py erase-flash` или вызов `node_config_mark_configured(false)` в отладочной сборке).
2. Перезапустите устройство — оно снова войдёт в setup mode и появится в `/setup`.

### 5.3 Удаление зависших записей
В интерфейсе `/setup` можно удалить карточку вручную. Backend удалит строку из таблицы `new_nodes`.

---

## 6. Тестирование

1. **Unit**  
   `tests/unit_mesh_protocol`:  
   ```powershell
   cd tests/unit_mesh_protocol
   cmd.exe /c "call ""C:\Espressif\idf_cmd_init.bat"" esp-idf-1dcc643656a1439837fdf6ab63363005 && idf.py build"
   ```
   Загружайте бинарь на ESP32 для автоматического запуска Unity‑тестов протокола.

2. **Backend**  
   ```powershell
   docker exec -it hydro_backend_dev php artisan test
   ```

3. **Frontend**  
   ```powershell
   docker exec -it hydro_frontend_dev npm run test
   ```

---

## 7. Частые проблемы

| Симптом | Возможная причина | Решение |
|---------|------------------|---------|
| ROOT остается в списке «Новые узлы» после конфигурации | Устройство продолжает отправлять `discovery` вместо `heartbeat` | Проверьте прошивку `root_node` ≥ текущей версии. Убедитесь, что backend обновлён (файлы `MqttService`). |
| Ошибка `Missing required fields` на ROOT | Фронтенд не отправил Wi‑Fi параметры | Обновите фронтенд, проверьте форму мастера. В журнале `root_main` найдите строку `Setup config payload`. |
| `Cannot send discovery/heartbeat: ESP_ERR_HTTP_CONNECT` | Backend недоступен по HTTP | Убедитесь, что `docker-compose` запущен, проверьте сетевой доступ к `hydro_backend_dev`. |
| Нода не появляется в мастере | Нет MQTT соединения между ROOT и backend | Проверьте `hydro_mosquitto_dev` и подписки ROOT. Посмотрите логи `hydro_mqtt_dev`. |

---

## 8. Очистка устаревших артефактов

После успешной миграции на новую систему удалите старые инструкции (например, `SETUP_GUIDE.md`) и любые персональные заметки, не относящиеся к текущему процессу добавления узлов. Актуальная документация теперь находится в:
- `doc/NODE_SETUP_USER_GUIDE.md` (этот файл)
- `NODE_SETUP_SYSTEM_PLAN.md` (архитектура и технические детали)
- `NODE_SETUP_FRONTEND_FULL_DESIGN.md` (макеты мастера настройки)

---

**Готово!** Теперь вся команда может добавлять узлы через единый мастер настройки без ручного прописывания параметров в прошивке. Шьётся, подключается, подтверждается — и работает 🚀

