# Единая централизованная система уведомлений

## Обзор

Единая система, объединяющая модальные диалоги (для подтверждений) и toast-уведомления (для событий системы).

**Файл:** `server/frontend/src/composables/useNotification.ts`

## Быстрый старт

```vue
<script setup>
import { useNotification } from '@/composables/useNotification'

const notify = useNotification()

// Подтверждение удаления
const handleDelete = () => {
  notify.confirm(
    'Вы уверены, что хотите удалить этот узел?',
    'Подтверждение удаления',
    async () => {
      await deleteNode()
      notify.showSuccess('Узел успешно удален')
    }
  )
}

// Ввод данных
const handleInput = () => {
  notify.prompt(
    'Введите новое название узла',
    'Редактирование',
    async (value) => {
      await updateNodeName(value)
      notify.showSuccess('Название обновлено')
    }
  )
}

// Простые уведомления
const handleSuccess = () => {
  notify.showSuccess('Данные сохранены')
}

const handleError = () => {
  notify.showError('Произошла ошибка')
}

const handleWarning = () => {
  notify.showWarning('Внимание!')
}

const handleInfo = () => {
  notify.showInfo('Информация')
}
</script>
```

## API Reference

### useNotification()

Единый composable для всех типов уведомлений и диалогов.

#### Диалоги (Модальные окна)

##### confirm(message, title?, onConfirm?)

Показывает модальный диалог подтверждения.

```typescript
notify.confirm(
  'Вы уверены, что хотите удалить узел?',
  'Подтверждение удаления',
  async () => {
    await deleteNode()
    notify.showSuccess('Узел удален')
  }
)
```

##### prompt(message, title?, onConfirm?)

Показывает модальный диалог ввода данных.

```typescript
notify.prompt(
  'Введите новое значение',
  'Редактирование',
  async (value) => {
    await updateValue(value)
    notify.showSuccess('Значение обновлено')
  }
)
```

##### openDialog(config)

Показывает кастомный модальный диалог.

```typescript
notify.openDialog({
  title: 'Кастомный диалог',
  message: 'Содержимое',
  type: 'confirm',
  confirmText: 'OK',
  cancelText: 'Отмена',
  onConfirm: async () => {
    await performAction()
  }
})
```

##### closeDialog()

Закрывает текущий открытый диалог.

#### Уведомления (Toast/Snackbar)

##### showSuccess(message)

Показывает уведомление об успешной операции.

```typescript
notify.showSuccess('Данные сохранены')
```

##### showError(message)

Показывает уведомление об ошибке.

```typescript
notify.showError('Не удалось сохранить данные')
```

##### showWarning(message)

Показывает предупреждающее уведомление.

```typescript
notify.showWarning('Мало свободного места')
```

##### showInfo(message)

Показывает информационное уведомление.

```typescript
notify.showInfo('Операция выполнена')
```

##### addNotification(event)

Добавляет уведомление о системном событии (с данными узла и т.д.).

```typescript
// Простое уведомление
notify.addNotification('Новое событие')

// С событием узла
notify.addNotification({
  id: event.id,
  level: 'critical',
  message: 'Критическое событие',
  node_id: 'ph_ec_01',
  data: {
    ph: 6.5,
    target: 6.0
  }
})
```

##### removeNotification(id)

Удаляет уведомление по ID.

##### clearNotifications()

Очищает все уведомления.

#### Состояние

##### isDialogOpen()

Возвращает, открыт ли диалог.

```typescript
if (notify.isDialogOpen()) {
  console.log('Диалог открыт')
}
```

##### dialogLoading()

Возвращает, выполняется ли действие в диалоге.

##### dialogError()

Возвращает ошибку диалога, если есть.

##### notifications

Реактивный список всех активных уведомлений.

## Примеры использования

### Удаление с подтверждением

```typescript
import { useNotification } from '@/composables/useNotification'

const notify = useNotification()

const deleteNode = (nodeId: string) => {
  notify.confirm(
    `Вы уверены, что хотите удалить узел "${nodeId}"?`,
    'Подтверждение удаления',
    async () => {
      try {
        await api.deleteNode(nodeId)
        notify.showSuccess('Узел успешно удален')
        await nodesStore.fetchNodes()
      } catch (error) {
        notify.showError('Не удалось удалить узел')
      }
    }
  )
}
```

### Ввод данных

```typescript
const editNodeName = (currentName: string) => {
  notify.prompt(
    'Введите новое название узла',
    'Редактирование',
    async (newName) => {
      if (!newName || newName.trim() === '') {
        notify.showWarning('Название не может быть пустым')
        return
      }
      
      try {
        await api.updateNodeName(nodeId, newName)
        notify.showSuccess('Название обновлено')
        await nodesStore.fetchNodes()
      } catch (error) {
        notify.showError('Не удалось обновить название')
      }
    }
  )
}
```

### Обработка ошибок API

```typescript
const fetchData = async () => {
  try {
    const data = await api.getData()
    return data
  } catch (error) {
    const message = error.response?.data?.message || 'Произошла ошибка'
    notify.showError(message)
    throw error
  }
}
```

### Успешная операция

```typescript
const saveSettings = async () => {
  try {
    await api.saveSettings(settings)
    notify.showSuccess('Настройки сохранены')
  } catch (error) {
    notify.showError('Не удалось сохранить настройки')
  }
}
```

### Системное событие

```typescript
// В обработчике WebSocket событий
channel.listen('.event.created', (data) => {
  notify.addNotification({
    id: data.id,
    level: data.level || 'info',
    message: data.message,
    node_id: data.node_id,
    data: data.data
  })
})
```

## Глобальный экземпляр

Для использования без импорта composable в каждом компоненте:

```typescript
import { notificationManager } from '@/composables/useNotification'

// Использование
notificationManager.confirm('Вы уверены?', 'Подтверждение', async () => {
  await performAction()
})

notificationManager.showSuccess('Операция выполнена')
```

## Разделение ответственности

### Диалоги (confirm, prompt, openDialog)
**Используйте для:**
- ✅ Подтверждений удаления/опасных действий
- ✅ Ввода данных пользователем
- ✅ Модальных окон с действиями
- ✅ Кастомных диалогов

**Характеристики:**
- Блокируют интерфейс (модальные)
- Требуют действий пользователя
- Центрированы на экране

### Уведомления (showSuccess, showError, showWarning, showInfo)
**Используйте для:**
- ✅ Успешных операций
- ✅ Ошибок операций
- ✅ Предупреждений
- ✅ Информационных сообщений
- ✅ Системных событий (через addNotification)

**Характеристики:**
- Не блокируют интерфейс (non-blocking)
- Автоматически закрываются
- Располагаются в правом нижнем углу
- Могут показываться множественные

## Преимущества

✅ **Единая точка входа** - все уведомления через один API  
✅ **Разделение ответственности** - диалоги для действий, уведомления для событий  
✅ **Типобезопасность** - полная поддержка TypeScript  
✅ **Централизованное управление** - один менеджер для всего  
✅ **Удобство использования** - простой и понятный API  
✅ **Обратная совместимость** - старые composables (useSnackbar) продолжают работать  

## Миграция

### Старый способ (все еще работает)

```typescript
import { useDialog } from '@/composables/useDialog'
import { useSnackbar } from '@/composables/useSnackbar'

const dialog = useDialog()
const snackbar = useSnackbar()

dialog.confirm('Вы уверены?', 'Подтверждение', async () => {
  await performAction()
  snackbar.showSuccess('Готово')
})
```

### Новый способ (рекомендуется)

```typescript
import { useNotification } from '@/composables/useNotification'

const notify = useNotification()

notify.confirm('Вы уверены?', 'Подтверждение', async () => {
  await performAction()
  notify.showSuccess('Готово')
})
```

## Best Practices

1. **Используйте диалоги для действий пользователя** - confirm, prompt
2. **Используйте уведомления для событий системы** - success, error, warning, info
3. **Проверяйте ошибки** в async операциях
4. **Будьте лаконичны** в сообщениях
5. **Используйте правильный тип** - диалоги для критичных действий, уведомления для информации
6. **Обрабатывайте ошибки** - показывайте понятные сообщения пользователю

