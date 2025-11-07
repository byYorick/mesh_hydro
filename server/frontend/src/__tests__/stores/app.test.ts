import { beforeEach, describe, expect, it, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useAppStore } from '@/stores/app'

vi.mock('@/services/api', () => {
  return {
    default: {
      getStatus: vi.fn(),
      getDashboardSummary: vi.fn(),
      health: vi.fn()
    }
  }
})

const toastMocks = {
  success: vi.fn(),
  error: vi.fn(),
  warning: vi.fn(),
  info: vi.fn()
}

vi.mock('@/composables/usePopup', () => {
  const popupManager = { toast: toastMocks }
  return {
    usePopup: () => popupManager,
    popupManager
  }
})

const apiMock = vi.mocked((await import('@/services/api')).default)

describe('useAppStore', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
    localStorage.getItem = vi.fn().mockReturnValue(null)
    localStorage.setItem = vi.fn()
  })

  it('switches theme and persists value', () => {
    const store = useAppStore()
    store.theme = 'light'

    store.toggleTheme()

    expect(store.theme).toBe('dark')
    expect(localStorage.setItem).toHaveBeenCalledWith('theme', 'dark')
  })

  it('fetches system status and updates connectivity flags', async () => {
    apiMock.getStatus.mockResolvedValue({
      database: 'ok',
      mqtt: 'connected'
    })

    const store = useAppStore()
    const status = await store.fetchSystemStatus()

    expect(status).toEqual({ database: 'ok', mqtt: 'connected' })
    expect(store.backendConnected).toBe(true)
    expect(store.mqttConnected).toBe(true)
  })

  it('handles fetchSystemStatus errors and marks backend disconnected', async () => {
    const error = new Error('boom')
    apiMock.getStatus.mockRejectedValue(error)

    const store = useAppStore()

    await expect(store.fetchSystemStatus()).rejects.toThrow(error)
    expect(store.backendConnected).toBe(false)
  })

  it('fetchDashboardSummary toggles loading and shows toast on failure', async () => {
    apiMock.getDashboardSummary.mockRejectedValue(new Error('fail'))

    const store = useAppStore()

    await expect(store.fetchDashboardSummary()).rejects.toThrow()
    expect(store.loading).toBe(false)
    await Promise.resolve()
    expect(toastMocks.error).toHaveBeenCalledWith('Ошибка загрузки данных')
  })

  it('healthCheck returns boolean and updates connected flag', async () => {
    apiMock.health.mockResolvedValue({})
    const store = useAppStore()
    await expect(store.healthCheck()).resolves.toBe(true)
    expect(store.connected).toBe(true)

    apiMock.health.mockRejectedValue(new Error('offline'))
    await expect(store.healthCheck()).resolves.toBe(false)
    expect(store.connected).toBe(false)
  })

  it('detectErrorType categorizes different errors', () => {
    const store = useAppStore()
    expect(store.detectErrorType({ response: { status: 500 } })).toBe('server')
    expect(store.detectErrorType({ response: { status: 422 } })).toBe('validation')
    expect(store.detectErrorType({ request: {} })).toBe('network')
    expect(store.detectErrorType({ message: 'critical failure' })).toBe('critical')
    expect(store.detectErrorType(null)).toBe('unknown')
  })

  it('getErrorMessage prefers server messages', () => {
    const store = useAppStore()
    const serverMessage = store.getErrorMessage({ response: { data: { message: 'Server says no' } } }, 'ctx')
    expect(serverMessage).toBe('Server says no')

    const fallbackMessage = store.getErrorMessage({ message: 'Local error' }, 'ctx')
    expect(fallbackMessage).toBe('Local error')
  })
})
