# Реализация системы управления конфигурациями узлов

## ✅ Созданные компоненты

### 1. Pinia Store
**Файл:** `server/frontend/src/stores/nodeConfig.js`
- ✅ Управление состоянием конфигурации
- ✅ Actions: loadConfig, saveConfig, calibratePump, runPump
- ✅ State: config, loading, history, calibrations

### 2. Калибровка насосов
**Файл:** `server/frontend/src/components/node-config/PumpCalibrationDialog.vue`
- ✅ Форма выбора насоса, объема и длительности
- ✅ Прогресс-бар и таймер во время калибровки
- ✅ Отображение результатов
- ✅ Валидация данных

### 3. Ручное управление насосами
**Файл:** `server/frontend/src/components/node-config/ManualPumpControl.vue`
- ✅ Выбор насоса
- ✅ Слайдер длительности (0.1-30 сек)
- ✅ Быстрые кнопки (1, 3, 5, 10, 15, 30 сек)
- ✅ Кнопка запуска/остановки
- ✅ Прогресс и обратный отсчет времени

### 4. Композабл уведомлений
**Файл:** `server/frontend/src/composables/useSnackbar.js`
- ✅ Показ успешных/ошибочных сообщений
- ✅ TODO: Интеграция с Vuetify snackbar

## 📝 Инструкция по интеграции

### Вариант 1: Добавить кнопку в NodeCard

```vue
<!-- В NodeCard.vue -->
<template>
  <!-- ... существующий код ... -->
  
  <v-card-actions>
    <!-- ... существующие кнопки ... -->
    
    <!-- Новая кнопка управления -->
    <NodeConfigButton :node="node" />
  </v-card-actions>
</template>

<script setup>
import NodeConfigButton from '@/components/NodeConfigButton.vue'
// ... остальные импорты
</script>
```

### Вариант 2: Создать отдельную страницу

Создать `NodeConfig.vue`:

```vue
<template>
  <v-container>
    <v-row>
      <v-col cols="12" md="6">
        <ManualPumpControl
          :node-id="node.node_id"
          :is-online="node.online"
        />
      </v-col>
      
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Калибровка насосов</v-card-title>
          <v-card-text>
            <v-btn @click="calibrationDialog = true">
              Открыть калибровку
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
    
    <!-- Dialogs -->
    <PumpCalibrationDialog
      v-model="calibrationDialog"
      :node-id="node.node_id"
      @calibrated="onCalibrated"
    />
  </v-container>
</template>

<script setup>
import { ref } from 'vue'
import ManualPumpControl from '@/components/node-config/ManualPumpControl.vue'
import PumpCalibrationDialog from '@/components/node-config/PumpCalibrationDialog.vue'

const calibrationDialog = ref(false)

function onCalibrated(result) {
  console.log('Pump calibrated:', result)
}
</script>
```

## 🎯 Следующие шаги

1. **Доработать ConfigViewDialog.vue**
   - Добавить редактирование параметров
   - Интегрировать с nodeConfig store
   - Добавить вкладки (Основные, Насосы, PID, История)

2. **Создать основной диалог конфигурации**
   - `NodeConfigDialog.vue` с табами
   - Объединить все компоненты

3. **Добавить кнопку в NodeCard**
   - "Настройки" или "Конфигурация"
   - Открывает главный диалог

4. **Реализовать snackbar через Vuetify**
   - Глобальный компонент v-snackbar
   - Интеграция с useSnackbar composable

## 📚 Примеры использования

### Калибровка насоса

```vue
<template>
  <PumpCalibrationDialog
    v-model="calibrationDialog"
    :node-id="node.node_id"
    :pump-options="[
      { label: 'pH Up', value: 0 },
      { label: 'pH Down', value: 1 }
    ]"
    @calibrated="handleCalibration"
  />
</template>

<script setup>
import { ref } from 'vue'
import PumpCalibrationDialog from '@/components/node-config/PumpCalibrationDialog.vue'

const calibrationDialog = ref(false)

function handleCalibration(result) {
  console.log('Calibrated:', result)
  // Обновить UI, показать уведомление
}
</script>
```

### Ручное управление

```vue
<template>
  <ManualPumpControl
    :node-id="node.node_id"
    :is-online="node.online"
    @pump-started="onPumpStart"
    @pump-stopped="onPumpStop"
  />
</template>

<script setup>
import ManualPumpControl from '@/components/node-config/ManualPumpControl.vue'

function onPumpStart(event) {
  console.log('Pump started:', event)
}

function onPumpStop() {
  console.log('Pump stopped')
}
</script>
```

## ✨ Готово к использованию!

Все основные компоненты созданы и готовы к интеграции. Следующий шаг - добавить их в существующие компоненты (NodeCard, NodeDetail и т.д.).
