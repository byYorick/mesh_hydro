# Диагностика проблем с передачей конфигурации

## Проблема: Config не доходит до pH ноды

### Симптомы (из логов Root node):
```
I (191666718) mqtt_manager: MQTT data received: hydro/config/ph_3f0c00
I (191666719) data_router: MQTT data received: hydro/config/ph_3f0c00 (153 bytes)
I (191666723) data_router: Forwarding config to ph_3f0c00
...
W (191680301) node_registry: Node ph_3f0c00 TIMEOUT -> OFFLINE (elapsed: 22389 ms)
```

**Root node получает config от MQTT, но pH нода не отвечает и уходит в TIMEOUT.**

---

## Диагностика по шагам

### Шаг 1: Проверить статус pH ноды

**Откройте monitor pH ноды:**
```bash
cd c:\esp\hydro\mesh\mesh_hydro\node_ph
idf.py monitor -p COM4
```

**Проверьте:**
1. ✅ Нода загружается без ошибок?
2. ✅ Подключается к mesh сети?
3. ✅ Отправляет телеметрию?

**Ожидаемые логи:**
```
I (12345) mesh_manager: Mesh connected
I (23456) ph_manager: Telemetry sent
I (34567) ph_manager: Heartbeat sent
```

---

### Шаг 2: Отправить config и смотреть логи pH ноды

**В frontend:**
1. Откройте узел `ph_3f0c00`
2. Раскройте "🔧 Управление конфигурацией"
3. Измените pH target на `6.8`
4. Нажмите "Сохранить конфигурацию"

**В логах pH ноды должно появиться:**
```
I (45678) mesh_manager: ✓ Mesh data received: XXX bytes
I (45679) app_main: === JSON PARSING DEBUG ===
I (45680) app_main: Data length: XXX
I (45681) app_main: Data: {"type":"config","node_id":"ph_3f0c00","config":{...}}
I (45682) app_main: JSON parsed successfully
I (45683) app_main: Message type: 2
I (45684) ph_manager: Config update received
I (45685) ph_manager: pH target updated: 6.80
I (45686) ph_manager: Configuration saved to NVS
I (45687) ph_manager: Config sent to ROOT (XXX bytes)
I (45688) ph_manager: Config confirmation sent to server
```

**Если логов нет:**
- Нода не получает сообщения через mesh
- Проблема в Root node (не пересылает)
- Проблема в mesh connectivity

---

### Шаг 3: Проверить Root node

**В логах Root node проверьте:**

**3.1. Получает ли Root MQTT сообщение?**
```
I (XXX) mqtt_manager: MQTT data received: hydro/config/ph_3f0c00
I (XXX) data_router: MQTT data received: hydro/config/ph_3f0c00 (153 bytes)
```
✅ Если да → Root получает от MQTT

**3.2. Пытается ли Root отправить на pH ноду?**
```
I (XXX) data_router: Forwarding config to ph_3f0c00
```
✅ Если да → Root пытается отправить

**3.3. Отправляет ли Root через mesh?**
Ищите в `mesh_manager.c`:
```
I (XXX) mesh_manager: Sending to node: ph_3f0c00
```
❌ Если нет → проблема в `mesh_manager_send_to_node()`

**3.4. Узел в реестре?**
```
I (XXX) node_registry: Node ph_3f0c00 found in registry
```
❌ Если нет → pH нода не зарегистрирована

---

### Шаг 4: Проверить Backend

**Проверьте логи backend:**
```bash
cd c:\esp\hydro\mesh\mesh_hydro\server
docker-compose logs -f mqtt_listener | grep -E "config|Config"
```

**Ожидаемые логи:**
```
[INFO] 📋 Config sent: ph_3f0c00
[INFO] 📋 MQTT Published: hydro/config/ph_3f0c00
```

**Если дублирование:**
```
[INFO] 📋 Config sent: ph_3f0c00  <-- 1
[INFO] 📋 Config sent: ph_3f0c00  <-- 2 (дубль!)
[INFO] 📋 Config sent: ph_3f0c00  <-- 3 (дубль!)
```
⚠️ **Проблема:** Frontend отправляет несколько раз → добавлен debounce

---

## Решения частых проблем

### Проблема 1: pH нода не получает config через mesh

**Причина:** Root node не может отправить на pH ноду

**Решение:**
```c
// В root_node/components/data_router/data_router.c
// Проверить функцию forward_config_to_node()

// Проверить что MAC адрес pH ноды известен
node_info_t *node = node_registry_find_by_id(node_id);
if (!node) {
    ESP_LOGE(TAG, "Node %s not found in registry!", node_id);
    return;
}

// Отправить через mesh
esp_err_t err = mesh_manager_send_to_node(node->mac, data, len);
if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to send to node: %s", esp_err_to_name(err));
}
```

---

### Проблема 2: pH нода уходит в TIMEOUT

**Причина:** Нода не отвечает на ping/heartbeat

**Решение:**
1. Увеличить `NODE_TIMEOUT_MS` в `node_registry.h`:
   ```c
   #define NODE_TIMEOUT_MS 60000  // 60 секунд
   ```

2. Проверить интервал отправки heartbeat в pH ноде:
   ```c
   // В ph_manager.c
   #define HEARTBEAT_INTERVAL_MS 10000  // Должно быть < NODE_TIMEOUT_MS
   ```

---

### Проблема 3: Config дублируется (8 раз)

**Причина:** Frontend отправляет несколько раз при клике

**Решение:** ✅ Добавлен debounce в `PhDetail.vue`:
```javascript
const configSaveDebounce = ref(false)

const saveConfig = async () => {
  if (configSaveDebounce.value) {
    toast.warning('⏳ Подождите, сохранение уже выполняется...')
    return
  }
  
  configSaveDebounce.value = true
  // ...
  
  setTimeout(() => {
    configSaveDebounce.value = false
  }, 2000)
}
```

---

### Проблема 4: Config_response не доходит до backend

**Причина:** pH нода не отправляет или Root не пересылает

**Диагностика:**

**4.1. pH нода отправляет?**
```
I (XXX) ph_manager: Config sent to ROOT (XXX bytes)
I (XXX) ph_manager: Config confirmation sent to server
```

**4.2. Root получает?**
```
I (XXX) mesh_manager: ✓ Mesh data received: XXX bytes
I (XXX) data_router: 📋 Response from ph_3f0c00 → MQTT
```

**4.3. Root публикует в MQTT?**
```
I (XXX) mqtt_manager: ✅ MQTT Published: hydro/config_response/ph_3f0c00
```

**4.4. Backend получает?**
```
[INFO] 📋 [CONFIG_RESPONSE] hydro/config_response/ph_3f0c00
[INFO] 📋 handleConfigResponse called
[INFO] 📋 Config response processed successfully: ph_3f0c00
```

**4.5. Frontend получает через WebSocket?**
```javascript
// В консоли браузера (F12)
Config update confirmed from node: {node_id: 'ph_3f0c00', config: {...}}
```

---

## Текущий статус

### Что работает:
✅ Frontend отправляет config на backend
✅ Backend сохраняет в БД
✅ Backend публикует в MQTT (`hydro/config/ph_3f0c00`)
✅ Root node получает от MQTT
✅ Root node пытается отправить на pH ноду

### Что не работает:
❌ pH нода не получает config через mesh
❌ Нода уходит в TIMEOUT

### Что проверить:
1. **Прошита ли pH нода с новым кодом?**
   ```bash
   cd c:\esp\hydro\mesh\mesh_hydro\node_ph
   idf.py build
   idf.py flash -p COM4
   ```

2. **Подключена ли pH нода к mesh?**
   ```
   I (XXX) mesh_manager: Mesh connected
   ```

3. **Отправляет ли pH нода телеметрию?**
   ```
   I (XXX) ph_manager: Telemetry sent
   ```

4. **Зарегистрирована ли pH нода в Root registry?**
   ```
   I (XXX) node_registry: Node ph_3f0c00 is now ONLINE
   ```

---

## Следующие шаги

1. **Дождаться окончания сборки pH ноды**
2. **Прошить pH ноду:**
   ```bash
   cd c:\esp\hydro\mesh\mesh_hydro\node_ph
   idf.py flash -p COM4 monitor
   ```
3. **Отправить config с frontend**
4. **Смотреть логи pH ноды** — должны появиться:
   ```
   I (XXX) ph_manager: Config update received
   I (XXX) ph_manager: Configuration saved to NVS
   I (XXX) ph_manager: Config confirmation sent to server
   ```
5. **Проверить frontend** — должен появиться:
   ```
   ✅ Конфигурация применена
   Узел подтвердил получение и применение конфигурации
   ```

---

## Полезные команды

### Мониторинг всех компонентов одновременно

**Терминал 1: Root node**
```bash
cd c:\esp\hydro\mesh\mesh_hydro\root_node
idf.py monitor -p COM3
```

**Терминал 2: pH node**
```bash
cd c:\esp\hydro\mesh\mesh_hydro\node_ph
idf.py monitor -p COM4
```

**Терминал 3: MQTT**
```bash
mosquitto_sub -h localhost -t "hydro/#" -v
```

**Терминал 4: Backend logs**
```bash
cd c:\esp\hydro\mesh\mesh_hydro\server
docker-compose logs -f mqtt_listener
```

**Браузер: Frontend console (F12)**
```javascript
// Смотреть WebSocket события
```

---

## Контрольный список ✅

- [ ] pH нода собрана с новым кодом
- [ ] pH нода прошита
- [ ] pH нода подключена к mesh
- [ ] pH нода отправляет телеметрию
- [ ] Root node видит pH ноду в registry
- [ ] Root node получает config от MQTT
- [ ] Root node отправляет config на pH ноду
- [ ] pH нода получает config
- [ ] pH нода применяет и сохраняет в NVS
- [ ] pH нода отправляет config_response
- [ ] Root node получает config_response
- [ ] Root node публикует в MQTT
- [ ] Backend получает config_response
- [ ] Backend broadcast через WebSocket
- [ ] Frontend получает подтверждение
- [ ] Frontend показывает "✅ Конфигурация применена"

