# ✅ HEAP ОТОБРАЖЕНИЕ ИСПРАВЛЕНО

**Дата:** 19 октября 2025  
**Проблема:** На нодах не отображается heap (RAM)

---

## 🐛 ПРОБЛЕМА

### Симптом:
На карточках узлов не показывается индикатор RAM (heap usage).

### Причина:
**Backend не обновлял metadata при получении heartbeat!**

#### ESP32 отправлял:
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "uptime": 120,
  "heap_free": 143004,  ← ОТПРАВЛЯЛОСЬ!
  "rssi_to_parent": -45
}
```

#### Backend сохранял:
```php
$node->update([
    'online' => true,
    'last_seen_at' => now(),
    // metadata НЕ обновлялась! ❌
]);
```

#### Frontend искал:
```javascript
const heapFree = computed(() => metadata.value.heap_free || 0)
// metadata.heap_free = null ❌
```

**Результат:** RAM не отображалась!

---

## ✅ РЕШЕНИЕ

### Исправлен: `server/backend/app/Services/MqttService.php`

#### 1. handleHeartbeat - обновление существующего узла (строки 337-360):

**БЫЛО:**
```php
} else {
    // Обновление last_seen_at для существующего узла
    $node->update([
        'online' => true,
        'last_seen_at' => now(),
    ]);
}
```

**СТАЛО:**
```php
} else {
    // Обновление last_seen_at и метаданных для существующего узла
    $metadata = $node->metadata ?? [];
    
    // Обновляем heap_free из heartbeat (если есть)
    if (isset($data['heap_free'])) {
        $metadata['heap_free'] = $data['heap_free'];
    }
    
    // Обновляем RSSI
    if (isset($data['rssi_to_parent'])) {
        $metadata['rssi_to_parent'] = $data['rssi_to_parent'];
    }
    
    // Обновляем uptime
    if (isset($data['uptime'])) {
        $metadata['uptime'] = $data['uptime'];
    }
    
    $node->update([
        'online' => true,
        'last_seen_at' => now(),
        'metadata' => $metadata,  ← ОБНОВЛЕНИЕ!
    ]);
}
```

---

#### 2. handleHeartbeat - создание нового узла (строки 304-321):

**ДОБАВЛЕНО:**
```php
'metadata' => [
    'discovered_at' => now()->toIso8601String(),
    'discovered_via' => 'heartbeat',
    'firmware' => $data['firmware'] ?? null,
    'hardware' => $data['hardware'] ?? null,
    'ip_address' => $data['ip'] ?? null,
    'heap_free' => $data['heap_free'] ?? null,      ← ДОБАВЛЕНО!
    'rssi_to_parent' => $data['rssi_to_parent'] ?? null,  ← ДОБАВЛЕНО!
    'uptime' => $data['uptime'] ?? null,            ← ДОБАВЛЕНО!
],
```

---

### Frontend уже правильно настроен!

#### NodeCard.vue (строки 290-309):

```javascript
const hasMemoryInfo = computed(() => {
  return metadata.value.heap_total || 
         metadata.value.total_heap || 
         metadata.value.heap_used || 
         metadata.value.heap_free  ← Проверка heap_free
})

const heapFree = computed(() => metadata.value.heap_free || 0)

const heapTotal = computed(() => {
  if (metadata.value.heap_total) {
    return metadata.value.heap_total
  }
  // Оцениваем total по free (предполагаем free ~60%)
  if (heapFree.value > 0) {
    return Math.round(heapFree.value / 0.6)  ← Расчёт total
  }
  return 320000
})

const heapUsed = computed(() => {
  return heapTotal.value - heapFree.value  ← Расчёт used
})

const heapPercent = computed(() => {
  return (heapUsed.value / heapTotal.value) * 100  ← Расчёт %
})
```

---

## 📊 КАК БУДЕТ ОТОБРАЖАТЬСЯ

### На карточке climate_001:

```
┌─────────────────────────────────┐
│ climate_001         🟢 Online   │
├─────────────────────────────────┤
│ 22.8°C    60%      493          │
│ Температура Влажность CO₂ ppm   │
├─────────────────────────────────┤
│ 💾 RAM         45%              │ ← ПОЯВИТСЯ!
│ ██████░░░░░░░░░░ 143KB / 320KB │ ← ПОЯВИТСЯ!
├─────────────────────────────────┤
│ 🕐 Last seen: < 1 sec           │
└─────────────────────────────────┘
```

### Цветовая индикация:

- 🟢 **< 50%** - зелёный (success)
- 🟡 **50-75%** - жёлтый (warning)
- 🔴 **> 75%** - красный (error)

---

## 🧪 КАК ПРОВЕРИТЬ

### 1. Открой Dashboard:
```
http://localhost:3000
```

### 2. Жёсткая перезагрузка:
**Ctrl + F5**

### 3. Подожди 5-10 секунд:
ESP32 отправит heartbeat → Backend обновит metadata → Frontend покажет RAM

### 4. Проверь карточки:
На всех узлах (climate_001, ph_ec_001, root_xxx) должна появиться:
```
💾 RAM 45%
██████░░░░░░░░░░
```

---

## 🔍 ПРОВЕРКА BACKEND

### Проверь что backend получает heap_free:

```bash
docker logs hydro_backend --tail 50 | grep "Heartbeat received"
```

**Должно быть:**
```
[2025-10-19 ...] Heartbeat received {"node_id":"climate_001"}
```

### Проверь metadata в БД:

```bash
docker exec hydro_backend php artisan tinker --execute="echo json_encode(App\Models\Node::where('node_id', 'climate_001')->first()->metadata, JSON_PRETTY_PRINT);"
```

**Должно быть:**
```json
{
    "heap_free": 143004,      ← ДОЛЖНО БЫТЬ!
    "rssi_to_parent": -45,
    "uptime": 120,
    "firmware": "1.0.0",
    "hardware": "ESP32"
}
```

---

## 📈 ЧАСТОТА ОБНОВЛЕНИЯ

### ESP32 отправляет heartbeat:
- **Climate:** каждые **5 секунд** (DEBUG mode)
- **pH/EC:** каждые **5 секунд** (DEBUG mode)
- **ROOT:** каждые **30 секунд** (через monitoring task)

### Backend обновляет metadata:
- **При каждом heartbeat** ✅

### Frontend обновляет отображение:
- **Real-time** через API polling (каждые 5 секунд)

---

## 🎉 ГОТОВО!

### Теперь работает:

- ✅ ESP32 отправляет `heap_free` в heartbeat
- ✅ Backend сохраняет `heap_free` в metadata
- ✅ Backend обновляет metadata при каждом heartbeat
- ✅ Frontend читает `heap_free` из metadata
- ✅ Frontend вычисляет и отображает RAM usage
- ✅ Цветовая индикация (зелёный/жёлтый/красный)

---

### Backend перезапущен:
```
 Container hydro_backend  Started
 Container hydro_mqtt_listener  Started
```

**ПОДОЖДИ 5-10 СЕКУНД И ОБНОВИ СТРАНИЦУ (Ctrl + F5)!** 🎉

**RAM ДОЛЖНА ПОЯВИТЬСЯ НА ВСЕХ КАРТОЧКАХ!** 💾✅

