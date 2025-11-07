import { describe, expect, it, vi } from 'vitest'
import { mount } from '@vue/test-utils'
import { defineComponent, nextTick } from 'vue'
import { useOfflineMode } from '@/composables/useOfflineMode'

const popupMock = vi.hoisted(() => ({
  toast: {
    success: vi.fn(),
    warning: vi.fn(),
    error: vi.fn()
  }
}))

vi.mock('@/stores/app', () => ({
  useAppStore: () => ({})
}))

vi.mock('@/composables/usePopup', () => ({
  usePopup: () => popupMock,
  popupManager: popupMock
}))

function createOfflineComposable() {
  let composableReturn: ReturnType<typeof useOfflineMode>
  mount(defineComponent({
    setup() {
      composableReturn = useOfflineMode()
      return () => null
    }
  }))
  return composableReturn!
}

describe('useOfflineMode', () => {
  it('adds actions to offline queue when offline', async () => {
    Object.defineProperty(window.navigator, 'onLine', {
      configurable: true,
      value: false
    })

    const offline = createOfflineComposable()
    await nextTick()

    await offline.executeAction(
      { type: 'test' },
      () => Promise.resolve('online'),
      () => 'cached'
    )

    expect(offline.offlineQueue.value).toHaveLength(1)
  })

  it('switches to offline queue on network error', async () => {
    Object.defineProperty(window.navigator, 'onLine', {
      configurable: true,
      value: true
    })

    const offline = createOfflineComposable()
    const error = Object.assign(new Error('network'), { code: 'NETWORK_ERROR' })

    await offline.executeAction(
      { type: 'send_command' },
      () => Promise.reject(error),
      () => 'queued'
    )

    expect(offline.offlineQueue.value[0].type).toBe('send_command')
  })

  it('syncOfflineData clears queue on success', async () => {
    const offline = createOfflineComposable()
    offline.offlineQueue.value.push({ id: 'a', type: 'command' })

    await offline.syncOfflineData()

    expect(offline.offlineQueue.value).toHaveLength(0)
    expect(popupMock.toast.success).toHaveBeenCalledWith('Данные синхронизированы')
  })

  it('handles canExecuteOffline correctly', () => {
    const offline = createOfflineComposable()
    expect(offline.canExecuteOffline('view_nodes')).toBe(true)
    expect(offline.canExecuteOffline('mutate')).toBe(false)
  })
})
