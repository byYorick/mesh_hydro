import { describe, it, expect, beforeEach, vi } from 'vitest'
import axios from 'axios'

vi.mock('axios', () => {
  const instance = {
    get: vi.fn(),
    post: vi.fn(),
    put: vi.fn(),
    delete: vi.fn(),
    interceptors: {
      request: { use: vi.fn(), eject: vi.fn() },
      response: { use: vi.fn(), eject: vi.fn() }
    }
  }

  const createMock = vi.fn(() => instance)

  return {
    default: {
      create: createMock,
      get: instance.get,
      post: instance.post,
      put: instance.put,
      delete: instance.delete,
      interceptors: instance.interceptors
    }
  }
})

describe('API Service', () => {
  let api

  beforeEach(async () => {
    vi.clearAllMocks()
    vi.resetModules()
    const module = await import('@/services/api')
    api = module.default
  })

  it('creates axios instance with correct config', async () => {
    expect(axios.create).toHaveBeenCalled()
    const config = axios.create.mock.calls[0][0]

    expect(config.timeout).toBe(15000)
    expect(config.headers['Content-Type']).toBe('application/json')
  })

  it('health() exposes callable method', () => {
    expect(api.health).toBeDefined()
    expect(typeof api.health).toBe('function')
  })

  it('getNodes() exposes callable method', () => {
    expect(api.getNodes).toBeDefined()
    expect(typeof api.getNodes).toBe('function')
  })

  it('sendCommand() exposes callable method', () => {
    expect(api.sendCommand).toBeDefined()
    expect(typeof api.sendCommand).toBe('function')
  })

  it('exportTelemetry() exposes callable method', () => {
    expect(api.exportTelemetry).toBeDefined()
    expect(typeof api.exportTelemetry).toBe('function')
  })
})
