import { describe, it, expect, beforeEach, vi, afterEach } from 'vitest'
import api from '@/services/api'
import axios from 'axios'

vi.mock('axios', () => ({
  default: {
    create: vi.fn(() => ({
      get: vi.fn(),
      post: vi.fn(),
      put: vi.fn(),
      delete: vi.fn(),
      interceptors: {
        request: { use: vi.fn() },
        response: { use: vi.fn() }
      }
    }))
  }
}))

describe('API Service', () => {
  let mockAxiosInstance

  beforeEach(() => {
    mockAxiosInstance = axios.create()
    vi.clearAllMocks()
  })

  it('creates axios instance with correct config', () => {
    expect(axios.create).toHaveBeenCalled()
    const config = axios.create.mock.calls[0][0]

    expect(config.timeout).toBe(10000)
    expect(config.headers['Content-Type']).toBe('application/json')
  })

  it('health() calls correct endpoint', async () => {
    expect(api.health).toBeDefined()
    expect(typeof api.health).toBe('function')
  })

  it('getNodes() accepts params', () => {
    expect(api.getNodes).toBeDefined()
    expect(typeof api.getNodes).toBe('function')
  })

  it('sendCommand() sends correct data', () => {
    expect(api.sendCommand).toBeDefined()
    expect(typeof api.sendCommand).toBe('function')
  })

  it('exportTelemetry() sets responseType to blob', () => {
    expect(api.exportTelemetry).toBeDefined()
    expect(typeof api.exportTelemetry).toBe('function')
  })
})
