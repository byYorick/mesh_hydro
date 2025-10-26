import { describe, it, expect, beforeEach } from 'vitest'
import { mount } from '@vue/test-utils'
import { createPinia } from 'pinia'
import NodeCard from '@/components/NodeCard.vue'

describe('NodeCard.vue', () => {
  let wrapper
  let pinia

  beforeEach(() => {
    pinia = createPinia()
  })

  it('renders node information correctly', () => {
    const node = {
      node_id: 'climate_001',
      node_type: 'climate',
      zone: 'Zone 1',
      online: true,
      last_seen_at: new Date(),
      last_telemetry: {
        data: {
          temperature: 24.5,
          humidity: 65.2,
          co2: 850
        }
      },
      metadata: {}
    }

    wrapper = mount(NodeCard, {
      props: { node },
      global: {
        plugins: [pinia]
      }
    })

    expect(wrapper.text()).toContain('climate_001')
    expect(wrapper.text()).toContain('24.5')
    expect(wrapper.text()).toContain('65')
    expect(wrapper.text()).toContain('850')
  })

  it('displays correct status color for online node', () => {
    const node = {
      node_id: 'test_001',
      node_type: 'climate',
      online: true,
      last_seen_at: new Date(),
      metadata: {}
    }

    wrapper = mount(NodeCard, {
      props: { node },
      global: { plugins: [pinia] }
    })

    // Status should be success (green) for online node
    expect(wrapper.vm.statusColor).toBe('success')
    expect(wrapper.vm.statusText).toBe('Online')
  })

  it('displays correct status color for offline node', () => {
    const node = {
      node_id: 'test_001',
      node_type: 'climate',
      online: false,
      last_seen_at: new Date(Date.now() - 120000), // 2 min ago
      metadata: {}
    }

    wrapper = mount(NodeCard, {
      props: { node },
      global: { plugins: [pinia] }
    })

    expect(wrapper.vm.isOnline).toBe(false)
  })

  it('calculates heap percentage correctly', () => {
    const node = {
      node_id: 'test_001',
      node_type: 'climate',
      online: true,
      last_seen_at: new Date(),
      metadata: {
        heap_total: 320000,
        heap_free: 160000
      }
    }

    wrapper = mount(NodeCard, {
      props: { node },
      global: { plugins: [pinia] }
    })

    expect(wrapper.vm.heapPercent).toBe(50)
  })

  it('emits command event when action clicked', async () => {
    const node = {
      node_id: 'climate_001',
      node_type: 'climate',
      online: true,
      last_seen_at: new Date(),
      metadata: {}
    }

    wrapper = mount(NodeCard, {
      props: { node },
      global: { plugins: [pinia] }
    })

    // Simulate quick pump action for ph_ec node
    node.node_type = 'ph_ec'
    await wrapper.setProps({ node })

    wrapper.vm.quickPump('ph_up')

    expect(wrapper.emitted()).toHaveProperty('command')
    expect(wrapper.emitted('command')[0][0]).toEqual({
      command: 'run_pump',
      params: { pump: 'ph_up', duration: 5 }
    })
  })

  it('shows correct node type icon', () => {
    const testCases = [
      { type: 'ph_ec', expected: 'mdi-flask' },
      { type: 'climate', expected: 'mdi-thermometer' },
      { type: 'relay', expected: 'mdi-electric-switch' },
      { type: 'water', expected: 'mdi-water' },
      { type: 'display', expected: 'mdi-monitor' },
      { type: 'root', expected: 'mdi-server-network' }
    ]

    testCases.forEach(({ type, expected }) => {
      const node = {
        node_id: `${type}_001`,
        node_type: type,
        online: true,
        last_seen_at: new Date(),
        metadata: {}
      }

      wrapper = mount(NodeCard, {
        props: { node },
        global: { plugins: [pinia] }
      })

      expect(wrapper.vm.nodeIcon).toBe(expected)
    })
  })

  it('calculates RSSI percentage correctly', () => {
    const node = {
      node_id: 'test_001',
      node_type: 'climate',
      online: true,
      last_seen_at: new Date(),
      metadata: {
        wifi_rssi: -60 // -30 = 100%, -90 = 0%
      }
    }

    wrapper = mount(NodeCard, {
      props: { node },
      global: { plugins: [pinia] }
    })

    // -60 dBm should be ~50% ((-60 + 90) * 100 / 60 = 50)
    expect(wrapper.vm.rssiPercent).toBe(50)
  })
})
