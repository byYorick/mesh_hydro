import { beforeEach, describe, expect, it, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useSettingsStore } from '@/stores/settings'

describe('useSettingsStore', () => {
  const originalLocation = window.location
  const originalNotification = global.Notification
  const createObjectURL = vi.fn(() => 'blob:mock')
  const revokeObjectURL = vi.fn()
  const clickSpy = vi.fn()

  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())

    localStorage.getItem = vi.fn().mockReturnValue(null)
    localStorage.setItem = vi.fn()
    localStorage.clear = vi.fn()

    Object.defineProperty(window, 'location', {
      configurable: true,
      value: { reload: vi.fn() }
    })

    global.Notification = {
      requestPermission: vi.fn()
    } as any

    vi.spyOn(window.URL, 'createObjectURL').mockImplementation(createObjectURL)
    vi.spyOn(window.URL, 'revokeObjectURL').mockImplementation(revokeObjectURL)
    vi.spyOn(document, 'createElement').mockReturnValue({
      click: clickSpy
    } as any)
  })

  afterAll(() => {
    Object.defineProperty(window, 'location', { value: originalLocation })
    global.Notification = originalNotification
  })

  it('updates theme and persists setting', () => {
    const store = useSettingsStore()
    store.setTheme('dark')
    expect(store.ui.theme).toBe('dark')
    expect(localStorage.setItem).toHaveBeenCalledWith('theme', 'dark')
  })

  it('toggles animations flag', () => {
    const store = useSettingsStore()
    const initial = store.ui.animationsEnabled
    store.toggleAnimations()
    expect(store.ui.animationsEnabled).toBe(!initial)
    expect(localStorage.setItem).toHaveBeenCalledWith('animationsEnabled', !initial)
  })

  it('enables desktop notifications and requests permission', () => {
    const store = useSettingsStore()
    store.setDesktopNotifications(true)
    expect(store.notifications.desktop).toBe(true)
    expect(global.Notification?.requestPermission).toHaveBeenCalled()
  })

  it('updates chart defaults and stores values', () => {
    const store = useSettingsStore()
    store.setChartDefaults({ pointSize: 5, fillArea: false })
    expect(store.charts.pointSize).toBe(5)
    expect(store.charts.fillArea).toBe(false)
    expect(localStorage.setItem).toHaveBeenCalledWith('chartPointSize', 5)
    expect(localStorage.setItem).toHaveBeenCalledWith('chartFillArea', false)
  })

  it('sets PID parameter and saves to localStorage', () => {
    const store = useSettingsStore()
    store.setPid('ph', 'kp', 0.5)
    expect(store.pid.ph.kp).toBe(0.5)
    expect(localStorage.setItem).toHaveBeenCalledWith('pid_ph_kp', 0.5)
  })

  it('exports settings via blob download', () => {
    const store = useSettingsStore()
    store.exportSettings()
    expect(createObjectURL).toHaveBeenCalled()
    expect(clickSpy).toHaveBeenCalled()
    expect(revokeObjectURL).toHaveBeenCalled()
  })

  it('imports settings and persists values', () => {
    const store = useSettingsStore()
    const payload = JSON.stringify({ ui: { theme: 'dark' } })
    const result = store.importSettings(payload)
    expect(result).toBe(true)
    expect(store.ui.theme).toBe('dark')
    expect(localStorage.setItem).toHaveBeenCalledWith('theme', 'dark')
  })

  it('resetToDefaults clears storage and reloads page', () => {
    const store = useSettingsStore()
    store.resetToDefaults()
    expect(localStorage.clear).toHaveBeenCalled()
    expect(window.location.reload).toHaveBeenCalled()
  })
})
