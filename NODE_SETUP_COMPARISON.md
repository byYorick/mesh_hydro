# Сравнение архитектур: V1 vs V2

## Краткое резюме

### V1 (Оригинальный план)
✅ Простая реализация  
✅ Быстрый MVP  
⚠️ Недостаточная безопасность  
⚠️ Слабая обработка ошибок  
❌ Hardcoded WiFi credentials  
❌ Нет диагностики  

### V2 (Улучшенная архитектура)
✅ Промышленная безопасность  
✅ Надежная работа с ошибками  
✅ BLE WiFi Provisioning  
✅ Полная диагностика  
⚠️ Более сложная реализация  
⚠️ Требует больше времени  

---

## Детальное сравнение

### 1. Безопасность

| Критерий | V1 | V2 |
|----------|----|----|
| **Mesh ID** | `HYDRO_NEW` (одинаковый для всех) | `HYDRO_SETUP_XXXX` (уникальный из MAC) |
| **Mesh пароль** | `setup_2025` (фиксированный) | Случайный 32-символьный токен |
| **Discovery endpoint** | Без аутентификации | Токен с истечением (24 часа) |
| **Валидация конфигурации** | Минимальная | Полная с проверкой токена |

**Вывод:** V2 намного безопаснее для производства

---

### 2. WiFi Provisioning

| Аспект | V1 | V2 |
|--------|----|----|
| **Метод** | Hardcoded в `mesh_config.h` | BLE Provisioning через приложение |
| **Гибкость** | ❌ Нужно перепрошивать для каждого роутера | ✅ Настраивается пользователем |
| **UX** | ⭐☆☆☆☆ Требует знаний программирования | ⭐⭐⭐⭐⭐ Мобильное приложение + QR код |

**Код V1:**
```c
// Hardcoded - плохо для пользователей
wifi_connect(MESH_ROUTER_SSID, MESH_ROUTER_PASSWORD);
```

**Код V2:**
```c
// BLE Provisioning - настраивается пользователем
wifi_prov_start(&prov_config);
```

**Вывод:** V2 значительно удобнее для конечных пользователей

---

### 3. Управление состояниями

#### V1: Бинарное

```c
bool is_configured = node_config_is_configured();

if (!is_configured) {
    // Режим setup
    wait_for_config(); // ⚠️ Бесконечное ожидание
} else {
    // Нормальный режим
}
```

**Проблемы:**
- Нет промежуточных состояний
- Нет timeout'ов
- Невозможно отследить прогресс
- Нет обработки ошибок

#### V2: State Machine

```
UNINITIALIZED → SETUP_WAITING → SETUP_CONFIGURING 
               ↓                ↓
            ERROR ← ─────────────
               ↓
           RECOVERY → (retry)

SETUP_CONFIGURING → SETUP_VALIDATING → SETUP_COMPLETE → OPERATIONAL
```

**Преимущества:**
- ✅ Четкие переходы между состояниями
- ✅ Timeout для каждого состояния
- ✅ Автоматическое восстановление при ошибках
- ✅ Логирование всех переходов

**Вывод:** V2 намного надежнее и предсказуемее

---

### 4. LED Индикация

| Режим | V1 | V2 |
|-------|----|----|
| **Не настроен** | Не указано | Быстрое мигание (4 Hz) |
| **Ожидание WiFi (ROOT)** | Не указано | Медленное мигание (1 Hz) |
| **Настроен** | Не указано | Постоянно горит |
| **Ошибка** | Не указано | SOS паттерн (... --- ...) |

**Вывод:** V2 дает визуальную обратную связь пользователю

---

### 5. Обработка ошибок

#### V1: Минимальная

```c
void handle_write_config_command(cJSON *params) {
    // ⚠️ Нет проверки на NULL
    const char *node_id = cJSON_GetObjectItem(params, "node_id")->valuestring;
    
    // Сохранить
    node_config_save(&s_config, sizeof(s_config), "..._ns");
    
    // ⚠️ Нет проверки результата
    esp_restart();
}
```

**Проблемы:**
- Crash если поле отсутствует
- Нет валидации данных
- Нет rollback при ошибке сохранения
- Перезагрузка даже если ошибка

#### V2: Полная валидация

```c
static esp_err_t validate_config(cJSON *config_json) {
    // Проверка токена
    if (!node_config_validate_setup_token(token)) {
        ESP_LOGE(TAG, "Invalid token");
        return ESP_ERR_INVALID_ARG;
    }
    
    // Проверка обязательных полей
    const char *required_fields[] = {
        "node_id", "mesh_id", "zone", ...
    };
    for (int i = 0; i < sizeof(required_fields) / ...; i++) {
        if (!cJSON_HasObjectItem(config_json, required_fields[i])) {
            return ESP_ERR_INVALID_ARG;
        }
    }
    
    // Проверка длины строк
    if (strlen(node_id) > 31 || strlen(node_id) == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    
    return ESP_OK;
}

static esp_err_t apply_config(cJSON *config_json) {
    // State: CONFIGURING
    state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_RECEIVED);
    
    // Валидация
    esp_err_t err = validate_config(config_json);
    if (err != ESP_OK) {
        state_machine_transition(&s_state_machine, STATE_EVENT_ERROR);
        return err; // ⚠️ НЕ перезагружаемся при ошибке
    }
    
    // State: VALIDATING
    state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_VALIDATED);
    
    // Сохранение
    err = node_config_save(&s_config, sizeof(s_config), "root_ns");
    if (err != ESP_OK) {
        state_machine_transition(&s_state_machine, STATE_EVENT_ERROR);
        return err; // ⚠️ Rollback - можно повторить
    }
    
    // State: COMPLETE
    state_machine_transition(&s_state_machine, STATE_EVENT_CONFIG_SAVED);
    
    return ESP_OK;
}
```

**Вывод:** V2 защищена от всех edge cases

---

### 6. Discovery с Retry

#### V1: Одна попытка

```c
// Отправить discovery на сервер через HTTP
send_root_discovery_http();

// ⚠️ Что если сервер недоступен?
// ⚠️ Что если сеть медленная?
// ⚠️ Как узнать IP сервера?

wait_for_config(); // Бесконечное ожидание
```

#### V2: Retry механизм

```c
static void discovery_task(void *pvParameters) {
    const char *server_url = (const char *)pvParameters;
    int retry_count = 0;
    const int max_retries = 20; // 10 минут
    
    while (retry_count < max_retries) {
        esp_err_t err = send_discovery_http(server_url, &s_setup_creds);
        
        if (err == ESP_OK) {
            ESP_LOGI(TAG, "Discovery successful");
            vTaskDelay(pdMS_TO_TICKS(30000)); // Heartbeat каждые 30 сек
        } else {
            retry_count++;
            ESP_LOGW(TAG, "Retry %d/%d", retry_count, max_retries);
            vTaskDelay(pdMS_TO_TICKS(30000));
        }
    }
    
    if (retry_count >= max_retries) {
        state_machine_transition(&s_state_machine, STATE_EVENT_ERROR);
    }
}
```

**Вывод:** V2 справляется с нестабильной сетью

---

### 7. Backend - таблица new_nodes

#### V1: Базовая

```sql
CREATE TABLE new_nodes (
    id SERIAL PRIMARY KEY,
    mac_address VARCHAR(17) UNIQUE,
    node_type VARCHAR,
    is_root BOOLEAN,
    metadata JSONB,
    discovered_at TIMESTAMP,
    last_heartbeat_at TIMESTAMP
);
```

**Ограничения:**
- Нет статуса (discovered/configuring/failed)
- Нет setup_token для безопасности
- Нет логирования событий

#### V2: Расширенная

```sql
CREATE TABLE new_nodes (
    id SERIAL PRIMARY KEY,
    mac_address VARCHAR(17) UNIQUE,
    node_type VARCHAR,
    is_root BOOLEAN,
    
    -- ✅ Безопасность
    setup_token VARCHAR(32),
    temp_mesh_id VARCHAR(32),
    token_expires_at TIMESTAMP,
    
    -- ✅ Состояния
    status ENUM('discovered', 'configuring', 
                'waiting_confirmation', 'confirmed', 
                'failed', 'timeout'),
    
    metadata JSONB,
    discovered_at TIMESTAMP,
    last_heartbeat_at TIMESTAMP,
    configured_at TIMESTAMP
);

-- ✅ Логирование
CREATE TABLE setup_logs (
    id SERIAL PRIMARY KEY,
    mac_address VARCHAR(17),
    event ENUM('discovered', 'heartbeat', 
               'config_sent', 'config_confirmed', 
               'config_failed', 'timeout'),
    details JSONB,
    created_at TIMESTAMP
);
```

**Вывод:** V2 дает полную видимость процесса

---

### 8. Frontend UX

#### V1: Простая форма

```vue
<v-form>
  <v-text-field v-model="node_id" label="Node ID" />
  <v-text-field v-model="zone" label="Zone" />
  <v-btn @click="configure">Настроить</v-btn>
</v-form>
```

**Проблемы:**
- Нет инструкций
- Непонятно что делать
- Нет обратной связи

#### V2: Пошаговый wizard

```vue
<v-stepper v-model="step">
  <v-stepper-header>
    <v-stepper-item value="1">Подготовка</v-stepper-item>
    <v-stepper-item value="2">WiFi/Mesh</v-stepper-item>
    <v-stepper-item value="3">Параметры</v-stepper-item>
    <v-stepper-item value="4">Подтверждение</v-stepper-item>
  </v-stepper-header>
  
  <v-stepper-window>
    <!-- Шаг 1: Инструкции по подключению питания + LED индикация -->
    <!-- Шаг 2: QR код для BLE provisioning / ожидание mesh -->
    <!-- Шаг 3: Форма с валидацией -->
    <!-- Шаг 4: Предпросмотр перед применением -->
  </v-stepper-window>
</v-stepper>
```

**Преимущества:**
- ✅ Пошаговые инструкции
- ✅ QR коды для быстрой настройки
- ✅ Визуальная обратная связь
- ✅ Предпросмотр перед применением

**Вывод:** V2 значительно удобнее для новых пользователей

---

## 9. Матрица выбора

### Выбрать V1 если:
- ✅ Нужен быстрый прототип (MVP за 2-3 дня)
- ✅ Только для личного использования
- ✅ Все устройства на одном WiFi роутере
- ✅ Нет требований к безопасности
- ✅ Готовы перепрошивать для смены WiFi

**Время реализации:** ~36 часов  
**Сложность:** Низкая  
**Надежность:** ⭐⭐☆☆☆

### Выбрать V2 если:
- ✅ Нужен production-ready продукт
- ✅ Планируется продажа / распространение
- ✅ Разные WiFi роутеры у пользователей
- ✅ Важна безопасность
- ✅ Нужна диагностика проблем
- ✅ Нужен хороший UX

**Время реализации:** ~60 часов  
**Сложность:** Средняя  
**Надежность:** ⭐⭐⭐⭐⭐

---

## 10. Гибридный подход (рекомендуется)

### Фаза 1: MVP на базе V1 (1 неделя)
- Базовая функциональность без security
- Hardcoded WiFi для тестирования
- Простые формы без wizard
- **Цель:** Проверить концепцию

### Фаза 2: Добавить критичные фичи V2 (1 неделя)
- ✅ State Machine
- ✅ Setup tokens
- ✅ Валидация конфигурации
- ✅ LED индикация
- **Цель:** Сделать надежным

### Фаза 3: UX улучшения V2 (1 неделя)
- ✅ BLE WiFi Provisioning
- ✅ Wizard с инструкциями
- ✅ QR коды
- ✅ Setup logs
- **Цель:** Сделать удобным

---

## 11. Рекомендация

### Для текущего проекта mesh_hydro:

**🎯 Рекомендую V2 с поэтапной реализацией**

**Причины:**
1. Проект уже достаточно зрелый (есть root_node, несколько типов нод)
2. Планируется использование в реальных условиях (гидропоника)
3. Могут быть разные WiFi сети
4. Важна диагностика при проблемах
5. Дополнительные 24 часа разработки окупятся удобством

**План действий:**
1. **Неделя 1:** Реализовать firmware (State Machine + BLE Provisioning)
2. **Неделя 2:** Реализовать backend (расширенная таблица + logs)
3. **Неделя 3:** Реализовать frontend (wizard + инструкции)
4. **Неделя 4:** Тестирование и баг-фиксы

**Итого:** 1 месяц до production-ready системы

---

## 12. Следующие шаги

1. ✅ **Утвердить архитектуру** - выбрать V1, V2 или гибрид
2. ⬜ **Создать ветку** - `feature/node-setup-system`
3. ⬜ **Начать с firmware** - State Machine + Setup credentials
4. ⬜ **Параллельно backend** - Migration + Model
5. ⬜ **Тестировать на ROOT** - Прошить и проверить
6. ⬜ **Добавить обычные ноды** - pH, EC, и т.д.
7. ⬜ **Frontend последним** - Когда backend работает
8. ⬜ **End-to-end тест** - Полный цикл setup

---

**Вопросы для обсуждения:**
1. Какая версия больше подходит для ваших целей?
2. Критична ли возможность менять WiFi без перепрошивки?
3. Планируется ли использование несколькими пользователями?
4. Какой timeline приемлем?

**Готов приступить к реализации выбранной версии!** 🚀

