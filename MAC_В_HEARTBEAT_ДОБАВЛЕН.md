# ✅ MAC АДРЕС ДОБАВЛЕН В HEARTBEAT

**Дата:** 19 октября 2025  
**Проблема:** MAC адрес не отображается в деталях узла

---

## 🐛 ПРОБЛЕМА

### Причина:
1. Узлы отправляют Discovery **только один раз** при подключении
2. Узлы были созданы через Heartbeat (без MAC)
3. MAC приходил только в Discovery, но backend его не обновлял

### Результат:
```sql
SELECT node_id, mac_address FROM nodes;

node_id              | mac_address
---------------------|------------------
climate_001          | NULL          ❌
ph_ec_3cfd01         | NULL          ❌
root_98a316f5fde8    | 98:A3:16:F5:FD:E8  ✅
```

**Frontend отображает:** "Не указан"

---

## ✅ РЕШЕНИЕ

### Добавлен MAC адрес в Heartbeat (каждые 5 секунд)!

Теперь узлы отправляют MAC не только в Discovery, но и **в каждом Heartbeat**.

---

## 🔧 ИЗМЕНЕНИЯ

### 1. Climate узел - climate_controller.c (строки 301-328):

**БЫЛО:**
```c
static void send_heartbeat(void) {
    // ...
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
            "{\"type\":\"heartbeat\","
            "\"node_id\":\"%s\","
            "\"uptime\":%lu,"
            "\"heap_free\":%lu,"
            "\"rssi_to_parent\":%d}",
            // БЕЗ MAC! ❌
            s_config->base.node_id,
            (unsigned long)uptime,
            (unsigned long)heap_free,
            rssi);
}
```

**СТАЛО:**
```c
static void send_heartbeat(void) {
    // Получение MAC адреса
    uint8_t mac[6];
    mesh_manager_get_mac(mac);
    
    // ...
    snprintf(heartbeat_msg, sizeof(heartbeat_msg),
            "{\"type\":\"heartbeat\","
            "\"node_id\":\"%s\","
            "\"mac_address\":\"%02X:%02X:%02X:%02X:%02X:%02X\","  ← ДОБАВЛЕНО!
            "\"uptime\":%lu,"
            "\"heap_free\":%lu,"
            "\"rssi_to_parent\":%d}",
            s_config->base.node_id,
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],  ← ДОБАВЛЕНО!
            (unsigned long)uptime,
            (unsigned long)heap_free,
            rssi);
}
```

---

### 2. pH/EC узел - ph_ec_manager.c (строки 429-454):

**Такое же изменение!**

MAC адрес теперь в каждом heartbeat.

---

### 3. Backend - MqttService.php (строки 358-374):

**БЫЛО:**
```php
} else {
    // Обновление существующего узла
    $metadata = $node->metadata ?? [];
    
    // heap_free, rssi, uptime
    // ...
    
    $node->update([
        'online' => true,
        'last_seen_at' => now(),
        'metadata' => $metadata,
    ]);
    // БЕЗ mac_address! ❌
}
```

**СТАЛО:**
```php
} else {
    // Обновление существующего узла
    $metadata = $node->metadata ?? [];
    
    // heap_free, rssi, uptime, MAC
    // ...
    
    // Обновляем MAC адрес (если пришёл в heartbeat)
    if (isset($data['mac_address']) || isset($data['mac'])) {
        $metadata['mac_address'] = $data['mac_address'] ?? $data['mac'];
    }
    
    $updateData = [
        'online' => true,
        'last_seen_at' => now(),
        'metadata' => $metadata,
    ];
    
    // Обновляем также поле mac_address в таблице nodes (если пришло)
    if (isset($data['mac_address']) || isset($data['mac'])) {
        $updateData['mac_address'] = $data['mac_address'] ?? $data['mac'];  ← ДОБАВЛЕНО!
    }
    
    $node->update($updateData);
}
```

---

## 📊 НОВАЯ СТРУКТУРА HEARTBEAT

### Теперь Heartbeat содержит:

```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "mac_address": "00:4B:12:37:D5:A4",  ← НОВОЕ!
  "uptime": 120,
  "heap_free": 143004,
  "rssi_to_parent": -45
}
```

**Отправляется каждые 5 секунд!**

---

## 🚀 ДЕЙСТВИЯ

### Что запущено:

1. ✅ Backend перезапущен (уже готов!)
2. ⏳ Climate узел компилируется и прошивается (COM10)
3. ⏳ pH/EC узел нужно прошить (COM9)

---

## 🧪 ПРОВЕРКА ЧЕРЕЗ 3-5 МИНУТ

### 1. После прошивки climate узла:

Подожди 10 секунд после прошивки, затем проверь БД:

```bash
docker exec hydro_backend php artisan tinker --execute="echo App\Models\Node::where('node_id', 'climate_001')->first()->mac_address;"
```

**Должно быть:**
```
00:4B:12:37:D5:A4  ← MAC появится!
```

---

### 2. Открой Dashboard → Детали узла:

```
http://localhost:3000
```

Перейди в детали `climate_001` → Метаданные узла

**Должно появиться:**
```
🔌 MAC адрес
   00:4B:12:37:D5:A4
```

---

## ⏱️ ЧАСТОТА ОБНОВЛЕНИЯ

### ESP32 отправляет MAC:
- **В Discovery:** 1 раз при подключении
- **В Heartbeat:** каждые 5 секунд ← НОВОЕ!

### Backend обновляет MAC:
- **При каждом Heartbeat с MAC** ✅

### Результат:
**MAC обновится через 5-10 секунд после прошивки!**

---

## 📋 СТАТУС ПРОШИВОК

### ✅ Backend:
- Перезапущен
- Готов обрабатывать MAC из heartbeat

### ⏳ Climate (COM10):
- Компилируется
- Будет прошит автоматически
- После прошивки MAC появится в БД через 5-10 секунд

### ⏳ pH/EC (COM9):
- Нужно прошить после climate
- Код уже изменён (MAC в heartbeat добавлен)

---

**ПОДОЖДИ 3-5 МИНУТ ПОКА КОМПИЛЯЦИЯ ЗАВЕРШИТСЯ!** ⏳🔌

**ПОСЛЕ ПРОШИВКИ MAC АДРЕСА ПОЯВЯТСЯ АВТОМАТИЧЕСКИ!** ✅

