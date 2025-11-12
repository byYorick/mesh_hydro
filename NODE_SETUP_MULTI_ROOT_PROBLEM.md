# Проблема: Несколько ROOT нод рядом

## Сценарий

**Ситуация:** В одном помещении или рядом находятся несколько ROOT нод в режиме setup.

**Примеры:**
- Две гидропонные системы в одном помещении (HYDRO1, HYDRO2)
- Соседи тоже используют систему
- Сервисный центр настраивает несколько систем одновременно
- Расширение системы: добавляется вторая зона с новым ROOT

---

## Анализ проблем по версиям

### V1: 🔴 КРИТИЧЕСКАЯ ПРОБЛЕМА

#### Что произойдет:

```c
// Все ROOT создают одинаковую mesh сеть
#define TEMP_MESH_ID "HYDRO_NEW"
#define SETUP_PASSWORD "setup_2025"
```

**Проблемы:**

1. **Конфликт mesh сетей** ❌
   ```
   ROOT #1: mesh_id = "HYDRO_NEW"
   ROOT #2: mesh_id = "HYDRO_NEW"  // КОНФЛИКТ!
   ROOT #3: mesh_id = "HYDRO_NEW"  // КОНФЛИКТ!
   ```
   - ESP-MESH не позволяет несколько сетей с одинаковым ID
   - Первый ROOT создаст сеть
   - Второй и третий попытаются создать и получат ошибку
   - Либо объединятся в одну сеть (что неправильно)

2. **Обычные ноды подключатся не туда** ❌
   ```
   Node pH: Ищет "HYDRO_NEW" → Найдет любой ROOT → Может подключиться к чужому
   ```

3. **Backend не различит ROOT'ы** ❌
   ```
   discovery: { temp_mesh_id: "HYDRO_NEW", mac: "AA:BB:..." }
   discovery: { temp_mesh_id: "HYDRO_NEW", mac: "CC:DD:..." }
   // temp_mesh_id одинаковый, только MAC разный
   ```

4. **Конфигурация перепутается** ❌
   - Пользователь настраивает ROOT #1
   - Команда может уйти на ROOT #2
   - Хаос в системе

**Вердикт V1:** 🔴 Непригодна для нескольких ROOT

---

### V1.5: 🟡 ЧАСТИЧНАЯ ПРОБЛЕМА

#### Что произойдет:

```c
// Каждый ROOT создает уникальную mesh сеть
node_config_generate_temp_mesh_id(s_temp_mesh_id, sizeof(s_temp_mesh_id));
// ROOT #1: mesh_id = "HYDRO_A1B2C3"
// ROOT #2: mesh_id = "HYDRO_D4E5F6"
```

**Решенные проблемы:** ✅

1. **Mesh сети уникальные**
   ```
   ROOT #1: "HYDRO_A1B2C3"
   ROOT #2: "HYDRO_D4E5F6"
   ROOT #3: "HYDRO_123456"
   ```
   - Нет конфликта mesh ID
   - Каждый ROOT создает свою сеть

2. **Backend различает ROOT'ы**
   ```json
   {
     "mac": "AA:BB:CC:A1:B2:C3",
     "pin": "A1B2C3",
     "temp_mesh_id": "HYDRO_A1B2C3"
   }
   ```

**Новая проблема:** ⚠️

3. **Конфликт WiFi AP** (если в режиме setup)
   ```
   ROOT #1: WiFi AP = "HYDRO_SETUP_A1B2C3"
   ROOT #2: WiFi AP = "HYDRO_SETUP_D4E5F6"
   ```
   - Уникальные имена ✅
   - НО оба на канале 1 (по умолчанию) → интерференция
   - Могут мешать друг другу

4. **Путаница для пользователя** ⚠️
   ```
   Смартфон видит:
   - HYDRO_SETUP_A1B2C3
   - HYDRO_SETUP_D4E5F6
   - HYDRO_SETUP_123456
   
   Какой выбрать? Откуда я знаю PIN?
   ```

5. **Обычные ноды: к какому ROOT?** ⚠️
   ```c
   // Node сканирует и находит:
   // - HYDRO_A1B2C3
   // - HYDRO_D4E5F6
   // К какому подключаться?
   ```
   - Если auto-scan → может подключиться к чужому
   - Нужно указывать mesh_id явно

**Вердикт V1.5:** 🟡 Работает, но требует доработок

---

### V2: 🟢 МИНИМАЛЬНАЯ ПРОБЛЕМА

#### Что произойдет:

```c
// Каждый ROOT с уникальным токеном
node_config_generate_setup_credentials(&s_setup_creds);
// ROOT #1: mesh_id = "HYDRO_SETUP_A1B2C3", token = "k9mP7q2Ls8vBn4xT..."
// ROOT #2: mesh_id = "HYDRO_SETUP_D4E5F6", token = "z3W5jR0yE6uCa8..."
```

**Решенные проблемы:** ✅

1. **Mesh сети уникальные** ✅
2. **Токены уникальные** ✅
3. **Backend различает** ✅
4. **BLE не конфликтует** (разные сервисы) ✅

**Остаточные проблемы:** ⚠️

5. **BLE интерференция** (незначительно)
   - Несколько BLE устройств рядом могут мешать
   - Но BLE справляется лучше чем WiFi AP

6. **Пользователь видит несколько устройств** ⚠️
   ```
   В приложении ESP BLE Provisioning:
   - HYDRO_A1B2C3
   - HYDRO_D4E5F6
   - HYDRO_123456
   
   Нужна физическая маркировка устройств
   ```

**Вердикт V2:** 🟢 Работает хорошо, нужна маркировка

---

## Решения

### Решение 1: Физическая маркировка (для всех версий)

**Наклейка на устройство:**

```
┌─────────────────────────┐
│   HYDRO ROOT NODE       │
│                         │
│  🌱 Zone: _________    │
│                         │
│  📶 WiFi: HYDRO_A1B2C3 │
│  🔑 PIN: A1B2C3        │
│                         │
│  MAC: AA:BB:CC:A1:B2:C3│
└─────────────────────────┘
```

**Инструкция:**
1. Напишите название зоны на наклейке
2. Подключайтесь только к WiFi/BLE указанному на наклейке
3. Используйте PIN с этой же наклейки

---

### Решение 2: Разные WiFi каналы (V1.5)

**Проблема:** Все ROOT на канале 1 → интерференция

**Решение:** Генерировать канал из MAC

```c
void node_config_generate_temp_mesh_id(char *mesh_id_out, size_t size, uint8_t *channel_out) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    
    // mesh_id из последних 3 байт
    snprintf(mesh_id_out, size, "HYDRO_%02X%02X%02X", 
             mac[3], mac[4], mac[5]);
    
    // WiFi канал: 1, 6, или 11 (не пересекаются)
    // Используем последний байт MAC для выбора
    uint8_t channels[] = {1, 6, 11};
    *channel_out = channels[mac[5] % 3];
    
    ESP_LOGI(TAG, "Generated: mesh_id=%s, channel=%d", mesh_id_out, *channel_out);
}

// В wifi_setup_start_ap():
uint8_t channel;
node_config_generate_temp_mesh_id(s_temp_mesh_id, sizeof(s_temp_mesh_id), &channel);

wifi_config_t wifi_config = {
    .ap = {
        .ssid_len = strlen(ap_ssid),
        .channel = channel,  // ✅ Уникальный канал
        .max_connection = 4,
        .authmode = WIFI_AUTH_WPA2_PSK,
    },
};
```

**Результат:**
```
ROOT #1 (MAC ...A1:B2:C3): WiFi на канале 1
ROOT #2 (MAC ...D4:E5:F6): WiFi на канале 6
ROOT #3 (MAC ...12:34:56): WiFi на канале 11
```

Меньше интерференции ✅

---

### Решение 3: RSSI фильтрация для обычных нод

**Проблема:** Обычная нода видит несколько mesh сетей, не знает к какой подключаться

**Решение:** Подключаться к самой сильной (ближайшей)

**Файл: `common/mesh_manager/mesh_manager.c`**

```c
typedef struct {
    char mesh_id[32];
    int8_t rssi;
    uint8_t channel;
} discovered_mesh_t;

static esp_err_t scan_mesh_networks(discovered_mesh_t *meshes, size_t max_meshes, size_t *found_count) {
    wifi_scan_config_t scan_config = {
        .show_hidden = true,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    
    esp_wifi_scan_start(&scan_config, true);
    
    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    
    wifi_ap_record_t *ap_records = malloc(ap_count * sizeof(wifi_ap_record_t));
    esp_wifi_scan_get_ap_records(&ap_count, ap_records);
    
    *found_count = 0;
    
    for (int i = 0; i < ap_count && *found_count < max_meshes; i++) {
        // Проверка что это HYDRO mesh сеть
        if (strncmp((char*)ap_records[i].ssid, "HYDRO_", 6) == 0) {
            strcpy(meshes[*found_count].mesh_id, (char*)ap_records[i].ssid);
            meshes[*found_count].rssi = ap_records[i].rssi;
            meshes[*found_count].channel = ap_records[i].primary;
            
            ESP_LOGI(TAG, "Found mesh: %s (RSSI: %d, ch: %d)",
                     meshes[*found_count].mesh_id,
                     meshes[*found_count].rssi,
                     meshes[*found_count].channel);
            
            (*found_count)++;
        }
    }
    
    free(ap_records);
    
    // Сортировка по RSSI (сильнейший сигнал первым)
    for (int i = 0; i < *found_count - 1; i++) {
        for (int j = i + 1; j < *found_count; j++) {
            if (meshes[j].rssi > meshes[i].rssi) {
                discovered_mesh_t temp = meshes[i];
                meshes[i] = meshes[j];
                meshes[j] = temp;
            }
        }
    }
    
    return ESP_OK;
}

esp_err_t mesh_manager_auto_connect(void) {
    discovered_mesh_t meshes[10];
    size_t found_count;
    
    esp_err_t err = scan_mesh_networks(meshes, 10, &found_count);
    if (err != ESP_OK || found_count == 0) {
        ESP_LOGE(TAG, "No mesh networks found");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "Found %d mesh networks, connecting to strongest", found_count);
    
    // Подключиться к самой сильной (первой после сортировки)
    mesh_manager_config_t cfg = {
        .mode = MESH_MODE_NODE,
        .mesh_id = meshes[0].mesh_id,
        .mesh_password = "hydro_mesh_2025",
        .channel = meshes[0].channel,
    };
    
    ESP_LOGI(TAG, "Connecting to: %s (RSSI: %d)", 
             meshes[0].mesh_id, meshes[0].rssi);
    
    return mesh_manager_init(&cfg);
}
```

**Использование в node:**

```c
if (!is_configured) {
    // Автоматически подключиться к ближайшей mesh сети
    mesh_manager_auto_connect();
}
```

**Преимущества:**
- ✅ Подключается к ближайшему ROOT (вероятно правильному)
- ✅ Игнорирует дальние ROOT (соседи, другие зоны)
- ✅ Работает автоматически

**Недостатки:**
- ⚠️ Может ошибиться если чужой ROOT ближе
- ⚠️ Нужен fallback если выбрали неправильный

---

### Решение 4: Явное указание mesh_id (для уверенности)

**Вариант A: Через веб-интерфейс**

```vue
<!-- Frontend: компонент выбора mesh сети -->
<v-select
  v-model="config.mesh_id"
  :items="availableMeshes"
  label="Выберите mesh сеть вашего ROOT"
  hint="Посмотрите название на наклейке ROOT ноды"
>
  <template v-slot:item="{ item }">
    <v-list-item>
      <v-list-item-title>{{ item.mesh_id }}</v-list-item-title>
      <v-list-item-subtitle>
        RSSI: {{ item.rssi }} dBm | Канал: {{ item.channel }}
      </v-list-item-subtitle>
    </v-list-item>
  </template>
</v-select>
```

**Вариант B: QR код на ROOT (V2)**

```
┌─────────────────────────┐
│   HYDRO ROOT NODE       │
│                         │
│  Zone: Main Hall        │
│                         │
│  ████████████████████   │  ← QR код с mesh_id
│  ██  ██████  ██  ██ █   │
│  ████████████████████   │
│                         │
│  mesh_id: HYDRO_A1B2C3 │
└─────────────────────────┘
```

Обычная нода сканирует QR → получает mesh_id → подключается к нужной сети

---

### Решение 5: Whitelist в Backend

**Для многозональных систем**

```php
// Backend: проверка что нода подключается к правильному ROOT

class NewNode extends Model
{
    public function validateMeshAssignment(string $target_mesh_id): bool
    {
        // Получить ROOT этой mesh сети
        $root = Node::where('node_type', 'root')
                    ->where('metadata->mesh_id', $target_mesh_id)
                    ->first();
        
        if (!$root) {
            return false;
        }
        
        // Проверить что ROOT онлайн
        if (!$root->online) {
            return false;
        }
        
        // Проверить что это не чужая mesh сеть
        // (можно добавить проверку по owner_id, zone_id и т.д.)
        
        return true;
    }
    
    public function configure(array $config): bool
    {
        // Проверка mesh_id перед настройкой
        if (!$this->is_root) {
            if (!$this->validateMeshAssignment($config['mesh_id'])) {
                \Log::warning("Invalid mesh_id for node {$this->mac_address}");
                return false;
            }
        }
        
        // ... остальная логика
    }
}
```

---

## Рекомендованная стратегия

### Для V1.5 (оптимальный вариант):

**1. Уникальные mesh ID из MAC** ✅ (уже есть)

**2. Разные WiFi каналы** ✅ (добавить)

**3. Физическая маркировка** ✅ (печать наклеек)

**4. RSSI фильтрация + явный выбор** ✅ (добавить)
   - По умолчанию: подключаться к ближайшему
   - Опция: выбрать mesh_id вручную из списка

**5. Валидация в Backend** ✅ (добавить)

### Реализация:

**Шаг 1: Генерация уникального канала**

```c
// common/node_config/node_config.c
void node_config_generate_setup_params(
    char *mesh_id_out, 
    size_t mesh_id_size,
    char *pin_out,
    size_t pin_size,
    uint8_t *channel_out
) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    
    // PIN из последних 3 байт MAC
    snprintf(pin_out, pin_size, "%02X%02X%02X", mac[3], mac[4], mac[5]);
    
    // mesh_id
    snprintf(mesh_id_out, mesh_id_size, "HYDRO_%s", pin_out);
    
    // Канал: 1, 6, или 11 (не пересекаются)
    uint8_t channels[] = {1, 6, 11};
    *channel_out = channels[mac[5] % 3];
}
```

**Шаг 2: Scan + выбор в веб-интерфейсе**

```vue
<!-- Frontend: показать доступные mesh сети -->
<template>
  <v-card>
    <v-card-title>К какой mesh сети подключить?</v-card-title>
    
    <v-alert type="info" class="ma-4">
      Выберите mesh сеть вашего ROOT. Название написано на наклейке.
    </v-alert>
    
    <v-list>
      <v-list-item
        v-for="mesh in availableMeshes"
        :key="mesh.mesh_id"
        @click="selectMesh(mesh)"
      >
        <template v-slot:prepend>
          <v-icon :color="getSignalColor(mesh.rssi)">
            mdi-wifi-strength-{{ getSignalStrength(mesh.rssi) }}
          </v-icon>
        </template>
        
        <v-list-item-title>{{ mesh.mesh_id }}</v-list-item-title>
        <v-list-item-subtitle>
          Сигнал: {{ mesh.rssi }} dBm | Канал: {{ mesh.channel }}
        </v-list-item-subtitle>
        
        <template v-slot:append>
          <v-chip v-if="mesh.is_closest" color="success" size="small">
            Ближайший
          </v-chip>
        </template>
      </v-list-item>
    </v-list>
  </v-card>
</template>

<script setup lang="ts">
function getSignalStrength(rssi: number): number {
  if (rssi > -50) return 4
  if (rssi > -60) return 3
  if (rssi > -70) return 2
  return 1
}

function getSignalColor(rssi: number): string {
  if (rssi > -50) return 'success'
  if (rssi > -60) return 'warning'
  return 'error'
}
</script>
```

**Шаг 3: Наклейки (дизайн)**

```
┌───────────────────────────────────────┐
│  🌱 HYDRO ROOT NODE                   │
│                                       │
│  Zone: _____________________          │
│                                       │
│  📶 WiFi: HYDRO_SETUP_A1B2C3         │
│  🔒 Password: hydro2025               │
│                                       │
│  🔑 PIN: A1B2C3                      │
│                                       │
│  📡 Mesh ID: HYDRO_A1B2C3            │
│  📍 MAC: AA:BB:CC:A1:B2:C3           │
│                                       │
│  ⚠️  ПОДКЛЮЧАЙТЕСЬ ТОЛЬКО К ЭТОМУ    │
│      WiFi/Mesh ЕСЛИ НАСТРАИВАЕТЕ     │
│      НОДЫ ДЛЯ ЭТОЙ ЗОНЫ!             │
└───────────────────────────────────────┘
```

Распечатать на самоклеящейся бумаге и приклеить на корпус.

---

## Итоговая рекомендация

### ✅ Для защиты от конфликтов реализовать:

1. **Уникальный mesh_id из MAC** (V1.5, V2) - обязательно
2. **Разные WiFi каналы** (V1.5) - очень желательно
3. **Физическая маркировка** (все версии) - обязательно
4. **RSSI + выбор mesh** (все версии) - желательно
5. **Валидация в Backend** (все версии) - желательно

### Сложность реализации:

| Решение | Сложность | Время |
|---------|-----------|-------|
| Уникальный mesh_id | Низкая | 1 час |
| Разные каналы | Низкая | 1 час |
| Физическая маркировка | Нулевая | Печать |
| RSSI + выбор | Средняя | 4 часа |
| Валидация Backend | Низкая | 2 часа |
| **ИТОГО** | - | **8 часов** |

**Рекомендую добавить эти 8 часов к плану V1.5 → итого 53 часа**

---

## Тестирование

### Сценарий 1: Два ROOT в одной комнате

```
1. Включить ROOT #1 (PIN: A1B2C3)
2. Включить ROOT #2 (PIN: D4E5F6)
3. Проверить: они на разных каналах
4. Включить обычную ноду
5. Проверить: она подключилась к ближайшему
6. В веб-интерфейсе показывается список из 2 mesh сетей
7. Выбрать правильную по PIN с наклейки
8. Настроить успешно ✅
```

### Сценарий 2: Сосед тоже использует систему

```
1. Мой ROOT: HYDRO_A1B2C3, моя зона
2. Сосед ROOT: HYDRO_123456, его зона (слабый сигнал)
3. Моя нода видит оба, но подключается к моему (RSSI выше)
4. Если ошиблась → в веб-интерфейсе выбрать правильный
5. PIN соседа не знаю → конфигурация не пройдет ✅
```

**Защита работает!** ✅

---

**Дата:** 2025-01-07  
**Вывод:** Проблема решаема, требует 8 дополнительных часов на реализацию

