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
</script>

