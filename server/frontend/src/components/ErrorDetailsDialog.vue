<template>
  <v-dialog
    :model-value="modelValue"
    @update:model-value="$emit('update:modelValue', $event)"
    max-width="900px"
    scrollable
  >
    <v-card v-if="error">
      <v-card-title class="d-flex align-center bg-error">
        <v-icon icon="mdi-alert-circle" class="mr-2"></v-icon>
        Детали ошибки #{{ error.id }}
        
        <v-spacer></v-spacer>
        
        <v-btn
          icon="mdi-close"
          variant="text"
          @click="$emit('update:modelValue', false)"
        ></v-btn>
      </v-card-title>

      <v-tabs v-model="activeTab" bg-color="surface">
        <v-tab value="overview">
          <v-icon start>mdi-information</v-icon>
          Описание
        </v-tab>
        <v-tab value="diagnostics">
          <v-icon start>mdi-stethoscope</v-icon>
          Диагностика
        </v-tab>
        <v-tab value="stacktrace" v-if="error.stack_trace">
          <v-icon start>mdi-code-tags</v-icon>
          Stack Trace
        </v-tab>
        <v-tab value="resolve">
          <v-icon start>mdi-check</v-icon>
          Решение
        </v-tab>
      </v-tabs>

      <v-window v-model="activeTab">
        <!-- Overview Tab -->
        <v-window-item value="overview">
          <v-card-text>
            <v-list lines="two">
              <v-list-item>
                <v-list-item-title>Узел</v-list-item-title>
                <v-list-item-subtitle>{{ error.node_id }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <v-list-item-title>Код ошибки</v-list-item-title>
                <v-list-item-subtitle>
                  <v-chip size="small" color="error">{{ error.error_code }}</v-chip>
                </v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <v-list-item-title>Критичность</v-list-item-title>
                <v-list-item-subtitle>
                  <v-chip :color="getSeverityColor(error.severity)" size="small">
                    {{ error.severity }}
                  </v-chip>
                </v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <v-list-item-title>Тип</v-list-item-title>
                <v-list-item-subtitle>
                  <v-chip size="small" variant="outlined">
                    <v-icon :icon="getTypeIcon(error.error_type)" start size="small"></v-icon>
                    {{ error.error_type }}
                  </v-chip>
                </v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <v-list-item-title>Сообщение</v-list-item-title>
                <v-list-item-subtitle class="text-wrap">
                  {{ error.message }}
                </v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <v-list-item-title>Время возникновения</v-list-item-title>
                <v-list-item-subtitle>
                  {{ formatDateTime(error.occurred_at) }}
                </v-list-item-subtitle>
              </v-list-item>

              <v-list-item v-if="error.resolved_at">
                <v-list-item-title>Решено</v-list-item-title>
                <v-list-item-subtitle>
                  {{ formatDateTime(error.resolved_at) }} ({{ error.resolved_by }})
                </v-list-item-subtitle>
              </v-list-item>
            </v-list>
          </v-card-text>
        </v-window-item>

        <!-- Diagnostics Tab -->
        <v-window-item value="diagnostics">
          <v-card-text>
            <v-list v-if="error.diagnostics" lines="two">
              <v-list-item v-for="(value, key) in error.diagnostics" :key="key">
                <v-list-item-title class="text-capitalize">{{ formatKey(key) }}</v-list-item-title>
                <v-list-item-subtitle>{{ formatValue(value) }}</v-list-item-subtitle>
              </v-list-item>
            </v-list>
            <div v-else class="text-center pa-8 text-disabled">
              Нет диагностической информации
            </div>

            <!-- Copy button -->
            <v-divider class="my-4"></v-divider>
            <v-btn
              block
              color="primary"
              prepend-icon="mdi-content-copy"
              @click="copyDiagnostics"
            >
              Скопировать диагностику
            </v-btn>
          </v-card-text>
        </v-window-item>

        <!-- Stack Trace Tab -->
        <v-window-item value="stacktrace" v-if="error.stack_trace">
          <v-card-text>
            <pre class="stack-trace">{{ error.stack_trace }}</pre>
            
            <v-divider class="my-4"></v-divider>
            <v-btn
              block
              color="primary"
              prepend-icon="mdi-content-copy"
              @click="copyStackTrace"
            >
              Скопировать Stack Trace
            </v-btn>
          </v-card-text>
        </v-window-item>

        <!-- Resolve Tab -->
        <v-window-item value="resolve">
          <v-card-text>
            <v-form ref="resolveForm" v-model="resolveValid">
              <v-textarea
                v-model="resolutionNotes"
                label="Заметки о решении"
                variant="outlined"
                rows="5"
                hint="Опишите как была решена проблема"
                persistent-hint
              ></v-textarea>

              <v-btn
                block
                color="success"
                class="mt-4"
                :disabled="error.resolved_at !== null"
                :loading="resolving"
                prepend-icon="mdi-check-circle"
                @click="resolveErrorWithNotes"
              >
                {{ error.resolved_at ? 'Уже решено' : 'Пометить как решенное' }}
              </v-btn>

              <v-alert
                v-if="error.resolved_at"
                type="success"
                variant="tonal"
                class="mt-4"
              >
                <div class="text-subtitle-2">Решено: {{ formatDateTime(error.resolved_at) }}</div>
                <div v-if="error.resolution_notes" class="text-caption mt-2">
                  {{ error.resolution_notes }}
                </div>
              </v-alert>
            </v-form>
          </v-card-text>
        </v-window-item>
      </v-window>
    </v-card>
  </v-dialog>
</template>

<script setup>
import { ref, computed } from 'vue'
import { useAppStore } from '@/stores/app'
import { useErrorsStore } from '@/stores/errors'
import { formatDateTime } from '@/utils/time'

const props = defineProps({
  modelValue: Boolean,
  error: {
    type: Object,
    default: null,
  },
})

const emit = defineEmits(['update:modelValue', 'error-resolved'])

const appStore = useAppStore()
const errorsStore = useErrorsStore()

const activeTab = ref('overview')
const resolveForm = ref(null)
const resolveValid = ref(false)
const resolutionNotes = ref('')
const resolving = ref(false)

async function resolveErrorWithNotes() {
  if (!props.error || props.error.resolved_at) return
  
  resolving.value = true
  
  try {
    await errorsStore.resolveError(
      props.error.id,
      'manual',
      resolutionNotes.value
    )
    
    appStore.showSnackbar('Ошибка решена!', 'success')
    emit('error-resolved')
  } catch (error) {
    appStore.showSnackbar('Ошибка при резолвении', 'error')
  } finally {
    resolving.value = false
  }
}

function copyDiagnostics() {
  if (!props.error?.diagnostics) return
  
  const text = JSON.stringify(props.error.diagnostics, null, 2)
  navigator.clipboard.writeText(text)
  appStore.showSnackbar('Диагностика скопирована', 'success')
}

function copyStackTrace() {
  if (!props.error?.stack_trace) return
  
  navigator.clipboard.writeText(props.error.stack_trace)
  appStore.showSnackbar('Stack trace скопирован', 'success')
}

function formatKey(key) {
  return key.replace(/_/g, ' ')
}

function formatValue(value) {
  if (typeof value === 'object') {
    return JSON.stringify(value)
  }
  return String(value)
}

function getSeverityColor(severity) {
  const colors = {
    low: 'info',
    medium: 'warning',
    high: 'orange',
    critical: 'error',
  }
  return colors[severity] || 'grey'
}

function getTypeIcon(type) {
  const icons = {
    hardware: 'mdi-chip',
    software: 'mdi-bug',
    network: 'mdi-wifi-alert',
    sensor: 'mdi-thermometer-alert',
  }
  return icons[type] || 'mdi-alert-circle'
}
</script>

<style scoped>
.stack-trace {
  background: rgba(0, 0, 0, 0.05);
  padding: 16px;
  border-radius: 8px;
  overflow-x: auto;
  font-family: 'Courier New', monospace;
  font-size: 12px;
  line-height: 1.5;
}

.v-theme--dark .stack-trace {
  background: rgba(255, 255, 255, 0.05);
}
</style>

