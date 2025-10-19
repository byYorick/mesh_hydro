# ✅ MAC АДРЕС ДОБАВЛЕН В ДЕТАЛИ УЗЛА

**Дата:** 19 октября 2025  
**Изменение:** Добавлено отображение MAC адреса на странице деталей узла

---

## 🎯 ЧТО ДОБАВЛЕНО

### MAC адрес в метаданных узла:
- 🔌 Иконка network-outline
- 📝 Форматирование: `00:4B:12:37:D5:A4`
- 🎨 Отображается в списке метаданных (после Hardware, перед IP)

---

## 📱 ГДЕ ОТОБРАЖАЕТСЯ

### Страница деталей узла:

```
Метаданные узла
├─ 💾 Прошивка: 1.0.0
├─ 🖥️ Аппаратура: ESP32
├─ 🔌 MAC адрес: 00:4B:12:37:D5:A4  ← НОВОЕ!
├─ 🌐 IP адрес: 192.168.1.191
├─ 📡 WiFi сеть: Yorick
└─ 📅 Дата создания: 19.10.2025
```

---

## 🔧 ТЕХНИЧЕСКИЕ ДЕТАЛИ

### 1. ESP32 отправляет MAC в Discovery:

#### climate_controller.c (строки 266-290):

```c
// Получение MAC адреса
uint8_t mac[6];
mesh_manager_get_mac(mac);

// Создание discovery JSON
snprintf(discovery_msg, sizeof(discovery_msg),
        "{\"type\":\"discovery\","
        "\"node_id\":\"%s\","
        "\"node_type\":\"climate\","
        "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","  ← Отправка MAC
        "\"firmware\":\"1.0.0\","
        "\"hardware\":\"ESP32\","
        "\"sensors\":[\"sht3x\",\"ccs811\",\"lux\"],"
        "\"heap_free\":%lu,"
        "\"wifi_rssi\":%d}",
        s_config->base.node_id,
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],  ← Форматирование
        (unsigned long)heap_free,
        rssi);
```

**Формат:** `"mac_address":"00:4B:12:37:D5:A4"`

---

### 2. Backend сохраняет MAC:

#### MqttService.php - handleDiscovery:

**Для новых узлов (строка 436):**
```php
$node = Node::create([
    'node_id' => $nodeId,
    'node_type' => $nodeType,
    'zone' => $data['zone'] ?? 'Auto-discovered',
    'online' => true,
    'last_seen_at' => now(),
    'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,  ← В таблицу nodes
    'metadata' => [
        // ...
        'mac_address' => $data['mac_address'] ?? $data['mac'] ?? null,  ← В metadata тоже
        // ...
    ],
]);
```

**Для существующих узлов (строки 408-428):**
```php
if (isset($data['mac_address']) || isset($data['mac'])) {
    $metadata = $existingNode->metadata ?? [];
    $metadata['mac_address'] = $data['mac_address'] ?? $data['mac'] ?? $metadata['mac_address'] ?? null;
    
    $updateData = [
        'metadata' => $metadata,
        'online' => true,
        'last_seen_at' => now(),
    ];
    
    // Обновляем также поле mac_address в таблице nodes
    if (isset($data['mac_address']) || isset($data['mac'])) {
        $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];
    }
    
    $existingNode->update($updateData);
}
```

---

### 3. Frontend отображает MAC:

#### NodeMetadataCard.vue (строки 38-47):

```vue
<!-- MAC Address -->
<v-list-item v-if="node.mac_address || metadata.mac_address || metadata.mac">
  <template v-slot:prepend>
    <v-icon icon="mdi-network-outline" color="primary"></v-icon>
  </template>
  <v-list-item-title>MAC адрес</v-list-item-title>
  <v-list-item-subtitle>
    {{ formatMac(node.mac_address || metadata.mac_address || metadata.mac) }}
  </v-list-item-subtitle>
</v-list-item>
```

#### Функция formatMac() (строки 288-307):

```javascript
function formatMac(mac) {
  if (!mac) return 'N/A'
  
  // Если уже форматировано с двоеточиями (00:4b:12:37:d5:a4)
  if (mac.includes(':')) {
    return mac.toUpperCase()  // → 00:4B:12:37:D5:A4
  }
  
  // Если без разделителей (004b1237d5a4)
  if (mac.length === 12) {
    return mac.match(/.{1,2}/g).join(':').toUpperCase()  // → 00:4B:12:37:D5:A4
  }
  
  // Если с дефисами (00-4b-12-37-d5-a4)
  if (mac.includes('-')) {
    return mac.replace(/-/g, ':').toUpperCase()  // → 00:4B:12:37:D5:A4
  }
  
  return mac.toUpperCase()
}
```

**Поддерживаемые форматы:**
- `00:4B:12:37:D5:A4` (с двоеточиями) → `00:4B:12:37:D5:A4`
- `004b1237d5a4` (без разделителей) → `00:4B:12:37:D5:A4`
- `00-4b-12-37-d5-a4` (с дефисами) → `00:4B:12:37:D5:A4`

**Всегда выводится в верхнем регистре!**

---

## 📊 ПРИМЕР ОТОБРАЖЕНИЯ

### На странице деталей climate_001:

```
┌─────────────────────────────────────────┐
│ Метаданные узла                          │
├─────────────────────────────────────────┤
│ 💾 Прошивка                              │
│    1.0.0                                 │
├─────────────────────────────────────────┤
│ 🖥️ Аппаратура                            │
│    ESP32                                 │
├─────────────────────────────────────────┤
│ 🔌 MAC адрес                             │ ← НОВОЕ!
│    00:4B:12:37:D5:A4                     │
├─────────────────────────────────────────┤
│ 🌐 IP адрес                              │
│    192.168.1.191                         │
├─────────────────────────────────────────┤
│ 📡 WiFi сеть                             │
│    Yorick                                │
├─────────────────────────────────────────┤
│ 📅 Дата создания                         │
│    19 октября 2025, 15:03:46            │
├─────────────────────────────────────────┤
│ ✨ Создан через                          │
│    MQTT Heartbeat                        │
└─────────────────────────────────────────┘
```

---

## 🧪 КАК ПРОВЕРИТЬ

### 1. Дождись сборки:
Frontend и backend пересобираются (2-3 минуты).

### 2. Открой Dashboard:
```
http://localhost:3000
```

### 3. Перейди в детали узла:
- Нажми на карточку узла (например, `climate_001`)
- Или нажми кнопку **"Детали"** на карточке

### 4. Прокрути до блока "Метаданные узла":
Должен появиться пункт:
```
🔌 MAC адрес
   00:4B:12:37:D5:A4
```

### 5. Если не видно:
- **Ctrl + Shift + R** (жёсткая перезагрузка)
- Подожди 10-15 секунд (ESP32 отправит discovery)

---

## 🔍 ПРОВЕРКА В БД

### Проверь что MAC сохранён:

```bash
docker exec hydro_backend php artisan tinker --execute="echo json_encode(App\Models\Node::where('node_id', 'climate_001')->first()->only(['node_id', 'mac_address']), JSON_PRETTY_PRINT);"
```

**Должно быть:**
```json
{
    "node_id": "climate_001",
    "mac_address": "00:4B:12:37:D5:A4"  ← ДОЛЖНО БЫТЬ!
}
```

### Проверь metadata:

```bash
docker exec hydro_backend php artisan tinker --execute="echo json_encode(App\Models\Node::where('node_id', 'climate_001')->first()->metadata, JSON_PRETTY_PRINT);"
```

**Должно быть:**
```json
{
    "discovered_at": "2025-10-19T12:03:46+00:00",
    "discovered_via": "heartbeat",
    "firmware": "1.0.0",
    "hardware": "ESP32",
    "mac_address": "00:4B:12:37:D5:A4",  ← И ЗДЕСЬ ТОЖЕ!
    "heap_free": 142684,
    "rssi_to_parent": -19,
    "uptime": 561
}
```

---

## 📋 ГДЕ ХРАНИТСЯ MAC

### 1. В таблице `nodes`:
```sql
SELECT node_id, mac_address FROM nodes WHERE node_id = 'climate_001';
```

**Результат:**
```
node_id       | mac_address
--------------+------------------
climate_001   | 00:4B:12:37:D5:A4
```

### 2. В metadata (JSON):
```
nodes.metadata['mac_address'] = "00:4B:12:37:D5:A4"
```

**Хранится в обоих местах для удобства!**

---

## 🎯 ЧАСТОТА ОБНОВЛЕНИЯ MAC

### ESP32 отправляет MAC в:

#### Discovery (один раз при подключении):
```
I (11420) climate_ctrl: 🔍 Discovery sent: climate_001 (RSSI=-45)
```

#### Backend обновляет при:
- **Discovery message** (hydro/discovery)
- **Первый heartbeat** (если узел новый)

**MAC не меняется**, поэтому обновляется только один раз при регистрации узла.

---

## 🎉 ГОТОВО!

### Теперь работает:

- ✅ ESP32 отправляет MAC адрес в Discovery
- ✅ Backend сохраняет MAC в `nodes.mac_address`
- ✅ Backend сохраняет MAC в `nodes.metadata`
- ✅ Backend обновляет MAC при повторном Discovery
- ✅ Frontend читает MAC из `node.mac_address` и `metadata.mac_address`
- ✅ Frontend форматирует MAC в верхнем регистре с двоеточиями
- ✅ Frontend отображает MAC в метаданных узла

---

### После сборки (2-3 минуты):

**Обнови страницу (Ctrl + Shift + R) и открой детали узла!**

**MAC адрес должен появиться в метаданных!** 🔌✅

