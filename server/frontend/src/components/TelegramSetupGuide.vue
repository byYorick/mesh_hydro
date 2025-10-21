<template>
  <v-card>
    <v-card-title class="bg-telegram">
      <v-icon icon="mdi-telegram" class="mr-2"></v-icon>
      Настройка Telegram уведомлений
      
      <v-spacer></v-spacer>
      
      <v-chip :color="isConfigured ? 'success' : 'warning'" variant="elevated">
        <v-icon :icon="isConfigured ? 'mdi-check-circle' : 'mdi-alert-circle'" start></v-icon>
        {{ isConfigured ? 'Настроено' : 'Требует настройки' }}
      </v-chip>
    </v-card-title>

    <v-card-text class="pa-0">
      <v-stepper v-model="currentStep" alt-labels>
        <v-stepper-header>
          <v-stepper-item
            :complete="currentStep > 1"
            :value="1"
            title="Создать бота"
            icon="mdi-robot"
          ></v-stepper-item>

          <v-divider></v-divider>

          <v-stepper-item
            :complete="currentStep > 2"
            :value="2"
            title="Получить токен"
            icon="mdi-key"
          ></v-stepper-item>

          <v-divider></v-divider>

          <v-stepper-item
            :complete="currentStep > 3"
            :value="3"
            title="Chat ID"
            icon="mdi-account"
          ></v-stepper-item>

          <v-divider></v-divider>

          <v-stepper-item
            :complete="currentStep > 4"
            :value="4"
            title="Настроить"
            icon="mdi-cog"
          ></v-stepper-item>

          <v-divider></v-divider>

          <v-stepper-item
            :value="5"
            title="Готово"
            icon="mdi-check-bold"
          ></v-stepper-item>
        </v-stepper-header>

        <v-stepper-window>
          <!-- Шаг 1: Создать бота -->
          <v-stepper-window-item :value="1">
            <v-card flat>
              <v-card-text>
                <div class="text-h6 mb-4">Шаг 1: Создание Telegram бота</div>

                <v-list lines="two">
                  <v-list-item>
                    <template #prepend>
                      <v-avatar color="blue">1</v-avatar>
                    </template>
                    <v-list-item-title>Откройте Telegram</v-list-item-title>
                    <v-list-item-subtitle>На телефоне или в десктоп версии</v-list-item-subtitle>
                  </v-list-item>

                  <v-list-item>
                    <template #prepend>
                      <v-avatar color="blue">2</v-avatar>
                    </template>
                    <v-list-item-title>Найдите бота @BotFather</v-list-item-title>
                    <v-list-item-subtitle>
                      Официальный бот для создания ботов
                      <v-btn
                        size="x-small"
                        variant="tonal"
                        class="ml-2"
                        @click="copyToClipboard('@BotFather')"
                      >
                        Копировать
                      </v-btn>
                    </v-list-item-subtitle>
                  </v-list-item>

                  <v-list-item>
                    <template #prepend>
                      <v-avatar color="blue">3</v-avatar>
                    </template>
                    <v-list-item-title>Отправьте команду /newbot</v-list-item-title>
                    <v-list-item-subtitle>
                      Начните диалог с BotFather
                    </v-list-item-subtitle>
                  </v-list-item>

                  <v-list-item>
                    <template #prepend>
                      <v-avatar color="blue">4</v-avatar>
                    </template>
                    <v-list-item-title>Введите название бота</v-list-item-title>
                    <v-list-item-subtitle>
                      Например: <code>Hydro Mesh Bot</code>
                    </v-list-item-subtitle>
                  </v-list-item>

                  <v-list-item>
                    <template #prepend>
                      <v-avatar color="blue">5</v-avatar>
                    </template>
                    <v-list-item-title>Введите username бота</v-list-item-title>
                    <v-list-item-subtitle>
                      Например: <code>hydro_mesh_bot</code> (должен заканчиваться на _bot)
                    </v-list-item-subtitle>
                  </v-list-item>
                </v-list>

                <v-alert type="success" variant="tonal" class="mt-4">
                  <v-icon icon="mdi-lightbulb-on" class="mr-2"></v-icon>
                  После создания бота вы получите токен. Сохраните его для следующего шага!
                </v-alert>
              </v-card-text>

              <v-card-actions>
                <v-spacer></v-spacer>
                <v-btn color="primary" @click="currentStep = 2">
                  Далее
                  <v-icon end>mdi-chevron-right</v-icon>
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-stepper-window-item>

          <!-- Шаг 2: Получить токен -->
          <v-stepper-window-item :value="2">
            <v-card flat>
              <v-card-text>
                <div class="text-h6 mb-4">Шаг 2: Получение токена бота</div>

                <v-alert type="info" variant="tonal" class="mb-4">
                  BotFather должен отправить вам сообщение с токеном
                </v-alert>

                <div class="mb-4">
                  <div class="text-subtitle-2 mb-2">Токен выглядит так:</div>
                  <v-sheet color="grey-lighten-4" class="pa-3 rounded">
                    <code>123456789:ABCdefGHIjklMNOpqrsTUVwxyz</code>
                  </v-sheet>
                </div>

                <v-text-field
                  v-model="botToken"
                  label="Введите токен бота"
                  placeholder="123456789:ABCdef..."
                  variant="outlined"
                  persistent-hint
                  hint="Скопируйте токен из сообщения BotFather"
                >
                  <template #append-inner>
                    <v-icon
                      v-if="botToken"
                      icon="mdi-check-circle"
                      color="success"
                    ></v-icon>
                  </template>
                </v-text-field>

                <v-alert type="warning" variant="tonal" class="mt-4">
                  <strong>⚠️ Важно:</strong> Не показывайте токен посторонним людям!
                </v-alert>
              </v-card-text>

              <v-card-actions>
                <v-btn @click="currentStep = 1">
                  <v-icon start>mdi-chevron-left</v-icon>
                  Назад
                </v-btn>
                <v-spacer></v-spacer>
                <v-btn
                  color="primary"
                  :disabled="!botToken"
                  @click="currentStep = 3"
                >
                  Далее
                  <v-icon end>mdi-chevron-right</v-icon>
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-stepper-window-item>

          <!-- Шаг 3: Получить Chat ID -->
          <v-stepper-window-item :value="3">
            <v-card flat>
              <v-card-text>
                <div class="text-h6 mb-4">Шаг 3: Получение Chat ID</div>

                <v-tabs v-model="chatIdMethod" color="primary" class="mb-4">
                  <v-tab value="simple">Простой способ</v-tab>
                  <v-tab value="manual">Ручной способ</v-tab>
                </v-tabs>

                <v-window v-model="chatIdMethod">
                  <!-- Простой способ -->
                  <v-window-item value="simple">
                    <v-list>
                      <v-list-item>
                        <template #prepend>
                          <v-avatar color="green">1</v-avatar>
                        </template>
                        <v-list-item-title>Найдите бота @userinfobot</v-list-item-title>
                        <v-list-item-subtitle>
                          <v-btn
                            size="small"
                            variant="tonal"
                            class="mt-1"
                            @click="copyToClipboard('@userinfobot')"
                          >
                            Копировать @userinfobot
                          </v-btn>
                        </v-list-item-subtitle>
                      </v-list-item>

                      <v-list-item>
                        <template #prepend>
                          <v-avatar color="green">2</v-avatar>
                        </template>
                        <v-list-item-title>Отправьте ему /start</v-list-item-title>
                      </v-list-item>

                      <v-list-item>
                        <template #prepend>
                          <v-avatar color="green">3</v-avatar>
                        </template>
                        <v-list-item-title>Скопируйте ваш ID</v-list-item-title>
                        <v-list-item-subtitle>
                          Бот ответит: <code>Id: 987654321</code>
                        </v-list-item-subtitle>
                      </v-list-item>
                    </v-list>
                  </v-window-item>

                  <!-- Ручной способ -->
                  <v-window-item value="manual">
                    <v-list>
                      <v-list-item>
                        <template #prepend>
                          <v-avatar color="orange">1</v-avatar>
                        </template>
                        <v-list-item-title>Отправьте /start вашему боту</v-list-item-title>
                      </v-list-item>

                      <v-list-item>
                        <template #prepend>
                          <v-avatar color="orange">2</v-avatar>
                        </template>
                        <v-list-item-title>Откройте ссылку в браузере:</v-list-item-title>
                        <v-list-item-subtitle class="mt-2">
                          <v-sheet color="grey-lighten-4" class="pa-2 rounded">
                            <code style="font-size: 0.8em; word-break: break-all;">
                              https://api.telegram.org/bot{{ botToken || 'YOUR_TOKEN' }}/getUpdates
                            </code>
                          </v-sheet>
                          <v-btn
                            v-if="botToken"
                            size="small"
                            variant="tonal"
                            class="mt-2"
                            @click="openGetUpdatesUrl"
                          >
                            Открыть в браузере
                          </v-btn>
                        </v-list-item-subtitle>
                      </v-list-item>

                      <v-list-item>
                        <template #prepend>
                          <v-avatar color="orange">3</v-avatar>
                        </template>
                        <v-list-item-title>Найдите в JSON: "chat":{"id":...</v-list-item-title>
                      </v-list-item>
                    </v-list>
                  </v-window-item>
                </v-window>

                <v-text-field
                  v-model="chatId"
                  label="Введите Chat ID"
                  placeholder="987654321"
                  variant="outlined"
                  type="number"
                  class="mt-4"
                  persistent-hint
                  hint="Это ваш ID пользователя в Telegram"
                >
                  <template #append-inner>
                    <v-icon
                      v-if="chatId"
                      icon="mdi-check-circle"
                      color="success"
                    ></v-icon>
                  </template>
                </v-text-field>
              </v-card-text>

              <v-card-actions>
                <v-btn @click="currentStep = 2">
                  <v-icon start>mdi-chevron-left</v-icon>
                  Назад
                </v-btn>
                <v-spacer></v-spacer>
                <v-btn
                  color="primary"
                  :disabled="!chatId"
                  @click="currentStep = 4"
                >
                  Далее
                  <v-icon end>mdi-chevron-right</v-icon>
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-stepper-window-item>

          <!-- Шаг 4: Настройка -->
          <v-stepper-window-item :value="4">
            <v-card flat>
              <v-card-text>
                <div class="text-h6 mb-4">Шаг 4: Настройка системы</div>

                <v-alert type="info" variant="tonal" class="mb-4">
                  Скопируйте эти настройки и добавьте в файл <code>.env</code> на сервере
                </v-alert>

                <v-sheet color="grey-darken-4" class="pa-4 rounded">
                  <pre style="color: white; margin: 0;">TELEGRAM_ENABLED=true
TELEGRAM_BOT_TOKEN={{ botToken || 'YOUR_BOT_TOKEN' }}
TELEGRAM_CHAT_ID={{ chatId || 'YOUR_CHAT_ID' }}
TELEGRAM_NOTIFY_WARNINGS=true
TELEGRAM_NOTIFY_INFO=false</pre>
                </v-sheet>

                <v-btn
                  color="primary"
                  variant="tonal"
                  prepend-icon="mdi-content-copy"
                  class="mt-3"
                  @click="copyEnvSettings"
                >
                  Копировать настройки
                </v-btn>

                <v-divider class="my-6"></v-divider>

                <div class="text-subtitle-2 mb-3">Дополнительные настройки:</div>

                <v-switch
                  v-model="notifyCritical"
                  label="Критические события"
                  color="error"
                  hide-details
                  class="mb-2"
                ></v-switch>

                <v-switch
                  v-model="notifyWarnings"
                  label="Предупреждения"
                  color="warning"
                  hide-details
                  class="mb-2"
                ></v-switch>

                <v-switch
                  v-model="notifyInfo"
                  label="Информационные сообщения"
                  color="info"
                  hide-details
                  class="mb-4"
                ></v-switch>

                <v-alert type="warning" variant="tonal">
                  <strong>⚠️ После изменения .env файла:</strong>
                  <ul class="mt-2">
                    <li>Перезапустите backend: <code>docker-compose restart backend</code></li>
                    <li>Или очистите кэш: <code>php artisan config:clear</code></li>
                  </ul>
                </v-alert>
              </v-card-text>

              <v-card-actions>
                <v-btn @click="currentStep = 3">
                  <v-icon start>mdi-chevron-left</v-icon>
                  Назад
                </v-btn>
                <v-spacer></v-spacer>
                <v-btn color="primary" @click="currentStep = 5">
                  Далее
                  <v-icon end>mdi-chevron-right</v-icon>
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-stepper-window-item>

          <!-- Шаг 5: Готово -->
          <v-stepper-window-item :value="5">
            <v-card flat>
              <v-card-text>
                <div class="text-center py-8">
                  <v-icon icon="mdi-check-circle" color="success" size="80" class="mb-4"></v-icon>
                  
                  <div class="text-h5 mb-4">Настройка завершена! 🎉</div>

                  <v-alert type="success" variant="tonal" class="mb-4">
                    Теперь система будет отправлять уведомления в Telegram при:
                    <ul class="mt-2 text-left">
                      <li>🔴 Критичных событиях (pH/EC вне диапазона)</li>
                      <li>🟡 Предупреждениях (если включено)</li>
                      <li>✅ Калибровке насосов (если включено)</li>
                      <li>⚠️ Когда нода уходит в offline</li>
                    </ul>
                  </v-alert>

                  <div class="text-subtitle-1 mb-4">Что дальше?</div>

                  <v-list lines="two">
                    <v-list-item>
                      <template #prepend>
                        <v-icon icon="mdi-test-tube" color="primary"></v-icon>
                      </template>
                      <v-list-item-title>Протестируйте уведомления</v-list-item-title>
                      <v-list-item-subtitle>
                        <v-btn
                          size="small"
                          variant="outlined"
                          class="mt-2"
                          :loading="testing"
                          @click="sendTestMessage"
                        >
                          Отправить тестовое сообщение
                        </v-btn>
                      </v-list-item-subtitle>
                    </v-list-item>

                    <v-list-item>
                      <template #prepend>
                        <v-icon icon="mdi-book-open" color="primary"></v-icon>
                      </template>
                      <v-list-item-title>Прочитайте документацию</v-list-item-title>
                      <v-list-item-subtitle>
                        Файл: <code>КАК_НАСТРОИТЬ_TELEGRAM.md</code>
                      </v-list-item-subtitle>
                    </v-list-item>

                    <v-list-item>
                      <template #prepend>
                        <v-icon icon="mdi-restart" color="primary"></v-icon>
                      </template>
                      <v-list-item-title>Перезапустите сервисы</v-list-item-title>
                      <v-list-item-subtitle>
                        Чтобы настройки вступили в силу
                      </v-list-item-subtitle>
                    </v-list-item>
                  </v-list>
                </div>
              </v-card-text>

              <v-card-actions>
                <v-btn @click="currentStep = 4">
                  <v-icon start>mdi-chevron-left</v-icon>
                  Назад
                </v-btn>
                <v-spacer></v-spacer>
                <v-btn color="success" variant="elevated" @click="closeDialog">
                  Завершить
                  <v-icon end>mdi-check</v-icon>
                </v-btn>
              </v-card-actions>
            </v-card>
          </v-stepper-window-item>
        </v-stepper-window>
      </v-stepper>
    </v-card-text>

    <!-- Дополнительная информация -->
    <v-expansion-panels class="mt-4">
      <v-expansion-panel>
        <v-expansion-panel-title>
          <v-icon icon="mdi-help-circle" class="mr-2"></v-icon>
          Часто задаваемые вопросы
        </v-expansion-panel-title>
        <v-expansion-panel-text>
          <v-list>
            <v-list-item>
              <v-list-item-title class="font-weight-bold">
                Как изменить настройки уведомлений?
              </v-list-item-title>
              <v-list-item-subtitle>
                Измените параметры TELEGRAM_NOTIFY_* в .env файле и перезапустите backend
              </v-list-item-subtitle>
            </v-list-item>

            <v-list-item>
              <v-list-item-title class="font-weight-bold">
                Можно ли отправлять в несколько чатов?
              </v-list-item-title>
              <v-list-item-subtitle>
                Пока нет, но можно расширить TelegramService для поддержки нескольких chat_id
              </v-list-item-subtitle>
            </v-list-item>

            <v-list-item>
              <v-list-item-title class="font-weight-bold">
                Что делать если сообщения не приходят?
              </v-list-item-title>
              <v-list-item-subtitle>
                Проверьте: 1) Токен и Chat ID правильные, 2) Отправили /start боту, 3) TELEGRAM_ENABLED=true, 4) Перезапустили backend
              </v-list-item-subtitle>
            </v-list-item>

            <v-list-item>
              <v-list-item-title class="font-weight-bold">
                Как отключить уведомления?
              </v-list-item-title>
              <v-list-item-subtitle>
                Установите TELEGRAM_ENABLED=false в .env и перезапустите backend
              </v-list-item-subtitle>
            </v-list-item>
          </v-list>
        </v-expansion-panel-text>
      </v-expansion-panel>

      <v-expansion-panel>
        <v-expansion-panel-title>
          <v-icon icon="mdi-script-text" class="mr-2"></v-icon>
          Пример сообщения
        </v-expansion-panel-title>
        <v-expansion-panel-text>
          <v-sheet color="grey-lighten-4" class="pa-4 rounded">
            <div style="font-family: monospace; white-space: pre-line;">
🔴 <strong>[CRITICAL]</strong> 19:30

<strong>Узел:</strong> ph_001
<strong>Событие:</strong> pH critically low
<strong>Время:</strong> 21.10.2025 19:30:15

<strong>Данные:</strong>
• ph: 4.2
• ph_target: 6.0
• ph_min: 5.5
• ph_max: 6.5
            </div>
          </v-sheet>
        </v-expansion-panel-text>
      </v-expansion-panel>
    </v-expansion-panels>
  </v-card>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useToast } from 'vue-toastification'
import axios from 'axios'

const toast = useToast()

const currentStep = ref(1)
const botToken = ref('')
const chatId = ref('')
const chatIdMethod = ref('simple')
const notifyWarnings = ref(true)
const notifyInfo = ref(false)
const notifyCritical = ref(true)
const enabled = ref(false)
const loading = ref(false)
const testing = ref(false)
const hasToken = ref(false)
const tokenMasked = ref('')
const availableChats = ref([])
const loadingChats = ref(false)

// Проверка настройки
const isConfigured = computed(() => {
  return (hasToken.value || botToken.value) && chatId.value && enabled.value
})

// Загрузка настроек из БД
const loadSettings = async () => {
  loading.value = true
  try {
    const response = await axios.get('/api/settings/telegram')
    if (response.data.success) {
      const settings = response.data.telegram
      enabled.value = settings.enabled || false
      chatId.value = settings.chat_id || ''
      notifyCritical.value = settings.notify_critical !== false
      notifyWarnings.value = settings.notify_warnings !== false
      notifyInfo.value = settings.notify_info || false
      hasToken.value = settings.has_token || false
      tokenMasked.value = settings.bot_token_masked || ''
      
      // Если все настроено, перейти на последний шаг
      if (isConfigured.value) {
        currentStep.value = 5
      }
    }
  } catch (error) {
    console.error('Failed to load Telegram settings:', error)
  } finally {
    loading.value = false
  }
}

// Сохранить настройки в БД
const saveSettings = async () => {
  loading.value = true
  try {
    const data = {
      enabled: enabled.value,
      notify_critical: notifyCritical.value,
      notify_warnings: notifyWarnings.value,
      notify_info: notifyInfo.value,
    }

    // Добавить токен только если введен
    if (botToken.value) {
      data.bot_token = botToken.value
    }

    // Добавить chat_id только если введен
    if (chatId.value) {
      data.chat_id = chatId.value
    }

    const response = await axios.post('/api/settings/telegram', data)
    
    if (response.data.success) {
      toast.success('✅ Настройки сохранены в БД!')
      await loadSettings()
      return true
    }
  } catch (error) {
    const errorMsg = error.response?.data?.error || error.message || 'Ошибка сохранения'
    toast.error(errorMsg)
    return false
  } finally {
    loading.value = false
  }
}

// Получить Chat ID автоматически
const getChatIdAuto = async () => {
  // Если токен введен но не сохранен - сохранить сначала
  if (botToken.value && !hasToken.value) {
    const saved = await saveSettings()
    if (!saved) return
  }

  if (!hasToken.value && !botToken.value) {
    toast.warning('Сначала введите и сохраните токен бота')
    return
  }

  loadingChats.value = true
  try {
    const response = await axios.get('/api/settings/telegram/chat-id')
    if (response.data.success && response.data.chats.length > 0) {
      availableChats.value = response.data.chats
      toast.success(`Найдено ${response.data.count} чат(ов)`)
    } else {
      toast.warning(response.data.error || 'Нет сообщений от бота. Отправьте боту /start')
    }
  } catch (error) {
    toast.error(error.response?.data?.error || 'Ошибка получения Chat ID')
  } finally {
    loadingChats.value = false
  }
}

// Выбрать Chat ID из списка
const selectChatId = (chat) => {
  chatId.value = chat.chat_id.toString()
  toast.success(`Chat ID выбран: ${chat.title}`)
}

// Отправить тестовое сообщение
const sendTestMessage = async () => {
  // Сначала сохраним настройки
  const saved = await saveSettings()
  if (!saved) return

  testing.value = true
  try {
    const response = await axios.post('/api/settings/telegram/test')
    
    if (response.data.success) {
      toast.success('🎉 Тестовое сообщение отправлено! Проверьте Telegram.')
    } else {
      toast.error(response.data.error || 'Ошибка отправки')
    }
  } catch (error) {
    toast.error(error.response?.data?.error || 'Ошибка теста')
  } finally {
    testing.value = false
  }
}

// Завершить настройку
const closeDialog = async () => {
  enabled.value = true
  const saved = await saveSettings()
  
  if (saved) {
    toast.success('✅ Telegram уведомления активированы!')
  }
}

// Копировать в буфер
const copyToClipboard = (text) => {
  navigator.clipboard.writeText(text)
  toast.success(`Скопировано: ${text}`)
}

// Копировать настройки .env
const copyEnvSettings = () => {
  const envText = `TELEGRAM_ENABLED=${enabled.value}
TELEGRAM_BOT_TOKEN=${botToken.value || '[см. в БД]'}
TELEGRAM_CHAT_ID=${chatId.value}
TELEGRAM_NOTIFY_CRITICAL=${notifyCritical.value}
TELEGRAM_NOTIFY_WARNINGS=${notifyWarnings.value}
TELEGRAM_NOTIFY_INFO=${notifyInfo.value}`

  navigator.clipboard.writeText(envText)
  toast.success('Настройки скопированы в буфер обмена')
}

// Открыть getUpdates URL
const openGetUpdatesUrl = () => {
  const url = `https://api.telegram.org/bot${botToken.value}/getUpdates`
  window.open(url, '_blank')
}

// При монтировании загрузить настройки
onMounted(() => {
  loadSettings()
})
</script>

<style scoped>
.bg-telegram {
  background: linear-gradient(135deg, #0088cc 0%, #00a8e8 100%);
  color: white;
}

code {
  background: rgba(0, 0, 0, 0.05);
  padding: 2px 6px;
  border-radius: 4px;
  font-family: monospace;
}
</style>

