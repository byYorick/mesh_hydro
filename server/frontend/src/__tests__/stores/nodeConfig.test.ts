import { beforeEach, describe, expect, it, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useNodeConfigStore } from '@/stores/nodeConfig'

vi.mock('@/services/api', () => {
  return {
    default: {
      get: vi.fn(),
      put: vi.fn(),
      post: vi.fn()
    }
  }
})

const apiMock = vi.mocked((await import('@/services/api')).default)

describe('useNodeConfigStore', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
  })

  it('loads config and updates state', async () => {
    apiMock.get.mockResolvedValueOnce({ config: { interval: 30 } })
    const store = useNodeConfigStore()

    await store.loadConfig('node-1')

    expect(store.nodeId).toBe('node-1')
    expect(store.config).toEqual({ interval: 30 })
    expect(store.loading).toBe(false)
  })

  it('handles loadConfig errors and sets error message', async () => {
    apiMock.get.mockRejectedValueOnce({ response: { data: { message: 'Fail' } } })
    const store = useNodeConfigStore()

    await expect(store.loadConfig('node-2')).rejects.toBeDefined()
    expect(store.error).toBe('Fail')
    expect(store.loading).toBe(false)
  })

  it('saveConfig persists changes and reloads config', async () => {
    const store = useNodeConfigStore()
    const loadSpy = vi.spyOn(store, 'loadConfig').mockResolvedValue()
    apiMock.put.mockResolvedValue({ ok: true })

    const response = await store.saveConfig('node-3', { mode: 'auto' }, 'test')

    expect(response).toEqual({ ok: true })
    expect(apiMock.put).toHaveBeenCalledWith('/nodes/node-3/config', {
      config: { mode: 'auto' },
      comment: 'test'
    })
    expect(loadSpy).toHaveBeenCalledWith('node-3')
    expect(store.saving).toBe(false)
  })

  it('calibratePump triggers refresh of config and calibrations', async () => {
    const store = useNodeConfigStore()
    const loadConfigSpy = vi.spyOn(store, 'loadConfig').mockResolvedValue()
    const loadCalibrationsSpy = vi.spyOn(store, 'loadCalibrations').mockResolvedValue()
    apiMock.post.mockResolvedValue({ ok: true })

    const params = { pump_id: 'p1', duration_sec: 10, volume_ml: 20 }
    await store.calibratePump('node-4', params)

    expect(apiMock.post).toHaveBeenCalledWith('/nodes/node-4/pump/calibrate', params)
    expect(loadConfigSpy).toHaveBeenCalledWith('node-4')
    expect(loadCalibrationsSpy).toHaveBeenCalledWith('node-4')
    expect(store.calibrating).toBe(false)
  })

  it('runPumpManually forwards payload to API', async () => {
    apiMock.post.mockResolvedValue({ started: true })
    const store = useNodeConfigStore()
    const params = { pump_id: 'p2', duration_sec: 15 }

    const result = await store.runPumpManually('node-5', params)
    expect(result).toEqual({ started: true })
    expect(apiMock.post).toHaveBeenCalledWith('/nodes/node-5/pump/run', params)
  })

  it('reset clears state', () => {
    const store = useNodeConfigStore()
    store.nodeId = 'node-x'
    store.config = { mode: 'manual' }
    store.history = [{ id: 1 } as any]

    store.reset()

    expect(store.nodeId).toBeNull()
    expect(store.config).toBeNull()
    expect(store.history).toHaveLength(0)
    expect(store.calibrations).toHaveLength(0)
  })
})
