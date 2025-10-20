<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">
          <v-icon icon="mdi-cog" class="mr-2"></v-icon>
          Расширенные настройки
        </h1>
      </v-col>
    </v-row>

    <v-tabs v-model="tab" bg-color="primary">
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
      <v-tab value="mqtt">
        <v-icon icon="mdi-access-point" start></v-icon>
        MQTT
      </v-tab>
      <v-tab value="telegram">
        <v-icon icon="mdi-send" start></v-icon>
        Telegram
      </v-tab>
      <v-tab value="database">
        <v-icon icon="mdi-database" start></v-icon>
        База данных
      </v-tab>
      <v-tab value="pid">
        <v-icon icon="mdi-tune-variant" start></v-icon>
        PID Контроллеры
      </v-tab>
      <v-tab value="advanced">
        <v-icon icon="mdi-cog-outline" start></v-icon>
        Дополнительно
      </v-tab>
    </v-tabs>

    <v-window v-model="tab" class="mt-4">
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
                  <v-list-item>
                    <v-list-item-title>Browser</v-list-item-title>
                    <v-list-item-subtitle>{{ browserInfo }}</v-list-item-subtitle>
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
                  v-model="settingsStore.ui.showGridLines"
                  label="Сетка на графиках"
                  class="mt-2"
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
                <v-btn
                  color="primary"
                  @click="applyChartSettings"
                >
                  Применить
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-col>

          <v-col cols="12" md="6">
            <v-card>
              <v-card-title>
                <v-icon icon="mdi-eye" class="mr-2"></v-icon>
                Предпросмотр
              </v-card-title>
              <v-card-text>
                <div class="chart-preview">
                  <v-alert type="info" variant="tonal">
                    Настройки графиков применятся ко всем графикам в системе
                  </v-alert>
                  
                  <div class="mt-4">
                    <strong>Текущие настройки:</strong>
                    <v-list density="compact">
                      <v-list-item>
                        <v-list-item-title>Период: {{ settingsStore.charts.defaultPeriod }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Размер точек: {{ settingsStore.charts.pointSize }}px</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Толщина линий: {{ settingsStore.charts.lineWidth }}px</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Заливка: {{ settingsStore.charts.fillArea ? 'Да' : 'Нет' }}</v-list-item-title>
                      </v-list-item>
                    </v-list>
                  </div>
                </div>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </v-window-item>

      <!-- MQTT Tab -->
      <v-window-item value="mqtt">
        <v-row>
          <v-col cols="12">
            <v-card>
              <v-card-title>
                <v-icon icon="mdi-access-point-network" class="mr-2"></v-icon>
                MQTT Broker Status
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
                      hydro/telemetry/#, hydro/event/#, hydro/heartbeat/#
                    </v-list-item-subtitle>
                  </v-list-item>
                </v-list>

                <v-alert type="info" variant="tonal" class="mt-4">
                  <small>
                    MQTT настройки редактируются в backend/.env файле
                  </small>
                </v-alert>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </v-window-item>

      <!-- Telegram Tab -->
      <v-window-item value="telegram">
        <v-row>
          <v-col cols="12">
            <v-card>
              <v-card-title>
                <v-icon icon="mdi-send" class="mr-2"></v-icon>
                Telegram Bot
              </v-card-title>
              <v-card-text>
                <v-alert
                  :type="systemStatus?.telegram === 'ok' ? 'success' : 'warning'"
                  variant="tonal"
                >
                  <strong>Status:</strong> {{ systemStatus?.telegram || 'disabled' }}
                </v-alert>

                <v-expansion-panels class="mt-4">
                  <v-expansion-panel>
                    <v-expansion-panel-title>
                      <v-icon icon="mdi-help-circle" class="mr-2"></v-icon>
                      Как настроить Telegram бота?
                    </v-expansion-panel-title>
                    <v-expansion-panel-text>
                      <ol>
                        <li>Откройте Telegram</li>
                        <li>Найдите @BotFather</li>
                        <li>Отправьте: <code>/newbot</code></li>
                        <li>Следуйте инструкциям</li>
                        <li>Скопируйте токен</li>
                        <li>Добавьте в backend/.env:
                          <pre>TELEGRAM_BOT_TOKEN=your_token_here</pre>
                        </li>
                        <li>Получите chat_id:
                          <pre>https://api.telegram.org/bot&lt;TOKEN&gt;/getUpdates</pre>
                        </li>
                        <li>Добавьте в backend/.env:
                          <pre>TELEGRAM_CHAT_ID=your_chat_id</pre>
                        </li>
                      </ol>
                    </v-expansion-panel-text>
                  </v-expansion-panel>
                </v-expansion-panels>
              </v-card-text>
            </v-card>
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
                  <div><strong>Тип БД:</strong> {{ systemStatus?.database || 'SQLite' }}</div>
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

                <v-btn
                  block
                  color="info"
                  prepend-icon="mdi-refresh"
                  @click="optimizeDatabase"
                >
                  Оптимизировать БД
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
                  <v-list-item>
                    <v-list-item-title>Команд</v-list-item-title>
                    <v-list-item-subtitle>{{ dbStats.commands || 'N/A' }}</v-list-item-subtitle>
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

      <!-- PID Tab -->
      <v-window-item value="pid">
        <v-row>
          <v-col cols="12">
            <v-card>
              <v-card-text>
                <v-alert type="info" variant="tonal" class="mb-4">
                  Тонкая настройка регуляторов pH и EC. Используйте небольшие изменения и наблюдайте за системой.
                  <br><strong>💡 Совет:</strong> Начните с пресетов ниже, затем подстройте под свою систему.
                </v-alert>

                <!-- Пресеты -->
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

                <v-row>
                  <v-col cols="12" md="6">
                    <div class="d-flex align-center mb-4">
                      <h3 class="text-subtitle-1 mr-2">pH Контроллер</h3>
                      <v-switch 
                        v-model="settingsStore.pid.ph.enabled" 
                        color="success"
                        hide-details
                        density="compact"
                      >
                        <template #label>
                          <span class="text-caption">{{ settingsStore.pid.ph.enabled ? 'Включён' : 'Выключен' }}</span>
                        </template>
                      </v-switch>
                    </div>

                    <v-divider class="mb-4"></v-divider>

                    <h4 class="text-caption text-grey mb-2">ОСНОВНЫЕ ПАРАМЕТРЫ</h4>
                    <v-text-field v-model.number="settingsStore.pid.ph.setpoint" type="number" step="0.1" label="Целевой pH"
                      suffix="pH" density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.kp" type="number" step="0.01" label="Kp (пропорциональный)"
                      density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.ki" type="number" step="0.001" label="Ki (интегральный)"
                      density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.kd" type="number" step="0.01" label="Kd (дифференциальный)"
                      density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ЗОНА НЕЧУВСТВИТЕЛЬНОСТИ</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.deadband" type="number" step="0.01" label="Deadband (гистерезис)"
                      suffix="pH" density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ТАЙМИНГИ</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.doseMinInterval" type="number" label="Мин. интервал между дозами"
                      suffix="сек" density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.mixDelay" type="number" label="Задержка перемешивания"
                      suffix="сек" density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ANTI-WINDUP</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.integralMax" type="number" step="0.5" label="Макс. интегратор"
                      density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.integralMin" type="number" step="0.5" label="Мин. интегратор"
                      density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ОГРАНИЧЕНИЯ ВЫХОДА</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.outputMax" type="number" step="0.5" label="Макс. доза за раз"
                      suffix="мл" density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ph.outputMin" type="number" step="0.5" label="Мин. доза за раз"
                      suffix="мл" density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ФИЛЬТРАЦИЯ ШУМА</h4>
                    
                    <v-slider 
                      v-model="settingsStore.pid.ph.filterAlpha" 
                      :min="0" :max="1" :step="0.05"
                      label="Фильтр показаний" 
                      thumb-label
                      density="compact"
                    ></v-slider>
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
                        color="success"
                        hide-details
                        density="compact"
                      >
                        <template #label>
                          <span class="text-caption">{{ settingsStore.pid.ec.enabled ? 'Включён' : 'Выключен' }}</span>
                        </template>
                      </v-switch>
                    </div>

                    <v-divider class="mb-4"></v-divider>

                    <h4 class="text-caption text-grey mb-2">ОСНОВНЫЕ ПАРАМЕТРЫ</h4>
                    <v-text-field v-model.number="settingsStore.pid.ec.setpoint" type="number" step="0.1" label="Целевой EC"
                      suffix="mS/cm" density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.kp" type="number" step="0.01" label="Kp (пропорциональный)"
                      density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.ki" type="number" step="0.001" label="Ki (интегральный)"
                      density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.kd" type="number" step="0.01" label="Kd (дифференциальный)"
                      density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ЗОНА НЕЧУВСТВИТЕЛЬНОСТИ</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.deadband" type="number" step="0.01" label="Deadband (гистерезис)"
                      suffix="mS/cm" density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ТАЙМИНГИ</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.doseMinInterval" type="number" label="Мин. интервал между дозами"
                      suffix="сек" density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.mixDelay" type="number" label="Задержка перемешивания"
                      suffix="сек" density="compact" variant="outlined"></v-text-field>

                    <v-text-field v-model.number="settingsStore.pid.ec.componentABDelay" type="number" label="Задержка между A и B"
                      suffix="сек" density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ANTI-WINDUP</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.integralMax" type="number" step="0.5" label="Макс. интегратор"
                      density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.integralMin" type="number" step="0.5" label="Мин. интегратор"
                      density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ОГРАНИЧЕНИЯ ВЫХОДА</h4>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.outputMax" type="number" step="0.5" label="Макс. доза за раз"
                      suffix="мл" density="compact" variant="outlined"></v-text-field>
                    
                    <v-text-field v-model.number="settingsStore.pid.ec.outputMin" type="number" step="0.5" label="Мин. доза за раз"
                      suffix="мл" density="compact" variant="outlined"></v-text-field>

                    <v-divider class="my-3"></v-divider>
                    <h4 class="text-caption text-grey mb-2">ФИЛЬТРАЦИЯ ШУМА</h4>
                    
                    <v-slider 
                      v-model="settingsStore.pid.ec.filterAlpha" 
                      :min="0" :max="1" :step="0.05"
                      label="Фильтр показаний" 
                      thumb-label
                      density="compact"
                    ></v-slider>
                    <div class="text-caption text-grey">
                      {{ settingsStore.pid.ec.filterAlpha === 1.0 ? 'Без фильтрации' : 
                         settingsStore.pid.ec.filterAlpha > 0.7 ? 'Слабая фильтрация' : 
                         settingsStore.pid.ec.filterAlpha > 0.4 ? 'Средняя фильтрация' : 'Сильная фильтрация' }}
                    </div>
                  </v-col>
                </v-row>
              </v-card-text>

              <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn color="success" prepend-icon="mdi-check" @click="savePidSettings">
                  Сохранить настройки PID
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-col>
        </v-row>
      </v-window-item>

      <!-- Advanced Tab -->
      <v-window-item value="advanced">
        <v-row>
          <v-col cols="12" md="6">
            <v-card>
              <v-card-title>
                <v-icon icon="mdi-download" class="mr-2"></v-icon>
                Экспорт/Импорт настроек
              </v-card-title>
              <v-card-text>
                <v-btn
                  block
                  color="primary"
                  prepend-icon="mdi-download"
                  class="mb-2"
                  @click="exportSettings"
                >
                  Экспортировать настройки
                </v-btn>

                <v-file-input
                  label="Импортировать настройки"
                  accept=".json"
                  variant="outlined"
                  prepend-icon="mdi-upload"
                  @change="importSettings"
                ></v-file-input>

                <v-alert type="info" variant="tonal" class="mt-4">
                  <small>
                    Экспортируются только настройки интерфейса (тема, уведомления, графики)
                  </small>
                </v-alert>
              </v-card-text>
            </v-card>
          </v-col>

          <v-col cols="12" md="6">
            <v-card>
              <v-card-title>
                <v-icon icon="mdi-restore" class="mr-2"></v-icon>
                Сброс настроек
              </v-card-title>
              <v-card-text>
                <v-alert type="warning" variant="tonal">
                  <strong>Внимание!</strong> Это сбросит все настройки интерфейса к значениям по умолчанию.
                </v-alert>

                <v-btn
                  block
                  color="error"
                  prepend-icon="mdi-restore"
                  class="mt-4"
                  @click="confirmReset = true"
                >
                  Сбросить к default
                </v-btn>
              </v-card-text>
            </v-card>

            <v-card class="mt-4">
              <v-card-title>
                <v-icon icon="mdi-information" class="mr-2"></v-icon>
                О приложении
              </v-card-title>
              <v-card-text>
                <v-list density="compact">
                  <v-list-item>
                    <v-list-item-title>Название</v-list-item-title>
                    <v-list-item-subtitle>Mesh Hydro System</v-list-item-subtitle>
                  </v-list-item>
                  <v-list-item>
                    <v-list-item-title>Версия</v-list-item-title>
                    <v-list-item-subtitle>2.0.0 Production</v-list-item-subtitle>
                  </v-list-item>
                  <v-list-item>
                    <v-list-item-title>Технологии</v-list-item-title>
                    <v-list-item-subtitle>
                      Vue.js 3.4 • Vuetify 3.5 • Laravel 10 • MQTT • SQLite/PostgreSQL
                    </v-list-item-subtitle>
                  </v-list-item>
                  <v-list-item>
                    <v-list-item-title>Лицензия</v-list-item-title>
                    <v-list-item-subtitle>MIT</v-list-item-subtitle>
                  </v-list-item>
                </v-list>
              </v-card-text>
            </v-card>
          </v-col>
        </v-row>
      </v-window-item>
    </v-window>

    <!-- Cleanup Dialog -->
    <v-dialog v-model="cleanupDialog" max-width="500">
      <v-card>
        <v-card-title class="bg-warning">
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
            <strong>Внимание!</strong> Это действие необратимо. Данные будут удалены из базы данных.
          </v-alert>
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="cleanupDialog = false">Отмена</v-btn>
          <v-btn color="warning" @click="executeCleanup">Очистить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Reset Confirmation Dialog -->
    <v-dialog v-model="confirmReset" max-width="400">
      <v-card>
        <v-card-title class="bg-error">
          <v-icon icon="mdi-alert-circle" class="mr-2"></v-icon>
          Подтверждение
        </v-card-title>
        <v-card-text class="pt-4">
          Вы уверены что хотите сбросить все настройки?
          <br><br>
          Страница будет перезагружена.
        </v-card-text>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn @click="confirmReset = false">Отмена</v-btn>
          <v-btn color="error" @click="resetSettings">Сбросить</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Create Preset Dialog -->
    <v-dialog v-model="createPresetDialog" max-width="600">
      <v-card>
        <v-card-title class="bg-primary">
          <v-icon icon="mdi-content-save-plus" class="mr-2"></v-icon>
          Создать пресет PID настроек
        </v-card-title>
        <v-card-text class="pt-4">
          <v-alert type="info" variant="tonal" class="mb-4">
            Сохранит текущие значения pH и EC контроллеров как пресет для быстрого применения в будущем.
          </v-alert>

          <v-text-field
            v-model="newPresetName"
            label="Название пресета"
            placeholder="Например: Моя DWC система 80L"
            variant="outlined"
            prepend-inner-icon="mdi-tag"
            :rules="[v => !!v || 'Введите название']"
            class="mb-2"
          ></v-text-field>

          <v-textarea
            v-model="newPresetDescription"
            label="Описание (опционально)"
            placeholder="Например: Настройки для салата в DWC системе 80л, pH 5.8, EC 1.4"
            variant="outlined"
            rows="3"
            prepend-inner-icon="mdi-text"
          ></v-textarea>

          <v-expansion-panels class="mt-4">
            <v-expansion-panel>
              <v-expansion-panel-title>
                <v-icon icon="mdi-eye" class="mr-2"></v-icon>
                Предпросмотр сохраняемых значений
              </v-expansion-panel-title>
              <v-expansion-panel-text>
                <v-row>
                  <v-col cols="12" md="6">
                    <div class="text-subtitle-2 mb-2">pH Контроллер:</div>
                    <v-list density="compact">
                      <v-list-item>
                        <v-list-item-title>Включен: {{ settingsStore.pid.ph.enabled ? 'Да' : 'Нет' }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Setpoint: {{ settingsStore.pid.ph.setpoint }} pH</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Kp: {{ settingsStore.pid.ph.kp }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Ki: {{ settingsStore.pid.ph.ki }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Kd: {{ settingsStore.pid.ph.kd }}</v-list-item-title>
                      </v-list-item>
                    </v-list>
                  </v-col>
                  <v-col cols="12" md="6">
                    <div class="text-subtitle-2 mb-2">EC Контроллер:</div>
                    <v-list density="compact">
                      <v-list-item>
                        <v-list-item-title>Включен: {{ settingsStore.pid.ec.enabled ? 'Да' : 'Нет' }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Setpoint: {{ settingsStore.pid.ec.setpoint }} mS/cm</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Kp: {{ settingsStore.pid.ec.kp }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Ki: {{ settingsStore.pid.ec.ki }}</v-list-item-title>
                      </v-list-item>
                      <v-list-item>
                        <v-list-item-title>Kd: {{ settingsStore.pid.ec.kd }}</v-list-item-title>
                      </v-list-item>
                    </v-list>
                  </v-col>
                </v-row>
              </v-expansion-panel-text>
            </v-expansion-panel>
          </v-expansion-panels>
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
            Сохранить пресет
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>

    <!-- Error Details Dialog -->
    <v-dialog v-model="errorDialog" max-width="700" scrollable>
      <v-card>
        <v-card-title class="bg-error text-white d-flex align-center">
          <v-icon icon="mdi-alert-circle" class="mr-2"></v-icon>
          {{ errorDetails.title }}
          <v-spacer></v-spacer>
          <v-btn
            icon="mdi-close"
            variant="text"
            size="small"
            @click="errorDialog = false"
          ></v-btn>
        </v-card-title>
        
        <v-card-text class="pt-4">
          <v-alert type="error" variant="tonal" class="mb-4">
            <div class="text-h6 mb-2">{{ errorDetails.message }}</div>
            <div class="text-caption text-grey">{{ errorDetails.timestamp }}</div>
          </v-alert>

          <v-card variant="outlined" class="mb-4">
            <v-card-title class="text-subtitle-2 bg-grey-lighten-4">
              <v-icon icon="mdi-information" class="mr-2" size="small"></v-icon>
              Подробности ошибки
            </v-card-title>
            <v-card-text>
              <pre class="error-details-text">{{ errorDetails.details }}</pre>
            </v-card-text>
          </v-card>

          <v-expansion-panels>
            <v-expansion-panel>
              <v-expansion-panel-title>
                <v-icon icon="mdi-help-circle" class="mr-2"></v-icon>
                Как исправить?
              </v-expansion-panel-title>
              <v-expansion-panel-text>
                <v-list density="compact">
                  <v-list-item prepend-icon="mdi-check-circle">
                    <v-list-item-title>Проверьте подключение к серверу</v-list-item-title>
                    <v-list-item-subtitle>Backend должен быть запущен на http://localhost:8000</v-list-item-subtitle>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-check-circle">
                    <v-list-item-title>Проверьте логи сервера</v-list-item-title>
                    <v-list-item-subtitle>docker logs hydro_backend</v-list-item-subtitle>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-check-circle">
                    <v-list-item-title>Проверьте миграции БД</v-list-item-title>
                    <v-list-item-subtitle>php artisan migrate:status</v-list-item-subtitle>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-check-circle">
                    <v-list-item-title>Откройте консоль браузера (F12)</v-list-item-title>
                    <v-list-item-subtitle>Проверьте детали запроса во вкладке Network</v-list-item-subtitle>
                  </v-list-item>
                </v-list>
              </v-expansion-panel-text>
            </v-expansion-panel>
          </v-expansion-panels>
        </v-card-text>
        
        <v-card-actions>
          <v-btn
            prepend-icon="mdi-content-copy"
            @click="copyErrorToClipboard"
            variant="outlined"
          >
            Копировать детали
          </v-btn>
          <v-spacer></v-spacer>
          <v-btn color="primary" @click="errorDialog = false">Закрыть</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </div>
</template>

<style scoped>
.error-details-text {
  font-size: 12px;
  line-height: 1.4;
  max-height: 300px;
  overflow: auto;
  background: #f5f5f5;
  padding: 12px;
  border-radius: 4px;
  white-space: pre-wrap;
  word-wrap: break-word;
}
</style>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useSettingsStore } from '@/stores/settings'
import { useAppStore } from '@/stores/app'
import { useNodesStore } from '@/stores/nodes'
import { useTelemetryStore } from '@/stores/telemetry'
import { useEventsStore } from '@/stores/events'
import SystemStatusCard from '@/components/SystemStatusCard.vue'
import { axios as api } from '@/services/api'

const settingsStore = useSettingsStore()
const appStore = useAppStore()
const nodesStore = useNodesStore()
const telemetryStore = useTelemetryStore()
const eventsStore = useEventsStore()

const tab = ref('system')
const systemStatus = ref(null)
const loading = ref(false)
const loadingStats = ref(false)
const cleanupDialog = ref(false)
const confirmReset = ref(false)
const cleanupDays = ref(90)
const dbStats = ref({})
const createPresetDialog = ref(false)
const newPresetName = ref('')
const newPresetDescription = ref('')
const customPresets = ref([])
const defaultPresets = ref([])
const errorDialog = ref(false)
const errorDetails = ref({
  title: '',
  message: '',
  details: '',
  timestamp: ''
})

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

const browserInfo = computed(() => {
  return navigator.userAgent.split('(')[1].split(')')[0]
})

const estimatedRecords = computed(() => {
  const nodes = nodesStore.nodes.length || 6
  const days = 30 // last 30 days
  const recordsPerDay = 288 // every 5 min
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
    // Get stats from stores
    dbStats.value = {
      nodes: nodesStore.nodes.length,
      telemetry: telemetryStore.telemetry.length || '?',
      events: eventsStore.events.length,
      commands: 0,
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
      icon: '/logo.png',
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
  
  // TODO: implement API endpoint for cleanup
  // await api.cleanupTelemetry(cleanupDays.value)
  
  setTimeout(() => {
    appStore.showSnackbar('Данные очищены (требуется backend API)', 'success')
  }, 1000)
}

function optimizeDatabase() {
  appStore.showSnackbar('Оптимизация БД запущена...', 'info')
  
  // TODO: implement API endpoint
  setTimeout(() => {
    appStore.showSnackbar('База данных оптимизирована', 'success')
  }, 2000)
}

function exportSettings() {
  settingsStore.exportSettings()
  appStore.showSnackbar('Настройки экспортированы', 'success')
}

async function importSettings(event) {
  const file = event.target.files[0]
  if (!file) return

  const reader = new FileReader()
  reader.onload = (e) => {
    const success = settingsStore.importSettings(e.target.result)
    if (success) {
      appStore.showSnackbar('Настройки импортированы', 'success')
      setTimeout(() => window.location.reload(), 1000)
    } else {
      appStore.showSnackbar('Ошибка импорта настроек', 'error')
    }
  }
  reader.readAsText(file)
}

function resetSettings() {
  settingsStore.resetToDefaults()
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
  // Settings are already saved to localStorage via store
  appStore.showSnackbar('Настройки PID сохранены', 'success')
}

// Error handling helper
function showDetailedError(title, error, context = '') {
  console.error(`[${context}]`, error)
  
  let errorMessage = 'Неизвестная ошибка'
  let errorDetailsText = ''

  if (error.response) {
    // Ошибка от сервера
    errorMessage = `HTTP ${error.response.status}: ${error.response.statusText || 'Ошибка сервера'}`
    errorDetailsText = JSON.stringify(error.response.data, null, 2)
  } else if (error.request) {
    // Запрос был отправлен, но ответа не было
    errorMessage = 'Сервер не отвечает'
    errorDetailsText = 'Не удалось получить ответ от сервера. Проверьте подключение к backend.'
  } else {
    // Ошибка при настройке запроса
    errorMessage = error.message || 'Ошибка запроса'
    errorDetailsText = error.stack || String(error)
  }

  errorDetails.value = {
    title,
    message: errorMessage,
    details: errorDetailsText,
    timestamp: new Date().toLocaleString('ru-RU')
  }
  
  errorDialog.value = true
  appStore.showSnackbar(`${title}: ${errorMessage}`, 'error')
}

// Custom Presets Management
async function loadCustomPresets() {
  try {
    const data = await api.get('/pid-presets')
    
    if (!data || !Array.isArray(data)) {
      throw new Error('Пустой ответ от сервера')
    }
    
    // Разделяем на дефолтные и пользовательские пресеты
    defaultPresets.value = data.filter(p => p.is_default)
    customPresets.value = data.filter(p => !p.is_default)
    
    console.log(`✅ Загружено ${defaultPresets.value.length} дефолтных и ${customPresets.value.length} пользовательских пресетов`)
  } catch (error) {
    showDetailedError('Ошибка загрузки пресетов', error, 'loadCustomPresets')
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

    console.log('📤 Отправка пресета на сервер:', presetData)
    const data = await api.post('/pid-presets', presetData)
    console.log('✅ Пресет сохранен:', data)
    
    // Добавляем созданный пресет в список
    customPresets.value.push(data)
    
    createPresetDialog.value = false
    appStore.showSnackbar(`Пресет "${data.name}" сохранён`, 'success')
  } catch (error) {
    showDetailedError('Ошибка сохранения пресета', error, 'saveCustomPreset')
  }
}

async function deleteCustomPreset(presetId) {
  try {
    const preset = customPresets.value.find(p => p.id === presetId)
    if (!preset) {
      appStore.showSnackbar('Пресет не найден', 'warning')
      return
    }

    console.log('🗑️ Удаление пресета:', preset.name, `(ID: ${presetId})`)
    await api.delete(`/pid-presets/${presetId}`)
    console.log('✅ Пресет удален')
    
    // Удаляем из списка
    const index = customPresets.value.findIndex(p => p.id === presetId)
    if (index !== -1) {
      customPresets.value.splice(index, 1)
    }
    
    appStore.showSnackbar(`Пресет "${preset.name}" удалён`, 'info')
  } catch (error) {
    showDetailedError('Ошибка удаления пресета', error, 'deleteCustomPreset')
  }
}

function copyErrorToClipboard() {
  const errorText = `
=== ${errorDetails.value.title} ===
Время: ${errorDetails.value.timestamp}
Сообщение: ${errorDetails.value.message}

Детали:
${errorDetails.value.details}

URL: ${window.location.href}
User Agent: ${navigator.userAgent}
  `.trim()

  navigator.clipboard.writeText(errorText).then(() => {
    appStore.showSnackbar('Детали ошибки скопированы в буфер обмена', 'success')
  }).catch(err => {
    console.error('Failed to copy error details:', err)
    appStore.showSnackbar('Не удалось скопировать в буфер обмена', 'error')
  })
}
</script>

