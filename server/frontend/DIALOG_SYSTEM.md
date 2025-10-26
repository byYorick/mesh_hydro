# Система попапов - Улучшенная версия

## Обзор

Централизованная система управления диалогами для унификации попапов в приложении.

## Быстрый старт

```vue
<script setup>
import { useDialog } from '@/composables/useDialog'

const dialog = useDialog()

// Подтверждение
const handleDelete = () => {
  dialog.confirm(
    'Вы уверены, что хотите удалить этот узел?',
    'Подтверждение удаления',
    async () => {
      await deleteNode()
      dialog.success('Узел успешно удален')
    }
  )
}

// Сообщение об ошибке
const handleError = () => {
  dialog.error('Не удалось подключиться к серверу')
}

// Уведомление об успехе
const handleSuccess = () => {
  dialog.success('Данные успешно сохранены')
}

// Предупреждение
const handleWarning = () => {
  dialog.warning('У вас осталось мало места на диске')
}

// Обычное уведомление
const handleAlert = () => {
  dialog.alert('Настройки сохранены')
}
</script>
```

## API Reference

### useDialog()

Composable функция для работы с диалогами.

#### Методы

##### confirm(message, title?, onConfirm?)

Показывает диалог подтверждения.

```typescript
dialog.confirm(
  'Вы действительно хотите выйти?',
  'Подтверждение',
  async () => {
    await logout()
  }
)
```

##### alert(message, title?, onClose?)

Показывает диалог-уведомление.

```typescript
dialog.alert('Настройки сохранены')
```

##### error(message, title?)

Показывает диалог с ошибкой.

```typescript
dialog.error('Произошла ошибка соединения')
```

##### success(message, title?)

Показывает диалог успешного выполнения.

```typescript
dialog.success('Данные сохранены')
```

##### warning(message, title?)

Показывает диалог-предупреждение.

```typescript
dialog.warning('Недостаточно памяти')
```

##### prompt(message, title?, onConfirm?)

Показывает диалог ввода (в разработке).

##### openDialog(config)

Показывает кастомный диалог.

```typescript
dialog.openDialog({
  title: 'Кастомный диалог',
  message: 'Содержимое диалога',
  type: 'confirm',
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

##### closeDialog()

Закрывает текущий диалог.

##### confirmDialog()

Подтверждает текущий диалог.

#### Состояние

##### isOpen()

Возвращает, открыт ли диалог.

```typescript
if (dialog.isOpen()) {
  console.log('Dialog is open')
}
```

##### loading()

Возвращает, загружается ли действие.

```typescript
const isLoading = dialog.loading()
```

##### error()

Возвращает ошибку, если есть.

```typescript
const errorMessage = dialog.error()
```

##### config()

Возвращает конфигурацию текущего диалога.

## Advanced Usage

### Кастомный компонент в диалоге

```typescript
import CustomComponent from '@/components/CustomComponent.vue'

dialog.openDialog({
  title: 'Кастомный контент',
  component: CustomComponent,
  props: {
    data: someData
  },
  maxWidth: 800
})
```

### Async операции

```typescript
const handleDelete = () => {
  dialog.confirm(
    'Удалить узел?',
    'Подтверждение',
    async () => {
      // Dialog automatically shows loading state
      await api.deleteNode(id)
      // Dialog automatically closes on success
    }
  )
}
```

### Persistent dialogs

```typescript
dialog.openDialog({
  title: 'Важное уведомление',
  message: 'Это диалог нельзя закрыть',
  persistent: true,
  confirmText: 'Понял'
})
```

## Конфигурация DialogConfig

```typescript
interface DialogConfig {
  // Текст
  title?: string
  message?: string
  confirmText?: string
  cancelText?: string
  
  // Поведение
  persistent?: boolean
  maxWidth?: string | number
  
  // Тип
  type?: 'confirm' | 'alert' | 'prompt' | 'custom'
  
  // Стиль
  color?: string
  icon?: string
  
  // Callbacks
  onConfirm?: () => void | Promise<void>
  onCancel?: () => void
  
  // Кастомный компонент
  component?: any
  props?: Record<string, any>
}
```

## Примеры использования

### Удаление с подтверждением

```typescript
const deleteNode = (nodeId: string) => {
  dialog.confirm(
    `Вы уверены, что хотите удалить узел "${nodeId}"?`,
    'Подтверждение удаления',
    async () => {
      try {
        await api.deleteNode(nodeId)
        dialog.success('Узел успешно удален')
        await nodesStore.fetchNodes()
      } catch (error) {
        dialog.error('Не удалось удалить узел')
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
    dialog.error(
      error.response?.data?.message || 'Произошла ошибка'
    )
    throw error
  }
}
```

### Успешная операция

```typescript
const saveSettings = async () => {
  try {
    await api.saveSettings(settings)
    dialog.success('Настройки сохранены')
  } catch (error) {
    dialog.error('Не удалось сохранить настройки')
  }
}
```

### Предупреждение

```typescript
const checkDiskSpace = () => {
  const freeSpace = getDiskSpace()
  if (freeSpace < 1024) {
    dialog.warning(
      `Свободного места осталось: ${freeSpace}MB`,
      'Мало места на диске'
    )
  }
}
```

## Миграция со старых диалогов

### Было:

```vue
<template>
  <v-dialog v-model="showDialog" max-width="500">
    <v-card>
      <v-card-title>Подтверждение</v-card-title>
      <v-card-text>
        Вы уверены?
      </v-card-text>
      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn @click="showDialog = false">Отмена</v-btn>
        <v-btn color="primary" @click="confirm">Да</v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup>
const showDialog = ref(false)

const confirm = async () => {
  await performAction()
  showDialog.value = false
}
</script>
```

### Стало:

```vue
<script setup>
import { useDialog } from '@/composables/useDialog'

const dialog = useDialog()

const handleConfirm = () => {
  dialog.confirm(
    'Вы уверены?',
    'Подтверждение',
    async () => {
      await performAction()
    }
  )
}
</script>
```

## Преимущества

✅ **Единый стиль** - все диалоги выглядят одинаково  
✅ **Меньше кода** - не нужно создавать компоненты для каждого диалога  
✅ **Централизованное управление** - один менеджер для всех диалогов  
✅ **Автоматический loading** - показывается автоматически во время async операций  
✅ **Обработка ошибок** - автоматическое отображение ошибок  
✅ **Type-safe** - полная поддержка TypeScript  
✅ **Производительность** - один компонент для всех диалогов  

## Компоненты

### UniversalDialog.vue

Основной компонент для отображения диалогов.

**Props:** Нет (использует composable)

### useDialog.ts

Composable для управления диалогами.

**Exports:**
- `useDialog()` - функция для получения экземпляра
- `dialogManager` - глобальный экземпляр менеджера
- `DialogConfig` - тип конфигурации

## Best Practices

1. **Используйте методы-хелперы** вместо `openDialog` когда возможно
2. **Проверяйте ошибки** в async операциях
3. **Используйте persistent** для важных диалогов
4. **Не перегружайте сообщениями** - будьте лаконичны
5. **Используйте иконки** для лучшего UX

## Troubleshooting

### Диалог не закрывается

Убедитесь, что `onConfirm` не выбрасывает исключения.

### Loading не показывается

Проверьте, что callback - `async` функция.

### Ошибка в TypeScript

Убедитесь, что импортирован правильный тип:

```typescript
import type { DialogConfig } from '@/composables/useDialog'
```
