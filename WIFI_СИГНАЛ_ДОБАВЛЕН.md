# ✅ Wi-Fi СИГНАЛ (RSSI) ДОБАВЛЕН НА DASHBOARD

**Дата:** 19 октября 2025  
**Изменение:** Добавлен индикатор уровня Wi-Fi сигнала для всех узлов

---

## 🎯 ЧТО ДОБАВЛЕНО

### Индикатор Wi-Fi сигнала (RSSI):
- 📶 Иконка Wi-Fi (4 уровня силы)
- 📊 Прогресс-бар с цветовой индикацией
- 🔢 Значение в dBm (-30 до -90)
- 🎨 Цвет: зелёный/жёлтый/красный

---

## 📱 КАК ВЫГЛЯДИТ НА КАРТОЧКЕ

### БЫЛО (без Wi-Fi):
```
┌─────────────────────────────────┐
│ climate_001         🟢 Online   │
├─────────────────────────────────┤
│ 22.8°C    60%      493          │
├─────────────────────────────────┤
│ 💾 RAM         45%              │
│ ██████░░░░░░░░░░                │
├─────────────────────────────────┤
│ 🕐 Last seen: < 1 sec           │
└─────────────────────────────────┘
```

### СТАЛО (с Wi-Fi):
```
┌─────────────────────────────────┐
│ climate_001         🟢 Online   │
├─────────────────────────────────┤
│ 22.8°C    60%      493          │
├─────────────────────────────────┤
│ 💾 RAM         45%              │
│ ██████░░░░░░░░░░                │
│                                 │
│ 📶 WiFi        -45 dBm          │ ← НОВОЕ!
│ ████████████░░░░                │ ← НОВОЕ!
├─────────────────────────────────┤
│ 🕐 Last seen: < 1 sec           │
└─────────────────────────────────┘
```

---

## 📊 УРОВНИ СИГНАЛА

### Отличный (🟢 зелёный, > 60%):
```
RSSI: -30 до -50 dBm
Иконка: 📶 mdi-wifi-strength-4 (4 полоски)
Цвет: success (зелёный)
Качество: отличное
```

### Хороший (🟡 жёлтый, 30-60%):
```
RSSI: -50 до -70 dBm
Иконка: 📶 mdi-wifi-strength-2 или 3
Цвет: warning (жёлтый)
Качество: среднее
```

### Плохой (🔴 красный, < 30%):
```
RSSI: -70 до -90 dBm
Иконка: 📶 mdi-wifi-strength-1 (1 полоска)
Цвет: error (красный)
Качество: плохое
```

---

## 🔧 ТЕХНИЧЕСКИЕ ДЕТАЛИ

### 1. ESP32 отправляет RSSI:

#### Heartbeat (каждые 5 секунд):
```json
{
  "type": "heartbeat",
  "node_id": "climate_001",
  "uptime": 120,
  "heap_free": 143004,
  "rssi_to_parent": -45  ← RSSI к родительскому узлу (ROOT)
}
```

📍 Файлы:
- `node_climate/components/climate_controller/climate_controller.c` (строка 308)
- `node_ph_ec/components/ph_ec_manager/ph_ec_manager.c` (строка 436)

#### Функция получения RSSI:
```c
static int8_t get_rssi_to_parent(void) {
    return mesh_manager_get_parent_rssi();
}
```

---

### 2. Backend сохраняет RSSI:

#### MqttService.php - handleHeartbeat (строки 340-348):

```php
// Обновляем RSSI
if (isset($data['rssi_to_parent'])) {
    $metadata['rssi_to_parent'] = $data['rssi_to_parent'];
}

$node->update([
    'online' => true,
    'last_seen_at' => now(),
    'metadata' => $metadata,  // ← rssi_to_parent сохраняется!
]);
```

---

### 3. Frontend отображает RSSI:

#### NodeCard.vue - Template (строки 98-113):

```vue
<!-- WiFi Signal (RSSI) -->
<div v-if="hasRssi" class="mb-2">
  <div class="d-flex justify-space-between mb-1">
    <span class="text-caption">
      <v-icon :icon="rssiIcon" size="x-small" class="mr-1"></v-icon>
      WiFi
    </span>
    <span class="text-caption">{{ rssiValue }} dBm</span>
  </div>
  <v-progress-linear
    :model-value="rssiPercent"
    :color="getRssiColor(rssiPercent)"
    height="6"
    rounded
  ></v-progress-linear>
</div>
```

#### NodeCard.vue - Script (строки 334-364):

```javascript
// Проверка наличия RSSI
const hasRssi = computed(() => {
  return metadata.value.rssi_to_parent != null || 
         metadata.value.wifi_rssi != null
})

// Значение RSSI
const rssiValue = computed(() => {
  return metadata.value.rssi_to_parent || 
         metadata.value.wifi_rssi || 0
})

// Преобразование RSSI (-90 до -30 dBm) в проценты (0-100%)
const rssiPercent = computed(() => {
  const rssi = rssiValue.value
  if (rssi === 0) return 0
  // -30 dBm = 100% (отлично), -90 dBm = 0% (плохо)
  const percent = Math.min(100, Math.max(0, (rssi + 90) * (100 / 60)))
  return percent
})

// Иконка Wi-Fi (4 уровня)
const rssiIcon = computed(() => {
  const percent = rssiPercent.value
  if (percent > 75) return 'mdi-wifi-strength-4'  // 4 полоски
  if (percent > 50) return 'mdi-wifi-strength-3'  // 3 полоски
  if (percent > 25) return 'mdi-wifi-strength-2'  // 2 полоски
  return 'mdi-wifi-strength-1'                    // 1 полоска
})

// Цвет прогресс-бара
function getRssiColor(percent) {
  if (percent > 60) return 'success'  // 🟢 зелёный
  if (percent > 30) return 'warning'  // 🟡 жёлтый
  return 'error'                       // 🔴 красный
}
```

---

## 📈 ФОРМУЛА РАСЧЁТА

### Преобразование dBm → Проценты:

```javascript
// RSSI: -30 (отлично) до -90 (плохо)
// Процент: 100% до 0%

percent = (RSSI + 90) * (100 / 60)

// Примеры:
// -30 dBm → (-30 + 90) * 1.67 = 100%  ← отлично
// -45 dBm → (-45 + 90) * 1.67 = 75%   ← хорошо
// -60 dBm → (-60 + 90) * 1.67 = 50%   ← средне
// -75 dBm → (-75 + 90) * 1.67 = 25%   ← плохо
// -90 dBm → (-90 + 90) * 1.67 = 0%    ← очень плохо
```

---

## 🧪 КАК ПРОВЕРИТЬ

### 1. Открой Dashboard:
```
http://localhost:3000
```

### 2. Жёсткая перезагрузка:
**Ctrl + F5**

### 3. Подожди 5-10 секунд:
ESP32 отправит heartbeat → Backend обновит metadata → Frontend покажет Wi-Fi

### 4. Проверь карточки:
На всех узлах (climate_001, ph_ec_001, root_xxx) должна появиться:
```
📶 WiFi        -45 dBm
████████████░░░░
```

---

## 🔍 ПРОВЕРКА В КОНСОЛИ БРАУЗЕРА

### Открой DevTools (F12) → Console:

```javascript
// Проверь metadata узла:
fetch('/api/nodes/climate_001')
  .then(r => r.json())
  .then(data => console.log(data.metadata))

// Должно быть:
{
  heap_free: 143004,
  rssi_to_parent: -45,  ← ДОЛЖНО БЫТЬ!
  uptime: 120,
  firmware: "1.0.0",
  hardware: "ESP32"
}
```

---

## 📊 ПРИМЕРЫ ОТОБРАЖЕНИЯ

### Отличный сигнал (-40 dBm):
```
┌─────────────────────────────────┐
│ 📶 WiFi        -40 dBm          │
│ ██████████████░░ (85%)          │
│ Цвет: 🟢 зелёный               │
│ Иконка: 📶 (4 полоски)         │
└─────────────────────────────────┘
```

### Средний сигнал (-60 dBm):
```
┌─────────────────────────────────┐
│ 📶 WiFi        -60 dBm          │
│ ████████░░░░░░░░ (50%)          │
│ Цвет: 🟡 жёлтый                │
│ Иконка: 📶 (2-3 полоски)       │
└─────────────────────────────────┘
```

### Плохой сигнал (-80 dBm):
```
┌─────────────────────────────────┐
│ 📶 WiFi        -80 dBm          │
│ ████░░░░░░░░░░░░ (17%)          │
│ Цвет: 🔴 красный                │
│ Иконка: 📶 (1 полоска)          │
└─────────────────────────────────┘
```

---

## 🎯 ЧАСТОТА ОБНОВЛЕНИЯ

### ESP32 отправляет RSSI:
- **Climate:** каждые **5 секунд** (heartbeat)
- **pH/EC:** каждые **5 секунд** (heartbeat)
- **ROOT:** каждые **30 секунд** (monitoring task)

### Backend обновляет rssi_to_parent:
- **При каждом heartbeat** ✅

### Frontend обновляет отображение:
- **Real-time** через API polling (каждые 5 секунд)

---

## 🎉 ГОТОВО!

### Теперь работает:

- ✅ ESP32 отправляет `rssi_to_parent` в heartbeat
- ✅ Backend сохраняет `rssi_to_parent` в metadata
- ✅ Backend обновляет metadata при каждом heartbeat
- ✅ Frontend читает `rssi_to_parent` из metadata
- ✅ Frontend вычисляет и отображает Wi-Fi сигнал
- ✅ Цветовая индикация (зелёный/жёлтый/красный)
- ✅ Динамическая иконка (1-4 полоски)

---

### Frontend перезапущен:
```
 Container hydro_frontend  Started
```

---

## 📋 ИТОГОВАЯ КАРТОЧКА УЗЛА

```
┌─────────────────────────────────┐
│ climate_001         🟢 Online   │
├─────────────────────────────────┤
│ 22.8°C    60%      493          │
│ Температура Влажность CO₂ ppm   │
├─────────────────────────────────┤
│ 💾 RAM         45%              │ ✅ Память
│ ██████░░░░░░░░░░                │
│                                 │
│ 📶 WiFi        -45 dBm          │ ✅ НОВОЕ!
│ ████████████░░░░                │
├─────────────────────────────────┤
│ 🕐 Last seen: < 1 sec           │
└─────────────────────────────────┘
```

**ПОДОЖДИ 5-10 СЕКУНД И ОБНОВИ СТРАНИЦУ (Ctrl + F5)!** 🎉

**Wi-Fi ИНДИКАТОР ДОЛЖЕН ПОЯВИТЬСЯ НА ВСЕХ КАРТОЧКАХ!** 📶✅

