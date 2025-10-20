import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useTelemetryStore } from '@/stores/telemetry'
import api from '@/services/api'

// Mock API
vi.mock('@/services/api', () => ({
  default: {
    getTelemetry: vi.fn(),
    getLatestTelemetry: vi.fn(),
    getAggregateTelemetry: vi.fn(),
    exportTelemetry: vi.fn(),
  },
}))

describe('useTelemetryStore', () => {
  let store

  beforeEach(() => {
    setActivePinia(createPinia())
    store = useTelemetryStore()
    vi.clearAllMocks()
  })

  it('initializes with empty state', () => {
    expect(store.telemetry).toEqual([])
    expect(store.latestTelemetry).toEqual([])
    expect(store.aggregatedData).toEqual({})
    expect(store.loading).toBe(false)
  })

  it('fetchTelemetry loads data from API', async () => {
    const mockData = {
      data: [
        { node_id: 'climate_001', data: { temp: 24.5 }, received_at: new Date() },
        { node_id: 'climate_001', data: { temp: 24.6 }, received_at: new Date() },
      ]
    }

    api.getTelemetry.mockResolvedValue(mockData)

    await store.fetchTelemetry({ node_id: 'climate_001', hours: 24 })

    expect(api.getTelemetry).toHaveBeenCalledWith({ node_id: 'climate_001', hours: 24 })
    expect(store.telemetry).toEqual(mockData.data)
    expect(store.loading).toBe(false)
  })

  it('fetchLatestTelemetry loads latest data', async () => {
    const mockLatest = [
      { node_id: 'climate_001', telemetry: { data: { temp: 24.5 } } },
      { node_id: 'ph_ec_001', telemetry: { data: { ph: 6.5 } } },
    ]

    api.getLatestTelemetry.mockResolvedValue(mockLatest)

    await store.fetchLatestTelemetry()

    expect(store.latestTelemetry).toEqual(mockLatest)
  })

  it('getTelemetryByNode getter filters correctly', () => {
    store.telemetry = [
      { node_id: 'climate_001', data: { temp: 24 } },
      { node_id: 'climate_001', data: { temp: 25 } },
      { node_id: 'ph_ec_001', data: { ph: 6.5 } },
    ]

    const climateTelemetry = store.getTelemetryByNode('climate_001')

    expect(climateTelemetry).toHaveLength(2)
    expect(climateTelemetry.every(t => t.node_id === 'climate_001')).toBe(true)
  })

  it('getLatestByNode getter returns correct data', () => {
    store.latestTelemetry = [
      { node_id: 'climate_001', telemetry: { data: { temp: 24.5 } } },
      { node_id: 'ph_ec_001', telemetry: { data: { ph: 6.5 } } },
    ]

    const latest = store.getLatestByNode('ph_ec_001')

    expect(latest).toBeDefined()
    expect(latest.node_id).toBe('ph_ec_001')
  })

  it('fetchAggregatedData stores with key', async () => {
    const mockAggregate = {
      data: [
        { time_bucket: '2025-10-20 10:00:00', avg: 24.5, min: 24.0, max: 25.0 },
      ]
    }

    api.getAggregateTelemetry.mockResolvedValue(mockAggregate)

    await store.fetchAggregatedData('climate_001', 'temp', 24, '1hour')

    const key = 'climate_001_temp_24_1hour'
    expect(store.aggregatedData[key]).toEqual(mockAggregate)
  })

  it('addTelemetryRealtime adds to beginning', () => {
    const newData = {
      node_id: 'climate_001',
      data: { temp: 25.0 },
      received_at: new Date(),
    }

    store.addTelemetryRealtime(newData)

    expect(store.telemetry).toHaveLength(1)
    expect(store.telemetry[0]).toEqual(newData)
  })

  it('addTelemetryRealtime limits to 1000 items', () => {
    // Fill with 1000 items
    store.telemetry = Array.from({ length: 1000 }, (_, i) => ({
      node_id: 'test',
      data: { value: i },
      received_at: new Date(),
    }))

    const newData = {
      node_id: 'test',
      data: { value: 1000 },
      received_at: new Date(),
    }

    store.addTelemetryRealtime(newData)

    expect(store.telemetry).toHaveLength(1000)
    expect(store.telemetry[0]).toEqual(newData)
  })

  it('addTelemetryRealtime updates latestTelemetry', () => {
    store.latestTelemetry = [
      { node_id: 'climate_001', data: { temp: 24.0 } },
    ]

    const newData = {
      node_id: 'climate_001',
      data: { temp: 25.0 },
      received_at: new Date(),
    }

    store.addTelemetryRealtime(newData)

    expect(store.latestTelemetry[0].data.temp).toBe(25.0)
  })

  it('addTelemetryRealtime adds new node to latestTelemetry', () => {
    store.latestTelemetry = []

    const newData = {
      node_id: 'ph_ec_001',
      data: { ph: 6.5 },
      received_at: new Date(),
    }

    store.addTelemetryRealtime(newData)

    expect(store.latestTelemetry).toHaveLength(1)
    expect(store.latestTelemetry[0].node_id).toBe('ph_ec_001')
  })

  it('clearTelemetry clears all data', () => {
    store.telemetry = [{ node_id: 'test', data: {} }]
    store.aggregatedData = { key: 'value' }

    store.clearTelemetry()

    expect(store.telemetry).toEqual([])
    expect(store.aggregatedData).toEqual({})
  })
})

