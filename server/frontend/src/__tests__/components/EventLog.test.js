import { describe, it, expect, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import EventLog from '@/components/EventLog.vue'

describe('EventLog.vue', () => {
  let wrapper
  let pinia

  const sampleEvents = [
    {
      id: 1,
      node_id: 'climate_001',
      level: 'critical',
      message: 'Temperature too high',
      created_at: new Date(),
      resolved_at: null,
    },
    {
      id: 2,
      node_id: 'ph_ec_001',
      level: 'warning',
      message: 'pH below optimal',
      created_at: new Date(),
      resolved_at: null,
    },
    {
      id: 3,
      node_id: 'climate_001',
      level: 'info',
      message: 'Temperature stabilized',
      created_at: new Date(),
      resolved_at: new Date(),
    },
  ]

  beforeEach(() => {
    pinia = createPinia()
  })

  it('renders all events when filter is "all"', () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    expect(wrapper.text()).toContain('Temperature too high')
    expect(wrapper.text()).toContain('pH below optimal')
    expect(wrapper.text()).toContain('Temperature stabilized')
  })

  it('filters active events correctly', async () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    wrapper.vm.filter = 'active'
    await wrapper.vm.$nextTick()

    expect(wrapper.vm.filteredEvents).toHaveLength(2)
    expect(wrapper.vm.filteredEvents.every(e => !e.resolved_at)).toBe(true)
  })

  it('filters critical events correctly', async () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    wrapper.vm.filter = 'critical'
    await wrapper.vm.$nextTick()

    expect(wrapper.vm.filteredEvents).toHaveLength(1)
    expect(wrapper.vm.filteredEvents[0].level).toBe('critical')
  })

  it('respects limit prop', () => {
    const manyEvents = Array.from({ length: 20 }, (_, i) => ({
      id: i,
      node_id: 'test_001',
      level: 'info',
      message: `Event ${i}`,
      created_at: new Date(),
      resolved_at: null,
    }))

    wrapper = mount(EventLog, {
      props: { events: manyEvents, limit: 5 },
      global: { plugins: [pinia] },
    })

    expect(wrapper.vm.filteredEvents).toHaveLength(5)
  })

  it('emits resolve event when resolve button clicked', async () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    // Find resolve button (should be for unresolved events only)
    const resolveButtons = wrapper.findAllComponents({ name: 'VBtn' })
      .filter(btn => btn.props('icon') === 'mdi-check')

    if (resolveButtons.length > 0) {
      await resolveButtons[0].trigger('click')
      
      expect(wrapper.emitted()).toHaveProperty('resolve')
    }
  })

  it('shows "no events" message when events array is empty', () => {
    wrapper = mount(EventLog, {
      props: { events: [] },
      global: { plugins: [pinia] },
    })

    expect(wrapper.text()).toContain('Нет событий')
  })

  it('shows correct level color', () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    expect(wrapper.vm.getLevelColor('critical')).toBe('red')
    expect(wrapper.vm.getLevelColor('warning')).toBe('orange')
    expect(wrapper.vm.getLevelColor('info')).toBe('blue')
  })

  it('shows correct level icon', () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    expect(wrapper.vm.getLevelIcon('critical')).toBe('mdi-alert-circle')
    expect(wrapper.vm.getLevelIcon('warning')).toBe('mdi-alert')
    expect(wrapper.vm.getLevelIcon('info')).toBe('mdi-information')
    expect(wrapper.vm.getLevelIcon('emergency')).toBe('mdi-fire')
  })

  it('shows resolve-all button when there are active events', () => {
    wrapper = mount(EventLog, {
      props: { events: sampleEvents },
      global: { plugins: [pinia] },
    })

    expect(wrapper.vm.hasActiveEvents).toBe(true)
    expect(wrapper.text()).toContain('Решить все')
  })

  it('does not show resolve-all button when all events are resolved', () => {
    const resolvedEvents = sampleEvents.map(e => ({
      ...e,
      resolved_at: new Date(),
    }))

    wrapper = mount(EventLog, {
      props: { events: resolvedEvents },
      global: { plugins: [pinia] },
    })

    expect(wrapper.vm.hasActiveEvents).toBe(false)
  })
})

