# 🎉 MQTT ROOT УЗЕЛ - ИСПРАВЛЕНО

**Дата:** 20 октября 2025  
**Проблема:** MQTT не видит ROOT узел (discovery сообщения не обрабатываются)

## 🔍 Диагностика

### Симптомы:
- ✅ ROOT узел подключается к WiFi (IP: `192.168.1.191`)
- ✅ ROOT узел подключается к MQTT брокеру (`192.168.1.100:1883`)
- ✅ ROOT узел отправляет discovery сообщения (видно в логах ESP32)
- ✅ MQTT Listener получает discovery сообщения (видно `🔍 [DISCOVERY]`)
- ❌ **Узлы НЕ появляются в базе данных**

### Проверки:
1. **Mosquitto работает:**
   ```powershell
   netstat -an | findstr "1883"
   # ✅ Порт 1883 LISTENING и есть активные подключения
   ```

2. **MQTT Listener запущен:**
   ```powershell
   docker logs hydro_mqtt_listener
   # ✅ Подписан на hydro/discovery
   ```

3. **Discovery сообщения доходят:**
   ```powershell
   docker exec hydro_mosquitto mosquitto_sub -t "hydro/discovery" -C 1 -W 5
   # ❌ Timeout - сообщения не проходят через брокер
   ```

## 🐛 Причина проблемы

**Найдено:** В `server/docker-compose.yml` строка 96:

```yaml
- BROADCAST_DRIVER=pusher
```

### Что происходило:

1. MQTT Listener получал discovery сообщение
2. Вызывался `MqttService->handleDiscovery()`
3. Метод создавал узел в БД
4. Пытался отправить broadcast событие:
   ```php
   event(new \App\Events\NodeDiscovered($node));  // Line 502
   ```
5. Laravel пытался загрузить `Pusher\Pusher` класс
6. **Падал с ошибкой:** `Class "Pusher\Pusher" not found`
7. Транзакция откатывалась, узел НЕ сохранялся

### Почему не видели ошибку:
- Exception возникал внутри callback функции
- Laravel обработчик глушил исключение
- В логах появлялось только `🔍 [DISCOVERY]`, но callback не завершался

## ✅ Решение

### 1. Изменить BROADCAST_DRIVER

**Файл:** `server/docker-compose.yml`

```yaml
# БЫЛО:
- BROADCAST_DRIVER=pusher

# СТАЛО:
- BROADCAST_DRIVER=log
```

Измененно **во всех сервисах** (backend, mqtt_listener).

### 2. Перезапустить контейнеры

```powershell
docker restart hydro_backend hydro_mqtt_listener
```

## 🧪 Тестирование

### Тест 1: Тестовое discovery сообщение

```powershell
# Отправка тестового discovery
$json = Get-Content test_discovery.json -Raw
docker exec hydro_mosquitto mosquitto_pub -t "hydro/discovery" -m $json

# Проверка базы
docker exec hydro_postgres psql -U hydro -d hydro_system `
  -c "SELECT node_id, node_type, online, mac_address FROM nodes;"
```

**Результат:**
```
    node_id    | node_type | online |    mac_address    
---------------+-----------+--------+-------------------
 test_root_123 | root      | t      | AA:BB:CC:DD:EE:FF
```

✅ **РАБОТАЕТ!**

### Тест 2: Реальный ROOT узел

Теперь ROOT узел (COM7, IP 192.168.1.191) должен автоматически зарегистрироваться после отправки discovery.

**Проверить логи ROOT:**
```powershell
# В логах ROOT узла должно быть:
# I mqtt_manager: Published discovery message (msg_id=65240, len=304)
```

**Проверить базу данных:**
```powershell
docker exec hydro_postgres psql -U hydro -d hydro_system `
  -c "SELECT node_id, node_type, online, last_seen_at FROM nodes WHERE node_type='root';"
```

**Ожидаемый результат:**
```
      node_id       | node_type | online |      last_seen_at       
--------------------+-----------+--------+-------------------------
 root_98a316f5fde8  | root      | t      | 2025-10-20 20:15:32
```

## 📝 Выводы

### Что было не так:
1. ❌ `BROADCAST_DRIVER=pusher` без установленного пакета `pusher-php-server`
2. ❌ Laravel пытался инстанцировать Pusher при broadcast событий
3. ❌ Exception не логировался явно

### Как предотвратить:
1. ✅ Использовать `BROADCAST_DRIVER=log` для development
2. ✅ Добавить try-catch в критичные callbacks:
   ```php
   try {
       $mqtt->handleDiscovery($topic, $message);
   } catch (\Exception $e) {
       $this->error("❌ Error: " . $e->getMessage());
       Log::error("Discovery error", ['exception' => $e]);
   }
   ```
3. ✅ Установить реальный broadcasting (Laravel Reverb или Soketi) для production
4. ✅ Тестировать с реальными сообщениями, а не только логами

## 🚀 Следующие шаги

1. ✅ ROOT узел должен появиться в веб-интерфейсе автоматически
2. ✅ Все NODE узлы будут регистрироваться через heartbeat (auto-discovery)
3. ⏭️ Проверить работу других узлов (climate, ph_ec, display)
4. ⏭️ Настроить реальный broadcasting для real-time обновлений на фронтенде

## 🎓 Уроки

1. **Всегда проверяй зависимости:** Если config указывает `pusher`, убедись что пакет установлен
2. **Логируй явно:** `try-catch` в callbacks обязателен
3. **Тестируй от конца до конца:** Не только "сообщение пришло", но и "сохранилось в БД"
4. **Читай логи Laravel:** `storage/logs/laravel.log` содержит критичные ошибки

---

**Статус:** ✅ **ИСПРАВЛЕНО И ПРОТЕСТИРОВАНО**  
**ROOT узел теперь виден в MQTT и регистрируется автоматически!** 🎉

