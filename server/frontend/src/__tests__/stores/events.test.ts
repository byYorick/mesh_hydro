import { beforeEach, describe, expect, it, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useEventsStore } from '@/stores/events'

vi.mock('@/services/api', () => {
  return {
    default: {
      getEvents: vi.fn(),
      getEventStatistics: vi.fn(),
      resolveEvent: vi.fn(),
      resolveBulkEvents: vi.fn(),
      deleteEvent: vi.fn()
    }
  }
})

const apiMock = vi.mocked((await import('@/services/api')).default)

describe('useEventsStore', () => {
  beforeEach(() => {
    vi.clearAllMocks()
    setActivePinia(createPinia())
  })

  const sampleEvents = [
    { id: 1, level: 'critical', resolved_at: null, node_id: 'n1' },
    { id: 2, level: 'warning', resolved_at: null, node_id: 'n2' },
    { id: 3, level: 'emergency', resolved_at: '2023-01-01', node_id: 'n1' }
  ]

  it('derives critical events and counts', () => {
    const store = useEventsStore()
    store.events = sampleEvents

    expect(store.criticalEvents).toHaveLength(1)
    expect(store.criticalEvents[0].id).toBe(1)
    expect(store.criticalCount).toBe(1)
  })

  it('fetchEvents stores data and toggles loading', async () => {
    apiMock.getEvents.mockResolvedValue(sampleEvents)

    const store = useEventsStore()
    const events = await store.fetchEvents({ level: 'critical' })

    expect(events).toEqual(sampleEvents)
    expect(store.loading).toBe(false)
    expect(apiMock.getEvents).toHaveBeenCalledWith({ level: 'critical' })
  })

  it('fetchEvents stores error message on failure', async () => {
    apiMock.getEvents.mockRejectedValue(new Error('fail'))
    const store = useEventsStore()

    await expect(store.fetchEvents()).rejects.toThrow()
    expect(store.error).toBe('fail')
    expect(store.loading).toBe(false)
  })

  it('resolveEvent updates store entry', async () => {
    const now = new Date()
    apiMock.resolveEvent.mockResolvedValue({ ok: true })

    const store = useEventsStore()
    store.events = sampleEvents.map(e => ({ ...e }))

    await store.resolveEvent(1, 'tester')
    const updated = store.events.find(e => e.id === 1)
    expect(updated?.resolved_by).toBe('tester')
    expect(updated?.resolved_at).toBeInstanceOf(Date)
  })

  it('resolveBulk updates multiple events', async () => {
    apiMock.resolveBulkEvents.mockResolvedValue({ ok: true })

    const store = useEventsStore()
    store.events = sampleEvents.map(e => ({ ...e }))

    const ids = [1, 2]
    await store.resolveBulk(ids, 'batch')

    ids.forEach(id => {
      const event = store.events.find(e => e.id === id)
      expect(event?.resolved_by).toBe('batch')
      expect(event?.resolved_at).toBeInstanceOf(Date)
    })
  })

  it('deleteEvent removes entry', async () => {
    apiMock.deleteEvent.mockResolvedValue({})
    const store = useEventsStore()
    store.events = sampleEvents.map(e => ({ ...e }))

    await store.deleteEvent(2)
    expect(store.events.find(e => e.id === 2)).toBeUndefined()
  })

  it('addEventRealtime updates or prepends events', () => {
    const store = useEventsStore()
    store.events = [{ id: 1, level: 'info' } as any]

    store.addEventRealtime({ id: 1, level: 'warning' })
    expect(store.events[0]).toEqual({ id: 1, level: 'warning' })

    store.addEventRealtime({ id: 2, level: 'critical' })
    expect(store.events[0].id).toBe(2)
    expect(store.events).toHaveLength(2)
  })
})
