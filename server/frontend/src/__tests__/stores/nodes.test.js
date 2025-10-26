import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useNodesStore } from '@/stores/nodes'
import api from '@/services/api'

// Mock API
vi.mock('@/services/api', () => ({
  default: {
    getNodes: vi.fn(),
    getNode: vi.fn(),
    sendCommand: vi.fn(),
    updateNodeConfig: vi.fn(),
    deleteNode: vi.fn()
  }
}))

describe('useNodesStore', () => {
  let store

  beforeEach(() => {
    setActivePinia(createPinia())
    store = useNodesStore()
    vi.clearAllMocks()
  })

  it('initializes with empty nodes array', () => {
    expect(store.nodes).toEqual([])
    expect(store.selectedNode).toBeNull()
    expect(store.loading).toBe(false)
  })

  it('fetchNodes loads nodes from API', async () => {
    const mockNodes = [
      { node_id: 'climate_001', node_type: 'climate', online: true },
      { node_id: 'ph_ec_001', node_type: 'ph_ec', online: false }
    ]

    api.getNodes.mockResolvedValue(mockNodes)

    await store.fetchNodes()

    expect(api.getNodes).toHaveBeenCalled()
    expect(store.nodes).toEqual(mockNodes)
    expect(store.loading).toBe(false)
  })

  it('fetches single node and updates in array', async () => {
    store.nodes = [
      { node_id: 'climate_001', node_type: 'climate', online: true }
    ]

    const updatedNode = {
      node_id: 'climate_001',
      node_type: 'climate',
      online: false,
      last_seen_at: new Date()
    }

    api.getNode.mockResolvedValue(updatedNode)

    await store.fetchNode('climate_001')

    expect(api.getNode).toHaveBeenCalledWith('climate_001')
    expect(store.selectedNode).toEqual(updatedNode)
    expect(store.nodes[0].online).toBe(false)
  })

  it('nodesByType getter filters correctly', () => {
    store.nodes = [
      { node_id: 'climate_001', node_type: 'climate' },
      { node_id: 'climate_002', node_type: 'climate' },
      { node_id: 'ph_ec_001', node_type: 'ph_ec' }
    ]

    const climateNodes = store.nodesByType('climate')

    expect(climateNodes).toHaveLength(2)
    expect(climateNodes.every(n => n.node_type === 'climate')).toBe(true)
  })

  it('onlineNodes getter filters correctly', () => {
    store.nodes = [
      { node_id: 'node_001', online: true },
      { node_id: 'node_002', is_online: true },
      { node_id: 'node_003', online: false }
    ]

    const onlineNodes = store.onlineNodes

    expect(onlineNodes).toHaveLength(2)
  })

  it('offlineNodes getter filters correctly', () => {
    store.nodes = [
      { node_id: 'node_001', online: true },
      { node_id: 'node_002', online: false, is_online: false },
      { node_id: 'node_003', online: false }
    ]

    const offlineNodes = store.offlineNodes

    expect(offlineNodes).toHaveLength(2)
  })

  it('sendCommand calls API correctly', async () => {
    api.sendCommand.mockResolvedValue({ success: true })

    await store.sendCommand('climate_001', 'update_sensors', { force: true })

    expect(api.sendCommand).toHaveBeenCalledWith('climate_001', 'update_sensors', { force: true })
  })

  it('updateNodeRealtime adds new node if not exists', () => {
    const newNode = {
      node_id: 'new_001',
      node_type: 'climate',
      online: true
    }

    store.updateNodeRealtime(newNode)

    expect(store.nodes).toHaveLength(1)
    expect(store.nodes[0].node_id).toBe('new_001')
  })

  it('updateNodeRealtime updates existing node', () => {
    store.nodes = [
      { node_id: 'climate_001', node_type: 'climate', online: false }
    ]

    const updatedData = {
      node_id: 'climate_001',
      online: true,
      last_seen_at: new Date()
    }

    store.updateNodeRealtime(updatedData)

    expect(store.nodes).toHaveLength(1)
    expect(store.nodes[0].online).toBe(true)
    expect(store.nodes[0].last_seen_at).toBeDefined()
  })

  it('deleteNode removes node from store', async () => {
    store.nodes = [
      { node_id: 'climate_001', node_type: 'climate' },
      { node_id: 'ph_ec_001', node_type: 'ph_ec' }
    ]

    api.deleteNode.mockResolvedValue({ success: true })

    await store.deleteNode('climate_001')

    expect(store.nodes).toHaveLength(1)
    expect(store.nodes[0].node_id).toBe('ph_ec_001')
  })

  it('getNodeById returns correct node', () => {
    store.nodes = [
      { node_id: 'climate_001', node_type: 'climate' },
      { node_id: 'ph_ec_001', node_type: 'ph_ec' }
    ]

    const node = store.getNodeById('ph_ec_001')

    expect(node).toBeDefined()
    expect(node.node_id).toBe('ph_ec_001')
  })

  it('nodeCountByType returns correct counts', () => {
    store.nodes = [
      { node_id: 'climate_001', node_type: 'climate' },
      { node_id: 'climate_002', node_type: 'climate' },
      { node_id: 'ph_ec_001', node_type: 'ph_ec' }
    ]

    const counts = store.nodeCountByType

    expect(counts.climate).toBe(2)
    expect(counts.ph_ec).toBe(1)
  })
})
