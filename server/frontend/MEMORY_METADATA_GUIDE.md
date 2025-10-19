# 🧠 Руководство по отображению памяти и метаданных

## Обзор

Система теперь отображает детальную информацию о памяти и метаданных для каждого ESP32 узла в сети.

## 📊 Компоненты

### 1. NodeMemoryCard
Компонент для отображения использования памяти узла.

**Расположение:** `frontend/src/components/NodeMemoryCard.vue`

**Отображаемая информация:**
- **Heap (RAM):** Использование оперативной памяти с цветным индикатором
- **Flash Memory:** Использование флэш-памяти
- **PSRAM:** Использование внешней PSRAM (если доступна)
- **Uptime:** Время работы с момента последней перезагрузки
- **Free Heap:** Свободная оперативная память
- **CPU Frequency:** Частота процессора
- **WiFi Signal:** Сила сигнала WiFi с цветовым индикатором

**Цветовые индикаторы:**
- 🟢 **Зеленый (< 50%):** Нормальное использование
- 🟡 **Желтый (50-75%):** Умеренное использование
- 🔴 **Красный (> 75%):** Высокое использование

**Предупреждения:**
- ⚠️ Появляется при использовании памяти > 80%

### 2. NodeMetadataCard
Компонент для отображения метаданных узла.

**Расположение:** `frontend/src/components/NodeMetadataCard.vue`

**Отображаемая информация:**
- **Firmware:** Версия прошивки
- **Hardware:** Модель оборудования (ESP32-WROOM-32, ESP32-WROVER и т.д.)
- **IP Address:** IP адрес узла
- **WiFi SSID:** Название WiFi сети
- **Boot Count:** Количество перезагрузок
- **Last Calibration:** Последняя калибровка (для pH/EC узлов)
- **Sensors:** Список датчиков (для климатических узлов)
- **Created Via:** Способ создания узла
- **Created At:** Дата создания записи

**Специальные индикаторы:**
- 🆕 **Обновление доступно:** Если доступна новая версия прошивки
- ⚠️ **Требуется калибровка:** Если прошло > 30 дней с последней калибровки

### 3. Компактное отображение в NodeCard
Краткая информация о памяти прямо в карточке узла на Dashboard.

## 📍 Где отображается

### Dashboard (/)
- Компактное отображение RAM в каждой карточке узла

### Node Detail (/nodes/:nodeId)
- **NodeMemoryCard** - детальная информация о памяти
- **NodeMetadataCard** - полная информация о метаданных
- Обе карточки расположены горизонтально для удобного просмотра

### Nodes List (/nodes)
- Компактное отображение RAM в каждой карточке узла

## 🔧 Структура данных metadata

### Обязательные поля
```json
{
  "firmware": "1.0.0",
  "hardware": "ESP32-WROOM-32"
}
```

### Поля памяти
```json
{
  "heap_total": 320000,      // Общий размер heap (байты)
  "heap_used": 156000,       // Используется heap (байты)
  "free_heap": 164000,       // Свободно heap (байты)
  "flash_total": 4194304,    // Общий размер flash (байты)
  "flash_used": 1572864,     // Используется flash (байты)
  "psram_total": 4194304,    // Общий размер PSRAM (байты, опционально)
  "psram_used": 524288       // Используется PSRAM (байты, опционально)
}
```

### Системные поля
```json
{
  "uptime": 259200,          // Uptime в секундах
  "cpu_freq": 240,           // Частота CPU в MHz
  "boot_count": 12           // Количество перезагрузок
}
```

### WiFi поля
```json
{
  "wifi_rssi": -58,          // Сила сигнала в dBm
  "wifi_ssid": "HydroMesh",  // Название WiFi сети
  "ip_address": "192.168.1.101"  // IP адрес
}
```

### Специфичные для типа узла
```json
// pH/EC узлы
{
  "last_calibration": "2025-10-11"  // Дата последней калибровки
}

// Climate узлы
{
  "sensors": ["DHT22", "MHZ19"]     // Список установленных датчиков
}
```

### Мета-информация
```json
{
  "created_via": "mqtt",                    // mqtt|web_ui|api|seeder|quick_add
  "created_at": "2025-10-03T12:00:00Z"     // ISO 8601 timestamp
}
```

## 🚀 Использование

### Обновление базы данных с новыми данными
```bash
cd backend
reseed-database.bat
```

### Запуск системы
```bash
# Terminal 1 - Backend
cd backend
start-dev.bat

# Terminal 2 - Frontend
cd frontend
npm run dev
```

### Просмотр
1. Откройте http://localhost:5173
2. Перейдите на Dashboard или в детали узла
3. Увидите информацию о памяти и метаданных

## 💡 Советы

### Для разработчиков ESP32

1. **Отправка данных памяти через MQTT:**
```cpp
// Пример для ESP32
void sendMemoryStats() {
    cJSON *root = cJSON_CreateObject();
    
    // Heap info
    cJSON_AddNumberToObject(root, "heap_total", ESP.getHeapSize());
    cJSON_AddNumberToObject(root, "heap_used", ESP.getHeapSize() - ESP.getFreeHeap());
    cJSON_AddNumberToObject(root, "free_heap", ESP.getFreeHeap());
    
    // Flash info
    cJSON_AddNumberToObject(root, "flash_total", ESP.getFlashChipSize());
    cJSON_AddNumberToObject(root, "flash_used", ESP.getSketchSize());
    
    // PSRAM (if available)
    if (psramFound()) {
        cJSON_AddNumberToObject(root, "psram_total", ESP.getPsramSize());
        cJSON_AddNumberToObject(root, "psram_used", ESP.getPsramSize() - ESP.getFreePsram());
    }
    
    // System info
    cJSON_AddNumberToObject(root, "uptime", esp_timer_get_time() / 1000000);
    cJSON_AddNumberToObject(root, "cpu_freq", ESP.getCpuFreqMHz());
    cJSON_AddNumberToObject(root, "wifi_rssi", WiFi.RSSI());
    cJSON_AddStringToObject(root, "wifi_ssid", WiFi.SSID().c_str());
    cJSON_AddStringToObject(root, "ip_address", WiFi.localIP().toString().c_str());
    
    char *json_str = cJSON_Print(root);
    
    // Publish to MQTT
    char topic[64];
    snprintf(topic, sizeof(topic), "hydro/nodes/%s/metadata", node_id);
    mqtt_publish(topic, json_str, 0, false);
    
    cJSON_Delete(root);
    free(json_str);
}

// Периодически обновлять (например, каждые 5 минут)
void update_task(void *pvParameters) {
    while(1) {
        sendMemoryStats();
        vTaskDelay(300000 / portTICK_PERIOD_MS); // 5 minutes
    }
}
```

2. **Обработка на backend (MqttService):**
Backend автоматически обрабатывает сообщения с топика `hydro/nodes/+/metadata` и обновляет поле `metadata` в базе данных.

3. **Цветовые пороги:**
- Следите за использованием памяти
- Зеленый < 50% - оптимально
- Желтый 50-75% - нормально
- Красный > 75% - внимание!
- Красный > 80% - предупреждение, рассмотрите оптимизацию

4. **WiFi сигнал:**
- > -50 dBm: Отлично (зеленый)
- -50 to -60 dBm: Хорошо (синий)
- -60 to -70 dBm: Удовлетворительно (желтый)
- < -70 dBm: Слабо (красный)

## 🔍 Диагностика

### Узел с высоким использованием памяти
- Появляется предупреждение в NodeMemoryCard
- Индикатор становится красным
- Рассмотрите:
  - Перезагрузку узла
  - Оптимизацию кода
  - Уменьшение размера буферов
  - Очистку кэша

### Узел с частыми перезагрузками
- Высокий `boot_count`
- Проверьте:
  - Питание
  - Стабильность WiFi
  - Watchdog таймеры
  - Memory leaks

### Узел со слабым WiFi
- `wifi_rssi` < -70 dBm
- Рассмотрите:
  - Перемещение роутера
  - Добавление WiFi усилителя
  - Проверку помех
  - Смену канала WiFi

## 📝 Примеры использования

### Создание нового узла с metadata
```javascript
// Frontend
await api.createNode({
  node_id: 'new_node_001',
  node_type: 'climate',
  zone: 'Zone 1',
  mac_address: 'AA:BB:CC:DD:EE:FF',
  config: { /* ... */ },
  metadata: {
    firmware: '1.0.0',
    hardware: 'ESP32-WROOM-32',
    heap_total: 320000,
    heap_used: 100000,
    // ... другие поля
  }
})
```

### Обновление metadata узла
```javascript
// Frontend
await api.updateNode('node_id', {
  metadata: {
    ...node.metadata,
    firmware: '1.0.1',
    heap_used: 120000,
  }
})
```

## 🎨 Кастомизация

### Изменение цветовых порогов
Отредактируйте функцию `getMemoryColor` в компонентах:
```javascript
function getMemoryColor(percent) {
  if (percent < 40) return 'success'  // Изменено с 50
  if (percent < 70) return 'warning'  // Изменено с 75
  return 'error'
}
```

### Добавление новых полей metadata
1. Добавьте поле в seeder
2. Отобразите в NodeMetadataCard:
```vue
<v-list-item v-if="metadata.new_field">
  <template v-slot:prepend>
    <v-icon icon="mdi-icon-name" color="color"></v-icon>
  </template>
  <v-list-item-title>Название поля</v-list-item-title>
  <v-list-item-subtitle>{{ metadata.new_field }}</v-list-item-subtitle>
</v-list-item>
```

## 🔗 Связанные файлы

- `frontend/src/components/NodeMemoryCard.vue`
- `frontend/src/components/NodeMetadataCard.vue`
- `frontend/src/components/NodeCard.vue`
- `frontend/src/views/NodeDetail.vue`
- `backend/database/seeders/NodeSeeder.php`
- `backend/app/Models/Node.php`

## ✅ Что дальше?

- [ ] Real-time обновление metadata через WebSocket
- [ ] История изменений memory usage
- [ ] Графики использования памяти во времени
- [ ] Алерты при критическом использовании памяти
- [ ] Экспорт отчетов по использованию ресурсов
- [ ] Сравнение узлов по использованию памяти

---

**Версия:** 1.0.0  
**Дата:** 18.10.2025  
**Автор:** AI Assistant

