/**
 * ⭐ GROWTH PLANNER: Тесты для CreateCycleDialog компонента
 */
import { describe, it, expect, beforeEach, afterEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { setActivePinia, createPinia } from 'pinia'
import CreateCycleDialog from '@/components/growth/CreateCycleDialog.vue'
import { useGrowthStore } from '@/stores/growth'
import { useZonesStore } from '@/stores/zones'
import axios from 'axios'
const mockedAxios = axios as any

let wrapper: ReturnType<typeof mount> | null = null

describe('CreateCycleDialog.vue', () => {
  beforeEach(() => {
    setActivePinia(createPinia())
    vi.clearAllMocks()
    mockedAxios.get.mockResolvedValue({ data: [] })
    mockedAxios.post.mockResolvedValue({ data: {} })
    wrapper = null
  })

  afterEach(() => {
    if (wrapper) {
      wrapper.unmount()
      wrapper = null
    }
    document.body.innerHTML = ''
  })

  it('renders dialog when modelValue is true', async () => {
    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()
    await Promise.resolve()

    expect(document.body.innerHTML).toContain('Создать новый цикл роста')
  })

  it('hides dialog when modelValue is false', () => {
    wrapper = mount(CreateCycleDialog, {
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

  it('starts at step 1', async () => {
    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    await wrapper.vm.$nextTick()
    await Promise.resolve()

    expect(document.body.innerHTML).toContain('Шаг 1: Выбор зоны')
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

    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    const vm: any = wrapper.vm

    // Выбираем зону
    vm.formData.zone = zonesStore.zones[0]
    await vm.$nextTick()
    
    // Нажимаем "Далее"
    const nextButton = wrapper.findAll('button').find(btn => btn.text().includes('Далее'))
    if (nextButton) {
      await nextButton.trigger('click')
      await vm.$nextTick()
      
      // Должен быть шаг 2
      expect(wrapper.text()).toContain('Шаг 2')
    }
  })

  it('validates zone selection', async () => {
    wrapper = mount(CreateCycleDialog, {
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
      expect((wrapper.vm as any).errors.zone).toBeDefined()
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

    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Переходим на шаг 2
    const vm: any = wrapper.vm
    vm.currentStep = 2
    await vm.$nextTick()

    // Должны загрузиться пресеты
    expect(mockedAxios.get).toHaveBeenCalled()
  })

  it('validates preset selection', async () => {
    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Переходим на шаг 2 и пытаемся на шаг 3 без выбора пресета
    const vm: any = wrapper.vm
    vm.currentStep = 2
    await vm.$nextTick()

    const nextButton = wrapper.findAll('button').find(btn => btn.text().includes('Далее'))
    if (nextButton) {
      await nextButton.trigger('click')
      await vm.$nextTick()
      
      // Должна быть ошибка валидации
      expect(vm.errors.preset).toBeDefined()
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

    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Заполняем форму
    const vm: any = wrapper.vm
    vm.currentStep = 3
    vm.formData.zone = zonesStore.zones[0]
    vm.formData.preset = mockPreset as any
    vm.formData.plant_count = 10
    await vm.$nextTick()

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

    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Заполняем и создаём
    const vm: any = wrapper.vm
    vm.currentStep = 3
    vm.formData.zone = { id: 1 } as any
    vm.formData.preset = { id: 1 } as any

    await vm.createCycle()
    await vm.$nextTick()

    expect(wrapper.emitted('created')).toBeTruthy()
  })

  it('closes dialog when cancel is clicked', async () => {
    wrapper = mount(CreateCycleDialog, {
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
    wrapper = mount(CreateCycleDialog, {
      props: {
        modelValue: true,
      },
      global: {
        plugins: [createPinia()],
      },
    })

    // Заполняем форму
    const vm: any = wrapper.vm
    vm.currentStep = 2
    vm.formData.zone = { id: 1 } as any

    // Закрываем диалог
    await wrapper.setProps({ modelValue: false })
    await vm.$nextTick()
    await new Promise(resolve => setTimeout(resolve, 0))

    // Повторно открываем диалог, чтобы сработал reset
    await wrapper.setProps({ modelValue: true })
    await vm.$nextTick()
    await Promise.resolve()

    // Форма должна быть сброшена после повторного открытия
    expect(vm.currentStep).toBe(1)
    expect(vm.formData.zone).toBeNull()
  })
})

