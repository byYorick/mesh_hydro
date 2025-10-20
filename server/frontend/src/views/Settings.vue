<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">Настройки</h1>
      </v-col>
    </v-row>

    <!-- System Status -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>Статус системы</v-card-title>
          <v-card-text>
            <v-list v-if="systemStatus" density="compact">
              <v-list-item>
                <template v-slot:prepend>
                  <v-icon :color="systemStatus.database === 'ok' ? 'success' : 'error'">
                    {{ systemStatus.database === 'ok' ? 'mdi-database-check' : 'mdi-database-alert' }}
                  </v-icon>
                </template>
                <v-list-item-title>База данных</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.database }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <template v-slot:prepend>
                  <v-icon :color="systemStatus.mqtt === 'connected' ? 'success' : 'error'">
                    {{ systemStatus.mqtt === 'connected' ? 'mdi-access-point' : 'mdi-access-point-off' }}
                  </v-icon>
                </template>
                <v-list-item-title>MQTT Broker</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.mqtt }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item>
                <template v-slot:prepend>
                  <v-icon :color="systemStatus.telegram === 'ok' ? 'success' : 'warning'">
                    mdi-send
                  </v-icon>
                </template>
                <v-list-item-title>Telegram Bot</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.telegram }}</v-list-item-subtitle>
              </v-list-item>

              <v-divider class="my-4"></v-divider>

              <v-list-item v-if="systemStatus.system">
                <v-list-item-title>PHP Version</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.system.php_version }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item v-if="systemStatus.system">
                <v-list-item-title>Laravel Version</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.system.laravel_version }}</v-list-item-subtitle>
              </v-list-item>

              <v-list-item v-if="systemStatus.system">
                <v-list-item-title>Server Time</v-list-item-title>
                <v-list-item-subtitle>{{ systemStatus.system.server_time }}</v-list-item-subtitle>
              </v-list-item>
            </v-list>

            <div v-else class="text-center pa-4">
              <v-progress-circular indeterminate></v-progress-circular>
            </div>
          </v-card-text>

          <v-card-actions>
            <v-btn
              prepend-icon="mdi-refresh"
              @click="loadSystemStatus"
              :loading="loading"
            >
              Обновить статус
            </v-btn>
          </v-card-actions>
        </v-card>
      </v-col>
    </v-row>

    <!-- Appearance Settings -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>Внешний вид</v-card-title>
          <v-card-text>
            <v-switch
              :model-value="appStore.isDarkTheme"
              @update:model-value="appStore.toggleTheme()"
              label="Темная тема"
              color="primary"
            ></v-switch>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- PID Settings -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>PID Контроллеры</v-card-title>
          <v-card-text>
            <v-alert type="info" variant="tonal" class="mb-4">
              Тонкая настройка регуляторов pH и EC. Используйте небольшие изменения и наблюдайте за системой.
              <br><strong>💡 Совет:</strong> Начните с пресетов ниже, затем подстройте под свою систему.
            </v-alert>

            <!-- Пресеты -->
            <v-card variant="outlined" class="mb-4">
              <v-card-title class="text-subtitle-2">⚡ Быстрые пресеты (рекомендованные)</v-card-title>
              <v-card-text>
                <v-chip-group>
                  <v-chip 
                    @click="applyPreset('lettuce-nft')" 
                    prepend-icon="mdi-sprout" 
                    color="green"
                    variant="outlined"
                  >
                    Салат NFT (100L)
                  </v-chip>
                  <v-chip 
                    @click="applyPreset('tomato-drip')" 
                    prepend-icon="mdi-fruit-cherries" 
                    color="red"
                    variant="outlined"
                  >
                    Томаты капельный (300L)
                  </v-chip>
                  <v-chip 
                    @click="applyPreset('dwc-small')" 
                    prepend-icon="mdi-water" 
                    color="blue"
                    variant="outlined"
                  >
                    DWC малая (50L)
                  </v-chip>
                  <v-chip 
                    @click="applyPreset('conservative')" 
                    prepend-icon="mdi-shield-check" 
                    color="orange"
                    variant="outlined"
                  >
                    Консервативный (безопасный)
                  </v-chip>
                </v-chip-group>
              </v-card-text>
            </v-card>

            <v-row>
              <v-col cols="12" md="6">
                <div class="d-flex align-center mb-4">
                  <h3 class="text-subtitle-1 mr-2">pH Контроллер</h3>
                  <v-switch 
                    v-model="settingsStore.pid.ph.enabled" 
                    @update:model-value="onPidChange('ph','enabled', settingsStore.pid.ph.enabled)"
                    color="success"
                    hide-details
                    density="compact"
                  >
                    <template #label>
                      <span class="text-caption">{{ settingsStore.pid.ph.enabled ? 'Включён' : 'Выключен' }}</span>
                    </template>
                  </v-switch>
                  <HelpTooltip :text="pidMeta.ph.enabled" />
                </div>

                <v-divider class="mb-4"></v-divider>

                <h4 class="text-caption text-grey mb-2">ОСНОВНЫЕ ПАРАМЕТРЫ</h4>
                <v-text-field v-model.number="settingsStore.pid.ph.setpoint" type="number" step="0.1" label="Целевой pH"
                  @change="onPidChange('ph','setpoint', settingsStore.pid.ph.setpoint)"
                  suffix="pH" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.setpoint" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ph.kp" type="number" step="0.01" label="Kp (пропорциональный)"
                  @change="onPidChange('ph','kp', settingsStore.pid.ph.kp)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.kp" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ph.ki" type="number" step="0.001" label="Ki (интегральный)"
                  @change="onPidChange('ph','ki', settingsStore.pid.ph.ki)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.ki" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ph.kd" type="number" step="0.01" label="Kd (дифференциальный)"
                  @change="onPidChange('ph','kd', settingsStore.pid.ph.kd)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.kd" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ЗОНА НЕЧУВСТВИТЕЛЬНОСТИ</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ph.deadband" type="number" step="0.01" label="Deadband (гистерезис)"
                  @change="onPidChange('ph','deadband', settingsStore.pid.ph.deadband)" suffix="pH" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.deadband" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ТАЙМИНГИ</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ph.doseMinInterval" type="number" label="Мин. интервал между дозами"
                  @change="onPidChange('ph','doseMinInterval', settingsStore.pid.ph.doseMinInterval)" suffix="сек" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.doseMinInterval" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ph.mixDelay" type="number" label="Задержка перемешивания"
                  @change="onPidChange('ph','mixDelay', settingsStore.pid.ph.mixDelay)" suffix="сек" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.mixDelay" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ANTI-WINDUP</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ph.integralMax" type="number" step="0.5" label="Макс. интегратор"
                  @change="onPidChange('ph','integralMax', settingsStore.pid.ph.integralMax)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.integralMax" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ph.integralMin" type="number" step="0.5" label="Мин. интегратор"
                  @change="onPidChange('ph','integralMin', settingsStore.pid.ph.integralMin)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.integralMin" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ОГРАНИЧЕНИЯ ВЫХОДА</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ph.outputMax" type="number" step="0.5" label="Макс. доза за раз"
                  @change="onPidChange('ph','outputMax', settingsStore.pid.ph.outputMax)" suffix="мл" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.outputMax" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ph.outputMin" type="number" step="0.5" label="Мин. доза за раз"
                  @change="onPidChange('ph','outputMin', settingsStore.pid.ph.outputMin)" suffix="мл" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ph.outputMin" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ФИЛЬТРАЦИЯ ШУМА</h4>
                
                <v-slider 
                  v-model="settingsStore.pid.ph.filterAlpha" 
                  @update:model-value="onPidChange('ph','filterAlpha', settingsStore.pid.ph.filterAlpha)"
                  :min="0" :max="1" :step="0.05"
                  label="Фильтр показаний" 
                  thumb-label
                  density="compact"
                >
                  <template #append><HelpTooltip :text="pidMeta.ph.filterAlpha" /></template>
                </v-slider>
                <div class="text-caption text-grey">
                  {{ settingsStore.pid.ph.filterAlpha === 1.0 ? 'Без фильтрации' : 
                     settingsStore.pid.ph.filterAlpha > 0.7 ? 'Слабая фильтрация' : 
                     settingsStore.pid.ph.filterAlpha > 0.4 ? 'Средняя фильтрация' : 'Сильная фильтрация' }}
                </div>
              </v-col>

              <v-col cols="12" md="6">
                <div class="d-flex align-center mb-4">
                  <h3 class="text-subtitle-1 mr-2">EC Контроллер</h3>
                  <v-switch 
                    v-model="settingsStore.pid.ec.enabled" 
                    @update:model-value="onPidChange('ec','enabled', settingsStore.pid.ec.enabled)"
                    color="success"
                    hide-details
                    density="compact"
                  >
                    <template #label>
                      <span class="text-caption">{{ settingsStore.pid.ec.enabled ? 'Включён' : 'Выключен' }}</span>
                    </template>
                  </v-switch>
                  <HelpTooltip :text="pidMeta.ec.enabled" />
                </div>

                <v-divider class="mb-4"></v-divider>

                <h4 class="text-caption text-grey mb-2">ОСНОВНЫЕ ПАРАМЕТРЫ</h4>
                <v-text-field v-model.number="settingsStore.pid.ec.setpoint" type="number" step="0.1" label="Целевой EC"
                  @change="onPidChange('ec','setpoint', settingsStore.pid.ec.setpoint)" suffix="mS/cm" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.setpoint" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ec.kp" type="number" step="0.01" label="Kp (пропорциональный)"
                  @change="onPidChange('ec','kp', settingsStore.pid.ec.kp)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.kp" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ec.ki" type="number" step="0.001" label="Ki (интегральный)"
                  @change="onPidChange('ec','ki', settingsStore.pid.ec.ki)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.ki" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ec.kd" type="number" step="0.01" label="Kd (дифференциальный)"
                  @change="onPidChange('ec','kd', settingsStore.pid.ec.kd)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.kd" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ЗОНА НЕЧУВСТВИТЕЛЬНОСТИ</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ec.deadband" type="number" step="0.01" label="Deadband (гистерезис)"
                  @change="onPidChange('ec','deadband', settingsStore.pid.ec.deadband)" suffix="mS/cm" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.deadband" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ТАЙМИНГИ</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ec.doseMinInterval" type="number" label="Мин. интервал между дозами"
                  @change="onPidChange('ec','doseMinInterval', settingsStore.pid.ec.doseMinInterval)" suffix="сек" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.doseMinInterval" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ec.mixDelay" type="number" label="Задержка перемешивания"
                  @change="onPidChange('ec','mixDelay', settingsStore.pid.ec.mixDelay)" suffix="сек" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.mixDelay" /></template>
                </v-text-field>

                <v-text-field v-model.number="settingsStore.pid.ec.componentABDelay" type="number" label="Задержка между A и B"
                  @change="onPidChange('ec','componentABDelay', settingsStore.pid.ec.componentABDelay)" suffix="сек" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.componentABDelay" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ANTI-WINDUP</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ec.integralMax" type="number" step="0.5" label="Макс. интегратор"
                  @change="onPidChange('ec','integralMax', settingsStore.pid.ec.integralMax)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.integralMax" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ec.integralMin" type="number" step="0.5" label="Мин. интегратор"
                  @change="onPidChange('ec','integralMin', settingsStore.pid.ec.integralMin)" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.integralMin" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ОГРАНИЧЕНИЯ ВЫХОДА</h4>
                
                <v-text-field v-model.number="settingsStore.pid.ec.outputMax" type="number" step="0.5" label="Макс. доза за раз"
                  @change="onPidChange('ec','outputMax', settingsStore.pid.ec.outputMax)" suffix="мл" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.outputMax" /></template>
                </v-text-field>
                
                <v-text-field v-model.number="settingsStore.pid.ec.outputMin" type="number" step="0.5" label="Мин. доза за раз"
                  @change="onPidChange('ec','outputMin', settingsStore.pid.ec.outputMin)" suffix="мл" density="compact">
                  <template #append><HelpTooltip :text="pidMeta.ec.outputMin" /></template>
                </v-text-field>

                <v-divider class="my-3"></v-divider>
                <h4 class="text-caption text-grey mb-2">ФИЛЬТРАЦИЯ ШУМА</h4>
                
                <v-slider 
                  v-model="settingsStore.pid.ec.filterAlpha" 
                  @update:model-value="onPidChange('ec','filterAlpha', settingsStore.pid.ec.filterAlpha)"
                  :min="0" :max="1" :step="0.05"
                  label="Фильтр показаний" 
                  thumb-label
                  density="compact"
                >
                  <template #append><HelpTooltip :text="pidMeta.ec.filterAlpha" /></template>
                </v-slider>
                <div class="text-caption text-grey">
                  {{ settingsStore.pid.ec.filterAlpha === 1.0 ? 'Без фильтрации' : 
                     settingsStore.pid.ec.filterAlpha > 0.7 ? 'Слабая фильтрация' : 
                     settingsStore.pid.ec.filterAlpha > 0.4 ? 'Средняя фильтрация' : 'Сильная фильтрация' }}
                </div>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- Safety Settings -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>Безопасности дозирования и сенсоров</v-card-title>
          <v-card-text>
            <v-row>
              <v-col cols="12" md="6">
                <h3 class="text-subtitle-1 mb-2">Дозирование</h3>
                <v-text-field v-model.number="settingsStore.safety.dosing.maxDailyPhDown" type="number" label="Макс. суточный pH Down"
                  @change="onSafetyChange('dosing','maxDailyPhDown', settingsStore.safety.dosing.maxDailyPhDown)" suffix="мл">
                  <template #append><HelpTooltip :text="safetyMeta.dosing.maxDailyPhDown" /></template>
                </v-text-field>
                <v-text-field v-model.number="settingsStore.safety.dosing.maxDailyPhUp" type="number" label="Макс. суточный pH Up"
                  @change="onSafetyChange('dosing','maxDailyPhUp', settingsStore.safety.dosing.maxDailyPhUp)" suffix="мл">
                  <template #append><HelpTooltip :text="safetyMeta.dosing.maxDailyPhUp" /></template>
                </v-text-field>
                <v-text-field v-model.number="settingsStore.safety.dosing.maxDailyNutrients" type="number" label="Макс. суточные удобрения"
                  @change="onSafetyChange('dosing','maxDailyNutrients', settingsStore.safety.dosing.maxDailyNutrients)" suffix="мл">
                  <template #append><HelpTooltip :text="safetyMeta.dosing.maxDailyNutrients" /></template>
                </v-text-field>
                <v-text-field v-model.number="settingsStore.safety.dosing.minWaterLevel" type="number" label="Мин. уровень воды"
                  @change="onSafetyChange('dosing','minWaterLevel', settingsStore.safety.dosing.minWaterLevel)" suffix="ед.">
                  <template #append><HelpTooltip :text="safetyMeta.dosing.minWaterLevel" /></template>
                </v-text-field>
                <v-switch v-model="settingsStore.safety.dosing.tempLockout" label="Блокировка по температуре"
                  @update:model-value="onSafetyChange('dosing','tempLockout', settingsStore.safety.dosing.tempLockout)">
                  <template #append><HelpTooltip :text="safetyMeta.dosing.tempLockout" /></template>
                </v-switch>
              </v-col>

              <v-col cols="12" md="6">
                <h3 class="text-subtitle-1 mb-2">Сенсоры</h3>
                <v-text-field v-model.number="settingsStore.safety.sensors.phCalibrationDueDays" type="number" label="Калибровка pH каждые"
                  @change="onSafetyChange('sensors','phCalibrationDueDays', settingsStore.safety.sensors.phCalibrationDueDays)" suffix="дней">
                  <template #append><HelpTooltip :text="safetyMeta.sensors.phCalibrationDueDays" /></template>
                </v-text-field>
                <v-text-field v-model.number="settingsStore.safety.sensors.ecCalibrationDueDays" type="number" label="Калибровка EC каждые"
                  @change="onSafetyChange('sensors','ecCalibrationDueDays', settingsStore.safety.sensors.ecCalibrationDueDays)" suffix="дней">
                  <template #append><HelpTooltip :text="safetyMeta.sensors.ecCalibrationDueDays" /></template>
                </v-text-field>
                <v-switch v-model="settingsStore.safety.sensors.tempCompensation" label="Темп. компенсация EC"
                  @update:model-value="onSafetyChange('sensors','tempCompensation', settingsStore.safety.sensors.tempCompensation)">
                  <template #append><HelpTooltip :text="safetyMeta.sensors.tempCompensation" /></template>
                </v-switch>
              </v-col>
            </v-row>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>

    <!-- About -->
    <v-row>
      <v-col cols="12">
        <v-card>
          <v-card-title>О системе</v-card-title>
          <v-card-text>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title>Название</v-list-item-title>
                <v-list-item-subtitle>Mesh Hydro System</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Версия</v-list-item-title>
                <v-list-item-subtitle>2.0.0</v-list-item-subtitle>
              </v-list-item>
              <v-list-item>
                <v-list-item-title>Технологии</v-list-item-title>
                <v-list-item-subtitle>
                  Vue.js 3 • Vuetify 3 • Laravel 10 • PostgreSQL 15 • MQTT
                </v-list-item-subtitle>
              </v-list-item>
            </v-list>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useAppStore } from '@/stores/app'
import { useSettingsStore } from '@/stores/settings'
import HelpTooltip from '@/components/HelpTooltip.vue'
import { pidMeta, safetyMeta, systemMeta } from '@/services/configMeta'

const appStore = useAppStore()
const settingsStore = useSettingsStore()

const systemStatus = ref(null)
const loading = ref(false)

onMounted(async () => {
  await loadSystemStatus()
})

async function loadSystemStatus() {
  loading.value = true
  try {
    systemStatus.value = await appStore.fetchSystemStatus()
  } catch (error) {
    appStore.showSnackbar('Ошибка загрузки статуса', 'error')
  } finally {
    loading.value = false
  }
}

function onPidChange(group, key, value) {
  settingsStore.setPid(group, key, value)
}

function onSafetyChange(group, key, value) {
  settingsStore.setSafety(group, key, value)
}

function applyPreset(presetName) {
  const presets = {
    'lettuce-nft': {
      ph: {
        kp: 0.3, ki: 0.02, kd: 0.0, setpoint: 5.8, deadband: 0.08,
        doseMinInterval: 90, mixDelay: 45,
        integralMax: 10.0, integralMin: -10.0,
        outputMax: 5.0, outputMin: -5.0,
        filterAlpha: 0.25, enabled: true
      },
      ec: {
        kp: 0.2, ki: 0.01, kd: 0.0, setpoint: 1.6, deadband: 0.08,
        doseMinInterval: 180, mixDelay: 60, componentABDelay: 30,
        integralMax: 8.0, integralMin: -8.0,
        outputMax: 10.0, outputMin: 0.0,
        filterAlpha: 0.4, enabled: true
      }
    },
    'tomato-drip': {
      ph: {
        kp: 0.4, ki: 0.015, kd: 0.0, setpoint: 6.0, deadband: 0.1,
        doseMinInterval: 120, mixDelay: 60,
        integralMax: 10.0, integralMin: -10.0,
        outputMax: 8.0, outputMin: -8.0,
        filterAlpha: 0.3, enabled: true
      },
      ec: {
        kp: 0.25, ki: 0.008, kd: 0.0, setpoint: 2.7, deadband: 0.1,
        doseMinInterval: 240, mixDelay: 90, componentABDelay: 30,
        integralMax: 10.0, integralMin: -10.0,
        outputMax: 15.0, outputMin: 0.0,
        filterAlpha: 0.5, enabled: true
      }
    },
    'dwc-small': {
      ph: {
        kp: 0.25, ki: 0.025, kd: 0.0, setpoint: 5.8, deadband: 0.05,
        doseMinInterval: 60, mixDelay: 30,
        integralMax: 8.0, integralMin: -8.0,
        outputMax: 3.0, outputMin: -3.0,
        filterAlpha: 0.2, enabled: true
      },
      ec: {
        kp: 0.15, ki: 0.012, kd: 0.0, setpoint: 1.4, deadband: 0.05,
        doseMinInterval: 120, mixDelay: 45, componentABDelay: 25,
        integralMax: 6.0, integralMin: -6.0,
        outputMax: 5.0, outputMin: 0.0,
        filterAlpha: 0.3, enabled: true
      }
    },
    'conservative': {
      ph: {
        kp: 0.2, ki: 0.008, kd: 0.0, setpoint: 5.8, deadband: 0.1,
        doseMinInterval: 120, mixDelay: 60,
        integralMax: 5.0, integralMin: -5.0,
        outputMax: 3.0, outputMin: -3.0,
        filterAlpha: 0.2, enabled: true
      },
      ec: {
        kp: 0.15, ki: 0.005, kd: 0.0, setpoint: 1.6, deadband: 0.1,
        doseMinInterval: 300, mixDelay: 90, componentABDelay: 40,
        integralMax: 5.0, integralMin: -5.0,
        outputMax: 5.0, outputMin: 0.0,
        filterAlpha: 0.3, enabled: true
      }
    }
  }

  const preset = presets[presetName]
  if (!preset) return

  // Применить пресет для pH
  Object.keys(preset.ph).forEach(key => {
    settingsStore.setPid('ph', key, preset.ph[key])
  })

  // Применить пресет для EC
  Object.keys(preset.ec).forEach(key => {
    settingsStore.setPid('ec', key, preset.ec[key])
  })

  appStore.showSnackbar(`Применён пресет: ${getPresetName(presetName)}`, 'success')
}

function getPresetName(presetName) {
  const names = {
    'lettuce-nft': 'Салат NFT (100L)',
    'tomato-drip': 'Томаты капельный (300L)',
    'dwc-small': 'DWC малая (50L)',
    'conservative': 'Консервативный'
  }
  return names[presetName] || presetName
}
</script>

