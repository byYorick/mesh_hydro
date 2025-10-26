# Система управления конфигурациями узлов

## 📋 Обзор

Система управления конфигурациями узлов с возможностями:
- Просмотр и редактирование конфигураций
- Калибровка насосов
- Ручное управление насосами
- История изменений
- PID настройки
- Автоматическое управление

## 🏗️ Архитектура

### Backend API (уже реализовано)

```
POST   /api/nodes/{nodeId}/config            - Обновить конфиг
GET    /api/nodes/{nodeId}/config/request    - Запросить конфиг у узла
GET    /api/nodes/{nodeId}/config/history    - История изменений
POST   /api/nodes/{nodeId}/pump/calibrate    - Калибровка насоса
POST   /api/nodes/{nodeId}/pump/run          - Ручной запуск насоса
GET    /api/nodes/{nodeId}/pump/calibrations - Список калибровок
POST   /api/nodes/{nodeId}/command           - Отправить команду
```

### Frontend Components

```
NodeConfigDialog.vue           - Главный диалог управления конфигурацией
  ├─ ConfigTabs.vue            - Табы: Основные, Насосы, PID, История
  ├─ BasicConfigForm.vue       - Основные параметры
  ├─ PumpCalibrationDialog.vue - Калибровка насосов
  ├─ ManualPumpControl.vue     - Ручное управление насосами
  ├─ PIDConfigForm.vue         - PID параметры
  └─ ConfigHistoryTable.vue    - История изменений
```

## 🎯 Функциональность

### 1. Основная конфигурация

**Настройки:**
- pH Target/Min/Max
- EC Target/Min/Max
- Калибровочные сдвиги
- Emergency пороги
- Макс. время работы насоса
- Cooldown период
- Макс. суточный объем
- Автономное управление

**UI:** Форма с валидацией значений

### 2. Калибровка насосов

**Процесс:**
1. Выбор насоса (ph_up, ph_down, ec_up, ...)
2. Ввод объема (мл)
3. Автоматический запуск насоса
4. Фиксация времени работы
5. Расчет мл/сек
6. Сохранение в NVS

**UI:** Диалог с прогресс-баром и таймером

### 3. Ручное управление насосами

**Возможности:**
- Выбор насоса
- Длительность (0.1-30 сек)
- Мгновенный запуск
- Отображение оставшегося времени
- Автоматическое отключение

**UI:** Кнопки с индикатором времени

### 4. PID настройки

**Параметры:**
- Kp, Ki, Kd для каждого насоса
- Включение/выключение PID
- Setpoint
- Визуализация процесса

**UI:** Таблица с редактируемыми полями

### 5. История изменений

**Информация:**
- Дата/время изменения
- Кто изменил
- Старое значение
- Новое значение
- Комментарий

**UI:** Таблица с фильтрацией

## 🎨 UI/UX Design

### Дизайн-система

**Цвета:**
- Primary: #1976D2 (blue)
- Success: #4CAF50 (green)
- Warning: #FF9800 (orange)
- Error: #F44336 (red)
- Info: #2196F3 (light blue)

**Типография:**
- Заголовки: Roboto Medium, 16-20px
- Текст: Roboto Regular, 14px
- Код: 'Courier New', 12px

**Интерактивность:**
- Hover эффекты на кнопках
- Loading states
- Валидация в реальном времени
- Подтверждение критических действий
- Toast уведомления

### Responsive Layout

**Desktop (>1024px):**
- Полная форма с боковой навигацией
- Таблицы с 6+ колонок
- Горизонтальное размещение элементов

**Tablet (768-1024px):**
- Адаптивная таблица с скроллом
- Стекинг формы
- Компактные кнопки

**Mobile (<768px):**
- Вертикальные табы
- Full-width формы
- Скрытие второстепенных колонок
- Bottom sheet для действий

## 🔒 Безопасность и валидация

### Валидация на фронте

```typescript
const pHMin = 0
const pHMax = 14
const ECMin = 0
const ECMax = 50
const pumpDurationMin = 0.1
const pumpDurationMax = 30
const volumeMin = 0.1
const volumeMax = 1000
```

### Проверки

- pH: 0-14, 2 знака после запятой
- EC: 0-50, 2 знака после запятой
- Объем: 0.1-1000 мл
- Время: 0.1-30 сек
- Целевые значения в пределах min/max
- Emergency пороги в допустимых диапазонах

### Confirmation Dialogs

- Калибровка насоса
- Запуск насоса >10 сек
- Изменение PID параметров
- Включение/выключение автономного режима

## 📊 State Management

### Pinia Store: `useNodeConfig`

```typescript
{
  nodeId: string
  loading: boolean
  saving: boolean
  config: NodeConfig
  history: ConfigHistory[]
  calibrations: PumpCalibration[]
  
  // Actions
  loadConfig(): Promise<void>
  saveConfig(changes: Partial<NodeConfig>): Promise<void>
  requestConfig(): Promise<void>
  calibratePump(params: CalibrationParams): Promise<void>
  runPumpManually(params: RunPumpParams): Promise<void>
  loadHistory(): Promise<void>
  loadCalibrations(): Promise<void>
}
```

## 🧪 Пример использования

### Полный workflow калибровки

```typescript
// 1. Открываем диалог конфигурации
const configDialog = ref(false)

// 2. Загружаем текущую конфигурацию
await nodeConfigStore.loadConfig(nodeId)

// 3. Открываем диалог калибровки
const calibratePump = async (pumpId: number) => {
  const volume = prompt('Введите объем (мл):')
  if (!volume) return
  
  // 4. Запускаем калибровку
  loading.value = true
  try {
    await api.post(`/nodes/${nodeId}/pump/calibrate`, {
      pump_id: pumpId,
      duration_sec: 5, // Запускаем на 5 сек
      volume_ml: parseFloat(volume)
    })
    
    // 5. Показываем результат
    showSuccess(`Насос ${pumpId} откалиброван: X мл/сек`)
    
    // 6. Обновляем конфигурацию
    await nodeConfigStore.loadConfig(nodeId)
  } catch (error) {
    showError(error.message)
  } finally {
    loading.value = false
  }
}
```

## 🚀 Roadmap

### Phase 1: MVP (Текущий этап)
- ✅ Backend API
- ⏳ Frontend компоненты
- ⏳ Базовая валидация
- ⏳ История изменений

### Phase 2: Enhancement
- Графики PID процесса
- Экспорт конфигураций
- Импорт конфигураций
- Bulk операции
- Шаблоны конфигураций

### Phase 3: Advanced
- AI-assisted calibration
- Predictive maintenance
- Auto-tuning PID
- Multi-zone configuration

## 📝 API Reference

### Update Config

```typescript
PUT /api/nodes/{nodeId}/config
Body: {
  config: {
    ph_target: 6.5,
    ph_min: 5.5,
    ph_max: 7.5,
    // ...
  },
  comment?: string
}
```

### Calibrate Pump

```typescript
POST /api/nodes/{nodeId}/pump/calibrate
Body: {
  pump_id: 0,
  duration_sec: 5,
  volume_ml: 10.0
}
Response: {
  ml_per_second: 2.0,
  calibration: { ... }
}
```

### Run Pump Manually

```typescript
POST /api/nodes/{nodeId}/pump/run
Body: {
  pump_id: 0,
  duration_sec: 5.0
}
```

## 🎓 Best Practices

1. **Всегда проверяйте онлайн статус** перед отправкой команд
2. **Используйте loading states** для длительных операций
3. **Показывайте прогресс** для калибровок
4. **Логируйте все изменения** с комментариями
5. **Подтверждайте критичные действия**
6. **Валидируйте данные** на клиенте и сервере
7. **Предотвращайте race conditions** при concurrent изменениях
8. **Обеспечивайте feedback** пользователю

## 📚 Дополнительные ресурсы

- Backend API: `server/backend/app/Http/Controllers/NodeController.php`
- Frontend Store: `server/frontend/src/stores/nodeConfig.js` (TODO)
- Components: `server/frontend/src/components/node-config/`
- API Documentation: `/api/nodes`
