<template>
  <div>
    <!-- Управление узлом с раскрывающимися секциями -->
    <v-card v-if="node" class="mb-4">
      <v-card-title class="d-flex align-center">
        <v-icon icon="mdi-cog" class="mr-2"></v-icon>
        Управление узлом
        <v-spacer></v-spacer>
        <v-chip 
          :color="statusColor" 
          size="small"
          :prepend-icon="statusIcon"
        >
          {{ statusText }}
        </v-chip>
      </v-card-title>

      <!-- Debug info -->
      <v-alert type="info" variant="tonal" density="compact" class="ma-2">
        <div class="text-caption">
          <strong>Debug NodeActions:</strong><br>
          isOnline: {{ isOnline }}<br>
          isPumpRunning: {{ isPumpRunning }}<br>
          canPerformActions: {{ canPerformActions }}<br>
          canRunPumps: {{ canRunPumps }}<br>
          node_id: {{ node.node_id }}<br>
          node_type: {{ node.node_type }}
        </div>
      </v-alert>

      <!-- Раскрывающиеся секции -->
      <v-expansion-panels v-model="expandedPanels" multiple>
        
        <!-- 1. Калибровка -->
        <v-expansion-panel value="calibration">
          <v-expansion-panel-title>
            <v-icon icon="mdi-tune-variant" class="mr-2"></v-icon>
            Калибровка датчиков
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <template v-if="node.node_type === 'ph_ec' || node.node_type === 'ph' || node.node_type === 'ec'">
              <v-row>
                <v-col cols="12" sm="6" md="4">
                  <v-btn
                    block
                    color="primary"
                    variant="elevated"
                    prepend-icon="mdi-flask"
                    @click="openCalibratePh"
                    :disabled="!canPerformActions"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>Калибровка pH</span>
                      <span class="text-caption">Настройка датчика</span>
                    </div>
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="6" md="4">
                  <v-btn
                    block
                    color="success"
                    variant="elevated"
                    prepend-icon="mdi-waves"
                    @click="openCalibrateEc"
                    :disabled="!canPerformActions"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>Калибровка EC</span>
                      <span class="text-caption">Настройка датчика</span>
                    </div>
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="6" md="4">
                  <v-btn
                    block
                    color="info"
                    variant="elevated"
                    prepend-icon="mdi-pump"
                    @click="openPumpCalibration"
                    :disabled="!canPerformActions"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>Калибровка насосов</span>
                      <span class="text-caption">Производительность</span>
                    </div>
                  </v-btn>
                </v-col>
              </v-row>
            </template>
            <template v-else>
              <v-alert type="info" variant="tonal">
                Калибровка доступна только для pH/EC узлов
              </v-alert>
            </template>
          </v-expansion-panel-text>
        </v-expansion-panel>

        <!-- 2. Управление насосами -->
        <v-expansion-panel value="pump_control">
          <v-expansion-panel-title>
            <v-icon icon="mdi-pump" class="mr-2"></v-icon>
            Управление насосами
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <template v-if="node.node_type === 'ph_ec' || node.node_type === 'ph' || node.node_type === 'ec'">
              <v-row>
                <v-col cols="12" sm="6" md="3">
                  <v-btn
                    block
                    color="primary"
                    variant="elevated"
                    prepend-icon="mdi-arrow-up"
                    @click="runPump('ph_up')"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>pH Up</span>
                      <span class="text-caption">Повысить pH</span>
                    </div>
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="6" md="3">
                  <v-btn
                    block
                    color="primary"
                    variant="elevated"
                    prepend-icon="mdi-arrow-down"
                    @click="runPump('ph_down')"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>pH Down</span>
                      <span class="text-caption">Понизить pH</span>
                    </div>
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="6" md="3">
                  <v-btn
                    block
                    color="success"
                    variant="elevated"
                    prepend-icon="mdi-waves"
                    @click="runPump('ec_up')"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>EC Up</span>
                      <span class="text-caption">Повысить EC</span>
                    </div>
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="6" md="3">
                  <v-btn
                    block
                    color="info"
                    variant="elevated"
                    prepend-icon="mdi-water"
                    @click="runPump('water')"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none font-weight-medium"
                  >
                    <div class="d-flex flex-column align-center">
                      <span>Вода</span>
                      <span class="text-caption">Полив</span>
                    </div>
                  </v-btn>
                </v-col>
              </v-row>
              
              <!-- Быстрые действия -->
              <v-divider class="my-4"></v-divider>
              <h4 class="text-subtitle-1 mb-3">Быстрые действия (5 сек)</h4>
              <v-row>
                <v-col cols="12" sm="4">
                  <v-btn
                    block
                    color="primary"
                    variant="outlined"
                    prepend-icon="mdi-arrow-up"
                    @click="quickPumpUp"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none"
                  >
                    pH Up (5 сек)
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="4">
                  <v-btn
                    block
                    color="primary"
                    variant="outlined"
                    prepend-icon="mdi-arrow-down"
                    @click="quickPumpDown"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none"
                  >
                    pH Down (5 сек)
                  </v-btn>
                </v-col>
                <v-col cols="12" sm="4">
                  <v-btn
                    block
                    color="success"
                    variant="outlined"
                    prepend-icon="mdi-waves"
                    @click="quickPumpEc"
                    :disabled="!canRunPumps"
                    size="large"
                    class="text-none"
                  >
                    EC Up (5 сек)
                  </v-btn>
                </v-col>
              </v-row>
            </template>
            <template v-else>
              <v-alert type="info" variant="tonal">
                Управление насосами доступно только для pH/EC узлов
              </v-alert>
            </template>
          </v-expansion-panel-text>
        </v-expansion-panel>

        <!-- 3. Состояние системы -->
        <v-expansion-panel value="system_status">
          <v-expansion-panel-title>
            <v-icon icon="mdi-chart-line" class="mr-2"></v-icon>
            Состояние системы
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="info"
                  variant="elevated"
                  prepend-icon="mdi-chart-line"
                  @click="sendCommand('get_status')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Статус узла</span>
                    <span class="text-caption">Текущее состояние</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="success"
                  variant="elevated"
                  prepend-icon="mdi-database"
                  @click="sendCommand('get_stats')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Статистика</span>
                    <span class="text-caption">Работа насосов</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="warning"
                  variant="elevated"
                  prepend-icon="mdi-refresh"
                  @click="sendCommand('reset_stats')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Сброс статистики</span>
                    <span class="text-caption">Обнулить счетчики</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="secondary"
                  variant="elevated"
                  prepend-icon="mdi-cog"
                  @click="openPidPresets"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>PID Настройки</span>
                    <span class="text-caption">Контроллеры</span>
                  </div>
                </v-btn>
              </v-col>
            </v-row>
            
            <!-- Дополнительная информация -->
            <v-divider class="my-4"></v-divider>
            <v-row>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="primary"
                  variant="outlined"
                  prepend-icon="mdi-information"
                  @click="sendCommand('get_info')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Информация о узле
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="info"
                  variant="outlined"
                  prepend-icon="mdi-memory"
                  @click="sendCommand('get_memory_info')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Информация о памяти
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="success"
                  variant="outlined"
                  prepend-icon="mdi-wifi"
                  @click="sendCommand('get_network_info')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Информация о сети
                </v-btn>
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>

        <!-- 4. Конфигурация -->
        <v-expansion-panel value="configuration">
          <v-expansion-panel-title>
            <v-icon icon="mdi-cog" class="mr-2"></v-icon>
            Конфигурация
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="info"
                  variant="elevated"
                  prepend-icon="mdi-download"
                  @click="sendCommand('get_config')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Получить конфиг</span>
                    <span class="text-caption">Текущие настройки</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <ConfigEditor :node="node" @config-updated="handleConfigUpdate">
                  <template v-slot:activator="{ props }">
                    <v-btn
                      block
                      color="secondary"
                      variant="elevated"
                      prepend-icon="mdi-cog"
                      v-bind="props"
                      size="large"
                      class="text-none font-weight-medium"
                    >
                      <div class="d-flex flex-column align-center">
                        <span>Редактировать конфиг</span>
                        <span class="text-caption">Расширенные настройки</span>
                      </div>
                    </v-btn>
                  </template>
                </ConfigEditor>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="warning"
                  variant="elevated"
                  prepend-icon="mdi-cog"
                  @click="openSystemControl"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Системные настройки</span>
                    <span class="text-caption">Режимы работы</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="error"
                  variant="elevated"
                  prepend-icon="mdi-stop"
                  @click="sendCommand('emergency_stop')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Экстренная остановка</span>
                    <span class="text-caption">Все насосы</span>
                  </div>
                </v-btn>
              </v-col>
            </v-row>
            
            <!-- Дополнительные конфигурационные действия -->
            <v-divider class="my-4"></v-divider>
            <v-row>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="primary"
                  variant="outlined"
                  prepend-icon="mdi-history"
                  @click="sendCommand('get_config_history')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  История конфигураций
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="success"
                  variant="outlined"
                  prepend-icon="mdi-backup-restore"
                  @click="sendCommand('restore_config')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Восстановить конфиг
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="warning"
                  variant="outlined"
                  prepend-icon="mdi-file-export"
                  @click="sendCommand('export_config')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Экспорт конфигурации
                </v-btn>
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>

        <!-- 5. Ручной режим -->
        <v-expansion-panel value="manual_mode">
          <v-expansion-panel-title>
            <v-icon icon="mdi-hand" class="mr-2"></v-icon>
            Ручной режим
          </v-expansion-panel-title>
          <v-expansion-panel-text>
            <v-row>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="primary"
                  variant="elevated"
                  prepend-icon="mdi-play"
                  @click="sendCommand('set_manual_mode')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Включить ручной режим</span>
                    <span class="text-caption">Отключить авто</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="success"
                  variant="elevated"
                  prepend-icon="mdi-pause"
                  @click="sendCommand('set_auto_mode')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Включить авто режим</span>
                    <span class="text-caption">Восстановить авто</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="warning"
                  variant="elevated"
                  prepend-icon="mdi-restart"
                  @click="openRebootDialog"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Перезагрузка</span>
                    <span class="text-caption">Узел</span>
                  </div>
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="3">
                <v-btn
                  block
                  color="info"
                  variant="elevated"
                  prepend-icon="mdi-information"
                  @click="sendCommand('get_info')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none font-weight-medium"
                >
                  <div class="d-flex flex-column align-center">
                    <span>Информация</span>
                    <span class="text-caption">О узле</span>
                  </div>
                </v-btn>
              </v-col>
            </v-row>
            
            <!-- Дополнительные действия -->
            <v-divider class="my-4"></v-divider>
            <v-row>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="error"
                  variant="outlined"
                  prepend-icon="mdi-alert"
                  @click="sendCommand('set_emergency_mode')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Аварийный режим
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="warning"
                  variant="outlined"
                  prepend-icon="mdi-test-tube"
                  @click="sendCommand('set_mock_mode')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Режим тестирования
                </v-btn>
              </v-col>
              <v-col cols="12" sm="6" md="4">
                <v-btn
                  block
                  color="info"
                  variant="outlined"
                  prepend-icon="mdi-update"
                  @click="sendCommand('check_for_updates')"
                  :disabled="!canPerformActions"
                  size="large"
                  class="text-none"
                >
                  Проверить обновления
                </v-btn>
              </v-col>
            </v-row>
          </v-expansion-panel-text>
        </v-expansion-panel>

      </v-expansion-panels>
    </v-card>

    <!-- Все диалоги остаются без изменений -->
    <!-- Pump Duration Dialog -->
    <v-dialog v-model="pumpDialog" max-width="500">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-pump" class="mr-2"></v-icon>
          {{ isCalibrationMode ? 'Калибровка насоса' : 'Запуск насоса' }}: {{ selectedPump }}
        </v-card-title>
        <v-card-text>
          <v-text-field
            v-model.number="pumpDuration"
            label="Длительность (секунды)"
            type="number"
            min="1"
            max="300"
            :disabled="isPumpRunning"
          ></v-text-field>
          
          <v-checkbox
            v-model="isCalibrationMode"
            label="Режим калибровки"
            :disabled="isPumpRunning"
          ></v-checkbox>
          
          <v-text-field
            v-if="isCalibrationMode"
            v-model.number="calibrationVolume"
            label="Объем калибровки (мл)"
            type="number"
            min="1"
            max="1000"
            :disabled="isPumpRunning"
          ></v-text-field>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="pumpDialog = false" :disabled="isPumpRunning">Отмена</v-btn>
          <v-btn 
            color="primary" 
            @click="isCalibrationMode ? executeCalibration() : executePump()"
            :disabled="(isCalibrationMode && (!calibrationVolume || calibrationVolume <= 0)) || isPumpRunning"
            :loading="isPumpRunning"
          >
            {{ isCalibrationMode ? 'Калибровать' : (isPumpRunning ? `Запуск... ${remainingSeconds}s` : 'Запустить') }}
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- System Control Dialog -->
    <v-dialog v-model="systemControlDialog" max-width="600">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-cog" class="mr-2"></v-icon>
          Системные настройки
        </v-card-title>
        <v-card-text>
          <v-row>
            <v-col cols="12" md="6">
              <v-select
                v-model="systemMode"
                :items="['autonomous', 'manual', 'emergency']"
                label="Режим работы"
                prepend-icon="mdi-cog"
              ></v-select>
            </v-col>
            <v-col cols="12" md="6">
              <v-switch
                v-model="isMockMode"
                label="Режим тестирования"
                prepend-icon="mdi-test-tube"
              ></v-switch>
            </v-col>
          </v-row>
          
          <v-switch
            v-model="isEmergencyMode"
            label="Аварийный режим"
            prepend-icon="mdi-alert"
            color="error"
          ></v-switch>
          
          <!-- Дополнительные настройки -->
          <v-divider class="my-4"></v-divider>
          <h4 class="text-subtitle-1 mb-3">Дополнительные настройки</h4>
          
          <v-text-field
            v-model.number="telemetryInterval"
            label="Интервал телеметрии (сек)"
            type="number"
            min="1"
            max="3600"
          ></v-text-field>
          
          <v-switch
            v-model="enableLogging"
            label="Включить логирование"
            prepend-icon="mdi-file-document"
          ></v-switch>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="systemControlDialog = false">Отмена</v-btn>
          <v-btn 
            color="primary" 
            @click="applySystemSettings"
            :loading="applyingSettings"
          >
            Применить
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Reboot Dialog -->
    <v-dialog v-model="rebootDialog" max-width="400">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-restart" class="mr-2"></v-icon>
          Перезагрузка узла
        </v-card-title>
        <v-card-text>
          <p>Вы уверены, что хотите перезагрузить узел?</p>
          <p class="text-caption">Узел будет недоступен в течение нескольких секунд.</p>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="rebootDialog = false">Отмена</v-btn>
          <v-btn 
            color="warning" 
            @click="sendCommand('reboot')"
          >
            Перезагрузить
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- PID Presets Dialog -->
    <v-dialog v-model="pidPresetsDialog" max-width="800">
      <v-card>
        <v-card-title>
          <v-icon icon="mdi-tune-variant" class="mr-2"></v-icon>
          PID Настройки
        </v-card-title>
        <v-card-text>
          <v-row v-if="loadingPresets">
            <v-col cols="12" class="text-center">
              <v-progress-circular indeterminate></v-progress-circular>
              <p class="mt-2">Загрузка настроек...</p>
            </v-col>
          </v-row>
          <v-row v-else>
            <v-col 
              v-for="preset in pidPresets" 
              :key="preset.id"
              cols="12" 
              sm="6" 
              md="4"
            >
              <v-card 
                class="preset-card"
                :class="{ 'selected': selectedPreset === preset.id }"
                @click="selectPreset(preset.id)"
              >
                <v-card-title class="text-subtitle-1">
                  {{ preset.name }}
                </v-card-title>
                <v-card-text>
                  <div class="text-caption">
                    <div><strong>pH:</strong> {{ preset.ph_target }}</div>
                    <div><strong>EC:</strong> {{ preset.ec_target }}</div>
                    <div><strong>Kp:</strong> {{ preset.kp }}</div>
                    <div><strong>Ki:</strong> {{ preset.ki }}</div>
                    <div><strong>Kd:</strong> {{ preset.kd }}</div>
                  </div>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="pidPresetsDialog = false">Отмена</v-btn>
          <v-btn 
            color="primary" 
            @click="applyPreset"
            :disabled="!selectedPreset"
            :loading="sendingPreset"
          >
            Применить
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'
import { useRouter } from 'vue-router'
import { useNodeStatus } from '@/composables/useNodeStatus'
import api from '@/services/api'
import ConfigEditor from './ConfigEditor.vue'

const props = defineProps({
  node: {
    type: Object,
    required: true,
  },
})

const emit = defineEmits(['command'])

const router = useRouter()

// Централизованная система статусов
const {
  isOnline,
  isPumpRunning,
  statusColor,
  statusIcon,
  statusText,
  lastSeenText,
  canPerformActions,
  canRunPumps
} = useNodeStatus({ value: () => props.node })

// Раскрывающиеся панели (изначально все свернуты)
const expandedPanels = ref([])

// Диалоги
const pumpDialog = ref(false)
const systemControlDialog = ref(false)
const rebootDialog = ref(false)
const pidPresetsDialog = ref(false)

// Настройки насосов
const selectedPump = ref(null)
const pumpDuration = ref(5)
const calibrationVolume = ref(10) // мл
const isCalibrationMode = ref(false)
const remainingSeconds = ref(0)
let pumpIntervalId = null

// Системные настройки
const systemMode = ref('autonomous')
const isMockMode = ref(false)
const isEmergencyMode = ref(false)
const applyingSettings = ref(false)
const telemetryInterval = ref(30)
const enableLogging = ref(true)

// PID настройки
const pidPresets = ref([])
const selectedPreset = ref(null)
const loadingPresets = ref(false)
const sendingPreset = ref(false)

// Функции для открытия диалогов
function openCalibratePh() {
  selectedPump.value = 'ph_calibration'
  pumpDialog.value = true
}

function openCalibrateEc() {
  selectedPump.value = 'ec_calibration'
  pumpDialog.value = true
}

function openPumpCalibration() {
  selectedPump.value = 'pump_calibration'
  pumpDialog.value = true
}

function openSystemControl() {
  systemControlDialog.value = true
}

function openRebootDialog() {
  rebootDialog.value = true
}

function openPidPresets() {
  pidPresetsDialog.value = true
  loadPidPresets()
}

// Функции для работы с насосами
function runPump(pumpType) {
  selectedPump.value = pumpType
  pumpDialog.value = true
}

function quickPumpUp() {
  sendCommand('run_pump_manual', { pump: 'ph_up', duration: 5 })
}

function quickPumpDown() {
  sendCommand('run_pump_manual', { pump: 'ph_down', duration: 5 })
}

function quickPumpEc() {
  sendCommand('run_pump_manual', { pump: 'ec_up', duration: 5 })
}

// Функции для отправки команд
function sendCommand(command, params = {}) {
  emit('command', { command, params })
}

function handleConfigUpdate() {
  // Обработка обновления конфигурации
  console.log('Конфигурация обновлена')
}

// Функции для работы с насосами
async function executePump() {
  try {
    console.log('🚀 executePump вызван, isPumpRunning:', isPumpRunning.value)
    if (isPumpRunning.value) {
      console.log('⚠️ Насос уже запущен, игнорируем запрос')
      return
    }

    const response = await api.sendCommand(props.node.node_id, 'run_pump_manual', {
      pump: selectedPump.value,
      duration: pumpDuration.value
    })

    if (response.data.success) {
      console.log('✅ Насос успешно запущен')
      // Блокируем кнопки и показываем загрузку на время выполнения
      remainingSeconds.value = Math.max(1, Math.round(Number(pumpDuration.value) || 0))
      console.log('⏰ Устанавливаем таймер на', remainingSeconds.value, 'секунд')
      
      if (pumpIntervalId) clearInterval(pumpIntervalId)
      pumpIntervalId = setInterval(() => {
        if (remainingSeconds.value > 0) {
          remainingSeconds.value -= 1
          console.log('⏱️ Осталось секунд:', remainingSeconds.value)
        }
        if (remainingSeconds.value <= 0) {
          console.log('🏁 Таймер завершен, разблокируем кнопки')
          clearInterval(pumpIntervalId)
          pumpIntervalId = null
          // isPumpRunning теперь управляется централизованной системой
          pumpDialog.value = false
        }
      }, 1000)
    } else {
      console.log('❌ Сервер вернул success: false')
    }
  } catch (error) {
    console.error('❌ Ошибка при запуске насоса:', {
      message: error.message,
      response: error.response?.data,
      status: error.response?.status,
      config: error.config
    })
    // Показываем уведомление об ошибке
    // isPumpRunning теперь управляется централизованной системой
  }
}

async function executeCalibration() {
  try {
    console.log('🔬 executeCalibration вызван')
    const response = await api.sendCommand(props.node.node_id, 'calibrate_pump', {
      pump: selectedPump.value,
      volume: calibrationVolume.value
    })

    if (response.data.success) {
      console.log('✅ Калибровка успешно запущена')
      pumpDialog.value = false
    } else {
      console.log('❌ Сервер вернул success: false')
    }
  } catch (error) {
    console.error('❌ Ошибка при калибровке:', error)
  }
}

// Функции для системных настроек
async function applySystemSettings() {
  try {
    applyingSettings.value = true
    await api.sendCommand(props.node.node_id, 'set_system_mode', {
      mode: systemMode.value,
      mock_mode: isMockMode.value,
      emergency_mode: isEmergencyMode.value,
      telemetry_interval: telemetryInterval.value,
      enable_logging: enableLogging.value
    })
    systemControlDialog.value = false
  } catch (error) {
    console.error('❌ Ошибка при применении системных настроек:', error)
  } finally {
    applyingSettings.value = false
  }
}

// Функции для PID настроек
async function loadPidPresets() {
  try {
    loadingPresets.value = true
    // Загрузка предустановок PID
    pidPresets.value = [
      { id: 1, name: 'Консервативный', ph_target: 6.5, ec_target: 1.2, kp: 0.5, ki: 0.1, kd: 0.05 },
      { id: 2, name: 'Агрессивный', ph_target: 6.0, ec_target: 1.5, kp: 1.0, ki: 0.2, kd: 0.1 },
      { id: 3, name: 'Сбалансированный', ph_target: 6.2, ec_target: 1.3, kp: 0.7, ki: 0.15, kd: 0.08 }
    ]
  } catch (error) {
    console.error('❌ Ошибка при загрузке PID настроек:', error)
  } finally {
    loadingPresets.value = false
  }
}

function selectPreset(presetId) {
  selectedPreset.value = presetId
}

async function applyPreset() {
  if (!selectedPreset.value) return
  
  try {
    sendingPreset.value = true
    const preset = pidPresets.value.find(p => p.id === selectedPreset.value)
    await api.sendCommand(props.node.node_id, 'set_pid_preset', preset)
    pidPresetsDialog.value = false
  } catch (error) {
    console.error('❌ Ошибка при применении PID настроек:', error)
  } finally {
    sendingPreset.value = false
  }
}
</script>

<style scoped>
.preset-card {
  cursor: pointer;
  transition: all 0.3s ease;
  border: 2px solid transparent;
}

.preset-card:hover {
  transform: translateY(-2px);
  box-shadow: 0 4px 8px rgba(0,0,0,0.1);
}

.preset-card.selected {
  border-color: rgb(var(--v-theme-primary));
  background-color: rgba(var(--v-theme-primary), 0.1);
}
</style>
