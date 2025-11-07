/**
 * ⭐ GROWTH PLANNER: Тесты для CreateCycleDialog компонента
 */
import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia } from 'pinia'
import CreateCycleDialog from '@/components/growth/CreateCycleDialog.vue'
import { useGrowthStore } from '@/stores/growth'
import { useZonesStore } from '@/stores/zones'
import axios from 'axios'

vi.mock('axios')
const mockedAxios = axios as any

describe('CreateCycleDialog.vue', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
  })

  it('renders dialog when modelValue is true', () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    expect(wrapper.text()).toContain('Создать новый цикл роста')
  })

  it('hides dialog when modelValue is false', () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: false,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Диалог должен быть скрыт
    const dialog = wrapper.findComponent({ name: 'VDialog' })
    expect(dialog.props('modelValue')).toBe(false)
  })

  it('starts at step 1', () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    expect(wrapper.text()).toContain('Шаг 1: Выбор зоны')
  })

  it('shows step 2 when next is clicked', async () => {
    const zonesStore = useZonesStore()
    zonesStore.zones = [
      {
        id: 1,
        name: 'Zone 1',
        is_active: true,
        is_available: true,
        current_cycle_id: null,
      } as any,
    ]

    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Выбираем зону
    await wrapper.setData({ 'formData.zone': zonesStore.zones[0] })
    
    // Нажимаем "Далее"
    const nextButton = wrapper.findAll('button').find(btn => btn.text().includes('Далее'))
    if (nextButton) {
      await nextButton.trigger('click')
      await wrapper.vm.$nextTick()
      
      // Должен быть шаг 2
      expect(wrapper.text()).toContain('Шаг 2')
    }
  })

  it('validates zone selection', async () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Пытаемся перейти на шаг 2 без выбора зоны
    const nextButton = wrapper.findAll('button').find(btn => btn.text().includes('Далее'))
    if (nextButton) {
      await nextButton.trigger('click')
      await wrapper.vm.$nextTick()
      
      // Должна быть ошибка валидации
      expect(wrapper.vm.errors.zone).toBeDefined()
    }
  })

  it('loads presets on step 2', async () => {
    const growthStore = useGrowthStore()
    mockedAxios.get.mockResolvedValue({
      data: [
        { id: 1, name: 'Preset 1', culture_id: 1 },
        { id: 2, name: 'Preset 2', culture_id: 2 },
      ],
    })

    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Переходим на шаг 2
    await wrapper.setData({ currentStep: 2 })
    await wrapper.vm.$nextTick()

    // Должны загрузиться пресеты
    expect(mockedAxios.get).toHaveBeenCalled()
  })

  it('validates preset selection', async () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Переходим на шаг 2 и пытаемся на шаг 3 без выбора пресета
    await wrapper.setData({ currentStep: 2 })
    await wrapper.vm.$nextTick()

    const nextButton = wrapper.findAll('button').find(btn => btn.text().includes('Далее'))
    if (nextButton) {
      await nextButton.trigger('click')
      await wrapper.vm.$nextTick()
      
      // Должна быть ошибка валидации
      expect(wrapper.vm.errors.preset).toBeDefined()
    }
  })

  it('creates cycle when form is valid', async () => {
    const growthStore = useGrowthStore()
    const zonesStore = useZonesStore()

    zonesStore.zones = [
      {
        id: 1,
        name: 'Zone 1',
        is_active: true,
        is_available: true,
        current_cycle_id: null,
      } as any,
    ]

    const mockPreset = {
      id: 1,
      name: 'Preset 1',
      culture_id: 1,
      stages: [],
    }

    const createdCycle = {
      id: 1,
      zone_id: 1,
      preset_id: 1,
      status: 'active',
    }

    mockedAxios.post.mockResolvedValue({ data: createdCycle })

    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Заполняем форму
    await wrapper.setData({
      currentStep: 3,
      'formData.zone': zonesStore.zones[0],
      'formData.preset': mockPreset,
      'formData.plant_count': 10,
    })

    await wrapper.vm.$nextTick()

    // Нажимаем "Создать цикл"
    const createButton = wrapper.findAll('button').find(btn => btn.text().includes('Создать цикл'))
    if (createButton) {
      await createButton.trigger('click')
      await new Promise(resolve => setTimeout(resolve, 100))

      expect(mockedAxios.post).toHaveBeenCalledWith('/api/growth/cycles', {
        zone_id: 1,
        preset_id: 1,
        plant_count: 10,
        notes: '',
      })
    }
  })

  it('emits created event when cycle is created', async () => {
    const growthStore = useGrowthStore()
    mockedAxios.post.mockResolvedValue({
      data: { id: 1, zone_id: 1, preset_id: 1, status: 'active' },
    })

    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Заполняем и создаём
    await wrapper.setData({
      currentStep: 3,
      'formData.zone': { id: 1 } as any,
      'formData.preset': { id: 1 } as any,
    })

    await wrapper.vm.createCycle()
    await wrapper.vm.$nextTick()

    expect(wrapper.emitted('created')).toBeTruthy()
  })

  it('closes dialog when cancel is clicked', async () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    const cancelButton = wrapper.findAll('button').find(btn => btn.text().includes('Отмена'))
    if (cancelButton) {
      await cancelButton.trigger('click')
      await wrapper.vm.$nextTick()

      expect(wrapper.emitted('update:modelValue')).toBeTruthy()
      expect(wrapper.emitted('update:modelValue')?.[0]?.[0]).toBe(false)
    }
  })

  it('resets form when dialog is closed', async () => {
    const wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Заполняем форму
    await wrapper.setData({
      currentStep: 2,
      'formData.zone': { id: 1 } as any,
    })

    // Закрываем диалог
    await wrapper.setProps({ modelValue: false })
    await wrapper.vm.$nextTick()

    // Форма должна быть сброшена
    expect(wrapper.vm.currentStep).toBe(1)
    expect(wrapper.vm.formData.zone).toBeNull()
  })
})

