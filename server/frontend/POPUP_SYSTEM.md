# Система попапов usePopup

## Обзор

Единая централизованная система управления всеми попапами в приложении: диалоги, уведомления (toast), drawers и bottom sheets.

**Ключевые решения**:
- Название: `usePopup`
- API: методы по типу (`popup.dialog()`, `popup.toast()`, `popup.drawer()`, `popup.sheet()`)
- Формы: через компонент в props
- Drawer: только справа, без overlay (persistent)
- Bottom sheets: на всех устройствах, snap points (50%, 75%, 100%)

## Быстрый старт

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

// Подтверждение удаления
const handleDelete = () => {
  popup.dialog.confirm(
    'Вы уверены, что хотите удалить этот узел?',
    'Подтверждение удаления',
    async () => {
      await deleteNode()
      popup.toast.success('Узел успешно удален')
    }
  )
}

// Показать ошибку
const handleError = () => {
  popup.toast.error('Не удалось сохранить изменения')
}

// Показать предупреждение
const handleWarning = () => {
  popup.toast.warning('Уровень воды критичен')
}
</script>
```

## API Reference

### usePopup()

Composable функция для работы с попапами.

#### Методы диалогов

##### dialog.open(config)

Открыть кастомный диалог.

```typescript
popup.dialog.open({
  type: 'custom',
  title: 'Кастомный диалог',
  message: 'Содержимое диалога',
  confirmText: 'OK',
  cancelText: 'Отмена',
  maxWidth: 600,
  persistent: true,
  color: 'primary',
  icon: 'mdi-alert',
  onConfirm: async () => {
    await performAction()
  },
  onCancel: () => {
    console.log('Cancelled')
  }
})
```

##### dialog.confirm(message, title?, onConfirm?)

Показать диалог подтверждения.

```typescript
popup.dialog.confirm(
  'Вы действительно хотите выйти?',
  'Подтверждение',
  async () => {
    await logout()
  }
)
```

##### dialog.prompt(message, title?, onConfirm?)

Показать диалог ввода (в разработке).

```typescript
popup.dialog.prompt(
  'Введите новое название узла',
  'Редактирование',
  async (value) => {
    await updateNodeName(value)
    popup.toast.success('Название обновлено')
  }
)
```

##### dialog.close()

Закрыть текущий диалог.

##### dialog.closeAll()

Закрыть все диалоги.

##### dialog.isOpen()

Проверить, открыт ли диалог.

```typescript
if (popup.dialog.isOpen()) {
  console.log('Dialog is open')
}
```

#### Методы уведомлений (Toast)

##### toast.add(config)

Добавить уведомление.

```typescript
popup.toast.add({
  level: 'error',
  message: 'Ошибка подключения к узлу',
  nodeId: 'ph_01',
  data: {
    ph: 7.2,
    target: 6.0
  }
})
```

##### toast.success(message)

Показать уведомление об успехе.

```typescript
popup.toast.success('Настройки сохранены')
```

##### toast.error(message)

Показать уведомление об ошибке.

```typescript
popup.toast.error('Не удалось подключиться к узлу')
```

##### toast.warning(message)

Показать предупреждение.

```typescript
popup.toast.warning('Уровень воды ниже нормы')
```

##### toast.info(message)

Показать информационное сообщение.

```typescript
popup.toast.info('Новый узел обнаружен')
```

##### toast.critical(message, nodeId?, data?)

Показать критичное уведомление.

```typescript
popup.toast.critical(
  'pH вне критичного диапазона',
  'ph_01',
  { ph: 8.5, target: 6.0 }
)
```

##### toast.emergency(message, nodeId?, data?)

Показать аварийное уведомление.

```typescript
popup.toast.emergency(
  'Аварийное отключение насоса',
  'pump_01',
  { reason: 'Overheat' }
)
```

##### toast.remove(id)

Удалить уведомление по ID.

##### toast.clear()

Очистить все уведомления.

#### Состояние

##### state

Реактивное состояние всех попапов.

```typescript
const state = popup.state
console.log(state.value.dialogs.length) // Количество диалогов
console.log(state.value.toasts.length)  // Количество уведомлений
```

##### dialogs

Computed список всех диалогов.

```typescript
const dialogs = popup.dialogs
dialogs.value.forEach(dialog => {
  console.log(dialog.title)
})
```

##### toasts

Computed список всех уведомлений.

```typescript
const toasts = popup.toasts
toasts.value.forEach(toast => {
  console.log(toast.message)
})
```

## Типы и интерфейсы

### DialogItem

```typescript
interface DialogItem {
  id: string
  type: 'confirm' | 'prompt' | 'custom'
  title?: string
  message?: string
  component?: any
  props?: Record<string, any>
  confirmText?: string
  cancelText?: string
  persistent?: boolean
  maxWidth?: number
  color?: string
  icon?: string
  zIndex: number
  loading: boolean
  error: string | null
  onConfirm?: () => void | Promise<void>
  onCancel?: () => void
}
```

### ToastItem

```typescript
interface ToastItem {
  id: string
  level: 'success' | 'error' | 'warning' | 'info' | 'critical' | 'emergency'
  message: string
  nodeId?: string
  data?: Record<string, any>
  timeout: number
  show: boolean
  timestamp: number
  priority: number
  grouped?: boolean
  count?: number
}
```

## Примеры использования

### Подтверждение удаления

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

const deleteNode = (nodeId: string) => {
  popup.dialog.confirm(
    `Вы уверены, что хотите удалить узел "${nodeId}"?`,
    'Подтверждение удаления',
    async () => {
      try {
        await api.deleteNode(nodeId)
        popup.toast.success('Узел успешно удален')
        await nodesStore.fetchNodes()
      } catch (error) {
        popup.toast.error('Не удалось удалить узел')
      }
    }
  )
}
</script>
```

### Кастомный диалог с формой

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'
import CustomForm from '@/components/CustomForm.vue'

const popup = usePopup()

const openFormDialog = () => {
  popup.dialog.open({
    title: 'Редактирование настроек',
    component: CustomForm,
    props: {
      initialData: settings.value
    },
    maxWidth: 800,
    confirmText: 'Сохранить',
    cancelText: 'Отмена',
    onConfirm: async () => {
      await saveSettings()
      popup.toast.success('Настройки сохранены')
    }
  })
}
</script>
```

### WebSocket события

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

channel.listen('.event.created', (data) => {
  popup.toast.add({
    level: data.level || 'info',
    message: data.message,
    nodeId: data.node_id,
    data: data.data
  })
})
</script>
```

### Обработка ошибок API

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

const fetchData = async () => {
  try {
    const data = await api.getData()
    return data
  } catch (error) {
    popup.toast.error(
      error.response?.data?.message || 'Произошла ошибка'
    )
    throw error
  }
}
</script>
```

### Успешная операция

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

const saveSettings = async () => {
  try {
    await api.saveSettings(settings)
    popup.toast.success('Настройки сохранены')
  } catch (error) {
    popup.toast.error('Не удалось сохранить настройки')
  }
}
</script>
```

### Предупреждение с данными

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

const checkPhLevel = () => {
  const currentPh = 7.2
  const targetPh = 6.0
  
  if (currentPh > targetPh + 0.5) {
    popup.toast.warning('pH вне целевого диапазона')
    // Или с деталями
    popup.toast.add({
      level: 'warning',
      message: 'pH вне целевого диапазона',
      nodeId: 'ph_01',
      data: {
        ph: currentPh,
        target: targetPh
      }
    })
  }
}
</script>
```

## Детали реализации

### z-index система

```typescript
const zIndexes = {
  dialog: 2000,      // Диалоги (модальные)
  toast: 10000,      // Уведомления (всегда сверху)
  drawer: 1500,      // Боковые панели (будущее)
  sheet: 2500        // Bottom sheets (будущее)
}
```

### Автоматические таймауты

```typescript
const timeouts = {
  success: 3000,      // 3 секунды
  info: 5000,         // 5 секунд
  warning: 10000,     // 10 секунд
  error: 8000,        // 8 секунд
  critical: 10000,    // 10 секунд
  emergency: 15000    // 15 секунд
}
```

### Приоритеты

```typescript
const priorities = {
  success: 1,
  info: 2,
  warning: 3,
  error: 4,
  critical: 5,
  emergency: 5
}
```

### Позиционирование Toast

Уведомления располагаются в **правом нижнем углу** в виде вертикального стека:
- Новые уведомления появляются снизу
- Старые поднимаются вверх
- Автоматический расчет смещения (offset) для каждого элемента

### Автоматическое удаление дубликатов

Система автоматически предотвращает появление одинаковых уведомлений:
- События с одинаковым ID обновляются вместо создания дубликата
- Простые уведомления проверяются по сообщению и времени (последняя секунда)

## Миграция со старой системы

### Было (useDialog + EventNotification):

```vue
<script setup>
import { useDialog } from '@/composables/useDialog'
import { useEventNotifications } from '@/composables/useEventNotifications'

const dialog = useDialog()
const { showSuccess, showError } = useEventNotifications()

const handleDelete = () => {
  dialog.confirm(
    'Delete?',
    'Confirm',
    async () => {
      await deleteNode()
      showSuccess('Deleted!')
    }
  )
}
</script>
```

### Стало (usePopup):

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

const handleDelete = () => {
  popup.dialog.confirm(
    'Delete?',
    'Confirm',
    async () => {
      await deleteNode()
      popup.toast.success('Deleted!')
    }
  )
}
</script>
```

### Было (useSnackbar):

```vue
<script setup>
import { useSnackbar } from '@/composables/useSnackbar'

const { showSuccess, showError } = useSnackbar()

const handleSave = async () => {
  try {
    await saveSettings()
    showSuccess('Saved!')
  } catch (error) {
    showError('Failed to save')
  }
}
</script>
```

### Стало (usePopup):

```vue
<script setup>
import { usePopup } from '@/composables/usePopup'

const popup = usePopup()

const handleSave = async () => {
  try {
    await saveSettings()
    popup.toast.success('Saved!')
  } catch (error) {
    popup.toast.error('Failed to save')
  }
}
</script>
```

## Best Practices

### Когда использовать диалоги

✅ **Используйте для**:
- Подтверждение удаления данных
- Подтверждение опасных действий (reset, factory settings)
- Ввод критичных параметров
- Формы с множественными полями
- Действия, требующие внимания пользователя

❌ **Не используйте для**:
- Информирования об успешной операции → `toast.success`
- Показа ошибок API → `toast.error`
- Простых уведомлений → `toast.info/showWarning`

### Когда использовать Toast

✅ **Используйте для**:
- Результаты операций (success/error)
- Системные события (offline, discovery)
- Предупреждения (thresholds, warnings)
- Информация о процессах
- WebSocket события

❌ **Не используйте для**:
- Действий требующих подтверждения → `dialog.confirm`
- Блокирующих ошибок → `dialog` + persistent
- Форм ввода → `dialog.prompt` или кастомный диалог

### Рекомендации по коду

1. **Импортируйте единожды**: Создавайте экземпляр `usePopup()` один раз на компонент
2. **Используйте короткие названия**: `const popup = usePopup()`
3. **Проверяйте ошибки**: Всегда обрабатывайте ошибки в async операциях
4. **Будьте лаконичны**: Короткие и понятные сообщения
5. **Используйте правильный уровень**: success/info/warning/error/critical/emergency

## Преимущества новой системы

✅ **Единый API** - один import для всех типов попапов
✅ **Type-safe** - полная поддержка TypeScript
✅ **Централизованное состояние** - один reactive store
✅ **Stack management** - поддержка множественных попапов
✅ **Автоматизация** - автоудаление, автотаймауты, антидубликаты
✅ **Расширяемость** - легко добавить drawer/sheet позже
✅ **Чистый код** - меньше дублирования
✅ **Consistent UX** - единый стиль для всех попапов
✅ **Accessibility** - поддержка screen readers (в планах)

## Компоненты

### UniversalPopup.vue

Главный компонент-контейнер для всех попапов.

**Props:** Нет (использует composable)

**Расположение:** `App.vue`

### PopupDialog.vue

Компонент для отображения одного диалога.

**Props:**
- `dialog: DialogItem` - конфигурация диалога

**Events:**
- `confirm` - подтверждение диалога
- `cancel` - отмена диалога

### PopupToast.vue

Компонент для отображения одного toast уведомления.

**Props:**
- `toast: ToastItem` - конфигурация уведомления
- `index: number` - индекс в стеке (для позиционирования)

**Events:**
- `remove: [id: string]` - удаление уведомления

## Troubleshooting

### Диалог не закрывается

Убедитесь, что `onConfirm` не выбрасывает исключения:

```typescript
popup.dialog.confirm('Delete?', 'Confirm', async () => {
  try {
    await deleteNode()
  } catch (error) {
    popup.toast.error('Failed to delete')
  }
})
```

### Loading не показывается

Проверьте, что callback - `async` функция:

```typescript
// ❌ Неправильно
popup.dialog.confirm('Save?', 'Confirm', () => {
  saveSettings() // не ждет завершения
})

// ✅ Правильно
popup.dialog.confirm('Save?', 'Confirm', async () => {
  await saveSettings() // ждет завершения
})
```

### Ошибка TypeScript

Убедитесь, что импортирован правильный тип:

```typescript
import type { DialogItem, ToastItem } from '@/composables/usePopup'
```

### Множественные уведомления

Используйте `toast.clear()` для очистки всех уведомлений перед показом критичного:

```typescript
popup.toast.clear()
popup.toast.critical('Critical error occurred')
```

## История изменений

### v1.0.0 (Текущая версия)

- ✅ Единая система usePopup
- ✅ Dialog и Toast функциональность
- ✅ Полная миграция со старых систем
- ✅ TypeScript поддержка
- ✅ Stack management
- ✅ Автоматические таймауты
- ✅ Антидубликаты

### Планируется

- 🔜 Drawer компонент
- 🔜 Bottom Sheet компонент
- 🔜 Настройки уведомлений (Pinia store)
- 🔜 Звуковые уведомления
- 🔜 Desktop notifications
- 🔜 Do Not Disturb режим
- 🔜 Группировка уведомлений
- 🔜 Notification center
- 🔜 Accessibility улучшения

## Контакты и поддержка

Если у вас есть вопросы или предложения по улучшению системы, создайте issue или свяжитесь с командой разработки.
