<template>
  <div class="settings-page">
    <v-container fluid>
      <v-row>
        <v-col cols="12">
          <div class="text-h4 mb-2">⚙️ Настройки системы</div>
          <div class="text-subtitle-1 text-medium-emphasis mb-6">
            Конфигурация и интеграции Hydro Mesh System
          </div>
        </v-col>
      </v-row>

      <!-- Табы для разных разделов -->
      <v-tabs v-model="activeTab" bg-color="primary" class="mb-4">
        <v-tab value="integrations">
          <v-icon icon="mdi-puzzle" start></v-icon>
          Интеграции
        </v-tab>
        <v-tab value="system">
          <v-icon icon="mdi-server" start></v-icon>
          Система
        </v-tab>
        <v-tab value="interface">
          <v-icon icon="mdi-palette" start></v-icon>
          Интерфейс
        </v-tab>
        <v-tab value="notifications">
          <v-icon icon="mdi-bell" start></v-icon>
          Уведомления
        </v-tab>
        <v-tab value="charts">
          <v-icon icon="mdi-chart-line" start></v-icon>
          Графики
        </v-tab>
        <v-tab value="pid">
          <v-icon icon="mdi-tune-variant" start></v-icon>
          PID Контроллеры
        </v-tab>
        <v-tab value="database">
          <v-icon icon="mdi-database" start></v-icon>
          База данных
        </v-tab>
      </v-tabs>

      <v-window v-model="activeTab">
        <!-- Integrations Tab (Telegram + Docker) -->
        <v-window-item value="integrations">
          <v-row>
            <v-col cols="12" lg="6">
              <TelegramSetupGuide />
            </v-col>
            <v-col cols="12" lg="6">
              <DockerManager />
            </v-col>
          </v-row>
        </v-window-item>

        <!-- System Tab -->
        <v-window-item value="system">
          <v-row>
            <v-col cols="12">
              <SystemStatusCard :status="systemStatus" @refresh="loadSystemStatus" :loading="loading" />
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-timer" class="mr-2"></v-icon>
                  Таймауты и интервалы
                </v-card-title>
                <v-card-text>
                  <v-text-field
                    v-model.number="settingsStore.system.nodeOfflineTimeout"
                    label="Таймаут офлайн узла (секунды)"
                    type="number"
                    variant="outlined"
                    hint="Через сколько секунд узел считается offline"
                    persistent-hint
                    suffix="сек"
                  ></v-text-field>

                  <v-text-field
                    v-model.number="settingsStore.system.telemetryRetentionDays"
                    label="Хранение телеметрии (дни)"
                    type="number"
                    variant="outlined"
                    hint="Сколько дней хранить записи телеметрии"
                    persistent-hint
                    class="mt-4"
                    suffix="дней"
                  ></v-text-field>

                  <v-text-field
                    v-model.number="settingsStore.system.eventAutoResolveHours"
                    label="Автоматическое резолвение событий (часы)"
                    type="number"
                    variant="outlined"
                    hint="Через сколько часов автоматически резолвить события"
                    persistent-hint
                    class="mt-4"
                    suffix="часов"
                  ></v-text-field>

                  <v-switch
                    v-model="settingsStore.system.emergencyNoAutoResolve"
                    label="Не резолвить Emergency события автоматически"
                    color="error"
                    class="mt-4"
                  ></v-switch>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-information" class="mr-2"></v-icon>
                  Информация о системе
                </v-card-title>
                <v-card-text>
                  <v-list density="compact">
                    <v-list-item v-if="systemStatus?.system">
                      <v-list-item-title>PHP Version</v-list-item-title>
                      <v-list-item-subtitle>{{ systemStatus.system.php_version }}</v-list-item-subtitle>
                    </v-list-item>
                    <v-list-item v-if="systemStatus?.system">
                      <v-list-item-title>Laravel Version</v-list-item-title>
                      <v-list-item-subtitle>{{ systemStatus.system.laravel_version }}</v-list-item-subtitle>
                    </v-list-item>
                    <v-list-item>
                      <v-list-item-title>Frontend Version</v-list-item-title>
                      <v-list-item-subtitle>Vue.js 3.4 + Vuetify 3.5</v-list-item-subtitle>
                    </v-list-item>
                    <v-list-item>
                      <v-list-item-title>Server Time</v-list-item-title>
                      <v-list-item-subtitle>{{ systemStatus?.system?.server_time || 'N/A' }}</v-list-item-subtitle>
                    </v-list-item>
                  </v-list>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>

        <!-- Interface Tab -->
        <v-window-item value="interface">
          <v-row>
            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-palette" class="mr-2"></v-icon>
                  Внешний вид
                </v-card-title>
                <v-card-text>
                  <v-radio-group v-model="settingsStore.ui.theme" @update:model-value="settingsStore.setTheme">
                    <v-radio label="Светлая тема" value="light">
                      <template v-slot:label>
                        <v-icon icon="mdi-weather-sunny" class="mr-2"></v-icon>
                        Светлая тема
                      </template>
                    </v-radio>
                    <v-radio label="Темная тема" value="dark">
                      <template v-slot:label>
                        <v-icon icon="mdi-weather-night" class="mr-2"></v-icon>
                        Темная тема
                      </template>
                    </v-radio>
                  </v-radio-group>

                  <v-divider class="my-4"></v-divider>

                  <v-select
                    v-model="settingsStore.ui.language"
                    :items="languageOptions"
                    label="Язык интерфейса"
                    variant="outlined"
                    @update:model-value="settingsStore.setLanguage"
                  ></v-select>

                  <v-switch
                    v-model="settingsStore.ui.compactView"
                    label="Компактный вид"
                    hint="Уменьшенные отступы и размеры элементов"
                    persistent-hint
                    @update:model-value="settingsStore.setCompactView"
                  ></v-switch>

                  <v-switch
                    v-model="settingsStore.ui.animationsEnabled"
                    label="Анимации"
                    hint="Плавные переходы и эффекты"
                    persistent-hint
                    class="mt-2"
                    @update:model-value="settingsStore.toggleAnimations"
                  ></v-switch>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-refresh" class="mr-2"></v-icon>
                  Автообновление
                </v-card-title>
                <v-card-text>
                  <v-switch
                    v-model="settingsStore.ui.autoRefresh"
                    label="Автоматическое обновление данных"
                    color="primary"
                    @update:model-value="settingsStore.setAutoRefresh"
                  ></v-switch>

                  <v-slider
                    v-if="settingsStore.ui.autoRefresh"
                    v-model="settingsStore.ui.refreshInterval"
                    :min="10"
                    :max="300"
                    :step="10"
                    label="Интервал обновления"
                    thumb-label="always"
                    @update:model-value="settingsStore.setRefreshInterval"
                  >
                    <template v-slot:append>
                      <v-text-field
                        v-model.number="settingsStore.ui.refreshInterval"
                        type="number"
                        style="width: 80px"
                        density="compact"
                        variant="outlined"
                        suffix="сек"
                        hide-details
                      ></v-text-field>
                    </template>
                  </v-slider>

                  <v-alert type="info" variant="tonal" class="mt-4">
                    <small>
                      Рекомендуется: 30-60 секунд для оптимальной производительности
                    </small>
                  </v-alert>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>

        <!-- Notifications Tab -->
        <v-window-item value="notifications">
          <v-row>
            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-bell-ring" class="mr-2"></v-icon>
                  Настройки уведомлений
                </v-card-title>
                <v-card-text>
                  <v-switch
                    v-model="settingsStore.notifications.enabled"
                    label="Включить уведомления"
                    color="primary"
                  ></v-switch>

                  <v-switch
                    v-model="settingsStore.notifications.sound"
                    label="Звуковые уведомления"
                    :disabled="!settingsStore.notifications.enabled"
                    @update:model-value="settingsStore.setNotificationSound"
                  ></v-switch>

                  <v-switch
                    v-model="settingsStore.notifications.desktop"
                    label="Desktop уведомления"
                    hint="Показывать уведомления вне браузера"
                    persistent-hint
                    :disabled="!settingsStore.notifications.enabled"
                    @update:model-value="settingsStore.setDesktopNotifications"
                  ></v-switch>

                  <v-btn
                    v-if="settingsStore.notifications.desktop"
                    size="small"
                    class="mt-2"
                    @click="testNotification"
                  >
                    Тестовое уведомление
                  </v-btn>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-alert-octagon" class="mr-2"></v-icon>
                  Уровни уведомлений
                </v-card-title>
                <v-card-text>
                  <v-list>
                    <v-list-item>
                      <v-switch
                        v-model="settingsStore.notifications.levels.info"
                        label="Info"
                        color="blue"
                        density="compact"
                      >
                        <template v-slot:label>
                          <v-chip color="blue" size="small" class="mr-2">Info</v-chip>
                          Информационные события
                        </template>
                      </v-switch>
                    </v-list-item>

                    <v-list-item>
                      <v-switch
                        v-model="settingsStore.notifications.levels.warning"
                        label="Warning"
                        color="orange"
                        density="compact"
                      >
                        <template v-slot:label>
                          <v-chip color="orange" size="small" class="mr-2">Warning</v-chip>
                          Предупреждения
                        </template>
                      </v-switch>
                    </v-list-item>

                    <v-list-item>
                      <v-switch
                        v-model="settingsStore.notifications.levels.critical"
                        label="Critical"
                        color="red"
                        density="compact"
                      >
                        <template v-slot:label>
                          <v-chip color="red" size="small" class="mr-2">Critical</v-chip>
                          Критичные события
                        </template>
                      </v-switch>
                    </v-list-item>

                    <v-list-item>
                      <v-switch
                        v-model="settingsStore.notifications.levels.emergency"
                        label="Emergency"
                        color="purple"
                        density="compact"
                      >
                        <template v-slot:label>
                          <v-chip color="purple" size="small" class="mr-2">Emergency</v-chip>
                          Чрезвычайные события
                        </template>
                      </v-switch>
                    </v-list-item>
                  </v-list>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>

        <!-- Charts Tab -->
        <v-window-item value="charts">
          <v-row>
            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-chart-line-variant" class="mr-2"></v-icon>
                  Настройки графиков
                </v-card-title>
                <v-card-text>
                  <v-select
                    v-model="settingsStore.charts.defaultPeriod"
                    :items="periodOptions"
                    label="Период по умолчанию"
                    variant="outlined"
                  ></v-select>

                  <v-slider
                    v-model="settingsStore.charts.pointSize"
                    :min="0"
                    :max="10"
                    :step="1"
                    label="Размер точек"
                    thumb-label
                    class="mt-4"
                  ></v-slider>

                  <v-slider
                    v-model="settingsStore.charts.lineWidth"
                    :min="1"
                    :max="5"
                    :step="0.5"
                    label="Толщина линий"
                    thumb-label
                  ></v-slider>

                  <v-switch
                    v-model="settingsStore.charts.fillArea"
                    label="Заливка под графиком"
                    class="mt-2"
                  ></v-switch>

                  <v-switch
                    v-model="settingsStore.charts.showLegend"
                    label="Показывать легенду"
                  ></v-switch>
                </v-card-text>
                <v-card-actions>
                  <v-btn color="primary" @click="applyChartSettings">Применить</v-btn>
                </v-card-actions>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-access-point-network" class="mr-2"></v-icon>
                  MQTT Broker
                </v-card-title>
                <v-card-text>
                  <v-alert
                    :type="systemStatus?.mqtt === 'connected' ? 'success' : 'error'"
                    variant="tonal"
                  >
                    <v-row align="center">
                      <v-col>
                        <strong>Status:</strong> {{ systemStatus?.mqtt || 'unknown' }}
                      </v-col>
                      <v-col cols="auto">
                        <v-icon
                          :icon="systemStatus?.mqtt === 'connected' ? 'mdi-lan-connect' : 'mdi-lan-disconnect'"
                          size="large"
                        ></v-icon>
                      </v-col>
                    </v-row>
                  </v-alert>

                  <v-list class="mt-4" density="compact">
                    <v-list-item>
                      <v-list-item-title>Host</v-list-item-title>
                      <v-list-item-subtitle>{{ settingsStore.mqtt.host }}:{{ settingsStore.mqtt.port }}</v-list-item-subtitle>
                    </v-list-item>
                    <v-list-item>
                      <v-list-item-title>Topics</v-list-item-title>
                      <v-list-item-subtitle>
                        hydro/telemetry/#, hydro/event/#
                      </v-list-item-subtitle>
                    </v-list-item>
                  </v-list>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>

        <!-- PID Tab -->
        <v-window-item value="pid">
          <v-row>
            <v-col cols="12">
              <v-alert type="info" variant="tonal" class="mb-4">
                Тонкая настройка регуляторов pH и EC. Используйте небольшие изменения и наблюдайте за системой.
                <br><strong>💡 Совет:</strong> Начните с пресетов, затем подстройте под свою систему.
              </v-alert>

              <!-- PID Presets -->
              <v-card variant="outlined" class="mb-4">
                <v-card-title class="text-subtitle-2 d-flex align-center">
                  ⚡ Быстрые пресеты
                  <v-spacer></v-spacer>
                  <v-btn 
                    size="small" 
                    color="primary" 
                    prepend-icon="mdi-content-save-plus"
                    @click="openCreatePresetDialog"
                  >
                    Создать пресет
                  </v-btn>
                </v-card-title>
                <v-card-text>
                  <div class="mb-2 text-caption text-grey">Предустановленные пресеты:</div>
                  <v-chip-group class="mb-4">
                    <v-chip 
                      v-for="preset in defaultPresets" 
                      :key="preset.id"
                      @click="applyPreset(preset)" 
                      :prepend-icon="getPresetIcon(preset.name)" 
                      :color="getPresetColor(preset.name)"
                      variant="outlined"
                    >
                      {{ preset.name }}
                    </v-chip>
                  </v-chip-group>

                  <div v-if="customPresets.length > 0">
                    <v-divider class="my-3"></v-divider>
                    <div class="mb-2 text-caption text-grey">Мои пресеты:</div>
                    <v-chip-group>
                      <v-chip 
                        v-for="preset in customPresets" 
                        :key="preset.id"
                        @click="applyPreset(preset)"
                        prepend-icon="mdi-star" 
                        color="purple"
                        variant="outlined"
                        closable
                        @click:close="deleteCustomPreset(preset.id)"
                      >
                        {{ preset.name }}
                      </v-chip>
                    </v-chip-group>
                  </div>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  pH Контроллер
                  <v-spacer></v-spacer>
                  <v-switch 
                    v-model="settingsStore.pid.ph.enabled" 
                    color="success"
                    hide-details
                    density="compact"
                  ></v-switch>
                </v-card-title>
                <v-card-text>
                  <v-text-field v-model.number="settingsStore.pid.ph.setpoint" type="number" step="0.1" label="Целевой pH"
                    suffix="pH" density="compact" variant="outlined"></v-text-field>
                  
                  <v-text-field v-model.number="settingsStore.pid.ph.kp" type="number" step="0.01" label="Kp (пропорциональный)"
                    density="compact" variant="outlined"></v-text-field>
                  
                  <v-text-field v-model.number="settingsStore.pid.ph.ki" type="number" step="0.001" label="Ki (интегральный)"
                    density="compact" variant="outlined"></v-text-field>
                  
                  <v-text-field v-model.number="settingsStore.pid.ph.kd" type="number" step="0.01" label="Kd (дифференциальный)"
                    density="compact" variant="outlined"></v-text-field>

                  <v-divider class="my-3"></v-divider>

                  <v-text-field v-model.number="settingsStore.pid.ph.deadband" type="number" step="0.01" label="Deadband (гистерезис)"
                    suffix="pH" density="compact" variant="outlined"></v-text-field>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  EC Контроллер
                  <v-spacer></v-spacer>
                  <v-switch 
                    v-model="settingsStore.pid.ec.enabled" 
                    color="success"
                    hide-details
                    density="compact"
                  ></v-switch>
                </v-card-title>
                <v-card-text>
                  <v-text-field v-model.number="settingsStore.pid.ec.setpoint" type="number" step="0.1" label="Целевой EC"
                    suffix="mS/cm" density="compact" variant="outlined"></v-text-field>
                  
                  <v-text-field v-model.number="settingsStore.pid.ec.kp" type="number" step="0.01" label="Kp (пропорциональный)"
                    density="compact" variant="outlined"></v-text-field>
                  
                  <v-text-field v-model.number="settingsStore.pid.ec.ki" type="number" step="0.001" label="Ki (интегральный)"
                    density="compact" variant="outlined"></v-text-field>
                  
                  <v-text-field v-model.number="settingsStore.pid.ec.kd" type="number" step="0.01" label="Kd (дифференциальный)"
                    density="compact" variant="outlined"></v-text-field>

                  <v-divider class="my-3"></v-divider>

                  <v-text-field v-model.number="settingsStore.pid.ec.deadband" type="number" step="0.01" label="Deadband (гистерезис)"
                    suffix="mS/cm" density="compact" variant="outlined"></v-text-field>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12">
              <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn color="success" prepend-icon="mdi-check" @click="savePidSettings">
                  Сохранить настройки PID
                </v-btn>
              </v-card-actions>
            </v-col>
          </v-row>
        </v-window-item>

        <!-- Database Tab -->
        <v-window-item value="database">
          <v-row>
            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-database-cog" class="mr-2"></v-icon>
                  Управление базой данных
                </v-card-title>
                <v-card-text>
                  <v-alert type="info" variant="tonal" class="mb-4">
                    <div><strong>Тип БД:</strong> {{ systemStatus?.database || 'PostgreSQL' }}</div>
                    <div><strong>Записей телеметрии:</strong> ~{{ estimatedRecords }}</div>
                  </v-alert>

                  <v-btn
                    block
                    color="warning"
                    prepend-icon="mdi-broom"
                    class="mb-2"
                    @click="cleanupDialog = true"
                  >
                    Очистить старые данные
                  </v-btn>

                  <v-btn
                    block
                    color="primary"
                    prepend-icon="mdi-download"
                    class="mb-2"
                    @click="backupDatabase"
                  >
                    Создать backup
                  </v-btn>
                </v-card-text>
              </v-card>
            </v-col>

            <v-col cols="12" md="6">
              <v-card>
                <v-card-title>
                  <v-icon icon="mdi-information" class="mr-2"></v-icon>
                  Статистика базы данных
                </v-card-title>
                <v-card-text>
                  <v-list density="compact">
                    <v-list-item>
                      <v-list-item-title>Узлов</v-list-item-title>
                      <v-list-item-subtitle>{{ dbStats.nodes || 'N/A' }}</v-list-item-subtitle>
                    </v-list-item>
                    <v-list-item>
                      <v-list-item-title>Записей телеметрии</v-list-item-title>
                      <v-list-item-subtitle>{{ dbStats.telemetry || 'N/A' }}</v-list-item-subtitle>
                    </v-list-item>
                    <v-list-item>
                      <v-list-item-title>Событий</v-list-item-title>
                      <v-list-item-subtitle>{{ dbStats.events || 'N/A' }}</v-list-item-subtitle>
                    </v-list-item>
                  </v-list>

                  <v-btn
                    block
                    variant="outlined"
                    prepend-icon="mdi-refresh"
                    class="mt-4"
                    @click="loadDbStats"
                    :loading="loadingStats"
                  >
                    Обновить статистику
                  </v-btn>
                </v-card-text>
              </v-card>
            </v-col>
          </v-row>
        </v-window-item>
      </v-window>
    </v-container>

    <!-- Cleanup Dialog -->
    <v-dialog v-model="cleanupDialog" max-width="500">
      <v-card>
        <v-card-title class="bg-warning text-white">
          <v-icon icon="mdi-alert" class="mr-2"></v-icon>
          Очистка старых данных
        </v-card-title>
        <v-card-text class="pt-4">
          <p>Удалить телеметрию старше:</p>
          <v-slider
            v-model="cleanupDays"
            :min="7"
            :max="365"
            :step="7"
            thumb-label="always"
            label="Дней"
          >
            <template v-slot:append>
              <v-text-field
                v-model.number="cleanupDays"
                type="number"
                style="width: 80px"
                density="compact"
                variant="outlined"
                suffix="дней"
                hide-details
              ></v-text-field>
            </template>
          </v-slider>

          <v-alert type="warning" variant="tonal" class="mt-4">
            <strong>Внимание!</strong> Это действие необратимо.
          </v-alert>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="cleanupDialog = false">Отмена</v-btn>
          <v-btn color="warning" @click="executeCleanup">Очистить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Create Preset Dialog -->
    <v-dialog v-model="createPresetDialog" max-width="600">
      <v-card>
        <v-card-title class="bg-primary text-white">
          <v-icon icon="mdi-content-save-plus" class="mr-2"></v-icon>
          Создать пресет PID
        </v-card-title>
        <v-card-text class="pt-4">
          <v-text-field
            v-model="newPresetName"
            label="Название пресета"
            placeholder="Например: Моя DWC система"
            variant="outlined"
            prepend-inner-icon="mdi-tag"
          ></v-text-field>

          <v-textarea
            v-model="newPresetDescription"
            label="Описание"
            variant="outlined"
            rows="3"
          ></v-textarea>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="createPresetDialog = false">Отмена</v-btn>
          <v-btn 
            color="primary" 
            prepend-icon="mdi-content-save"
            @click="saveCustomPreset"
            :disabled="!newPresetName"
          >
            Сохранить
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useSettingsStore } from '@/stores/settings'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useTelemetryStore } from '@/stores/telemetry'
import { useEventsStore } from '@/stores/events'
import SystemStatusCard from '@/components/SystemStatusCard.vue'
import TelegramSetupGuide from '@/components/TelegramSetupGuide.vue'
import DockerManager from '@/components/DockerManager.vue'
import { axios as api } from '@/services/api'

const settingsStore = useSettingsStore()
const appStore = useAppStore()
const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()
const eventsStore = useEventsStore()

const activeTab = ref('integrations')
const systemStatus = ref(null)
const loading = ref(false)
const loadingStats = ref(false)
const cleanupDialog = ref(false)
const cleanupDays = ref(90)
const dbStats = ref({})
const createPresetDialog = ref(false)
const newPresetName = ref('')
const newPresetDescription = ref('')
const customPresets = ref([])
const defaultPresets = ref([])

const languageOptions = [
  { title: 'Русский', value: 'ru' },
  { title: 'English', value: 'en' },
]

const periodOptions = [
  { title: '1 час', value: '1h' },
  { title: '6 часов', value: '6h' },
  { title: '24 часа', value: '24h' },
  { title: '7 дней', value: '7d' },
]

const estimatedRecords = computed(() => {
  const nodes = nodesStore.nodes.length || 6
  const days = 30
  const recordsPerDay = 288
  return (nodes * days * recordsPerDay).toLocaleString()
})

onMounted(async () => {
  await loadSystemStatus()
  await loadDbStats()
  loadCustomPresets()
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

async function loadDbStats() {
  loadingStats.value = true
  try {
    dbStats.value = {
      nodes: nodesStore.nodes.length,
      telemetry: telemetryStore.telemetry.length || '?',
      events: eventsStore.events.length,
    }
  } finally {
    loadingStats.value = false
  }
}

function applyChartSettings() {
  settingsStore.setChartDefaults(settingsStore.charts)
  appStore.showSnackbar('Настройки графиков применены', 'success')
}

function testNotification() {
  if ('Notification' in window && Notification.permission === 'granted') {
    new Notification('Hydro System', {
      body: 'Тестовое уведомление работает!',
      icon: '/favicon.svg',
    })
  } else if ('Notification' in window) {
    Notification.requestPermission().then(permission => {
      if (permission === 'granted') {
        new Notification('Hydro System', {
          body: 'Уведомления разрешены!',
        })
      }
    })
  } else {
    appStore.showSnackbar('Desktop уведомления не поддерживаются', 'warning')
  }
}

function backupDatabase() {
  appStore.showSnackbar('Функция backup доступна в backend/backup.bat', 'info')
}

async function executeCleanup() {
  appStore.showSnackbar(`Очистка данных старше ${cleanupDays.value} дней...`, 'info')
  cleanupDialog.value = false
}

async function loadCustomPresets() {
  try {
    const data = await api.get('/pid-presets')
    if (data && Array.isArray(data)) {
      defaultPresets.value = data.filter(p => p.is_default)
      customPresets.value = data.filter(p => !p.is_default)
    }
  } catch (error) {
    console.error('Error loading presets:', error)
    defaultPresets.value = []
    customPresets.value = []
  }
}

function openCreatePresetDialog() {
  newPresetName.value = ''
  newPresetDescription.value = ''
  createPresetDialog.value = true
}

async function saveCustomPreset() {
  if (!newPresetName.value.trim()) {
    appStore.showSnackbar('Введите название пресета', 'warning')
    return
  }

  try {
    const presetData = {
      name: newPresetName.value.trim(),
      description: newPresetDescription.value.trim(),
      ph_config: { ...settingsStore.pid.ph },
      ec_config: { ...settingsStore.pid.ec }
    }

    const data = await api.post('/pid-presets', presetData)
    customPresets.value.push(data)
    createPresetDialog.value = false
    appStore.showSnackbar(`Пресет "${data.name}" сохранён`, 'success')
  } catch (error) {
    appStore.showSnackbar('Ошибка сохранения пресета', 'error')
  }
}

async function deleteCustomPreset(presetId) {
  try {
    await api.delete(`/pid-presets/${presetId}`)
    const index = customPresets.value.findIndex(p => p.id === presetId)
    if (index !== -1) {
      customPresets.value.splice(index, 1)
    }
    appStore.showSnackbar('Пресет удалён', 'info')
  } catch (error) {
    appStore.showSnackbar('Ошибка удаления пресета', 'error')
  }
}

function applyPreset(preset) {
  Object.assign(settingsStore.pid.ph, preset.ph_config)
  Object.assign(settingsStore.pid.ec, preset.ec_config)
  appStore.showSnackbar(`Применён пресет: ${preset.name}`, 'success')
}

function getPresetIcon(name) {
  const icons = {
    'Салат NFT (100L)': 'mdi-sprout',
    'Томаты капельный (300L)': 'mdi-fruit-cherries',
    'DWC малая (50L)': 'mdi-water',
    'Консервативный (безопасный)': 'mdi-shield-check',
  }
  return icons[name] || 'mdi-tune'
}

function getPresetColor(name) {
  const colors = {
    'Салат NFT (100L)': 'green',
    'Томаты капельный (300L)': 'red',
    'DWC малая (50L)': 'blue',
    'Консервативный (безопасный)': 'orange',
  }
  return colors[name] || 'primary'
}

function savePidSettings() {
  appStore.showSnackbar('Настройки PID сохранены', 'success')
}
</script>

<style scoped>
.bg-warning {
  background: linear-gradient(135deg, #f59e0b 0%, #f97316 100%);
}

.bg-primary {
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
}
</style>
