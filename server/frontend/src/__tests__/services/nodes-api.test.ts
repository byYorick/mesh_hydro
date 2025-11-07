import { describe, expect, it, vi } from 'vitest'
import nodesApi from '@/services/nodes-api'

vi.mock('@/services/api', () => {
  return {
    default: {
      put: vi.fn(),
      getNodes: vi.fn(),
      getNode: vi.fn()
    }
  }
})

const apiMock = vi.mocked((await import('@/services/api')).default)

describe('nodes-api service', () => {
  it('updateNode proxies call to api', async () => {
    apiMock.put.mockResolvedValue({ ok: true })
    const result = await nodesApi.updateNode('node-1', { name: 'Test' })
    expect(apiMock.put).toHaveBeenCalledWith('/nodes/node-1', { name: 'Test' })
    expect(result).toEqual({ ok: true })
  })

  it('getZones returns unique zone list with fallback', async () => {
    apiMock.getNodes.mockResolvedValue([
      { zone: 'Z1' },
      { zone: 'Z2' },
      { zone: 'Z1' },
      { zone: null }
    ])
    const zones = await nodesApi.getZones()
    expect(zones).toEqual(['Z1', 'Z2'])

    apiMock.getNodes.mockResolvedValue([])
    expect(await nodesApi.getZones()).toEqual(['Zone 1', 'Zone 2', 'Main'])
  })

  it('getNodeStats aggregates stats', async () => {
    apiMock.getNodes.mockResolvedValue([
      { node_type: 'ph', online: true },
      { node_type: 'ph', online: false },
      { node_type: 'climate', online: true }
    ])
    const stats = await nodesApi.getNodeStats()
    expect(stats).toEqual({
      total: 3,
      online: 2,
      offline: 1,
      byType: { ph: 2, climate: 1 }
    })
  })

  it('validateNodeId returns false when node exists and true for 404', async () => {
    apiMock.getNode.mockResolvedValue({ id: 'exists' })
    await expect(nodesApi.validateNodeId('exists')).resolves.toBe(false)

    apiMock.getNode.mockRejectedValue({ response: { status: 404 } })
    await expect(nodesApi.validateNodeId('new')).resolves.toBe(true)

    const networkError = new Error('network')
    apiMock.getNode.mockRejectedValue(networkError)
    await expect(nodesApi.validateNodeId('err')).rejects.toBe(networkError)
  })
})
