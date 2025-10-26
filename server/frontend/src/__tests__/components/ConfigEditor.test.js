import { describe, it, expect, beforeEach, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import ConfigEditor from '@/components/ConfigEditor.vue'

describe('ConfigEditor.vue', () => {
  let wrapper
  let pinia

  const sampleNode = {
    node_id: 'climate_001',
    node_type: 'climate',
    zone: 'Zone 1',
    config: {
      interval: 30,
      temp_threshold_min: 18,
      temp_threshold_max: 30
    }
  }

  beforeEach(() => {
    pinia = createPinia()
  })

  it('renders dialog activator slot', () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] },
      slots: {
        activator: '<button>Open Config</button>'
      }
    })

    expect(wrapper.html()).toContain('Open Config')
  })

  it('initializes config from node props', async () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    // Open dialog
    wrapper.vm.dialog = true
    await wrapper.vm.$nextTick()

    expect(wrapper.vm.config.interval).toBe(30)
    expect(wrapper.vm.config.zone).toBe('Zone 1')
  })

  it('validates JSON correctly', () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true

    // Valid JSON
    wrapper.vm.validateJson('{"test": "value"}')
    expect(wrapper.vm.jsonError).toBeNull()

    // Invalid JSON
    wrapper.vm.validateJson('{ invalid json }')
    expect(wrapper.vm.jsonError).toContain('Невалидный JSON')
  })

  it('resetConfig restores original values', () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true
    wrapper.vm.config.interval = 60 // Change value

    wrapper.vm.resetConfig()

    expect(wrapper.vm.config.interval).toBe(30) // Restored
  })

  it('saveConfig emits config-updated event', async () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true
    wrapper.vm.config.interval = 60

    await wrapper.vm.saveConfig()

    expect(wrapper.emitted()).toHaveProperty('config-updated')
    expect(wrapper.emitted('config-updated')[0][0].interval).toBe(60)
  })

  it('saveConfig uses JSON if edited', async () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true
    wrapper.vm.configJson = '{"interval": 90, "new_field": true}'
    wrapper.vm.validateJson(wrapper.vm.configJson)

    await wrapper.vm.saveConfig()

    expect(wrapper.emitted('config-updated')[0][0].interval).toBe(90)
    expect(wrapper.emitted('config-updated')[0][0].new_field).toBe(true)
  })

  it('renders pH/EC specific fields', async () => {
    const phEcNode = {
      ...sampleNode,
      node_type: 'ph_ec',
      config: {
        interval: 30,
        ph_threshold_min: 5.5,
        ph_threshold_max: 6.5
      }
    }

    wrapper = mount(ConfigEditor, {
      props: { node: phEcNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true
    await wrapper.vm.$nextTick()

    expect(wrapper.html()).toContain('pH мин')
    expect(wrapper.html()).toContain('pH макс')
  })

  it('renders Climate specific fields', async () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true
    await wrapper.vm.$nextTick()

    expect(wrapper.html()).toContain('Температура мин')
    expect(wrapper.html()).toContain('CO₂ макс')
  })

  it('closes dialog after successful save', async () => {
    wrapper = mount(ConfigEditor, {
      props: { node: sampleNode },
      global: { plugins: [pinia] }
    })

    wrapper.vm.dialog = true
    expect(wrapper.vm.dialog).toBe(true)

    await wrapper.vm.saveConfig()

    expect(wrapper.vm.dialog).toBe(false)
  })
})
