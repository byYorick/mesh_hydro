<template>
  <div>
    <v-row>
      <v-col cols="12">
        <h1 class="text-h3 mb-4">📚 Документация</h1>
      </v-col>
    </v-row>

    <!-- Navigation -->
    <v-row>
      <v-col cols="12" md="3">
        <v-card>
          <v-list density="compact">
            <v-list-item
              v-for="section in sections"
              :key="section.id"
              :value="section.id"
              :active="activeSection === section.id"
              @click="activeSection = section.id"
              :prepend-icon="section.icon"
            >
              <v-list-item-title>{{ section.title }}</v-list-item-title>
            </v-list-item>
          </v-list>
        </v-card>
      </v-col>

      <v-col cols="12" md="9">
        <!-- Быстрый старт -->
        <v-card v-if="activeSection === 'quick-start'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-rocket-launch</v-icon>
            Быстрый старт
          </v-card-title>
          <v-card-text>
            <h3>1. Запуск системы</h3>
            <v-code class="my-2">
docker-compose up -d
            </v-code>

            <h3 class="mt-4">2. Проверка статуса</h3>
            <v-code class="my-2">
docker-compose ps
            </v-code>

            <h3 class="mt-4">3. Доступные сервисы</h3>
            <v-list density="compact">
              <v-list-item prepend-icon="mdi-web">
                <v-list-item-title>Frontend: <a href="http://localhost:3000" target="_blank">http://localhost:3000</a></v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-api">
                <v-list-item-title>Backend API: <a href="http://localhost:8000/api/health" target="_blank">http://localhost:8000</a></v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-message-processing">
                <v-list-item-title>MQTT: localhost:1883</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-database">
                <v-list-item-title>PostgreSQL: localhost:5432</v-list-item-title>
              </v-list-item>
            </v-list>

            <v-alert type="info" class="mt-4">
              <strong>Первый запуск:</strong> При первом запуске автоматически выполнятся миграции БД. 
              Для добавления тестовых данных выполните: <code>docker exec hydro_backend php artisan db:seed</code>
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Архитектура -->
        <v-card v-if="activeSection === 'architecture'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-sitemap</v-icon>
            Архитектура системы
          </v-card-title>
          <v-card-text>
            <v-alert type="info" class="mb-4">
              Mesh Hydro System — это распределённая IoT система для автоматизации гидропонных установок.
            </v-alert>

            <h3>🏗️ Общая архитектура</h3>
            <div class="architecture-diagram my-4">
              <pre style="line-height: 1.4;">
┌─────────────────────────────────────────────────────────────┐
│                    ВАШ ПК / СЕРВЕР                          │
│                                                             │
│  ╔═════════════════════════════════════════════════════╗   │
│  ║            🐳 DOCKER COMPOSE                        ║   │
│  ║                                                     ║   │
│  ║  📡 Mosquitto MQTT Broker         :1883            ║   │
│  ║  🐘 PostgreSQL 15 Database        :5432            ║   │
│  ║  🔧 Laravel 10 Backend API        :8000            ║   │
│  ║  🎧 MQTT Listener (PHP worker)                     ║   │
│  ║  📊 Vue.js 3 Frontend             :3000            ║   │
│  ║  🔌 Laravel Reverb WebSocket      :6001            ║   │
│  ║                                                     ║   │
│  ╚═════════════════════════════════════════════════════╝   │
│                         ↕ WiFi + MQTT                       │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      ТЕПЛИЦА / ГРОУБОКС                     │
│                                                             │
│  ┌──────────────────────────────────────────────┐          │
│  │  🔌 ROOT NODE (ESP32-S3)                     │          │
│  │     - Координатор ESP-WIFI-MESH              │          │
│  │     - MQTT клиент → связь с сервером         │          │
│  │     - Маршрутизация данных от всех узлов     │          │
│  └──────────────────────────────────────────────┘          │
│                 ↓ ESP-WIFI-MESH                             │
│  ┌───────────┐  ┌──────────┐  ┌─────────┐  ┌───────────┐  │
│  │ 🌡️ Climate │  │ 🧪 pH/EC │  │ 🪟 Relay│  │ 💧 Water  │  │
│  │  ESP32    │  │  ESP32-S3│  │  ESP32  │  │  ESP32-C3 │  │
│  │           │  │          │  │         │  │           │  │
│  │ SHT3x     │  │ pH sensor│  │ Форточки│  │ Насосы    │  │
│  │ CCS811    │  │ EC sensor│  │ Вентиляц│  │ Клапаны   │  │
│  │ Lux       │  │ 5 насосов│  │ LED     │  │ Датчик    │  │
│  └───────────┘  └──────────┘  └─────────┘  └───────────┘  │
│                                                             │
│  ┌──────────────────────────────────────────────┐          │
│  │  📺 Display NODE (ESP32-S3) — опционально    │          │
│  │     - TFT дисплей 240x320 (ILI9341)          │          │
│  │     - LVGL UI                                │          │
│  │     - Ротационный энкодер                    │          │
│  └──────────────────────────────────────────────┘          │
│                                                             │
└─────────────────────────────────────────────────────────────┘
              </pre>
            </div>

            <h3 class="mt-6">📂 Структура проекта</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>📁 Серверная часть (server/)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <v-code>
server/
├── backend/                  # Laravel 10 Backend
│   ├── app/
│   │   ├── Models/          # Node, Telemetry, Event, Command, NodeError
│   │   ├── Controllers/     # REST API endpoints
│   │   ├── Services/        # MqttService, TelegramService, NotificationThrottleService
│   │   └── Console/         # Artisan commands (mqtt:listen, commands:process-timeouts)
│   ├── database/
│   │   ├── migrations/      # Схема БД (PostgreSQL)
│   │   └── seeders/         # Тестовые данные
│   ├── routes/api.php       # API роуты
│   └── config/
│       ├── mqtt.php         # MQTT конфигурация
│       └── hydro.php        # Настройки системы
│
├── frontend/                 # Vue.js 3 Frontend
│   ├── src/
│   │   ├── views/           # Страницы (Dashboard, Nodes, Settings, Documentation)
│   │   ├── components/      # Компоненты (Charts, Cards, Dialogs, HelpTooltip)
│   │   ├── stores/          # Pinia stores (nodes, events, telemetry, settings)
│   │   ├── services/        # API, Echo (WebSocket), configMeta
│   │   └── router/          # Vue Router
│   └── package.json
│
├── websocket/                # Socket.IO (опциональный fallback)
│   └── server.js
│
├── mosquitto/                # MQTT Broker конфиг
│   └── config/mosquitto.conf
│
└── docker-compose.yml        # Оркестрация всех сервисов
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>📁 Прошивки ESP32 (узлы)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <v-code>
mesh_hydro/
├── common/                   # Общие компоненты
│   ├── mesh_manager/        # ESP-WIFI-MESH логика
│   ├── mesh_protocol/       # Протокол обмена JSON
│   ├── sensor_base/         # Базовый класс сенсоров
│   ├── actuator_base/       # Базовый класс актуаторов
│   ├── ota_manager/         # OTA обновления
│   └── node_config/         # Конфигурация узлов (NVS)
│
├── root_node/                # ROOT координатор
│   ├── components/
│   │   ├── mqtt_client/     # MQTT клиент
│   │   ├── node_registry/   # Реестр mesh узлов
│   │   └── data_router/     # Маршрутизация данных
│   └── main/
│
├── node_climate/             # Датчики климата
│   └── components/
│       ├── sht3x_driver/    # Температура/влажность
│       ├── ccs811_driver/   # CO2
│       └── lux_driver/      # Освещённость
│
├── node_ph_ec/               # pH/EC контроллер
│   └── components/
│       ├── ph_sensor/       # pH датчик (I2C)
│       ├── ec_sensor/       # EC датчик (I2C)
│       ├── pump_manager/    # Управление 5 насосами
│       ├── pid_controller/  # PID регулятор
│       └── oled_display/    # OLED 128x64
│
├── node_relay/               # Реле и актуаторы
├── node_water/               # Система полива
└── node_display/             # TFT дисплей
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">🔄 Поток данных</h3>
            <div class="my-4">
              <v-stepper alt-labels>
                <v-stepper-header>
                  <v-stepper-item title="1. Датчик" value="1" icon="mdi-thermometer"></v-stepper-item>
                  <v-divider></v-divider>
                  <v-stepper-item title="2. Mesh" value="2" icon="mdi-access-point-network"></v-stepper-item>
                  <v-divider></v-divider>
                  <v-stepper-item title="3. ROOT" value="3" icon="mdi-router-wireless"></v-stepper-item>
                  <v-divider></v-divider>
                  <v-stepper-item title="4. MQTT" value="4" icon="mdi-message-processing"></v-stepper-item>
                  <v-divider></v-divider>
                  <v-stepper-item title="5. Backend" value="5" icon="mdi-server"></v-stepper-item>
                  <v-divider></v-divider>
                  <v-stepper-item title="6. Frontend" value="6" icon="mdi-monitor"></v-stepper-item>
                </v-stepper-header>
              </v-stepper>
            </div>

            <v-timeline side="end" density="compact" class="mt-4">
              <v-timeline-item dot-color="primary" size="small">
                <template v-slot:opposite>
                  <strong>NODE Climate</strong>
                </template>
                <div>
                  <div class="text-caption">Чтение датчиков SHT3x</div>
                  <code>temp: 24.5°C, humidity: 65%</code>
                </div>
              </v-timeline-item>

              <v-timeline-item dot-color="blue" size="small">
                <template v-slot:opposite>
                  <strong>ESP-WIFI-MESH</strong>
                </template>
                <div>
                  <div class="text-caption">Отправка через mesh</div>
                  <code>mesh_send_to_root(telemetry_data)</code>
                </div>
              </v-timeline-item>

              <v-timeline-item dot-color="green" size="small">
                <template v-slot:opposite>
                  <strong>ROOT NODE</strong>
                </template>
                <div>
                  <div class="text-caption">Получение и маршрутизация</div>
                  <code>mqtt_publish("hydro/telemetry/climate_001", json)</code>
                </div>
              </v-timeline-item>

              <v-timeline-item dot-color="orange" size="small">
                <template v-slot:opposite>
                  <strong>MQTT Broker</strong>
                </template>
                <div>
                  <div class="text-caption">Mosquitto в Docker</div>
                  <code>Topic: hydro/telemetry/climate_001</code>
                </div>
              </v-timeline-item>

              <v-timeline-item dot-color="purple" size="small">
                <template v-slot:opposite>
                  <strong>Backend</strong>
                </template>
                <div>
                  <div class="text-caption">MQTT Listener обработка</div>
                  <code>Telemetry::create() → PostgreSQL</code>
                  <br>
                  <code>broadcast TelemetryReceived event</code>
                </div>
              </v-timeline-item>

              <v-timeline-item dot-color="success" size="small">
                <template v-slot:opposite>
                  <strong>Frontend</strong>
                </template>
                <div>
                  <div class="text-caption">Real-time обновление через WebSocket</div>
                  <code>Echo → Chart.js → UI обновлён</code>
                </div>
              </v-timeline-item>
            </v-timeline>

            <h3 class="mt-6">📡 MQTT Топики</h3>
            <v-simple-table density="compact" class="mt-2">
              <template v-slot:default>
                <thead>
                  <tr>
                    <th>Топик</th>
                    <th>Направление</th>
                    <th>Описание</th>
                  </tr>
                </thead>
                <tbody>
                  <tr>
                    <td><code>hydro/telemetry/{node_id}</code></td>
                    <td>NODE → Server</td>
                    <td>Данные датчиков (temp, pH, EC и т.д.)</td>
                  </tr>
                  <tr>
                    <td><code>hydro/heartbeat/{node_id}</code></td>
                    <td>NODE → Server</td>
                    <td>Проверка связи (каждые 10 сек)</td>
                  </tr>
                  <tr>
                    <td><code>hydro/discovery</code></td>
                    <td>NODE → Server</td>
                    <td>Автообнаружение новых узлов</td>
                  </tr>
                  <tr>
                    <td><code>hydro/event/{node_id}</code></td>
                    <td>NODE → Server</td>
                    <td>События (ошибки, алерты)</td>
                  </tr>
                  <tr>
                    <td><code>hydro/error/{node_id}</code></td>
                    <td>NODE → Server</td>
                    <td>Критичные ошибки с диагностикой</td>
                  </tr>
                  <tr>
                    <td><code>hydro/command/{node_id}</code></td>
                    <td>Server → NODE</td>
                    <td>Команды (calibrate, reboot, pump_on)</td>
                  </tr>
                  <tr>
                    <td><code>hydro/config/{node_id}</code></td>
                    <td>Server → NODE</td>
                    <td>Обновление конфигурации узла</td>
                  </tr>
                  <tr>
                    <td><code>hydro/response/{node_id}</code></td>
                    <td>NODE → Server</td>
                    <td>Ответы на команды (ack, completed, failed)</td>
                  </tr>
                </tbody>
              </template>
            </v-simple-table>

            <h3 class="mt-6">🧩 Компоненты системы</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="primary">mdi-router-wireless</v-icon>
                  <strong>ROOT NODE (ESP32-S3)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Роль:</strong> Координатор mesh-сети и шлюз MQTT</p>
                  
                  <p><strong>Функции:</strong></p>
                  <ul>
                    <li>✅ Инициализация и управление ESP-WIFI-MESH сетью</li>
                    <li>✅ MQTT клиент → подключение к брокеру на сервере</li>
                    <li>✅ Реестр всех узлов в mesh (Node Registry)</li>
                    <li>✅ Маршрутизация данных: mesh → MQTT и MQTT → mesh</li>
                    <li>✅ Heartbeat от всех узлов → отслеживание состояния</li>
                    <li>✅ Резервная логика климата (если Climate NODE offline)</li>
                  </ul>

                  <p><strong>Периферия:</strong></p>
                  <ul>
                    <li>WiFi → подключение к роутеру + создание mesh</li>
                    <li>Индикатор состояния (LED RGB)</li>
                    <li>Кнопка сброса конфигурации</li>
                  </ul>

                  <p><strong>Конфигурация:</strong></p>
                  <v-code class="mt-2">
// root_node/components/mqtt_client/mqtt_client_manager.c
#define MQTT_BROKER_URI "mqtt://192.168.1.100:1883"
#define WIFI_SSID "YourWiFi"
#define WIFI_PASS "password"
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="blue">mdi-thermometer</v-icon>
                  <strong>NODE Climate (ESP32)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Роль:</strong> Мониторинг микроклимата</p>
                  
                  <p><strong>Датчики:</strong></p>
                  <ul>
                    <li><strong>SHT3x:</strong> температура (±0.1°C) + влажность (±1.5%) по I2C</li>
                    <li><strong>CCS811:</strong> CO₂ (400–8192 ppm) + TVOC по I2C</li>
                    <li><strong>Trema Lux:</strong> освещённость (люксы) по аналоговому входу</li>
                  </ul>

                  <p><strong>Режимы работы:</strong></p>
                  <ul>
                    <li><strong>Реальный:</strong> данные с физических датчиков</li>
                    <li><strong>Mock:</strong> генерация тестовых данных (для разработки без датчиков)</li>
                  </ul>

                  <p><strong>Частота отправки:</strong> каждые 30 секунд (настраивается)</p>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="green">mdi-flask</v-icon>
                  <strong>NODE pH/EC (ESP32-S3)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Роль:</strong> Контроль и автоматическая коррекция pH и EC</p>
                  
                  <p><strong>Датчики:</strong></p>
                  <ul>
                    <li><strong>pH sensor:</strong> Atlas Scientific или аналог (I2C)</li>
                    <li><strong>EC sensor:</strong> Atlas Scientific или аналог (I2C)</li>
                    <li><strong>Температура:</strong> DS18B20 (для компенсации EC)</li>
                  </ul>

                  <p><strong>Актуаторы:</strong></p>
                  <ul>
                    <li>Насос #1: pH Down (кислота, H₃PO₄)</li>
                    <li>Насос #2: pH Up (щёлочь, KOH)</li>
                    <li>Насос #3: EC Concentrate A (Ca, NO₃, Fe)</li>
                    <li>Насос #4: EC Concentrate B (K, Mg, PO₄, микроэлементы)</li>
                    <li>Насос #5: Резервный (вода или добавки)</li>
                  </ul>

                  <p><strong>PID контроллер:</strong></p>
                  <ul>
                    <li>Автоматическая коррекция pH (целевое значение, гистерезис)</li>
                    <li>Автоматическая коррекция EC</li>
                    <li>Безопасности: лимиты доз, минимальный интервал, температурная блокировка</li>
                  </ul>

                  <p><strong>Автономная работа:</strong></p>
                  <ul>
                    <li>Продолжает работать при потере связи с ROOT/сервером</li>
                    <li>Хранит конфигурацию в NVS (энергонезависимая память)</li>
                    <li>OLED дисплей показывает текущие значения</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="orange">mdi-electric-switch</v-icon>
                  <strong>NODE Relay (ESP32)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Роль:</strong> Управление климатом и освещением</p>
                  
                  <p><strong>Актуаторы:</strong></p>
                  <ul>
                    <li>Линейные актуаторы форточек (2 шт) — открытие/закрытие для вентиляции</li>
                    <li>Вентилятор (PWM) — циркуляция воздуха</li>
                    <li>LED освещение (PWM диммирование) — фотопериод и интенсивность</li>
                  </ul>

                  <p><strong>Команды:</strong></p>
                  <ul>
                    <li><code>open_windows</code> — открыть форточки</li>
                    <li><code>close_windows</code> — закрыть форточки</li>
                    <li><code>fan_on</code> / <code>fan_off</code></li>
                    <li><code>light_on</code> / <code>light_off</code> / <code>light_dim(value)</code></li>
                  </ul>

                  <p><strong>Автоматика:</strong></p>
                  <ul>
                    <li>Форточки открываются при температуре > 28°C</li>
                    <li>Вентилятор включается при влажности > 80%</li>
                    <li>Свет по расписанию (фотопериод 16/8)</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="cyan">mdi-water-pump</v-icon>
                  <strong>NODE Water (ESP32-C3)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Роль:</strong> Система полива и управление уровнем воды</p>
                  
                  <p><strong>Актуаторы:</strong></p>
                  <ul>
                    <li>Насос подачи воды</li>
                    <li>Насос слива (дренаж)</li>
                    <li>Соленоидные клапаны (3 зоны полива)</li>
                  </ul>

                  <p><strong>Датчики:</strong></p>
                  <ul>
                    <li>Ультразвуковой датчик уровня</li>
                    <li>Датчики протечки (опционально)</li>
                  </ul>

                  <p><strong>Безопасности:</strong></p>
                  <ul>
                    <li>Блокировка насосов при низком уровне (dry-run protection)</li>
                    <li>Таймаут максимальной работы насоса (5 мин)</li>
                    <li>Защита от переполнения</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="purple">mdi-monitor</v-icon>
                  <strong>NODE Display (ESP32-S3)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Роль:</strong> Локальный интерфейс в теплице</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>TFT дисплей 240x320 ILI9341 (SPI)</li>
                    <li>LVGL UI библиотека</li>
                    <li>Ротационный энкодер (меню, прокрутка)</li>
                  </ul>

                  <p><strong>Экраны:</strong></p>
                  <ul>
                    <li>Главный — сводка всех узлов</li>
                    <li>Климат — температура, влажность, CO₂</li>
                    <li>pH/EC — текущие значения + целевые</li>
                    <li>Состояние mesh — топология, RSSI</li>
                  </ul>

                  <p><strong>Преимущество:</strong> доступ к данным без компьютера/телефона</p>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">🌐 ESP-WIFI-MESH логика</h3>
            <v-alert type="info" class="mb-2">
              ESP-WIFI-MESH — встроенная технология ESP32 для создания самоорганизующихся mesh-сетей.
            </v-alert>

            <v-expansion-panels>
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Как работает mesh?</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ol>
                    <li><strong>ROOT NODE</strong> создаёт mesh-сеть и подключается к WiFi роутеру</li>
                    <li><strong>Остальные узлы</strong> сканируют mesh-сеть и подключаются к ближайшему узлу</li>
                    <li><strong>Автоматическая маршрутизация:</strong> данные идут по цепочке узлов к ROOT</li>
                    <li><strong>Self-healing:</strong> при потере связи с узлом, mesh перестраивается</li>
                    <li><strong>Multi-hop:</strong> данные могут проходить через несколько узлов</li>
                  </ol>

                  <p class="mt-4"><strong>Пример топологии:</strong></p>
                  <pre class="mt-2" style="line-height: 1.3;">
        [ROOT]
       /   |   \
    [A]   [B]   [C]
    /       \
  [D]       [E]

D → A → ROOT → MQTT
E → B → ROOT → MQTT
                  </pre>

                  <p class="mt-4"><strong>Преимущества mesh:</strong></p>
                  <ul>
                    <li>✅ Увеличенная зона покрытия (до 100м через промежуточные узлы)</li>
                    <li>✅ Автоматическое восстановление при сбоях</li>
                    <li>✅ Не нужен WiFi в каждой точке теплицы</li>
                    <li>✅ До 1000 узлов теоретически (практически 50–100)</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Формат сообщений</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Telemetry (NODE → Server):</strong></p>
                  <v-code class="mt-2">
{
  "type": "telemetry",
  "node_id": "climate_001",
  "node_type": "climate",
  "timestamp": 1729425600,
  "data": {
    "temperature": 24.5,
    "humidity": 65.2,
    "co2": 450,
    "lux": 25000
  },
  "firmware": "v1.0.0",
  "heap_free": 245678,
  "rssi_to_parent": -45
}
                  </v-code>

                  <p class="mt-4"><strong>Command (Server → NODE):</strong></p>
                  <v-code class="mt-2">
{
  "type": "command",
  "command_id": 123,
  "node_id": "relay_001",
  "command": "open_windows",
  "params": {
    "duration": 300
  },
  "timestamp": 1729425700
}
                  </v-code>

                  <p class="mt-4"><strong>Response (NODE → Server):</strong></p>
                  <v-code class="mt-2">
{
  "type": "response",
  "command_id": 123,
  "node_id": "relay_001",
  "status": "completed",
  "response": {
    "windows_position": "open",
    "execution_time_ms": 1250
  },
  "timestamp": 1729425705
}
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Автообнаружение узлов (Discovery)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Процесс:</strong></p>
                  <ol>
                    <li>Новый узел прошит и включен</li>
                    <li>Подключается к mesh-сети (находит ROOT)</li>
                    <li>Отправляет discovery сообщение на топик <code>hydro/discovery</code></li>
                    <li>Backend автоматически регистрирует узел в БД</li>
                    <li>Узел появляется в интерфейсе через 5–10 секунд</li>
                    <li>Frontend показывает уведомление "Новый узел обнаружен"</li>
                  </ol>

                  <p class="mt-4"><strong>Discovery сообщение:</strong></p>
                  <v-code class="mt-2">
{
  "type": "discovery",
  "node_id": "ph_ec_002",
  "node_type": "ph_ec",
  "mac_address": "AA:BB:CC:DD:EE:FF",
  "firmware": "v1.2.0",
  "hardware": "ESP32-S3",
  "sensors": ["ph", "ec", "temperature"],
  "capabilities": ["dosing", "calibration", "pid"],
  "heap_free": 256000,
  "mesh_nodes": 5
}
                  </v-code>

                  <p class="mt-4"><strong>Определение типа узла:</strong></p>
                  <ul>
                    <li>По префиксу node_id: <code>ph_ec_*</code> → тип "ph_ec"</li>
                    <li>По полю node_type в discovery</li>
                    <li>По наличию сенсоров в массиве sensors</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">💾 База данных (PostgreSQL)</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Схема таблиц</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Основные таблицы:</strong></p>
                  <ul>
                    <li><strong>nodes:</strong> реестр всех узлов (node_id, node_type, zone, mac_address, online, config, metadata)</li>
                    <li><strong>telemetry:</strong> данные датчиков (node_id, data JSONB, received_at) — партиционируется по дате</li>
                    <li><strong>events:</strong> события системы (node_id, level, message, data JSONB, resolved_at)</li>
                    <li><strong>commands:</strong> команды узлам (node_id, command, params JSONB, status, timeout_at)</li>
                    <li><strong>node_errors:</strong> ошибки узлов (node_id, error_code, severity, diagnostics JSONB)</li>
                  </ul>

                  <p class="mt-4"><strong>JSONB индексы (GIN):</strong></p>
                  <ul>
                    <li>Быстрый поиск по полям внутри JSON</li>
                    <li>Пример: <code>WHERE data->>'temperature' > 25</code></li>
                    <li>Индексы на config, metadata, data, diagnostics</li>
                  </ul>

                  <p class="mt-4"><strong>Оптимизация:</strong></p>
                  <ul>
                    <li>Индексы на node_id, node_type, online, received_at</li>
                    <li>Партиционирование telemetry по месяцам (для больших объёмов)</li>
                    <li>Автоочистка старых данных (365 дней по умолчанию)</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Real-time обновления</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Laravel Broadcasting + Reverb:</strong></p>
                  <ul>
                    <li>Новая телеметрия → <code>TelemetryReceived</code> event → WebSocket → Frontend</li>
                    <li>Узел offline → <code>NodeStatusChanged</code> event → UI обновляется</li>
                    <li>Новый узел → <code>NodeDiscovered</code> event → всплывающее уведомление</li>
                    <li>Критичное событие → <code>EventCreated</code> event → красный алерт</li>
                  </ul>

                  <p class="mt-4"><strong>Fallback на polling:</strong></p>
                  <ul>
                    <li>Если WebSocket недоступен → автоматический переход на HTTP polling (каждые 5 сек)</li>
                    <li>Индикатор состояния в UI: "WebSocket: connected" / "WebSocket: fallback"</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">🔒 Безопасность и отказоустойчивость</h3>
            <v-list density="compact" class="mt-2">
              <v-list-item prepend-icon="mdi-shield-check">
                <v-list-item-title><strong>Автономная работа узлов:</strong> pH/EC продолжает дозирование при потере связи</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-restore">
                <v-list-item-title><strong>Mesh self-healing:</strong> автоматическое восстановление топологии при сбоях</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-clock-alert">
                <v-list-item-title><strong>Watchdog:</strong> автоперезагрузка при зависании (HW WDT)</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-content-save">
                <v-list-item-title><strong>NVS хранение:</strong> конфигурация сохраняется при отключении питания</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-update">
                <v-list-item-title><strong>OTA обновления:</strong> обновление прошивок по воздуху (dual partition + rollback)</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-shield-lock">
                <v-list-item-title><strong>Лимиты дозирования:</strong> защита от передозировки химикатов</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-lock">
                <v-list-item-title><strong>WPA2 шифрование:</strong> безопасная WiFi mesh-сеть</v-list-item-title>
              </v-list-item>
            </v-list>

            <h3 class="mt-6">🛠️ Технологический стек</h3>
            <v-row class="mt-2">
              <v-col cols="12" md="6">
                <h4>Backend</h4>
                <v-chip-group column>
                  <v-chip color="red" variant="flat">Laravel 10.49</v-chip>
                  <v-chip color="blue" variant="flat">PHP 8.2</v-chip>
                  <v-chip color="primary" variant="flat">PostgreSQL 15.4</v-chip>
                  <v-chip color="orange" variant="flat">PhpMqtt</v-chip>
                  <v-chip color="purple" variant="flat">Laravel Reverb</v-chip>
                </v-chip-group>

                <h4 class="mt-4">Infrastructure</h4>
                <v-chip-group column>
                  <v-chip color="cyan" variant="flat">Docker Compose</v-chip>
                  <v-chip color="green" variant="flat">Mosquitto 2.0.18</v-chip>
                  <v-chip color="blue-grey" variant="flat">Nginx (optional)</v-chip>
                </v-chip-group>
              </v-col>

              <v-col cols="12" md="6">
                <h4>Frontend</h4>
                <v-chip-group column>
                  <v-chip color="green" variant="flat">Vue.js 3</v-chip>
                  <v-chip color="blue" variant="flat">Vite 5.4</v-chip>
                  <v-chip color="purple" variant="flat">Pinia</v-chip>
                  <v-chip color="indigo" variant="flat">Vuetify 3</v-chip>
                  <v-chip color="pink" variant="flat">Chart.js</v-chip>
                  <v-chip color="orange" variant="flat">Laravel Echo</v-chip>
                </v-chip-group>

                <h4 class="mt-4">Firmware (ESP32)</h4>
                <v-chip-group column>
                  <v-chip color="teal" variant="flat">ESP-IDF v5.5</v-chip>
                  <v-chip color="blue" variant="flat">ESP32 / ESP32-S3 / C3</v-chip>
                  <v-chip color="green" variant="flat">FreeRTOS</v-chip>
                  <v-chip color="orange" variant="flat">ESP-WIFI-MESH</v-chip>
                  <v-chip color="purple" variant="flat">LVGL (Display)</v-chip>
                </v-chip-group>
              </v-col>
            </v-row>

            <h3 class="mt-6">📊 Масштабируемость</h3>
            <v-simple-table density="compact" class="mt-2">
              <template v-slot:default>
                <thead>
                  <tr>
                    <th>Параметр</th>
                    <th>Текущий</th>
                    <th>Максимум</th>
                  </tr>
                </thead>
                <tbody>
                  <tr>
                    <td>ESP32 узлов</td>
                    <td>6</td>
                    <td>50–100 (mesh limit)</td>
                  </tr>
                  <tr>
                    <td>MQTT клиентов</td>
                    <td>1 ROOT</td>
                    <td>~100 на один брокер</td>
                  </tr>
                  <tr>
                    <td>Телеметрия записей</td>
                    <td>~576/день</td>
                    <td>Миллионы (с партиционированием)</td>
                  </tr>
                  <tr>
                    <td>WebSocket подключений</td>
                    <td>1–5</td>
                    <td>1000+ (Reverb)</td>
                  </tr>
                  <tr>
                    <td>API запросов/сек</td>
                    <td>~10</td>
                    <td>100+ (с кэшированием)</td>
                  </tr>
                </tbody>
              </template>
            </v-simple-table>

            <v-alert type="success" class="mt-4">
              <strong>Готовность к масштабированию:</strong> Архитектура поддерживает рост от домашней установки до коммерческой фермы без изменения кода!
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- PID Система -->
        <v-card v-if="activeSection === 'pid-system'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-chart-bell-curve</v-icon>
            PID Контроллеры — Полное руководство
          </v-card-title>
          <v-card-text>
            <v-alert type="info" class="mb-4">
              PID (Proportional-Integral-Derivative) — это математический алгоритм управления с обратной связью, 
              используемый для автоматической коррекции pH и EC в гидропонных системах.
            </v-alert>

            <h3>🎯 Что такое PID?</h3>
            <p>PID-контроллер непрерывно вычисляет <strong>ошибку</strong> (разницу между желаемым и текущим значением) 
            и применяет коррекцию на основе трёх компонентов:</p>

            <v-row class="mt-4">
              <v-col cols="12" md="4">
                <v-card color="primary" variant="tonal">
                  <v-card-title class="text-h6">P — Пропорциональная</v-card-title>
                  <v-card-text>
                    <p><strong>Формула:</strong> P = Kp × error</p>
                    <p class="mt-2">Реагирует пропорционально текущей ошибке. Чем больше отклонение, тем сильнее коррекция.</p>
                    <v-divider class="my-2"></v-divider>
                    <p><strong>Пример:</strong> pH = 7.0, цель = 5.8<br>error = 1.2<br>Если Kp = 0.3 → доза = 0.36 мл</p>
                  </v-card-text>
                </v-card>
              </v-col>

              <v-col cols="12" md="4">
                <v-card color="blue" variant="tonal">
                  <v-card-title class="text-h6">I — Интегральная</v-card-title>
                  <v-card-text>
                    <p><strong>Формула:</strong> I = Ki × Σ(error × dt)</p>
                    <p class="mt-2">Накапливает прошлые ошибки. Устраняет постоянное смещение (offset).</p>
                    <v-divider class="my-2"></v-divider>
                    <p><strong>Пример:</strong> pH всегда на 0.1 выше цели → интегратор накопит и скорректирует</p>
                  </v-card-text>
                </v-card>
              </v-col>

              <v-col cols="12" md="4">
                <v-card color="green" variant="tonal">
                  <v-card-title class="text-h6">D — Дифференциальная</v-card-title>
                  <v-card-text>
                    <p><strong>Формула:</strong> D = Kd × (d(error)/dt)</p>
                    <p class="mt-2">Реагирует на скорость изменения ошибки. Гасит колебания и перерегулирование.</p>
                    <v-divider class="my-2"></v-divider>
                    <p><strong>Пример:</strong> pH быстро падает → D применяет торможение</p>
                  </v-card-text>
                </v-card>
              </v-col>
            </v-row>

            <v-alert type="warning" class="mt-4">
              <strong>Итоговый выход PID:</strong><br>
              <code>output = Kp×error + Ki×Σ(error×dt) + Kd×(Δerror/Δt)</code>
            </v-alert>

            <h3 class="mt-6">📈 Поведение компонентов</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Только P (пропорциональный регулятор)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Характеристики:</strong></p>
                  <ul>
                    <li>✅ Простота</li>
                    <li>✅ Быстрая реакция</li>
                    <li>❌ Остаётся постоянная ошибка (offset)</li>
                    <li>❌ Может колебаться при большом Kp</li>
                  </ul>

                  <p class="mt-4"><strong>График:</strong></p>
                  <pre class="mt-2" style="line-height: 1.2;">
Значение
  ^
  │    ┌─────────────────── Целевое (setpoint)
  │    │     ╱╲╱╲╱╲╱╲╱╲
  │    │    ╱          ╲    ← Колебания
  │    │   ╱            ╲
  │ ───┤──────────────────── Постоянная ошибка
  │    │ ╱
  │    │╱
  └────┴─────────────────────→ Время
       Старт
                  </pre>

                  <p class="mt-2"><strong>Когда использовать:</strong> простые системы, где постоянная ошибка допустима</p>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>PI (пропорционально-интегральный)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Характеристики:</strong></p>
                  <ul>
                    <li>✅ Устраняет постоянную ошибку (I компонент)</li>
                    <li>✅ Достигает точного целевого значения</li>
                    <li>⚠️ Может перерегулировать (overshoot)</li>
                    <li>⚠️ Риск windup (накопление интегратора)</li>
                  </ul>

                  <p class="mt-4"><strong>График:</strong></p>
                  <pre class="mt-2" style="line-height: 1.2;">
Значение
  ^
  │         ╱╲
  │        ╱  ╲     ← Overshoot
  │    ───────────── Целевое
  │      ╱      ╲╱╲
  │     ╱          ╲
  │    ╱
  └────┴──────────────→ Время
                  </pre>

                  <p class="mt-2"><strong>Когда использовать:</strong> большинство гидропонных систем (pH, EC)</p>

                  <p class="mt-4"><strong>Anti-windup (важно!):</strong></p>
                  <ul>
                    <li>Ограничить накопление интегратора (например, ±10 от целевого)</li>
                    <li>Сбрасывать интегратор при достижении цели</li>
                    <li>Условное накопление: только если output не насыщен</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>PID (полный контроллер)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Характеристики:</strong></p>
                  <ul>
                    <li>✅ Быстрое достижение цели (P)</li>
                    <li>✅ Нулевая постоянная ошибка (I)</li>
                    <li>✅ Гашение колебаний (D)</li>
                    <li>⚠️ Сложная настройка</li>
                    <li>⚠️ D чувствителен к шуму датчика</li>
                  </ul>

                  <p class="mt-4"><strong>График:</strong></p>
                  <pre class="mt-2" style="line-height: 1.2;">
Значение
  ^
  │      ╱──────────── Целевое (идеальный выход)
  │     ╱
  │    ╱
  │   ╱
  │  ╱
  └──┴────────────────→ Время
   Старт
                  </pre>

                  <p class="mt-2"><strong>Когда использовать:</strong> критичные системы, требующие точности и стабильности</p>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">🎛️ Параметры PID</h3>
            <v-simple-table density="compact" class="mt-2">
              <template v-slot:default>
                <thead>
                  <tr>
                    <th>Параметр</th>
                    <th>Влияние на систему</th>
                    <th>Увеличение параметра</th>
                    <th>Уменьшение параметра</th>
                  </tr>
                </thead>
                <tbody>
                  <tr>
                    <td><strong>Kp</strong></td>
                    <td>Скорость реакции</td>
                    <td>Быстрее достигает цели, но может колебаться</td>
                    <td>Медленнее, но стабильнее</td>
                  </tr>
                  <tr>
                    <td><strong>Ki</strong></td>
                    <td>Устранение смещения</td>
                    <td>Быстрее убирает offset, но может перерегулировать</td>
                    <td>Медленнее, может остаться малая ошибка</td>
                  </tr>
                  <tr>
                    <td><strong>Kd</strong></td>
                    <td>Демпфирование</td>
                    <td>Меньше overshoot, но усиливает шум</td>
                    <td>Больше колебаний</td>
                  </tr>
                </tbody>
              </template>
            </v-simple-table>

            <h3 class="mt-6">⚙️ Методы настройки PID</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="primary">mdi-numeric-1-circle</v-icon>
                  <strong>Метод Циглера-Николса (Ziegler-Nichols)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Процесс:</strong></p>
                  <ol>
                    <li>Установите Ki = 0, Kd = 0</li>
                    <li>Увеличивайте Kp до начала стабильных колебаний</li>
                    <li>Запишите <strong>Ku</strong> (критический Kp) и <strong>Tu</strong> (период колебаний в секундах)</li>
                    <li>Рассчитайте по формулам:</li>
                  </ol>

                  <v-simple-table density="compact" class="mt-2">
                    <template v-slot:default>
                      <thead>
                        <tr>
                          <th>Тип</th>
                          <th>Kp</th>
                          <th>Ki</th>
                          <th>Kd</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr>
                          <td>P</td>
                          <td>0.5 × Ku</td>
                          <td>0</td>
                          <td>0</td>
                        </tr>
                        <tr>
                          <td>PI</td>
                          <td>0.45 × Ku</td>
                          <td>0.54 × Ku / Tu</td>
                          <td>0</td>
                        </tr>
                        <tr>
                          <td>PID</td>
                          <td>0.6 × Ku</td>
                          <td>1.2 × Ku / Tu</td>
                          <td>0.075 × Ku × Tu</td>
                        </tr>
                      </tbody>
                    </template>
                  </v-simple-table>

                  <p class="mt-4"><strong>Пример для pH:</strong></p>
                  <ul>
                    <li>Ku = 1.0 (колебания начались при Kp=1.0)</li>
                    <li>Tu = 120 сек (период колебаний)</li>
                    <li>Расчёт PI: Kp = 0.45, Ki = 0.54/120 = 0.0045</li>
                  </ul>

                  <v-alert type="warning" density="compact" class="mt-2">
                    <strong>Внимание:</strong> Метод агрессивный, может привести к большому overshoot. 
                    Используйте уменьшенные коэффициенты (×0.7) для гидропоники.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="blue">mdi-numeric-2-circle</v-icon>
                  <strong>Метод ручной настройки (Manual Tuning)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Пошаговый процесс:</strong></p>
                  
                  <h4 class="mt-4">Шаг 1: Настройка Kp</h4>
                  <ol>
                    <li>Установите Ki = 0, Kd = 0</li>
                    <li>Начните с малого Kp = 0.1</li>
                    <li>Наблюдайте реакцию системы на ошибку</li>
                    <li>Увеличивайте Kp пока не появятся небольшие колебания</li>
                    <li>Уменьшите Kp на 20–30%</li>
                  </ol>

                  <h4 class="mt-4">Шаг 2: Добавление Ki</h4>
                  <ol>
                    <li>Начните с Ki = Kp / 10</li>
                    <li>Проверьте, устраняется ли постоянная ошибка</li>
                    <li>Если медленно → увеличьте Ki</li>
                    <li>Если перерегулирование → уменьшите Ki</li>
                    <li>Типично: Ki = 0.01–0.05 для pH</li>
                  </ol>

                  <h4 class="mt-4">Шаг 3: Добавление Kd (опционально)</h4>
                  <ol>
                    <li>Начните с Kd = Kp × 0.1</li>
                    <li>Проверьте демпфирование колебаний</li>
                    <li>Если шум усиливается → уменьшите или уберите Kd</li>
                    <li>В гидропонике часто Kd = 0 (из-за медленных процессов)</li>
                  </ol>

                  <v-alert type="success" density="compact" class="mt-4">
                    <strong>Рекомендация:</strong> Для гидропоники обычно достаточно PI-контроллера (без D компонента).
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="green">mdi-numeric-3-circle</v-icon>
                  <strong>Метод Cohen-Coon</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Метод основан на реакции системы на единичный ступенчатый вход (step response).</p>

                  <p><strong>Процесс:</strong></p>
                  <ol>
                    <li>Выключите автоматику (manual mode)</li>
                    <li>Дайте системе стабилизироваться</li>
                    <li>Примените фиксированную дозу (например, 1 мл pH Down)</li>
                    <li>Запишите кривую изменения pH</li>
                    <li>Определите:</li>
                    <ul>
                      <li><strong>L</strong> — задержка (lag time)</li>
                      <li><strong>T</strong> — постоянная времени (63% от конечного значения)</li>
                      <li><strong>K</strong> — коэффициент усиления (Δвыход/Δвход)</li>
                    </ul>
                    <li>Рассчитайте по формулам Cohen-Coon</li>
                  </ol>

                  <v-alert type="info" density="compact" class="mt-2">
                    Метод точнее Циглера-Николса для систем с задержкой (как гидропоника), но требует больше времени на измерения.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="orange">mdi-auto-fix</v-icon>
                  <strong>Автоматический тюнинг (Relay Method)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Система автоматически находит оптимальные параметры методом релейной обратной связи.</p>

                  <p><strong>Процесс:</strong></p>
                  <ol>
                    <li>Включить режим автотюнинга</li>
                    <li>Система включает/выключает дозирование (relay)</li>
                    <li>Измеряет амплитуду и период колебаний</li>
                    <li>Вычисляет Ku и Tu</li>
                    <li>Применяет формулы Циглера-Николса</li>
                  </ol>

                  <v-alert type="success" density="compact" class="mt-2">
                    <strong>В разработке:</strong> Функция автотюнинга будет добавлена в следующей версии Mesh Hydro System.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">🧪 Настройка PID для pH</h3>
            <v-card color="blue-grey-lighten-5" class="mt-2">
              <v-card-text>
                <h4>Рекомендуемые стартовые значения:</h4>
                <v-simple-table density="compact" class="mt-2">
                  <template v-slot:default>
                    <thead>
                      <tr>
                        <th>Параметр</th>
                        <th>Значение</th>
                        <th>Описание</th>
                      </tr>
                    </thead>
                    <tbody>
                      <tr>
                        <td><strong>Kp</strong></td>
                        <td>0.2–0.5</td>
                        <td>Зависит от объёма раствора и силы насоса</td>
                      </tr>
                      <tr>
                        <td><strong>Ki</strong></td>
                        <td>0.01–0.05</td>
                        <td>Для медленного устранения смещения</td>
                      </tr>
                      <tr>
                        <td><strong>Kd</strong></td>
                        <td>0–0.1</td>
                        <td>Обычно 0 для pH (медленный процесс)</td>
                      </tr>
                      <tr>
                        <td><strong>Setpoint</strong></td>
                        <td>5.5–6.5</td>
                        <td>Зависит от культуры</td>
                      </tr>
                      <tr>
                        <td><strong>Deadband</strong></td>
                        <td>0.05–0.1</td>
                        <td>Гистерезис, зона нечувствительности</td>
                      </tr>
                    </tbody>
                  </template>
                </v-simple-table>

                <h4 class="mt-4">Специфика pH:</h4>
                <ul>
                  <li><strong>Медленный процесс:</strong> изменение pH занимает 30–120 сек после дозы</li>
                  <li><strong>Нелинейность:</strong> логарифмическая шкала (pH 7→6 легче, чем 6→5)</li>
                  <li><strong>Буферность:</strong> вода/удобрения могут сопротивляться изменению pH</li>
                  <li><strong>Drift:</strong> pH имеет тенденцию расти со временем (растения потребляют кислоты)</li>
                </ul>

                <h4 class="mt-4">Безопасности для pH:</h4>
                <v-list density="compact" class="mt-2">
                  <v-list-item prepend-icon="mdi-timer">
                    <v-list-item-title><strong>Минимальный интервал доз:</strong> 60–120 сек (время смешивания)</v-list-item-title>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-clock">
                    <v-list-item-title><strong>Задержка оценки:</strong> 30–60 сек после дозы (mix delay)</v-list-item-title>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-flask-outline">
                    <v-list-item-title><strong>Максимум доз в сутки:</strong> 50–100 (защита от передозировки)</v-list-item-title>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-water-alert">
                    <v-list-item-title><strong>Блокировка при низком уровне воды:</strong> предотвращает концентрацию кислоты/щёлочи</v-list-item-title>
                  </v-list-item>
                </v-list>

                <h4 class="mt-4">Примерный алгоритм:</h4>
                <v-code class="mt-2">
1. Измерить pH
2. Вычислить error = setpoint - pH_current
3. Если |error| < deadband → ничего не делать
4. Если прошло < min_interval → подождать
5. Вычислить PID output
6. Ограничить output (min/max доза)
7. Выдать дозу соответствующего насоса (Up/Down)
8. Обновить integral накопитель (с anti-windup)
9. Запомнить prev_error для D компонента
10. Ждать mix_delay секунд перед следующей оценкой
                </v-code>
              </v-card-text>
            </v-card>

            <h3 class="mt-6">⚡ Настройка PID для EC</h3>
            <v-card color="green-lighten-5" class="mt-2">
              <v-card-text>
                <h4>Рекомендуемые стартовые значения:</h4>
                <v-simple-table density="compact" class="mt-2">
                  <template v-slot:default>
                    <thead>
                      <tr>
                        <th>Параметр</th>
                        <th>Значение</th>
                        <th>Описание</th>
                      </tr>
                    </thead>
                    <tbody>
                      <tr>
                        <td><strong>Kp</strong></td>
                        <td>0.1–0.3</td>
                        <td>Меньше чем pH (EC медленнее)</td>
                      </tr>
                      <tr>
                        <td><strong>Ki</strong></td>
                        <td>0.005–0.02</td>
                        <td>Очень малые значения</td>
                      </tr>
                      <tr>
                        <td><strong>Kd</strong></td>
                        <td>0</td>
                        <td>Не используется для EC</td>
                      </tr>
                      <tr>
                        <td><strong>Setpoint</strong></td>
                        <td>1.0–3.5</td>
                        <td>Зависит от культуры и стадии</td>
                      </tr>
                      <tr>
                        <td><strong>Deadband</strong></td>
                        <td>0.05–0.1</td>
                        <td>Гистерезис</td>
                      </tr>
                    </tbody>
                  </template>
                </v-simple-table>

                <h4 class="mt-4">Специфика EC:</h4>
                <ul>
                  <li><strong>Очень медленный процесс:</strong> изменение EC занимает 2–5 минут после дозы</li>
                  <li><strong>Линейная зависимость:</strong> EC пропорционален концентрации (в отличие от логарифмического pH)</li>
                  <li><strong>Испарение воды:</strong> повышает EC без добавления удобрений (компенсируйте чистой водой)</li>
                  <li><strong>Потребление растениями:</strong> снижает EC (нужны регулярные дозы)</li>
                  <li><strong>Температурная компенсация:</strong> EC зависит от температуры (автокомпенсация в датчике)</li>
                </ul>

                <h4 class="mt-4">Безопасности для EC:</h4>
                <v-list density="compact" class="mt-2">
                  <v-list-item prepend-icon="mdi-timer-sand">
                    <v-list-item-title><strong>Минимальный интервал:</strong> 120–300 сек (EC изменяется медленнее pH)</v-list-item-title>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-clock-time-eight">
                    <v-list-item-title><strong>Задержка оценки:</strong> 45–90 сек (mix + диффузия)</v-list-item-title>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-flask">
                    <v-list-item-title><strong>Максимум концентрата:</strong> 2000 мл/сутки (защита от перекорма)</v-list-item-title>
                  </v-list-item>
                  <v-list-item prepend-icon="mdi-bottle-tonic-skull">
                    <v-list-item-title><strong>Раздельная подача A и B:</strong> смешивать только в разбавленном виде!</v-list-item-title>
                  </v-list-item>
                </v-list>

                <h4 class="mt-4">Двухкомпонентные удобрения (A+B):</h4>
                <v-alert type="error" density="compact" class="mt-2">
                  <strong>КРИТИЧНО:</strong> НИКОГДА не смешивайте концентраты A и B напрямую! 
                  Ca из A + PO₄/SO₄ из B образуют нерастворимый осадок.
                </v-alert>
                <p class="mt-2"><strong>Правильная последовательность:</strong></p>
                <ol>
                  <li>Доза A → включить циркуляцию → ждать 30 сек</li>
                  <li>Доза B → включить циркуляцию → ждать 30 сек</li>
                  <li>Оценить EC</li>
                </ol>
              </v-card-text>
            </v-card>

            <h3 class="mt-6">🛡️ Продвинутые техники</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Anti-Windup (борьба с накоплением интегратора)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Проблема:</strong> Интегратор накапливает ошибку даже когда output насыщен (достиг лимита). 
                  При возврате в диапазон — огромный overshoot.</p>

                  <p class="mt-4"><strong>Решения:</strong></p>
                  
                  <h4 class="mt-2">1. Clamping (ограничение)</h4>
                  <v-code class="mt-2">
if (output > max_output) {
    output = max_output;
    // НЕ накапливать интегратор при насыщении
    integral = integral; // заморозить
}
                  </v-code>

                  <h4 class="mt-4">2. Conditional Integration</h4>
                  <v-code class="mt-2">
if (abs(error) < 1.0 && output_not_saturated) {
    integral += error * dt;  // накапливать только в рабочем диапазоне
}
                  </v-code>

                  <h4 class="mt-4">3. Back Calculation</h4>
                  <v-code class="mt-2">
// Если output обрезан
if (output_clamped) {
    integral -= (output_actual - output_desired) / Ki;
}
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Deadband (зона нечувствительности)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Зачем:</strong> Избежать постоянного дозирования при малых отклонениях (дрожание).</p>

                  <p class="mt-4"><strong>Реализация:</strong></p>
                  <v-code class="mt-2">
error = setpoint - current_value;

if (abs(error) < deadband) {
    // Ничего не делать, в пределах допустимого
    return;
}

// Иначе — применить PID коррекцию
                  </v-code>

                  <p class="mt-4"><strong>Рекомендуемые значения:</strong></p>
                  <ul>
                    <li><strong>pH:</strong> ±0.05–0.1 (например, 5.75–5.85 для цели 5.8)</li>
                    <li><strong>EC:</strong> ±0.05–0.1 mS/cm</li>
                  </ul>

                  <v-alert type="info" density="compact" class="mt-2">
                    Слишком узкий deadband → частые дозы. Слишком широкий → неточность.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Adaptive PID (адаптивный ПИД)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Коэффициенты меняются в зависимости от величины ошибки.</p>

                  <p class="mt-4"><strong>Пример для pH:</strong></p>
                  <v-code class="mt-2">
if (abs(error) > 1.0) {
    // Большая ошибка — агрессивная коррекция
    Kp = 0.5;
    Ki = 0.05;
} else if (abs(error) > 0.3) {
    // Средняя ошибка — нормальный режим
    Kp = 0.3;
    Ki = 0.02;
} else {
    // Малая ошибка — мягкая доводка
    Kp = 0.1;
    Ki = 0.01;
}
                  </v-code>

                  <p class="mt-4"><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Быстрая коррекция при больших отклонениях</li>
                    <li>✅ Стабильность вблизи целевого значения</li>
                    <li>✅ Меньше overshoot</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Feed-Forward контроль</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Предиктивная коррекция на основе известных возмущений.</p>

                  <p class="mt-4"><strong>Примеры:</strong></p>
                  <ul>
                    <li><strong>Долив воды:</strong> заранее корректируем pH/EC (вода обычно pH ~7–8)</li>
                    <li><strong>Добавление удобрений:</strong> предсказуемое изменение pH</li>
                    <li><strong>Время суток:</strong> днём pH растёт быстрее (фотосинтез)</li>
                  </ul>

                  <v-code class="mt-2">
// Предсказание изменения при доливе 10л воды (pH воды = 7.5)
predicted_pH_change = (10L × 7.5 - 100L × 5.8) / 110L;

// Превентивная доза pH Down
preventive_dose = calculate_dose(predicted_pH_change);
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">📊 Диагностика проблем PID</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="error">mdi-sine-wave</v-icon>
                  <strong>Постоянные колебания (oscillation)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Признаки:</strong> pH/EC постоянно колеблется вокруг целевого значения.</p>

                  <p class="mt-4"><strong>Причины и решения:</strong></p>
                  <ul>
                    <li><strong>Kp слишком велик</strong> → уменьшите Kp на 30–50%</li>
                    <li><strong>Ki слишком велик</strong> → уменьшите Ki в 2 раза</li>
                    <li><strong>Недостаточный mix delay</strong> → увеличьте задержку перемешивания</li>
                    <li><strong>Плохая циркуляция</strong> → проверьте насосы, добавьте барботёр</li>
                    <li><strong>Deadband слишком узкий</strong> → увеличьте зону нечувствительности</li>
                  </ul>

                  <v-alert type="success" density="compact" class="mt-2">
                    <strong>Быстрое решение:</strong> Уменьшите Kp вдвое и увеличьте mix_delay на 50%.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="warning">mdi-arrow-up-bold</v-icon>
                  <strong>Перерегулирование (overshoot)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Признаки:</strong> Система проскакивает целевое значение и возвращается.</p>

                  <p class="mt-4"><strong>Причины и решения:</strong></p>
                  <ul>
                    <li><strong>Kp слишком велик</strong> → уменьшите Kp</li>
                    <li><strong>Integral windup</strong> → добавьте anti-windup</li>
                    <li><strong>Нет D компонента</strong> → добавьте малый Kd (0.05–0.1)</li>
                    <li><strong>Слишком большая доза</strong> → ограничьте max_output</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="blue">mdi-clock-time-three</v-icon>
                  <strong>Медленное достижение цели</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Признаки:</strong> Система очень долго выходит на целевое значение.</p>

                  <p class="mt-4"><strong>Причины и решения:</strong></p>
                  <ul>
                    <li><strong>Kp слишком мал</strong> → увеличьте Kp на 50%</li>
                    <li><strong>Слабый насос</strong> → проверьте производительность</li>
                    <li><strong>Большой объём раствора</strong> → увеличьте Kp и дозу</li>
                    <li><strong>Deadband слишком широкий</strong> → уменьшите</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="orange">mdi-delta</v-icon>
                  <strong>Постоянное смещение (steady-state error)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Признаки:</strong> Система стабилизируется, но не на целевом значении (всегда ±0.2 от цели).</p>

                  <p class="mt-4"><strong>Причины и решения:</strong></p>
                  <ul>
                    <li><strong>Нет I компонента</strong> → добавьте Ki (начните с 0.01)</li>
                    <li><strong>Ki слишком мал</strong> → увеличьте Ki в 2 раза</li>
                    <li><strong>Deadband покрывает ошибку</strong> → уменьшите deadband</li>
                    <li><strong>Drift раствора</strong> → нормально, I компонент должен компенсировать</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="purple">mdi-creation</v-icon>
                  <strong>Шум и дрожание показаний</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Признаки:</strong> Показания датчика скачут (±0.1–0.3), PID реагирует на шум.</p>

                  <p class="mt-4"><strong>Причины и решения:</strong></p>
                  <ul>
                    <li><strong>Kd усиливает шум</strong> → уменьшите или уберите Kd</li>
                    <li><strong>Плохая калибровка датчика</strong> → откалибруйте заново</li>
                    <li><strong>Плохая циркуляция</strong> → датчик в зоне застоя</li>
                    <li><strong>Нет фильтрации</strong> → добавьте скользящее среднее (moving average):</li>
                  </ul>
                  
                  <v-code class="mt-2">
// Простой фильтр (экспоненциальное сглаживание)
filtered_value = alpha × new_reading + (1-alpha) × filtered_value;
// alpha = 0.1–0.3 (чем меньше, тем сильнее сглаживание)
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">🔬 Практические примеры настройки</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Пример 1: Салат в NFT системе (100L раствор)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Параметры системы:</strong></p>
                  <ul>
                    <li>Объём: 100 литров</li>
                    <li>Культура: салат (pH 5.5–6.0, EC 1.4–1.8)</li>
                    <li>Система: NFT с постоянной циркуляцией</li>
                    <li>Насосы pH: 5 мл/мин</li>
                    <li>Насосы EC: 10 мл/мин (концентрат 100x)</li>
                  </ul>

                  <p class="mt-4"><strong>Настройки PID для pH:</strong></p>
                  <v-code class="mt-2">
Kp = 0.3
Ki = 0.02
Kd = 0
Setpoint = 5.8
Deadband = 0.08 (зона 5.72–5.88)
Min interval = 90 сек
Mix delay = 45 сек
Max daily pH Down = 300 мл
Max daily pH Up = 200 мл
                  </v-code>

                  <p class="mt-4"><strong>Настройки PID для EC:</strong></p>
                  <v-code class="mt-2">
Kp = 0.2
Ki = 0.01
Kd = 0
Setpoint = 1.6
Deadband = 0.08 (зона 1.52–1.68)
Min interval = 180 сек
Mix delay = 60 сек
Max daily nutrients = 1500 мл
                  </v-code>

                  <p class="mt-4"><strong>Результат:</strong> Стабильный pH ±0.05, EC ±0.05 в течение дня. Дозирование 2–3 раза в час.</p>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Пример 2: Томаты в капельной системе (300L раствор)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Параметры системы:</strong></p>
                  <ul>
                    <li>Объём: 300 литров</li>
                    <li>Культура: томаты плодоношение (pH 5.8–6.2, EC 2.5–3.0)</li>
                    <li>Система: капельный полив на кокосе</li>
                    <li>Насосы pH: 10 мл/мин</li>
                    <li>Насосы EC: 20 мл/мин (концентрат 100x)</li>
                  </ul>

                  <p class="mt-4"><strong>Настройки PID для pH:</strong></p>
                  <v-code class="mt-2">
Kp = 0.4    // Больше объём → больше Kp
Ki = 0.015
Kd = 0
Setpoint = 6.0
Deadband = 0.1
Min interval = 120 сек
Mix delay = 60 сек
Max daily pH Down = 500 мл  // Томаты больше потребляют
Max daily pH Up = 300 мл
                  </v-code>

                  <p class="mt-4"><strong>Настройки PID для EC:</strong></p>
                  <v-code class="mt-2">
Kp = 0.25
Ki = 0.008  // Меньше, т.к. большой объём
Kd = 0
Setpoint = 2.7
Deadband = 0.1
Min interval = 240 сек  // Большие дозы → больше интервал
Mix delay = 90 сек
Max daily nutrients = 3000 мл
                  </v-code>

                  <p class="mt-4"><strong>Результат:</strong> Стабильный pH ±0.08, EC ±0.1. Дозирование 1–2 раза в час.</p>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">📐 Формулы и алгоритмы</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Классический PID алгоритм (Position Form)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <v-code class="mt-2">
// Инициализация
float integral = 0;
float prev_error = 0;
float dt = 1.0;  // Интервал цикла (сек)

// Основной цикл
void pid_update(float setpoint, float current_value) {
    // 1. Вычисление ошибки
    float error = setpoint - current_value;
    
    // 2. Проверка deadband
    if (fabs(error) < deadband) {
        return;  // В пределах допустимого
    }
    
    // 3. Пропорциональная составляющая
    float P = Kp * error;
    
    // 4. Интегральная составляющая
    integral += error * dt;
    
    // Anti-windup: ограничение накопления
    if (integral > integral_max) integral = integral_max;
    if (integral < integral_min) integral = integral_min;
    
    float I = Ki * integral;
    
    // 5. Дифференциальная составляющая
    float derivative = (error - prev_error) / dt;
    float D = Kd * derivative;
    
    // 6. Суммарный выход
    float output = P + I + D;
    
    // 7. Ограничение выхода
    if (output > max_output) output = max_output;
    if (output < min_output) output = min_output;
    
    // 8. Применение коррекции
    if (output > 0) {
        dose_ph_down(output);  // Понизить pH
    } else if (output < 0) {
        dose_ph_up(fabs(output));  // Повысить pH
    }
    
    // 9. Сохранение состояния
    prev_error = error;
}
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Velocity Form (инкрементальная форма)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Вычисляет изменение выхода вместо абсолютного значения. Лучше для систем с интеграторами.</p>

                  <v-code class="mt-2">
float prev_error = 0;
float prev_prev_error = 0;

void pid_velocity_update(float setpoint, float current_value) {
    float error = setpoint - current_value;
    
    // Вычисление приращения выхода
    float delta_P = Kp * (error - prev_error);
    float delta_I = Ki * error;
    float delta_D = Kd * (error - 2*prev_error + prev_prev_error);
    
    float delta_output = delta_P + delta_I + delta_D;
    
    // Применяем приращение
    apply_dose_increment(delta_output);
    
    // Сохранение
    prev_prev_error = prev_error;
    prev_error = error;
}
                  </v-code>

                  <p class="mt-4"><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Нет накопления интегратора (меньше windup)</li>
                    <li>✅ Плавные изменения выхода</li>
                    <li>✅ Bumpless transfer при смене setpoint</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Дискретная реализация (для микроконтроллеров)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Адаптация непрерывного PID для дискретных систем (ESP32).</p>

                  <v-code class="mt-2">
typedef struct {
    float Kp, Ki, Kd;
    float setpoint;
    float deadband;
    float integral;
    float prev_error;
    float integral_max;
    float integral_min;
    float output_max;
    float output_min;
} pid_controller_t;

float pid_compute(pid_controller_t *pid, float measured, float dt) {
    // Ошибка
    float error = pid->setpoint - measured;
    
    // Deadband check
    if (fabsf(error) < pid->deadband) {
        return 0.0f;
    }
    
    // P компонент
    float P_out = pid->Kp * error;
    
    // I компонент с anti-windup
    pid->integral += error * dt;
    
    // Clamping
    if (pid->integral > pid->integral_max) {
        pid->integral = pid->integral_max;
    } else if (pid->integral < pid->integral_min) {
        pid->integral = pid->integral_min;
    }
    
    float I_out = pid->Ki * pid->integral;
    
    // D компонент
    float derivative = (error - pid->prev_error) / dt;
    float D_out = pid->Kd * derivative;
    
    // Суммарный выход
    float output = P_out + I_out + D_out;
    
    // Ограничение выхода
    if (output > pid->output_max) output = pid->output_max;
    if (output < pid->output_min) output = pid->output_min;
    
    // Сохранение
    pid->prev_error = error;
    
    return output;
}
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">💡 Советы экспертов</h3>
            <v-list density="compact" class="mt-2">
              <v-list-item prepend-icon="mdi-lightbulb">
                <v-list-item-title><strong>Начинайте с PI:</strong> Для гидропоники обычно достаточно PI (без D)</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-tune-variant">
                <v-list-item-title><strong>Малые изменения:</strong> Меняйте параметры на 20–30%, не в 2 раза</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-clock-check">
                <v-list-item-title><strong>Дайте время:</strong> Наблюдайте 2–4 часа перед следующей корректировкой</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-water-check">
                <v-list-item-title><strong>Хорошее перемешивание:</strong> Критично! Без циркуляции PID не работает</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-chart-line">
                <v-list-item-title><strong>Логируйте всё:</strong> Сохраняйте время, дозу, pH до/после для анализа</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-shield-check">
                <v-list-item-title><strong>Безопасность превыше:</strong> Лимиты доз важнее идеального PID</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-test-tube">
                <v-list-item-title><strong>Калибровка датчиков:</strong> PID точен настолько, насколько точны датчики</v-list-item-title>
              </v-list-item>
            </v-list>

            <h3 class="mt-6">📚 Дополнительные ресурсы</h3>
            <v-list density="compact" class="mt-2">
              <v-list-item prepend-icon="mdi-book">
                <v-list-item-title><strong>Книга:</strong> "PID Controllers: Theory, Design, and Tuning" — Åström & Hägglund</v-list-item-title>
                <v-list-item-subtitle>Классический учебник по PID</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-youtube">
                <v-list-item-title><strong>Видео:</strong> "Understanding PID Control" — MATLAB Tech Talk</v-list-item-title>
                <v-list-item-subtitle>Визуальное объяснение (YouTube)</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-web">
                <v-list-item-title><strong>Онлайн симулятор:</strong> pidtuner.com</v-list-item-title>
                <v-list-item-subtitle>Интерактивная настройка PID</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-code-braces">
                <v-list-item-title><strong>Библиотека Arduino:</strong> Arduino PID Library by Brett Beauregard</v-list-item-title>
                <v-list-item-subtitle>Готовая реализация для ESP32</v-list-item-subtitle>
              </v-list-item>
            </v-list>

            <h3 class="mt-6">🎓 Чек-лист перед запуском PID</h3>
            <v-list density="compact" class="mt-2">
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Датчики откалиброваны (pH двухточечная 4.0+7.0, EC одноточечная)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Хорошая циркуляция раствора (датчики в потоке, не в застое)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Насосы дозирования работают (проверены вручную)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Установлены лимиты дозирования (max daily doses)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Настроены блокировки (уровень воды, температура)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Включено логирование всех доз (для анализа)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <template v-slot:prepend>
                  <v-checkbox-btn disabled model-value></v-checkbox-btn>
                </template>
                <v-list-item-title>Готовы наблюдать первые 24 часа и корректировать</v-list-item-title>
              </v-list-item>
            </v-list>

            <v-alert type="success" class="mt-4">
              <strong>Итого:</strong> PID — мощный инструмент автоматизации, но требует понимания и тонкой настройки. 
              Начните с консервативных параметров (малый Kp, Ki) и постепенно увеличивайте для улучшения производительности.
            </v-alert>

            <v-alert type="info" class="mt-2">
              <strong>Настройка в UI:</strong> Перейдите в <router-link to="/settings">Настройки → PID Контроллеры</router-link> 
              для изменения параметров pH и EC в реальном времени.
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Мониторинг -->
        <v-card v-if="activeSection === 'monitoring'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-monitor-dashboard</v-icon>
            Мониторинг системы
          </v-card-title>
          <v-card-text>
            <h3>Просмотр логов</h3>
            <v-code class="my-2">
# Все сервисы
docker-compose logs -f

# Отдельные сервисы
docker logs hydro_backend -f
docker logs hydro_mqtt_listener -f
docker logs hydro_frontend -f
            </v-code>

            <h3 class="mt-4">Проверка ресурсов</h3>
            <v-code class="my-2">
# Использование ресурсов
docker stats

# Статус контейнеров
docker-compose ps
            </v-code>

            <h3 class="mt-4">Проверка подключения узлов</h3>
            <v-list density="compact" class="mt-2">
              <v-list-item prepend-icon="mdi-check-circle">
                <v-list-item-title>Зелёная иконка — узел онлайн</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-alert-circle">
                <v-list-item-title>Красная иконка — узел офлайн более 30 сек</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-information">
                <v-list-item-title>Last Seen — время последнего сигнала</v-list-item-title>
              </v-list-item>
            </v-list>
          </v-card-text>
        </v-card>

        <!-- Настройка -->
        <v-card v-if="activeSection === 'configuration'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-cog</v-icon>
            Настройка системы
          </v-card-title>
          <v-card-text>
            <h3>Переменные окружения</h3>
            <p>Основные параметры настраиваются через <code>docker-compose.yml</code></p>

            <h4 class="mt-4">База данных</h4>
            <v-code class="my-2">
DB_CONNECTION=pgsql
DB_HOST=postgres
DB_PORT=5432
DB_DATABASE=hydro_system
DB_USERNAME=hydro
DB_PASSWORD=hydro_secure_pass_2025
            </v-code>

            <h4 class="mt-4">MQTT</h4>
            <v-code class="my-2">
MQTT_HOST=mosquitto
MQTT_PORT=1883
            </v-code>

            <h4 class="mt-4">Таймауты и лимиты</h4>
            <v-list density="compact" class="mt-2">
              <v-list-item>
                <v-list-item-title><strong>NODE_OFFLINE_TIMEOUT:</strong> 30 сек (таймаут офлайн узла)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>TELEMETRY_RETENTION_DAYS:</strong> 365 дней (хранение телеметрии)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>EVENT_AUTO_RESOLVE_HOURS:</strong> 24 часа (авторезолв событий)</v-list-item-title>
              </v-list-item>
            </v-list>

            <v-alert type="warning" class="mt-4">
              <strong>Важно:</strong> После изменения переменных окружения необходимо перезапустить контейнеры: 
              <code>docker-compose restart</code>
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Гидропоника -->
        <v-card v-if="activeSection === 'hydroponics'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-leaf</v-icon>
            Основы гидропоники
          </v-card-title>
          <v-card-text>
            <h3>Ключевые параметры</h3>
            
            <h4 class="mt-4">📊 pH (кислотность раствора)</h4>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title><strong>Оптимальный диапазон:</strong> 5.5–6.5</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Салаты:</strong> 5.5–6.0</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Томаты, огурцы:</strong> 5.8–6.2</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Зелень:</strong> 6.0–6.5</v-list-item-title>
              </v-list-item>
            </v-list>
            <v-alert type="info" density="compact" class="mt-2">
              pH влияет на доступность питательных веществ. Слишком низкий или высокий pH блокирует усвоение элементов.
            </v-alert>

            <h4 class="mt-4">⚡ EC (электропроводность)</h4>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title><strong>Рассада:</strong> 0.8–1.2 mS/cm</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Салаты:</strong> 1.2–1.8 mS/cm</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Томаты вегетация:</strong> 2.0–2.5 mS/cm</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Томаты плодоношение:</strong> 2.5–3.5 mS/cm</v-list-item-title>
              </v-list-item>
            </v-list>
            <v-alert type="info" density="compact" class="mt-2">
              EC показывает концентрацию питательных веществ. Выше EC = больше удобрений.
            </v-alert>

            <h4 class="mt-4">🌡️ Температура</h4>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title><strong>Раствор:</strong> 18–22°C (оптимально 20°C)</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Воздух день:</strong> 22–26°C</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Воздух ночь:</strong> 18–20°C</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Влажность:</strong> 60–70%</v-list-item-title>
              </v-list-item>
            </v-list>
            <v-alert type="warning" density="compact" class="mt-2">
              Температура раствора выше 25°C снижает содержание кислорода и способствует развитию патогенов.
            </v-alert>

            <h4 class="mt-4">💡 Освещение</h4>
            <v-list density="compact">
              <v-list-item>
                <v-list-item-title><strong>DLI салаты:</strong> 12–17 mol/m²/day</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>DLI томаты:</strong> 20–30 mol/m²/day</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Фотопериод салаты:</strong> 16–18 часов</v-list-item-title>
              </v-list-item>
              <v-list-item>
                <v-list-item-title><strong>Фотопериод томаты:</strong> 16–18 часов</v-list-item-title>
              </v-list-item>
            </v-list>

            <h3 class="mt-6">Типы гидропонных систем</h3>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="primary">mdi-water-outline</v-icon>
                  <strong>NFT (Nutrient Film Technique) — Техника питательного слоя</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Тонкий слой раствора (2–3 мм) постоянно течёт по наклонному каналу. Корни лежат в канале, частично погружены в раствор.</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Насос для циркуляции</li>
                    <li>Каналы NFT (ПВХ трубы или специальные лотки)</li>
                    <li>Резервуар</li>
                    <li>Угол наклона 1:30 – 1:40</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Минимальный расход воды</li>
                    <li>✅ Отличная оксигенация корней</li>
                    <li>✅ Простота контроля pH/EC</li>
                    <li>✅ Легко масштабируется</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Критична остановка насоса (15–30 мин до высыхания корней)</li>
                    <li>❌ Не подходит для крупных растений</li>
                    <li>❌ Требует идеальный уровень наклона</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> салат, шпинат, руккола, базилик, клубника</p>

                  <v-alert type="success" density="compact" class="mt-2">
                    <strong>Совет:</strong> Используйте резервное питание (UPS) для насоса!
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="blue">mdi-waves</v-icon>
                  <strong>DWC (Deep Water Culture) — Глубоководная культура</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Корни полностью погружены в аэрируемый раствор. Растения в сетчатых горшках на плавающей крышке.</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Резервуар (непрозрачный)</li>
                    <li>Мощный воздушный компрессор</li>
                    <li>Распылители (аэростоны)</li>
                    <li>Сетчатые горшки</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Быстрый рост (на 20–30% быстрее почвы)</li>
                    <li>✅ Простая конструкция</li>
                    <li>✅ Стабильность pH/EC (большой объём)</li>
                    <li>✅ Низкая стоимость</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Требует постоянную аэрацию (24/7)</li>
                    <li>❌ Контроль температуры критичен (&lt;25°C)</li>
                    <li>❌ Смена раствора трудоёмка</li>
                    <li>❌ Риск корневой гнили при высокой температуре</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> салат, базилик, мята, шпинат</p>

                  <v-alert type="warning" density="compact" class="mt-2">
                    <strong>Важно:</strong> Температура раствора не должна превышать 22°C! Используйте чиллер в жаркое время.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="green">mdi-water-pump</v-icon>
                  <strong>Капельная система (Drip System)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Раствор подаётся через капельницы точно к корням. Стекает в дренаж или рециркулирует.</p>
                  
                  <p><strong>Типы:</strong></p>
                  <ul>
                    <li><strong>Открытая (run-to-waste):</strong> дренаж сливается, не рециркулирует</li>
                    <li><strong>Закрытая (recirculating):</strong> дренаж собирается и возвращается</li>
                  </ul>

                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Насос с таймером</li>
                    <li>Капельницы (регулируемые/нерегулируемые)</li>
                    <li>Субстрат (кокос, минвата, перлит, керамзит)</li>
                    <li>Дренажная система</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Универсальность (все культуры, любые размеры)</li>
                    <li>✅ Точная дозировка для каждого растения</li>
                    <li>✅ Стабильность при отключении насоса</li>
                    <li>✅ Промышленный стандарт</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Засорение капельниц (соли, биоплёнка)</li>
                    <li>❌ Нужен дренаж и сбор стоков</li>
                    <li>❌ Сложность полной замены субстрата</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> томаты, огурцы, перцы, баклажаны, клубника, розы</p>

                  <v-alert type="info" density="compact" class="mt-2">
                    <strong>Совет:</strong> Используйте фильтр перед капельницами и промывайте систему кислотой (pH 2–3) раз в месяц.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="cyan">mdi-water-sync</v-icon>
                  <strong>Ebb & Flow (Приливы и отливы)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Раствор периодически затапливает корневую зону (прилив), затем сливается (отлив). Контролируется таймером.</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Насос с таймером</li>
                    <li>Лотки/столы для выращивания</li>
                    <li>Резервуар (ниже уровня лотков)</li>
                    <li>Субстрат (керамзит, гравий, кокос)</li>
                    <li>Переливная трубка</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Отличная оксигенация (корни на воздухе между циклами)</li>
                    <li>✅ Гибкость в размещении растений</li>
                    <li>✅ Устойчивость к отключению (несколько часов)</li>
                    <li>✅ Легко автоматизируется</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Сложность настройки таймингов</li>
                    <li>❌ Риск переливов при сбое</li>
                    <li>❌ Больше оборудования</li>
                  </ul>

                  <p><strong>Типичные циклы:</strong></p>
                  <ul>
                    <li>Салат: 15 мин затопление / 45 мин слив, каждые 2–4 часа</li>
                    <li>Томаты: 30 мин затопление / 2 часа слив</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> все культуры, особенно со средними корнями</p>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="purple">mdi-air-filter</v-icon>
                  <strong>Аэропоника (Aeroponics)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Корни висят в воздухе, опрыскиваются мелкодисперсным туманом раствора. Максимальная оксигенация.</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Туманообразующие форсунки (высокое давление) или ультразвуковые распылители</li>
                    <li>Насос высокого давления (для форсунок)</li>
                    <li>Таймер (короткие циклы: 5 сек вкл / 3–5 мин выкл)</li>
                    <li>Герметичная камера для корней</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Максимальная скорость роста (на 30–50% быстрее DWC)</li>
                    <li>✅ Минимальный расход воды/удобрений</li>
                    <li>✅ Легко осматривать корни</li>
                    <li>✅ Нет субстрата</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Критична остановка (корни высыхают за 5–10 мин!)</li>
                    <li>❌ Сложность и высокая стоимость</li>
                    <li>❌ Засорение форсунок</li>
                    <li>❌ Требует точную настройку</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> салат, зелень, рассада (коммерческое производство)</p>

                  <v-alert type="error" density="compact" class="mt-2">
                    <strong>Критично:</strong> Обязателен резервный насос и UPS! Остановка = гибель урожая.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="orange">mdi-image-filter-hdr</v-icon>
                  <strong>Фитильная система (Wick System)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Раствор поднимается к корням по фитилю (капиллярный эффект). Полностью пассивная система без насосов.</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Фитиль (хлопок, нейлон, полипропилен)</li>
                    <li>Субстрат (перлит, вермикулит, кокос)</li>
                    <li>Горшки</li>
                    <li>Резервуар</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Нет электричества</li>
                    <li>✅ Максимальная простота</li>
                    <li>✅ Низкая стоимость</li>
                    <li>✅ Идеально для начинающих</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Медленный рост</li>
                    <li>❌ Не подходит для крупных/влаголюбивых растений</li>
                    <li>❌ Риск засоления субстрата</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> салат (мелколистный), базилик, декоративные растения</p>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="teal">mdi-update</v-icon>
                  <strong>Kratky Method — Метод Кратки</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Принцип работы:</strong> Пассивная DWC. Корни частично в растворе, частично в воздухе. Уровень раствора постепенно снижается, корни растут за ним.</p>
                  
                  <p><strong>Оборудование:</strong></p>
                  <ul>
                    <li>Резервуар</li>
                    <li>Крышка с отверстиями</li>
                    <li>Сетчатые горшки</li>
                    <li>Начальный уровень раствора ~2.5 см от дна горшка</li>
                  </ul>

                  <p><strong>Преимущества:</strong></p>
                  <ul>
                    <li>✅ Нет насосов/компрессоров</li>
                    <li>✅ Абсолютная простота</li>
                    <li>✅ Идеально для одного цикла (салат)</li>
                  </ul>

                  <p><strong>Недостатки:</strong></p>
                  <ul>
                    <li>❌ Не пополняется раствор</li>
                    <li>❌ Только для краткосрочных культур</li>
                    <li>❌ Сложно масштабировать</li>
                  </ul>

                  <p><strong>Идеальные культуры:</strong> салат (30–40 дней), шпинат, пак-чой</p>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">Системы питания растений</h3>
            <v-alert type="info" class="mb-4">
              Питательный раствор заменяет почву и содержит ВСЕ необходимые элементы в доступной форме.
            </v-alert>

            <v-expansion-panels>
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Макроэлементы (NPK + вторичные)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Первичные макроэлементы:</strong></p>
                  <ul>
                    <li><strong>N (Азот):</strong> рост зелёной массы, белки, хлорофилл. Недостаток → светлые листья, замедленный рост.</li>
                    <li><strong>P (Фосфор):</strong> корни, цветение, плодоношение, энергия (ATP). Недостаток → тёмно-зелёные/фиолетовые листья, слабые корни.</li>
                    <li><strong>K (Калий):</strong> иммунитет, водный баланс, качество плодов, фотосинтез. Недостаток → некроз краёв листьев, слабые стебли.</li>
                  </ul>

                  <p><strong>Вторичные макроэлементы:</strong></p>
                  <ul>
                    <li><strong>Ca (Кальций):</strong> клеточные стенки, устойчивость тканей. Недостаток → вершинная гниль томатов, краевой ожог салата.</li>
                    <li><strong>Mg (Магний):</strong> центр молекулы хлорофилла. Недостаток → межжилковый хлороз старых листьев.</li>
                    <li><strong>S (Сера):</strong> белки, ферменты, витамины. Недостаток → хлороз молодых листьев.</li>
                  </ul>

                  <v-alert type="warning" density="compact" class="mt-2">
                    <strong>Важно:</strong> Ca НЕ смешивать с сульфатами и фосфатами в концентрированном виде → выпадет осадок!
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Микроэлементы (Fe, Mn, Zn...)</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p>Нужны в малых количествах, но критически важны:</p>
                  <ul>
                    <li><strong>Fe (Железо):</strong> хлорофилл, дыхание. Недостаток → межжилковый хлороз молодых листьев. Используйте хелатную форму (Fe-EDTA, Fe-DTPA).</li>
                    <li><strong>Mn (Марганец):</strong> фотосинтез, дыхание. Недостаток → межжилковый хлороз, некроз.</li>
                    <li><strong>Zn (Цинк):</strong> ферменты, рост. Недостаток → мелкие листья, укороченные междоузлия.</li>
                    <li><strong>Cu (Медь):</strong> фотосинтез, дыхание. Недостаток → увядание молодых листьев.</li>
                    <li><strong>B (Бор):</strong> клеточные стенки, репродукция. Недостаток → отмирание точки роста.</li>
                    <li><strong>Mo (Молибден):</strong> фиксация азота. Недостаток → хлороз средних листьев.</li>
                    <li><strong>Cl (Хлор):</strong> фотосинтез, водный баланс. Обычно достаточно в воде.</li>
                  </ul>

                  <v-alert type="info" density="compact" class="mt-2">
                    Используйте хелатные формы микроэлементов — они стабильны в широком диапазоне pH (5.0–7.0).
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Типы удобрений для гидропоники</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>1. Двухкомпонентные (A+B):</strong></p>
                  <ul>
                    <li><strong>Компонент A:</strong> Ca(NO₃)₂ (кальциевая селитра), Fe-хелат, нитратный азот</li>
                    <li><strong>Компонент B:</strong> K, Mg, PO₄, SO₄, микроэлементы</li>
                    <li><strong>Зачем разделять?</strong> Ca + PO₄/SO₄ в концентрате образуют нерастворимый осадок CaSO₄, CaPO₄</li>
                    <li><strong>Применение:</strong> разбавить A в 50% воды, затем B в оставшихся 50%, перемешать</li>
                  </ul>
                  <p><strong>Примеры:</strong> Flora Nova A+B, Hydro A+B, MasterBlend + CaNO₃ + MgSO₄</p>

                  <p><strong>2. Трёхкомпонентные (Grow/Micro/Bloom):</strong></p>
                  <ul>
                    <li><strong>Grow:</strong> высокий N для вегетации</li>
                    <li><strong>Micro:</strong> микроэлементы и Ca</li>
                    <li><strong>Bloom:</strong> высокий P и K для цветения/плодоношения</li>
                    <li><strong>Гибкость:</strong> можно менять соотношения по стадиям роста</li>
                  </ul>
                  <p><strong>Примеры:</strong> General Hydroponics Flora Series, Advanced Nutrients</p>

                  <p><strong>3. Однокомпонентные:</strong></p>
                  <ul>
                    <li>Все элементы в одной бутылке</li>
                    <li>Удобны для малых систем и новичков</li>
                    <li>Менее стабильны при хранении (могут выпадать осадки)</li>
                  </ul>

                  <p><strong>4. Порошковые (сухие смеси):</strong></p>
                  <ul>
                    <li><strong>MasterBlend 4-18-38:</strong> базовая смесь</li>
                    <li>+ Кальциевая селитра Ca(NO₃)₂</li>
                    <li>+ Сульфат магния MgSO₄ (английская соль)</li>
                    <li><strong>Экономично:</strong> ~$50 на год для 100 растений</li>
                    <li><strong>Рецепт:</strong> 12г MasterBlend + 12г CaNO₃ + 6г MgSO₄ на 10л воды</li>
                  </ul>

                  <v-alert type="success" density="compact" class="mt-2">
                    <strong>Для начинающих:</strong> рекомендуем готовые двухкомпонентные жидкие удобрения. Для опытных — порошковые смеси (экономия 5–10x).
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Рецепты питательных растворов</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <p><strong>Универсальный рецепт (Hoagland Solution):</strong></p>
                  <v-simple-table density="compact" class="mt-2">
                    <template v-slot:default>
                      <thead>
                        <tr>
                          <th>Компонент</th>
                          <th>Концентрация</th>
                        </tr>
                      </thead>
                      <tbody>
                        <tr><td>Ca(NO₃)₂·4H₂O</td><td>945 мг/л</td></tr>
                        <tr><td>KNO₃</td><td>607 мг/л</td></tr>
                        <tr><td>KH₂PO₄</td><td>136 мг/л</td></tr>
                        <tr><td>MgSO₄·7H₂O</td><td>493 мг/л</td></tr>
                        <tr><td>Fe-EDTA</td><td>26.1 мг/л</td></tr>
                        <tr><td>+ микроэлементы</td><td>см. таблицы</td></tr>
                      </tbody>
                    </template>
                  </v-simple-table>
                  <p class="mt-2"><strong>EC:</strong> ~2.0 mS/cm | <strong>pH:</strong> 5.8–6.2</p>

                  <p class="mt-4"><strong>Упрощённый рецепт (MasterBlend):</strong></p>
                  <ul>
                    <li>12 г MasterBlend 4-18-38</li>
                    <li>12 г Кальциевая селитра Ca(NO₃)₂</li>
                    <li>6 г Сульфат магния MgSO₄·7H₂O (эпсомская соль)</li>
                    <li>Растворить в 10 литрах воды</li>
                  </ul>

                  <p class="mt-4"><strong>Корректировка по стадиям:</strong></p>
                  <ul>
                    <li><strong>Вегетация:</strong> выше N (соотношение N:P:K = 3:1:2)</li>
                    <li><strong>Цветение:</strong> выше P (1:3:2)</li>
                    <li><strong>Плодоношение:</strong> выше K (1:2:3)</li>
                  </ul>

                  <v-alert type="info" density="compact" class="mt-2">
                    <strong>Калькулятор растворов:</strong> используйте бесплатное ПО <strong>HydroBuddy</strong> для точных рецептов под вашу воду и культуру.
                  </v-alert>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h3 class="mt-6">📚 Полезные ресурсы и книги</h3>
            
            <h4 class="mt-4">Книги (русскоязычные):</h4>
            <v-list density="compact">
              <v-list-item prepend-icon="mdi-book">
                <v-list-item-title><strong>"Гидропоника для всех"</strong> — Уильям Тексье (переведена на русский)</v-list-item-title>
                <v-list-item-subtitle>Библия гидропоники. Все системы, рецепты, диагностика проблем</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-book">
                <v-list-item-title><strong>"Гидропоника в домашних условиях"</strong> — А. Вологдин</v-list-item-title>
                <v-list-item-subtitle>Практическое руководство для начинающих</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-book">
                <v-list-item-title><strong>"Выращивание без почвы"</strong> — Алиев Э.А.</v-list-item-title>
                <v-list-item-subtitle>Научный подход, промышленные системы</v-list-item-subtitle>
              </v-list-item>
            </v-list>

            <h4 class="mt-4">Книги (англоязычные):</h4>
            <v-list density="compact">
              <v-list-item prepend-icon="mdi-book-open">
                <v-list-item-title><strong>"How to Hydroponics"</strong> — Keith Roberto</v-list-item-title>
                <v-list-item-subtitle>Классика для начинающих, простым языком</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-book-open">
                <v-list-item-title><strong>"Hydroponic Food Production"</strong> — Howard M. Resh</v-list-item-title>
                <v-list-item-subtitle>Профессиональный уровень, коммерческие теплицы</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-book-open">
                <v-list-item-title><strong>"The Cannabis Grow Bible"</strong> — Greg Green</v-list-item-title>
                <v-list-item-subtitle>Продвинутые техники (применимы к любым культурам)</v-list-item-subtitle>
              </v-list-item>
            </v-list>

            <h4 class="mt-4">Онлайн-ресурсы:</h4>
            <v-list density="compact">
              <v-list-item prepend-icon="mdi-web">
                <v-list-item-title><strong>HydroBuddy</strong> — scienceinhydroponics.com</v-list-item-title>
                <v-list-item-subtitle>Бесплатный калькулятор питательных растворов</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-web">
                <v-list-item-title><strong>GrowWeedEasy.com</strong></v-list-item-title>
                <v-list-item-subtitle>Диагностика проблем с фото, гиды по DWC</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-web">
                <v-list-item-title><strong>Maximum Yield Magazine</strong></v-list-item-title>
                <v-list-item-subtitle>Журнал о гидропонике, новые технологии</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-web">
                <v-list-item-title><strong>forum.growclub.one</strong> (русский)</v-list-item-title>
                <v-list-item-subtitle>Форум гроверов, вопросы/ответы</v-list-item-subtitle>
              </v-list-item>
              <v-list-item prepend-icon="mdi-youtube">
                <v-list-item-title><strong>YouTube:</strong> Jeb Gardener, Epic Gardening</v-list-item-title>
                <v-list-item-subtitle>Видео-гиды по системам</v-list-item-subtitle>
              </v-list-item>
            </v-list>

            <h4 class="mt-4">Калькуляторы и инструменты:</h4>
            <v-list density="compact">
              <v-list-item prepend-icon="mdi-calculator">
                <v-list-item-title><strong>DLI Calculator</strong> — waveformlighting.com/horticulture/daily-light-integral-calculator</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-calculator">
                <v-list-item-title><strong>EC/PPM Converter</strong> — bluelab.com</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-calculator">
                <v-list-item-title><strong>HydroBuddy</strong> — расчёт рецептов под вашу воду</v-list-item-title>
              </v-list-item>
            </v-list>

            <h4 class="mt-4">Производители оборудования:</h4>
            <v-expansion-panels class="mt-2">
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Датчики pH/EC</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ul>
                    <li><strong>Bluelab (NZ):</strong> премиум, промышленный стандарт, пожизненная гарантия на некоторые модели</li>
                    <li><strong>Atlas Scientific (USA):</strong> научные датчики, I2C/UART интерфейс для автоматизации</li>
                    <li><strong>Milwaukee (USA):</strong> средний сегмент, хорошее соотношение цена/качество</li>
                    <li><strong>Hanna Instruments (USA):</strong> широкий ассортимент</li>
                    <li><strong>Апера (Китай):</strong> бюджетные, требуют частой калибровки</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Контроллеры и автоматизация</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ul>
                    <li><strong>Grolab (Канада):</strong> полнофункциональные контроллеры, мобильное приложение</li>
                    <li><strong>Bluelab Pro Controller:</strong> pH + EC + температура, автодозирование</li>
                    <li><strong>AutoPilot (США):</strong> контроллеры CO₂, климата</li>
                    <li><strong>Trolmaster (Китай):</strong> модульные системы</li>
                    <li><strong>DIY Arduino/ESP32:</strong> ваш проект Mesh Hydro! 🚀</li>
                  </ul>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <strong>Освещение LED</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ul>
                    <li><strong>Fluence (США):</strong> топ для коммерческих ферм, спектр PhysioSpec</li>
                    <li><strong>HLG (Horticulture Lighting Group):</strong> quantum boards, высокая эффективность</li>
                    <li><strong>Spider Farmer:</strong> популярные, доступная цена</li>
                    <li><strong>Mars Hydro:</strong> бюджетные LED панели</li>
                    <li><strong>Samsung LM301B/H диоды:</strong> золотой стандарт эффективности</li>
                  </ul>
                  <p class="mt-2"><strong>Эффективность:</strong> 2.5–3.0 μmol/J (топовые модели)</p>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>

            <h4 class="mt-4">Telegram-каналы и сообщества:</h4>
            <v-list density="compact">
              <v-list-item prepend-icon="mdi-send">
                <v-list-item-title><strong>@hydroponics_ru</strong> — русскоязычное сообщество</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-send">
                <v-list-item-title><strong>@growdiariesofficial</strong> — дневники урожаев</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-reddit">
                <v-list-item-title><strong>r/Hydroponics</strong> — Reddit сообщество (350k+ участников)</v-list-item-title>
              </v-list-item>
            </v-list>

            <v-alert type="success" class="mt-4">
              <strong>💡 Совет:</strong> Начните с простой системы (DWC или Kratky), одной культуры (салат) и готовых удобрений. 
              После успешного первого урожая переходите к более сложным системам и культурам.
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Калибровка -->
        <v-card v-if="activeSection === 'calibration'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-tune</v-icon>
            Калибровка датчиков
          </v-card-title>
          <v-card-text>
            <h3>📐 Калибровка pH</h3>
            <v-stepper alt-labels>
              <v-stepper-header>
                <v-stepper-item title="Подготовка" value="1"></v-stepper-item>
                <v-divider></v-divider>
                <v-stepper-item title="pH 7.0" value="2"></v-stepper-item>
                <v-divider></v-divider>
                <v-stepper-item title="pH 4.0" value="3"></v-stepper-item>
              </v-stepper-header>
            </v-stepper>
            <v-list density="compact" class="mt-4">
              <v-list-item prepend-icon="mdi-numeric-1-circle">
                <v-list-item-title>Промойте датчик дистиллированной водой</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-2-circle">
                <v-list-item-title>Погрузите в буфер pH 7.0, дождитесь стабилизации</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-3-circle">
                <v-list-item-title>В интерфейсе: Узлы → pH/EC Node → Калибровка pH 7.0</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-4-circle">
                <v-list-item-title>Промойте, погрузите в буфер pH 4.0</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-5-circle">
                <v-list-item-title>Калибровка pH 4.0</v-list-item-title>
              </v-list-item>
            </v-list>
            <v-alert type="info" class="mt-4">
              <strong>Периодичность:</strong> Калибруйте pH датчик каждые 2 недели или при отклонениях показаний.
            </v-alert>

            <h3 class="mt-6">⚡ Калибровка EC</h3>
            <v-list density="compact" class="mt-2">
              <v-list-item prepend-icon="mdi-numeric-1-circle">
                <v-list-item-title>Промойте датчик дистиллированной водой</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-2-circle">
                <v-list-item-title>Погрузите в калибровочный раствор (обычно 1.413 mS/cm)</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-3-circle">
                <v-list-item-title>Дождитесь стабилизации температуры</v-list-item-title>
              </v-list-item>
              <v-list-item prepend-icon="mdi-numeric-4-circle">
                <v-list-item-title>В интерфейсе: Узлы → pH/EC Node → Калибровка EC</v-list-item-title>
              </v-list-item>
            </v-list>
            <v-alert type="info" class="mt-4">
              <strong>Периодичность:</strong> Калибруйте EC датчик каждый месяц.
            </v-alert>
          </v-card-text>
        </v-card>

        <!-- Устранение неполадок -->
        <v-card v-if="activeSection === 'troubleshooting'" class="mb-4">
          <v-card-title class="d-flex align-center">
            <v-icon class="mr-2">mdi-tools</v-icon>
            Устранение неполадок
          </v-card-title>
          <v-card-text>
            <v-expansion-panels>
              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="error">mdi-alert-circle</v-icon>
                  <strong>Узел не подключается</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ol>
                    <li>Проверьте питание узла</li>
                    <li>Убедитесь, что узел в той же WiFi сети</li>
                    <li>Проверьте MQTT брокер: <code>docker logs hydro_mosquitto</code></li>
                    <li>Проверьте конфигурацию WiFi в прошивке узла</li>
                    <li>Перезагрузите узел</li>
                  </ol>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="warning">mdi-chart-line-variant</v-icon>
                  <strong>Неточные показания датчиков</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ol>
                    <li>Выполните калибровку датчика</li>
                    <li>Проверьте чистоту датчика (налёт, водоросли)</li>
                    <li>Убедитесь в правильной температурной компенсации</li>
                    <li>Проверьте срок службы датчика (pH: 1–2 года, EC: 2–3 года)</li>
                  </ol>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="info">mdi-database-alert</v-icon>
                  <strong>Ошибки базы данных</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <v-code>
# Проверка логов БД
docker logs hydro_postgres

# Пересоздание БД (УДАЛИТ ВСЕ ДАННЫЕ!)
docker exec hydro_backend php artisan migrate:fresh --seed

# Бэкап БД
docker exec hydro_postgres pg_dump -U hydro hydro_system > backup.sql
                  </v-code>
                </v-expansion-panel-text>
              </v-expansion-panel>

              <v-expansion-panel>
                <v-expansion-panel-title>
                  <v-icon class="mr-2" color="warning">mdi-wifi-off</v-icon>
                  <strong>WebSocket не подключается</strong>
                </v-expansion-panel-title>
                <v-expansion-panel-text>
                  <ol>
                    <li>Проверьте, что Reverb запущен: <code>docker logs hydro_backend | grep reverb</code></li>
                    <li>Откройте консоль браузера (F12) и проверьте ошибки WebSocket</li>
                    <li>Система автоматически переключится на polling режим при недоступности WS</li>
                    <li>Убедитесь, что порт 6001 не заблокирован файрволом</li>
                  </ol>
                </v-expansion-panel-text>
              </v-expansion-panel>
            </v-expansion-panels>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>

<script setup>
import { ref } from 'vue'

const activeSection = ref('quick-start')

const sections = [
  { id: 'quick-start', title: 'Быстрый старт', icon: 'mdi-rocket-launch' },
  { id: 'architecture', title: 'Архитектура', icon: 'mdi-sitemap' },
  { id: 'pid-system', title: 'PID Контроллеры', icon: 'mdi-chart-bell-curve' },
  { id: 'monitoring', title: 'Мониторинг', icon: 'mdi-monitor-dashboard' },
  { id: 'configuration', title: 'Настройка', icon: 'mdi-cog' },
  { id: 'hydroponics', title: 'Гидропоника', icon: 'mdi-leaf' },
  { id: 'calibration', title: 'Калибровка', icon: 'mdi-tune' },
  { id: 'troubleshooting', title: 'Решение проблем', icon: 'mdi-tools' },
]
</script>

<style scoped>
h3 {
  color: rgb(var(--v-theme-primary));
  margin-top: 1rem;
  margin-bottom: 0.5rem;
}

h4 {
  margin-top: 1rem;
  margin-bottom: 0.5rem;
}

.v-code {
  display: block;
  background-color: rgba(0, 0, 0, 0.05);
  padding: 1rem;
  border-radius: 4px;
  font-family: 'Courier New', monospace;
  font-size: 0.875rem;
  overflow-x: auto;
  white-space: pre;
}

code {
  background-color: rgba(0, 0, 0, 0.05);
  padding: 0.2rem 0.4rem;
  border-radius: 3px;
  font-family: 'Courier New', monospace;
  font-size: 0.875rem;
}
</style>

