<template>
  <v-container fluid v-if="cycle">
    <v-row>
      <v-col cols="12">
        <div class="d-flex align-center mb-4">
          <v-btn
            icon="mdi-arrow-left"
            variant="text"
            @click="$router.back()"
            class="mr-2"
          ></v-btn>
          <div>
            <h1 class="text-h4 mb-1">{{ cycle.name || 'Цикл роста' }}</h1>
            <p class="text-medium-emphasis">{{ cycle.culture?.name || 'Культура' }} в {{ cycle.zone?.name || 'Зоне' }}</p>
          </div>
          <v-spacer></v-spacer>
          <v-chip
            :color="cycle.status_color"
            size="large"
            variant="flat"
          >
            {{ getStatusText(cycle.status) }}
          </v-chip>
        </div>
      </v-col>
    </v-row>

    <!-- Статистика -->
    <v-row class="mb-4">
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="text-caption text-medium-emphasis">День цикла</div>
            <div class="text-h5 font-weight-bold">{{ cycle.current_day || 0 }}</div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="text-caption text-medium-emphasis">Прогресс</div>
            <div class="text-h5 font-weight-bold">{{ Math.round(cycle.progress || 0) }}%</div>
            <v-progress-linear
              :model-value="cycle.progress || 0"
              color="success"
              height="8"
              class="mt-2"
            ></v-progress-linear>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="text-caption text-medium-emphasis">Текущая стадия</div>
            <div class="text-body-1 font-weight-bold">{{ cycle.current_stage?.name || 'Не начата' }}</div>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="3">
        <v-card variant="outlined">
          <v-card-text>
            <div class="text-caption text-medium-emphasis">Ожидаемый сбор</div>
            <div class="text-body-2">{{ formatDate(cycle.expected_harvest_at) }}</div>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Временная шкала стадий -->
    <v-row class="mb-4">
      <v-col cols="12">
        <v-card variant="outlined">
          <v-card-title>Стадии роста</v-card-title>
          <v-card-text>
            <StageTimeline
              :stages="cycle.preset?.stages || []"
              :current-stage-id="cycle.current_stage_id"
              :completed-stage-ids="completedStageIds"
              :show-details="true"
              :show-target-params="true"
            />
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Информация о цикле -->
    <v-row>
      <v-col cols="12" md="6">
        <v-card variant="outlined">
          <v-card-title>Информация о цикле</v-card-title>
          <v-card-text>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title>Пресет</v-list-item-title>
                <v-list-item-subtitle>{{ cycle.preset?.name || 'Не указан' }}</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Зона</v-list-item-title>
                <v-list-item-subtitle>{{ cycle.zone?.name || 'Не указана' }}</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Начало</v-list-item-title>
                <v-list-item-subtitle>{{ formatDateTime(cycle.started_at) }}</v-list-item-subtitle>
              </v-list-item>
              <v-list-item v-if="cycle.plant_count">
                <v-list-item-title>Количество растений</v-list-item-title>
                <v-list-item-subtitle>{{ cycle.plant_count }}</v-list-item-subtitle>
              </v-list-item>
            </v-list>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="6">
        <v-card variant="outlined">
          <v-card-title>Действия</v-card-title>
          <v-card-text>
            <v-btn
              v-if="cycle.status === 'active'"
              color="success"
              prepend-icon="mdi-check-circle"
              @click="handleHarvest"
              class="mb-2"
              block
            >
              Собрать урожай
            </v-btn>
            <v-btn
              v-if="cycle.status === 'active'"
              color="warning"
              prepend-icon="mdi-pause"
              @click="handlePause"
              class="mb-2"
              block
            >
              Приостановить
            </v-btn>
            <v-btn
              v-if="cycle.status === 'active'"
              color="error"
              prepend-icon="mdi-cancel"
              @click="handleCancel"
              block
            >
              Отменить цикл
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- TODO: Графики параметров, аналитика, история -->
  </v-container>

  <v-container v-else>
    <div class="text-center py-8">
      <v-progress-circular indeterminate color="primary" size="64"></v-progress-circular>
      <p class="mt-4 text-medium-emphasis">Загрузка цикла...</p>
    </div>
  </v-container>
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useRoute, useRouter } from 'vue-router'
import { useGrowthStore, type GrowthCycle } from '@/stores/growth'
import StageTimeline from '@/components/growth/StageTimeline.vue'
// Временные функции форматирования (TODO: вынести в utils)
function formatDate(date: string | Date | null | undefined): string {
  if (!date) return 'Не указано'
  const d = new Date(date)
  return d.toLocaleDateString('ru-RU')
}

function formatDateTime(date: string | Date | null | undefined): string {
  if (!date) return 'Не указано'
  const d = new Date(date)
  return d.toLocaleString('ru-RU')
}

const route = useRoute()
const router = useRouter()
const growthStore = useGrowthStore()

const cycle = ref<GrowthCycle | null>(null)
const loading = ref(false)

const completedStageIds = computed(() => {
  if (!cycle.value?.stage_history) return []
  return cycle.value.stage_history
    .filter((h: any) => h.ended_at)
    .map((h: any) => h.stage_id)
})

function getStatusText(status: string): string {
  const statuses: Record<string, string> = {
    pending: 'Планирование',
    active: 'Активен',
    paused: 'Приостановлен',
    completed: 'Завершён',
    cancelled: 'Отменён',
  }
  return statuses[status] || status
}

async function handleHarvest() {
  // TODO: Открыть диалог сбора урожая
  router.push({ query: { action: 'harvest' } })
}

async function handlePause() {
  // TODO: Приостановить цикл
  console.log('Pause cycle')
}

async function handleCancel() {
  if (confirm('Вы уверены, что хотите отменить этот цикл?')) {
    try {
      await growthStore.cancelCycle(cycle.value!.id, 'Отменено пользователем')
      await loadCycle()
    } catch (error) {
      console.error('Error cancelling cycle:', error)
    }
  }
}

async function loadCycle() {
  loading.value = true
  try {
    const cycleId = parseInt(route.params.id as string)
    await growthStore.fetchCycle(cycleId)
    cycle.value = growthStore.getCycleById(cycleId) || null
  } catch (error) {
    console.error('Error loading cycle:', error)
  } finally {
    loading.value = false
  }
}

onMounted(() => {
  loadCycle()
})
</script>

