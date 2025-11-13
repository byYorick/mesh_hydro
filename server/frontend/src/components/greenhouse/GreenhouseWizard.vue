<template>
  <v-dialog
    v-model="internalModel"
    max-width="960"
    persistent
  >
    <v-card>
      <v-toolbar color="primary" dark>
        <v-toolbar-title>
          <v-icon class="mr-3" icon="mdi-leaf-circle"></v-icon>
          Добавление теплицы
        </v-toolbar-title>
        <v-spacer></v-spacer>
        <v-btn icon="mdi-close" variant="text" @click="handleCancel"></v-btn>
      </v-toolbar>

      <v-card-text class="pt-6">
        <v-stepper v-model="step" alt-labels flat>
          <v-stepper-header>
            <v-stepper-item
              v-for="(label, index) in stepLabels"
              :key="label"
              :value="index + 1"
              :complete="step > index + 1"
              :title="label"
            ></v-stepper-item>
          </v-stepper-header>

          <v-stepper-window>
            <v-stepper-window-item :value="1">
              <v-card flat>
                <v-card-text>
                  <v-form ref="basicFormRef">
                    <v-row dense>
                      <v-col cols="12" md="6">
                        <v-text-field
                          v-model="basicForm.name"
                          label="Название теплицы *"
                          variant="outlined"
                          prepend-inner-icon="mdi-format-title"
                          :rules="[rules.required]"
                        ></v-text-field>
                      </v-col>

                      <v-col cols="12" md="3">
                        <v-text-field
                          v-model="basicForm.code"
                          label="Код"
                          variant="outlined"
                          prepend-inner-icon="mdi-pound"
                          @focus="codeTouched = true"
                        ></v-text-field>
                      </v-col>

                      <v-col cols="12" md="3">
                        <v-select
                          v-model="basicForm.status"
                          :items="statusOptions"
                          item-title="label"
                          item-value="value"
                          label="Статус"
                          variant="outlined"
                          prepend-inner-icon="mdi-lightbulb-on"
                        ></v-select>
                      </v-col>

                      <v-col cols="12">
                        <v-textarea
                          v-model="basicForm.description"
                          label="Описание *"
                          variant="outlined"
                          rows="3"
                          prepend-inner-icon="mdi-text"
                          :rules="[rules.required]"
                        ></v-textarea>
                      </v-col>

                      <v-col cols="12">
                        <v-card variant="outlined">
                          <v-card-text>
                            <div class="d-flex flex-column flex-md-row align-md-center justify-space-between gap-4 mb-4">
                              <div>
                                <h3 class="text-subtitle-1 mb-1">Профили климата</h3>
                                <p class="text-body-2 text-medium-emphasis mb-0">
                                  Добавьте один или несколько профилей с целевыми параметрами. Они используются при настройке зон и сценариев автоматики.
                                </p>
                              </div>
                            </div>

                            <v-row dense class="align-center mb-4 ga-4">
                              <v-col cols="12" md="6">
                                <v-select
                                  v-model="selectedExistingProfile"
                                  :items="existingClimateProfileOptions"
                                  item-title="title"
                                  item-value="value"
                                  label="Выберите существующий профиль"
                                  variant="outlined"
                                  prepend-inner-icon="mdi-archive-search"
                                  clearable
                                ></v-select>
                              </v-col>
                              <v-col cols="12" md="6" class="d-flex flex-wrap ga-2">
                                <v-btn
                                  color="secondary"
                                  variant="tonal"
                                  prepend-icon="mdi-pencil"
                                  @click="startEditExistingProfile"
                                >
                                  Редактировать профиль
                                </v-btn>
                                <v-btn
                                  color="primary"
                                  variant="text"
                                  prepend-icon="mdi-plus-circle"
                                  @click="startNewClimateProfile"
                                >
                                  Новый профиль
                                </v-btn>
                              </v-col>
                            </v-row>

                            <v-alert
                              v-if="editingProfileLabel"
                              type="info"
                              variant="tonal"
                              border="start"
                              class="mb-4"
                              icon="mdi-pencil"
                            >
                              Редактирование профиля: {{ editingProfileLabel }}
                            </v-alert>

                            <v-row dense>
                              <v-col cols="12" md="4">
                                <v-text-field
                                  v-model="climateDraft.name"
                                  label="Название профиля"
                                  variant="outlined"
                                  prepend-inner-icon="mdi-thermometer"
                                ></v-text-field>
                              </v-col>
                              <v-col cols="12" md="8">
                                <v-text-field
                                  v-model="climateDraft.notes"
                                  label="Заметки"
                                  variant="outlined"
                                  prepend-inner-icon="mdi-note-text-outline"
                                ></v-text-field>
                              </v-col>

                              <v-col cols="12" md="6">
                                <v-sheet class="pa-4 border rounded-lg" color="transparent">
                                  <h4 class="text-subtitle-2 mb-3">Дневные условия</h4>
                                  <v-row dense>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.dayTemperature"
                                        type="number"
                                        label="Температура, °C"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.dayHumidity"
                                        type="number"
                                        label="Влажность, %"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.dayCo2"
                                        type="number"
                                        label="CO₂, ppm"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.dayLight"
                                        type="number"
                                        label="Освещённость, lux"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.dayVpd"
                                        type="number"
                                        label="VPD"
                                        variant="outlined"
                                        density="comfortable"
                                        hint="Паровой дефицит"
                                        persistent-hint
                                      ></v-text-field>
                                    </v-col>
                                  </v-row>
                                </v-sheet>
                              </v-col>

                              <v-col cols="12" md="6">
                                <v-sheet class="pa-4 border rounded-lg" color="transparent">
                                  <h4 class="text-subtitle-2 mb-3">Ночные условия</h4>
                                  <v-row dense>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.nightTemperature"
                                        type="number"
                                        label="Температура, °C"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.nightHumidity"
                                        type="number"
                                        label="Влажность, %"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.nightCo2"
                                        type="number"
                                        label="CO₂, ppm"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.nightLight"
                                        type="number"
                                        label="Освещённость, lux"
                                        variant="outlined"
                                        density="comfortable"
                                      ></v-text-field>
                                    </v-col>
                                    <v-col cols="12" sm="6" md="4">
                                      <v-text-field
                                        v-model.number="climateDraft.nightVpd"
                                        type="number"
                                        label="VPD"
                                        variant="outlined"
                                        density="comfortable"
                                        hint="Паровой дефицит"
                                        persistent-hint
                                      ></v-text-field>
                                    </v-col>
                                  </v-row>
                                </v-sheet>
                              </v-col>
                            </v-row>

                            <v-row dense class="mt-2">
                              <v-col cols="12" class="d-flex flex-wrap ga-3">
                                <v-btn
                                  color="primary"
                                  variant="tonal"
                                  prepend-icon="mdi-content-save"
                                  @click="addClimateProfile"
                                >
                                  Сохранить профиль
                                </v-btn>
                                <v-btn
                                  v-if="editingProfileId"
                                  variant="text"
                                  color="secondary"
                                  prepend-icon="mdi-close-circle"
                                  @click="startNewClimateProfile"
                                >
                                  Отмена редактирования
                                </v-btn>
                              </v-col>
                            </v-row>
                          </v-card-text>
                        </v-card>
                      </v-col>

                      <v-col cols="12" v-if="climateProfiles.length">
                        <v-card variant="flat" class="border rounded">
                          <v-card-text>
                            <h4 class="text-subtitle-2 mb-3">Сохранённые профили</h4>
                            <v-list density="comfortable" variant="flat">
                              <v-list-item
                                v-for="profile in climateProfiles"
                                :key="profile.id"
                                class="mb-2"
                              >
                                <v-list-item-title>{{ profile.name }}</v-list-item-title>
                                <v-list-item-subtitle v-if="profile.notes">
                                  {{ profile.notes }}
                                </v-list-item-subtitle>
                                <v-list-item-subtitle class="text-caption text-medium-emphasis mt-2">
                                  <span v-if="profile.dayTemperature !== null">
                                    День: {{ profile.dayTemperature }}°C
                                  </span>
                                  <span v-if="profile.dayHumidity !== null">
                                    · влажность {{ profile.dayHumidity }}%
                                  </span>
                                  <span v-if="profile.nightTemperature !== null">
                                    <br />Ночь: {{ profile.nightTemperature }}°C
                                  </span>
                                  <span v-if="profile.nightHumidity !== null">
                                    · влажность {{ profile.nightHumidity }}%
                                  </span>
                                </v-list-item-subtitle>
                                <template #append>
                                <div class="d-flex ga-2">
                                  <v-btn
                                    icon="mdi-pencil-outline"
                                    variant="text"
                                    color="primary"
                                    @click="startEditDraftProfile(profile)"
                                  ></v-btn>
                                  <v-btn
                                    icon="mdi-delete-outline"
                                    variant="text"
                                    color="error"
                                    @click="removeClimateProfile(profile.id)"
                                  ></v-btn>
                                </div>
                                </template>
                              </v-list-item>
                            </v-list>
                          </v-card-text>
                        </v-card>
                      </v-col>

                    </v-row>
                  </v-form>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>

            <v-stepper-window-item :value="2">
              <v-card flat>
                <v-card-text>
                  <h3 class="text-subtitle-1 mb-4">Root узел (опционально)</h3>

                  <v-alert
                    border="start"
                    type="info"
                    variant="tonal"
                    class="mb-4"
                  >
                    Можно выбрать существующий ROOT-узел для управления теплицей. Шаг можно пропустить и назначить узел позже.
                  </v-alert>

                  <v-select
                    v-model="rootNodeId"
                    :items="rootNodeOptions"
                    item-title="label"
                    item-value="value"
                    label="ROOT узел"
                    variant="outlined"
                    prepend-inner-icon="mdi-router-network"
                    :loading="nodesLoading"
                    clearable
                  >
                    <template #item="{ props, item }">
                      <v-list-item v-bind="props">
                        <template #prepend>
                          <v-icon
                            :color="item.raw.online ? 'success' : 'warning'"
                            icon="mdi-access-point-network"
                          ></v-icon>
                        </template>
                        <v-list-item-title>{{ item.raw.label }}</v-list-item-title>
                        <v-list-item-subtitle>
                          {{ item.raw.online ? 'Онлайн' : 'Офлайн' }} · {{ item.raw.node_type }}
                        </v-list-item-subtitle>
                      </v-list-item>
                    </template>
                  </v-select>

                  <div v-if="rootNodeId" class="mt-3 text-body-2 text-medium-emphasis">
                    Выбранный узел будет использоваться как точка входа для mesh-сети теплицы.
                  </div>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>

            <v-stepper-window-item :value="3">
              <v-card flat>
                <v-card-text>
                  <h3 class="text-subtitle-1 mb-4">Настройка зон</h3>

                  <v-alert
                    v-if="!availableZones.length && !draftZones.length"
                    type="warning"
                    variant="tonal"
                    border="start"
                    class="mb-4"
                  >
                    В системе пока нет свободных зон. Добавьте новую зону ниже.
                  </v-alert>

                  <v-autocomplete
                    v-model="selectedZoneIds"
                    :items="zoneOptions"
                    item-title="label"
                    item-value="value"
                    label="Существующие зоны"
                    variant="outlined"
                    multiple
                    chips
                    prepend-inner-icon="mdi-map-marker-radius"
                    :disabled="!availableZones.length"
                    hint="Можно выбрать одну или несколько готовых зон"
                    persistent-hint
                  ></v-autocomplete>

                  <v-divider class="my-6"></v-divider>

                  <h4 class="text-subtitle-2 mb-2">Добавить новую зону</h4>
                  <v-row dense>
                    <v-col cols="12" md="5">
                      <v-text-field
                        v-model="draftZoneName"
                        label="Название зоны"
                        variant="outlined"
                        prepend-inner-icon="mdi-format-title"
                      ></v-text-field>
                    </v-col>
                    <v-col cols="12" md="3">
                      <v-select
                        v-model="draftZoneType"
                        :items="zoneTypes"
                        item-title="title"
                        item-value="value"
                        label="Тип системы"
                        variant="outlined"
                        prepend-inner-icon="mdi-water-check"
                      ></v-select>
                    </v-col>
                    <v-col cols="12" md="3">
                      <v-text-field
                        v-model="draftZoneLocation"
                        label="Расположение"
                        variant="outlined"
                        prepend-inner-icon="mdi-map-marker"
                      ></v-text-field>
                    </v-col>
                    <v-col cols="12" md="1" class="d-flex align-center">
                      <v-btn
                        color="primary"
                        variant="tonal"
                        icon="mdi-plus"
                        :disabled="!draftZoneName.trim()"
                        @click="addDraftZone"
                      ></v-btn>
                    </v-col>
                  </v-row>

                  <v-chip-group
                    v-if="draftZones.length"
                    column
                    class="mt-3"
                  >
                    <v-chip
                      v-for="zone in draftZones"
                      :key="zone.id"
                      closable
                      color="primary"
                      variant="tonal"
                      @click:close="removeDraftZone(zone.id)"
                    >
                      {{ zone.name }} · {{ zoneTypeLabel(zone.zone_type) }}
                    </v-chip>
                  </v-chip-group>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>

            <v-stepper-window-item :value="4">
              <v-card flat>
                <v-card-text>
                  <h3 class="text-subtitle-1 mb-4">Запуск цикла (необязательно)</h3>

                  <v-switch
                    v-model="autoPlan.enabled"
                    color="success"
                    label="Запустить цикл сразу после создания"
                    class="mb-4"
                  ></v-switch>

                  <v-expand-transition>
                    <div v-if="autoPlan.enabled">
                      <v-row dense>
                        <v-col cols="12" md="6">
                          <v-select
                            v-model="autoPlan.presetId"
                            :items="presetOptions"
                            item-title="label"
                            item-value="value"
                            label="Пресет роста"
                            variant="outlined"
                            prepend-inner-icon="mdi-sprout"
                            :loading="presetsLoading"
                          ></v-select>
                        </v-col>
                        <v-col cols="12" md="6">
                          <v-select
                            v-model="autoPlan.zoneId"
                            :items="cycleZoneOptions"
                            item-title="label"
                            item-value="value"
                            label="Зона для запуска"
                            variant="outlined"
                            prepend-inner-icon="mdi-map-marker-check"
                            :disabled="!cycleZoneOptions.length"
                          ></v-select>
                        </v-col>
                        <v-col cols="12" md="4">
                          <v-text-field
                            v-model.number="autoPlan.plantCount"
                            type="number"
                            min="1"
                            label="Количество растений"
                            variant="outlined"
                            prepend-inner-icon="mdi-sprout-outline"
                          ></v-text-field>
                        </v-col>
                        <v-col cols="12" md="8">
                          <v-textarea
                            v-model="autoPlan.notes"
                            label="Заметки"
                            variant="outlined"
                            rows="2"
                          ></v-textarea>
                        </v-col>
                      </v-row>

                      <v-alert
                        v-if="!cycleZoneOptions.length"
                        type="info"
                        variant="tonal"
                        border="start"
                      >
                        Выберите хотя бы одну существующую зону на шаге 3, чтобы запустить цикл автоматически.
                      </v-alert>
                    </div>
                  </v-expand-transition>

                  <v-divider class="my-6"></v-divider>

                  <h3 class="text-subtitle-1 mb-4">Итоговая конфигурация</h3>
                  <v-row>
                    <v-col cols="12" md="6">
                      <v-sheet class="pa-4 rounded-xl" variant="outlined">
                        <div class="text-caption text-medium-emphasis mb-1">Название</div>
                        <div class="text-body-1 font-weight-medium">{{ basicForm.name }}</div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Код</div>
                        <div class="text-body-1 font-weight-medium">{{ basicForm.code || '—' }}</div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Статус</div>
                        <div class="text-body-2 text-medium-emphasis">{{ statusLabel }}</div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Описание</div>
                        <div class="text-body-2">
                          {{ basicForm.description || 'Не заполнено' }}
                        </div>
                      </v-sheet>
                    </v-col>

                    <v-col cols="12" md="6">
                      <v-sheet class="pa-4 rounded-xl" variant="outlined">
                        <div class="text-caption text-medium-emphasis mb-1">Root узел</div>
                        <div class="text-body-1 font-weight-medium">
                          {{ rootNodeLabel || 'Не назначен' }}
                        </div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Профили климата</div>
                        <div class="text-body-2">
                          <template v-if="climateProfiles.length">
                            <div
                              v-for="profile in climateProfiles"
                              :key="profile.id"
                            >
                              • {{ profile.name }}
                            </div>
                          </template>
                          <template v-else>
                            Профили не добавлены
                          </template>
                        </div>

                        <div class="text-caption text-medium-emphasis mt-4 mb-1">Зоны</div>
                        <div class="text-body-2">
                          <template v-if="summaryZones.length">
                            <div
                              v-for="zone in summaryZones"
                              :key="zone.id"
                            >
                              • {{ zone.name }} ({{ zoneTypeLabel(zone.zone_type) }})
                            </div>
                          </template>
                          <template v-else>
                            Пока не выбраны
                          </template>
                        </div>
                      </v-sheet>
                    </v-col>
                  </v-row>
                </v-card-text>
              </v-card>
            </v-stepper-window-item>
          </v-stepper-window>
        </v-stepper>
      </v-card-text>

      <v-divider></v-divider>

      <v-card-actions class="pa-4">
        <v-btn
          variant="text"
          prepend-icon="mdi-arrow-left"
          :disabled="step === 1 || loading"
          @click="prevStep"
        >
          Назад
        </v-btn>

        <v-spacer></v-spacer>

        <v-btn
          color="primary"
          prepend-icon="mdi-arrow-right"
          v-if="step < maxStep"
          :disabled="loading || !isStepValid(step)"
          @click="nextStep"
        >
          Далее
        </v-btn>

        <v-btn
          color="success"
          prepend-icon="mdi-check-circle"
          v-else
          :loading="loading"
          :disabled="!isStepValid(step)"
          @click="submit"
        >
          Создать теплицу
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-dialog>
</template>

<script setup lang="ts">
import { ref, reactive, computed, watch } from 'vue'
import { useGreenhousesStore } from '@/stores/greenhouses'
import { useZonesStore } from '@/stores/zones'
import { useNodesStore } from '@/stores/nodes'
import { useGrowthStore } from '@/stores/growth'
import { useSetupStore } from '@/stores/setup'
import { usePopup } from '@/composables/usePopup'
import type { GreenhouseDetail, CreateGreenhousePayload, ClimateProfile } from '@/types/greenhouse'
import type { Zone } from '@/stores/zones'

interface DraftZone {
  id: string
  name: string
  zone_type: string
  location?: string
}

interface ClimateProfileDraftFields {
  name: string
  notes: string
  dayTemperature: number | null
  dayHumidity: number | null
  dayCo2: number | null
  dayLight: number | null
  dayVpd: number | null
  nightTemperature: number | null
  nightHumidity: number | null
  nightCo2: number | null
  nightLight: number | null
  nightVpd: number | null
}

interface ClimateProfileDraft extends ClimateProfileDraftFields {
  id: string
}

interface ClimateProfileOption {
  value: string
  title: string
  profile: ClimateProfile
}

interface SummaryZone {
  id: number | string
  name: string
  zone_type: string
  isDraft: boolean
}

interface ZoneTypeOption {
  title: string
  value: string
}

interface StatusOption {
  label: string
  value: string
}

interface ZoneLike {
  id: number
  name: string
  zone_type: string
  mesh_network_id?: string
  greenhouse_id?: number | null
}

const props = defineProps<{
  modelValue: boolean
}>()

const emit = defineEmits<{
  (event: 'update:modelValue', value: boolean): void
  (event: 'created', greenhouse: GreenhouseDetail): void
}>()

const internalModel = ref(false)
const step = ref(1)
const loading = ref(false)
const basicFormRef = ref()

const greenhousesStore = useGreenhousesStore()
const zonesStore = useZonesStore()
const nodesStore = useNodesStore()
const growthStore = useGrowthStore()
const setupStore = useSetupStore()
const popup = usePopup()

const rules = {
  required: (value: any) => !!value || 'Обязательное поле',
}

const basicForm = reactive({
  name: '',
  code: '',
  description: '',
  status: 'active',
})

const climateProfiles = ref<ClimateProfileDraft[]>([])
const climateDraft = reactive<ClimateProfileDraftFields>({
  name: '',
  notes: '',
  dayTemperature: null,
  dayHumidity: null,
  dayCo2: null,
  dayLight: null,
  dayVpd: null,
  nightTemperature: null,
  nightHumidity: null,
  nightCo2: null,
  nightLight: null,
  nightVpd: null,
})
const climateProfilesCatalog = computed<ClimateProfile[]>(() => greenhousesStore.knownClimateProfiles)
const existingClimateProfileOptions = computed<ClimateProfileOption[]>(() =>
  climateProfilesCatalog.value.map((profile: ClimateProfile) => ({
    value: profile.id ?? profile.name,
    title: profile.name,
    profile,
  })),
)
const selectedExistingProfile = ref<string | null>(null)
const editingProfileId = ref<string | null>(null)

const codeTouched = ref(false)
const rootNodeId = ref<string | null>(null)
const selectedZoneIds = ref<number[]>([])
const draftZones = ref<DraftZone[]>([])
const draftZoneName = ref('')
const draftZoneType = ref('nft')
const draftZoneLocation = ref('')
const autoPlan = reactive({
  enabled: false,
  presetId: null as number | null,
  zoneId: null as number | null,
  plantCount: null as number | null,
  notes: '',
})

const nodesLoading = ref(false)
const presetsLoading = ref(false)

const stepLabels = ['Описание', 'Root узел', 'Зоны', 'Цикл']
const maxStep = stepLabels.length

const availableRootNodes = computed(() =>
  nodesStore.nodes.filter((node: any) => node.node_type === 'root'),
)

const rootNodeOptions = computed(() =>
  availableRootNodes.value.map((node: any) => ({
    value: node.node_id,
    label: node.node_id,
    online: node.online ?? node.is_online,
    node_type: node.node_type,
  })),
)

const availableZones = computed(() =>
  zonesStore.zones.filter((zone: ZoneLike) => zone.greenhouse_id == null),
)

const zoneOptions = computed(() =>
  availableZones.value.map((zone: ZoneLike) => ({
    value: zone.id,
    label: zone.name || zone.mesh_network_id,
    type: zone.zone_type,
  })),
)

const selectedZonesDetailed = computed<ZoneLike[]>(() =>
  zonesStore.zones
    .filter((zone: ZoneLike) => selectedZoneIds.value.includes(zone.id))
    .map((zone: ZoneLike) => ({
      id: zone.id,
      name: zone.name,
      zone_type: zone.zone_type,
    })),
)

const summaryZones = computed<SummaryZone[]>(() => [
  ...selectedZonesDetailed.value.map((zone: ZoneLike) => ({
    id: zone.id,
    name: zone.name,
    zone_type: zone.zone_type,
    isDraft: false,
  })),
  ...draftZones.value.map((zone: DraftZone) => ({
    id: zone.id,
    name: zone.name,
    zone_type: zone.zone_type,
    isDraft: true,
  })),
])

const cycleZoneOptions = computed(() =>
  selectedZonesDetailed.value.map((zone: ZoneLike) => ({
    value: zone.id,
    label: zone.name,
  })),
)

const presetOptions = computed(() =>
  growthStore.presets.map((preset: { id: number; name: string }) => ({
    value: preset.id,
    label: preset.name,
  })),
)

const editingProfileLabel = computed(() => {
  if (!editingProfileId.value) {
    return null
  }

  const id = editingProfileId.value

  if (id.startsWith('existing-')) {
    const targetId = id.replace('existing-', '')
    const option = existingClimateProfileOptions.value.find(
      (item: ClimateProfileOption) => item.value === targetId,
    )
    return option?.profile.name ?? (climateDraft.name || 'Профиль')
  }

  const draft = climateProfiles.value.find((profile: ClimateProfileDraft) => profile.id === id)
  if (draft) {
    return draft.name || 'Профиль'
  }

  return climateDraft.name || 'Профиль'
})

const statusOptions: StatusOption[] = [
  { label: 'Активна', value: 'active' },
  { label: 'Черновик', value: 'draft' },
  { label: 'Техническое обслуживание', value: 'maintenance' },
]

const statusLabel = computed(() => {
  const match = statusOptions.find((option: StatusOption) => option.value === basicForm.status)
  return match ? match.label : 'Не выбран'
})

const zoneTypes: ZoneTypeOption[] = [
  { title: 'NFT', value: 'nft' },
  { title: 'DWC', value: 'dwc' },
  { title: 'Капельный полив', value: 'drip' },
  { title: 'Ebb & Flow', value: 'ebb_flow' },
  { title: 'Аэропоника', value: 'aeroponics' },
  { title: 'Другое', value: 'other' },
]

watch(
  () => props.modelValue,
  async (value: boolean) => {
    internalModel.value = value
    if (value) {
      resetWizard()
      await ensureData()
    }
  },
)

watch(internalModel, (value: boolean) => {
  emit('update:modelValue', value)
})

watch(
  () => basicForm.name,
  (value: string) => {
    if (!codeTouched.value) {
      basicForm.code = generateCode(value)
    }
  },
)

watch(
  () => internalModel.value,
  (value: boolean) => {
    if (!value) {
      autoPlan.enabled = false
    }
  },
)

async function ensureData() {
  const promises: Promise<any>[] = []

  if (!greenhousesStore.items.length) {
    promises.push(greenhousesStore.fetchGreenhouses())
  }

  if (!zonesStore.zones.length) {
    promises.push(zonesStore.fetchZones())
  }

  if (!nodesStore.nodes.length) {
    nodesLoading.value = true
    const promise = nodesStore.fetchNodes().finally(() => {
      nodesLoading.value = false
    })
    promises.push(promise)
  }

  if (!growthStore.presets.length) {
    presetsLoading.value = true
    const promise = growthStore.fetchPresets().finally(() => {
      presetsLoading.value = false
    })
    promises.push(promise)
  }

  await Promise.all(promises)
}

function resetWizard() {
  step.value = 1
  loading.value = false
  codeTouched.value = false
  basicForm.name = ''
  basicForm.code = ''
  basicForm.description = ''
  basicForm.status = 'active'
  climateProfiles.value = []
  selectedExistingProfile.value = null
  editingProfileId.value = null
  resetClimateDraft()
  rootNodeId.value = null
  selectedZoneIds.value = []
  draftZones.value = []
  draftZoneName.value = ''
  draftZoneType.value = 'nft'
  draftZoneLocation.value = ''
  autoPlan.enabled = false
  autoPlan.presetId = null
  autoPlan.zoneId = null
  autoPlan.plantCount = null
  autoPlan.notes = ''
}

function isStepValid(stepIndex: number) {
  if (stepIndex === 1) {
    return Boolean(basicForm.name.trim() && basicForm.description.trim())
  }
  if (stepIndex === 3) {
    return selectedZoneIds.value.length + draftZones.value.length > 0
  }
  if (stepIndex === 4 && autoPlan.enabled) {
    return Boolean(autoPlan.presetId && autoPlan.zoneId)
  }
  return true
}

function nextStep() {
  if (step.value >= maxStep) {
    return
  }
  if (!isStepValid(step.value)) {
    popup.toast.error('Заполните обязательные поля перед продолжением')
    return
  }
  step.value += 1
}

function prevStep() {
  if (step.value > 1) {
    step.value -= 1
  }
}

function addDraftZone() {
  if (!draftZoneName.value.trim()) {
    return
  }
  draftZones.value.push({
    id: `draft-${Date.now()}-${draftZones.value.length}`,
    name: draftZoneName.value.trim(),
    zone_type: draftZoneType.value,
    location: draftZoneLocation.value.trim() || undefined,
  })
  draftZoneName.value = ''
  draftZoneLocation.value = ''
}

function removeDraftZone(id: string) {
  draftZones.value = draftZones.value.filter((zone: DraftZone) => zone.id !== id)
}

function zoneTypeLabel(value: string) {
  const match = zoneTypes.find((zone: ZoneTypeOption) => zone.value === value)
  return match ? match.title : value
}

function generateCode(value: string) {
  if (!value) {
    return ''
  }
  return slugify(value).toUpperCase()
}

function slugify(value: string) {
  return value
    .normalize('NFD')
    .replace(/[\u0300-\u036f]/g, '')
    .replace(/[^a-zA-Z0-9]+/g, '_')
    .replace(/^_+|_+$/g, '')
}

function buildMeshId(zoneName: string) {
  const base = basicForm.code || generateCode(basicForm.name || 'GH')
  const suffix = slugify(zoneName || 'ZONE')
  return `${base}_${suffix}`.toUpperCase()
}

function buildMqttPrefix(zoneName: string) {
  const base = (basicForm.code || generateCode(basicForm.name || 'gh')).toLowerCase()
  const suffix = slugify(zoneName || 'zone').toLowerCase()
  return `hydro/${base}/${suffix}/`
}

function resetClimateDraft() {
  climateDraft.name = ''
  climateDraft.notes = ''
  climateDraft.dayTemperature = null
  climateDraft.dayHumidity = null
  climateDraft.dayCo2 = null
  climateDraft.dayLight = null
  climateDraft.dayVpd = null
  climateDraft.nightTemperature = null
  climateDraft.nightHumidity = null
  climateDraft.nightCo2 = null
  climateDraft.nightLight = null
  climateDraft.nightVpd = null
}

function loadDraftFromExistingProfile(profile: ClimateProfile) {
  resetClimateDraft()
  climateDraft.name = profile.name ?? ''
  climateDraft.notes = profile.notes ?? ''
  climateDraft.dayTemperature = profile.day?.temperature ?? null
  climateDraft.dayHumidity = profile.day?.humidity ?? null
  climateDraft.dayCo2 = profile.day?.co2 ?? null
  climateDraft.dayLight = profile.day?.light ?? null
  climateDraft.dayVpd = profile.day?.vpd ?? null
  climateDraft.nightTemperature = profile.night?.temperature ?? null
  climateDraft.nightHumidity = profile.night?.humidity ?? null
  climateDraft.nightCo2 = profile.night?.co2 ?? null
  climateDraft.nightLight = profile.night?.light ?? null
  climateDraft.nightVpd = profile.night?.vpd ?? null
}

function loadDraftFromDraftProfile(profile: ClimateProfileDraft) {
  resetClimateDraft()
  climateDraft.name = profile.name
  climateDraft.notes = profile.notes
  climateDraft.dayTemperature = profile.dayTemperature
  climateDraft.dayHumidity = profile.dayHumidity
  climateDraft.dayCo2 = profile.dayCo2
  climateDraft.dayLight = profile.dayLight
  climateDraft.dayVpd = profile.dayVpd
  climateDraft.nightTemperature = profile.nightTemperature
  climateDraft.nightHumidity = profile.nightHumidity
  climateDraft.nightCo2 = profile.nightCo2
  climateDraft.nightLight = profile.nightLight
  climateDraft.nightVpd = profile.nightVpd
}

function startNewClimateProfile() {
  selectedExistingProfile.value = null
  editingProfileId.value = null
  resetClimateDraft()
}

function startEditExistingProfile() {
  if (!selectedExistingProfile.value) {
    popup.toast.error('Выберите профиль из списка')
    return
  }

  const option = existingClimateProfileOptions.value.find(
    (item: ClimateProfileOption) => item.value === selectedExistingProfile.value,
  )

  if (!option) {
    popup.toast.error('Не удалось найти выбранный профиль')
    return
  }

  loadDraftFromExistingProfile(option.profile)
  editingProfileId.value = `existing-${option.value}`
}

function startEditDraftProfile(profile: ClimateProfileDraft) {
  selectedExistingProfile.value = null
  editingProfileId.value = profile.id
  loadDraftFromDraftProfile(profile)
}

function addClimateProfile() {
  if (!climateDraft.name.trim()) {
    popup.toast.error('Укажите название профиля климата')
    return
  }

  const id =
    editingProfileId.value ?? `profile-${Date.now()}-${climateProfiles.value.length}`

  const profile: ClimateProfileDraft = {
    id,
    name: climateDraft.name.trim(),
    notes: climateDraft.notes.trim(),
    dayTemperature: climateDraft.dayTemperature,
    dayHumidity: climateDraft.dayHumidity,
    dayCo2: climateDraft.dayCo2,
    dayLight: climateDraft.dayLight,
    dayVpd: climateDraft.dayVpd,
    nightTemperature: climateDraft.nightTemperature,
    nightHumidity: climateDraft.nightHumidity,
    nightCo2: climateDraft.nightCo2,
    nightLight: climateDraft.nightLight,
    nightVpd: climateDraft.nightVpd,
  }

  const index = climateProfiles.value.findIndex((item: ClimateProfileDraft) => item.id === id)
  if (index === -1) {
    climateProfiles.value.push(profile)
  } else {
    climateProfiles.value.splice(index, 1, profile)
  }

  startNewClimateProfile()
}

function removeClimateProfile(id: string) {
  climateProfiles.value = climateProfiles.value.filter((profile: ClimateProfileDraft) => profile.id !== id)
  if (editingProfileId.value === id) {
    startNewClimateProfile()
  }
}

function mapProfileToPayload(profile: ClimateProfileDraft) {
  const day = pickNumericSetpoints({
    temperature: profile.dayTemperature,
    humidity: profile.dayHumidity,
    co2: profile.dayCo2,
    light: profile.dayLight,
    vpd: profile.dayVpd,
  })

  const night = pickNumericSetpoints({
    temperature: profile.nightTemperature,
    humidity: profile.nightHumidity,
    co2: profile.nightCo2,
    light: profile.nightLight,
    vpd: profile.nightVpd,
  })

  const notes = profile.notes.trim()

  return {
    name: profile.name,
    notes: notes ? notes : undefined,
    day: Object.keys(day).length ? day : undefined,
    night: Object.keys(night).length ? night : undefined,
  }
}

function pickNumericSetpoints(values: Record<string, number | null>) {
  const result: Record<string, number> = {}
  Object.entries(values).forEach(([key, value]) => {
    if (value !== null && !Number.isNaN(value)) {
      result[key] = Number(value)
    }
  })
  return result
}

async function submit() {
  if (!isStepValid(step.value)) {
    popup.toast.error('Проверьте введённые данные перед созданием теплицы')
    return
  }

  loading.value = true

  try {
    const payload: CreateGreenhousePayload = {
      name: basicForm.name.trim(),
      code: (basicForm.code || generateCode(basicForm.name)).trim(),
      description: basicForm.description.trim(),
      status: basicForm.status as any,
    }

    const profiles = climateProfiles.value
      .map(mapProfileToPayload)
      .filter((profile: ReturnType<typeof mapProfileToPayload>) => Boolean(profile.name))
    if (profiles.length) {
      payload.climate_profiles = profiles
    }

    const greenhouse = await greenhousesStore.createGreenhouse(payload)
    if (!greenhouse) {
      throw new Error('Сервер не вернул созданную теплицу')
    }

    const greenhouseId = greenhouse.id

    if (rootNodeId.value) {
      await greenhousesStore.attachNode(greenhouseId, {
        node_id: rootNodeId.value,
        as_root: true,
      })
    }

    for (const zoneId of selectedZoneIds.value) {
      await greenhousesStore.attachZone(greenhouseId, { zone_id: zoneId })
    }

    for (const draft of draftZones.value) {
      const zonePayload: Partial<Zone> & { greenhouse_id?: number | null } = {
        name: draft.name,
        zone_type: draft.zone_type,
        location: draft.location,
        root_node_id: rootNodeId.value || '',
        mesh_network_id: buildMeshId(draft.name),
        mqtt_topic_prefix: buildMqttPrefix(draft.name),
        is_active: true,
        is_available: true,
        greenhouse_id: greenhouseId,
      }
      const zone = await zonesStore.createZone(zonePayload)
      if (zone?.id) {
        await greenhousesStore.attachZone(greenhouseId, { zone_id: zone.id })
      }
    }

    if (autoPlan.enabled && autoPlan.presetId && autoPlan.zoneId) {
      await growthStore.createCycle({
        zone_id: autoPlan.zoneId,
        preset_id: autoPlan.presetId,
        plant_count: autoPlan.plantCount ?? undefined,
        notes: autoPlan.notes || undefined,
        greenhouse_id: greenhouseId,
      })
    }

    await Promise.all([
      greenhousesStore.fetchGreenhouse(greenhouseId, { refresh: true }),
      greenhousesStore.fetchGreenhouses({ refresh: true }),
      greenhousesStore.fetchZones(greenhouseId, { refresh: true }),
      greenhousesStore.fetchNodes(greenhouseId, { refresh: true }),
    ])

    setupStore.setTargetGreenhouseId(greenhouseId)
    greenhousesStore.selectGreenhouse(greenhouseId)

    popup.toast.success('Теплица успешно создана')
    emit('created', greenhouse)
    internalModel.value = false
  } catch (error: any) {
    console.error('Ошибка создания теплицы', error)
    const message =
      error?.response?.data?.message ||
      error?.message ||
      'Не удалось создать теплицу'
    popup.toast.error(message)
  } finally {
    loading.value = false
  }
}

function handleCancel() {
  if (loading.value) {
    return
  }
  internalModel.value = false
}
</script>

<style scoped>
.text-medium-emphasis {
  color: rgba(0, 0, 0, 0.6);
}
</style>


