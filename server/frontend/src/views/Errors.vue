<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">Системные ошибки</h1>
      </v-col>
    </v-row>

    <!-- Statistics Cards -->
    <v-row>
      <v-col cols="12" sm="6" md="3">
        <v-card color="error" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ statistics?.total || 0 }}
            </div>
            <div class="text-subtitle-1">Всего ошибок</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="warning" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ statistics?.active || 0 }}
            </div>
            <div class="text-subtitle-1">Активных</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="purple" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ statistics?.critical || 0 }}
            </div>
            <div class="text-subtitle-1">Критичных</div>
          </v-card-text>
        </v-card>
      </v-col>

      <v-col cols="12" sm="6" md="3">
        <v-card color="success" variant="tonal">
          <v-card-text class="text-center">
            <div class="text-h3 font-weight-bold">
              {{ statistics?.resolved || 0 }}
            </div>
            <div class="text-subtitle-1">Решено</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Filters -->
    <v-row>
      <v-col cols="12" md="3">
        <v-select
          v-model="filters.node_id"
          :items="nodeOptions"
          label="Узел"
          variant="outlined"
          clearable
          prepend-inner-icon="mdi-access-point"
        ></v-select>
      </v-col>

      <v-col cols="12" md="3">
        <v-select
          v-model="filters.severity"
          :items="severityOptions"
          label="Критичность"
          variant="outlined"
          clearable
        ></v-select>
      </v-col>

      <v-col cols="12" md="3">
        <v-select
          v-model="filters.error_type"
          :items="typeOptions"
          label="Тип ошибки"
          variant="outlined"
          clearable
        ></v-select>
      </v-col>

      <v-col cols="12" md="3">
        <v-select
          v-model="filters.status"
          :items="statusOptions"
          label="Статус"
          variant="outlined"
          clearable
        ></v-select>
      </v-col>
    </v-row>

    <!-- Errors Table -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title class="d-flex align-center">
            <v-icon icon="mdi-bug" class="mr-2"></v-icon>
            Список ошибок
            
            <v-spacer></v-spacer>
            
            <v-btn
              v-if="hasUnresolvedErrors"
              color="primary"
              prepend-icon="mdi-check-all"
              @click="resolveAllErrors"
              :loading="resolving"
            >
              Решить все
            </v-btn>
          </v-card-title>

          <v-card-text class="pa-0">
            <v-data-table
              :headers="headers"
              :items="errorsStore.errors"
              :loading="errorsStore.loading"
              :items-per-page="20"
              class="elevation-0"
            >
              <template v-slot:item.severity="{ item }">
                <v-chip
                  :color="getSeverityColor(item.severity)"
                  size="small"
                >
                  <v-icon :icon="getSeverityIcon(item.severity)" start size="small"></v-icon>
                  {{ item.severity }}
                </v-chip>
              </template>

              <template v-slot:item.error_type="{ item }">
                <v-chip size="small" variant="outlined">
                  <v-icon :icon="getTypeIcon(item.error_type)" start size="small"></v-icon>
                  {{ item.error_type }}
                </v-chip>
              </template>

              <template v-slot:item.occurred_at="{ item }">
                <span class="text-caption">
                  {{ formatDateTime(item.occurred_at) }}
                </span>
              </template>

              <template v-slot:item.resolved_at="{ item }">
                <v-chip
                  v-if="item.resolved_at"
                  color="success"
                  size="small"
                  variant="flat"
                >
                  Решено
                </v-chip>
                <v-chip
                  v-else
                  color="warning"
                  size="small"
                  variant="outlined"
                >
                  Активно
                </v-chip>
              </template>

              <template v-slot:item.actions="{ item }">
                <v-btn
                  icon="mdi-eye"
                  size="small"
                  variant="text"
                  @click="viewError(item)"
                ></v-btn>
                
                <v-btn
                  v-if="!item.resolved_at"
                  icon="mdi-check"
                  size="small"
                  variant="text"
                  color="success"
                  @click="resolveError(item.id)"
                ></v-btn>
              </template>

              <template v-slot:no-data>
                <div class="text-center pa-8">
                  <v-icon icon="mdi-check-circle" size="64" color="success"></v-icon>
                  <div class="mt-4 text-h6">Ошибок не обнаружено!</div>
                  <div class="text-caption text-disabled">Система работает отлично</div>
                </div>
              </template>
            </v-data-table>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Error Details Dialog -->
    <ErrorDetailsDialog
      v-model="showDetailsDialog"
      :error="selectedError"
      @error-resolved="handleErrorResolved"
    />
  </div>
</template>

<script setup>
import { ref, computed, onMounted, watch } from 'vue'
import { useErrorsStore } from '@/stores/errors'
import { useNodesStore } from '@/stores/nodes'
import { useAppStore } from '@/stores/app'
import { formatDateTime } from '@/utils/time'
import ErrorDetailsDialog from '@/components/ErrorDetailsDialog.vue'

const errorsStore = useErrorsStore()
const nodesStore = useNodesStore()
const appStore = useAppStore()

const filters = ref({
  node_id: null,
  severity: null,
  error_type: null,
  status: null,
  hours: 24,
})

const statistics = ref(null)
const selectedError = ref(null)
const showDetailsDialog = ref(false)
const resolving = ref(false)

const headers = [
  { title: 'ID', key: 'id', width: '80px' },
  { title: 'Узел', key: 'node_id' },
  { title: 'Критичность', key: 'severity' },
  { title: 'Тип', key: 'error_type' },
  { title: 'Сообщение', key: 'message' },
  { title: 'Время', key: 'occurred_at' },
  { title: 'Статус', key: 'resolved_at' },
  { title: 'Действия', key: 'actions', sortable: false, width: '120px' },
]

const nodeOptions = computed(() => {
  return nodesStore.nodes.map(node => ({
    title: node.node_id,
    value: node.node_id,
  }))
})

const severityOptions = [
  { title: 'Low', value: 'low' },
  { title: 'Medium', value: 'medium' },
  { title: 'High', value: 'high' },
  { title: 'Critical', value: 'critical' },
]

const typeOptions = [
  { title: 'Hardware', value: 'hardware' },
  { title: 'Software', value: 'software' },
  { title: 'Network', value: 'network' },
  { title: 'Sensor', value: 'sensor' },
]

const statusOptions = [
  { title: 'Активные', value: 'active' },
  { title: 'Решенные', value: 'resolved' },
]

const hasUnresolvedErrors = computed(() => {
  return errorsStore.unresolvedErrors.length > 0
})

// Watch filters and reload data
watch(filters, () => {
  loadErrors()
}, { deep: true })

onMounted(async () => {
  await loadErrors()
  await loadStatistics()
})

async function loadErrors() {
  try {
    const params = {}
    
    if (filters.value.node_id) params.node_id = filters.value.node_id
    if (filters.value.severity) params.severity = filters.value.severity
    if (filters.value.error_type) params.error_type = filters.value.error_type
    if (filters.value.status) params.status = filters.value.status
    if (filters.value.hours) params.hours = filters.value.hours
    
    await errorsStore.fetchErrors(params)
  } catch (error) {
    appStore.showSnackbar('Ошибка загрузки данных', 'error')
  }
}

async function loadStatistics() {
  try {
    statistics.value = await errorsStore.fetchStatistics(filters.value.hours)
  } catch (error) {
    console.error('Error loading statistics:', error)
  }
}

function viewError(error) {
  selectedError.value = error
  showDetailsDialog.value = true
}

async function resolveError(errorId) {
  try {
    await errorsStore.resolveError(errorId, 'manual')
    appStore.showSnackbar('Ошибка помечена как решенная', 'success')
    await loadStatistics()
  } catch (error) {
    appStore.showSnackbar('Ошибка при резолвении', 'error')
  }
}

async function resolveAllErrors() {
  try {
    resolving.value = true
    const errorIds = errorsStore.unresolvedErrors.map(e => e.id)
    await errorsStore.resolveBulkErrors(errorIds, 'bulk_manual')
    appStore.showSnackbar(`Решено ${errorIds.length} ошибок`, 'success')
    await loadErrors()
    await loadStatistics()
  } catch (error) {
    appStore.showSnackbar('Ошибка массового резолвения', 'error')
  } finally {
    resolving.value = false
  }
}

function handleErrorResolved() {
  loadErrors()
  loadStatistics()
  showDetailsDialog.value = false
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

function getSeverityIcon(severity) {
  const icons = {
    low: 'mdi-information',
    medium: 'mdi-alert',
    high: 'mdi-alert-circle',
    critical: 'mdi-fire',
  }
  return icons[severity] || 'mdi-help-circle'
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

