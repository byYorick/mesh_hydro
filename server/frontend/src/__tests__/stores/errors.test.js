import { describe, it, expect, beforeEach, vi } from 'vitest'
import { setActivePinia, createPinia } from 'pinia'
import { useErrorsStore } from '@/stores/errors'
import axios from 'axios'

// Mock axios
vi.mock('axios', () => ({
  default: {
    create: vi.fn(() => ({
      get: vi.fn(),
      post: vi.fn(),
      delete: vi.fn(),
    })),
  },
}))

describe('useErrorsStore', () => {
  let store

  beforeEach(() => {
    setActivePinia(createPinia())
    store = useErrorsStore()
    vi.clearAllMocks()
  })

  it('initializes with empty state', () => {
    expect(store.errors).toEqual([])
    expect(store.statistics).toBeNull()
    expect(store.selectedError).toBeNull()
    expect(store.loading).toBe(false)
  })

  it('errorsByNode getter filters correctly', () => {
    store.errors = [
      { id: 1, node_id: 'climate_001', severity: 'high' },
      { id: 2, node_id: 'climate_001', severity: 'medium' },
      { id: 3, node_id: 'ph_ec_001', severity: 'critical' },
    ]

    const climateErrors = store.errorsByNode('climate_001')

    expect(climateErrors).toHaveLength(2)
    expect(climateErrors.every(e => e.node_id === 'climate_001')).toBe(true)
  })

  it('unresolvedErrors getter filters correctly', () => {
    store.errors = [
      { id: 1, resolved_at: null },
      { id: 2, resolved_at: new Date() },
      { id: 3, resolved_at: null },
    ]

    const unresolved = store.unresolvedErrors

    expect(unresolved).toHaveLength(2)
    expect(unresolved.every(e => !e.resolved_at)).toBe(true)
  })

  it('criticalErrors getter filters correctly', () => {
    store.errors = [
      { id: 1, severity: 'critical', resolved_at: null },
      { id: 2, severity: 'high', resolved_at: null },
      { id: 3, severity: 'critical', resolved_at: new Date() },
    ]

    const critical = store.criticalErrors

    expect(critical).toHaveLength(1)
    expect(critical[0].id).toBe(1)
  })

  it('errorsBySeverity getter filters correctly', () => {
    store.errors = [
      { id: 1, severity: 'critical' },
      { id: 2, severity: 'critical' },
      { id: 3, severity: 'medium' },
    ]

    const criticalErrors = store.errorsBySeverity('critical')

    expect(criticalErrors).toHaveLength(2)
  })

  it('errorsByType getter filters correctly', () => {
    store.errors = [
      { id: 1, error_type: 'hardware' },
      { id: 2, error_type: 'software' },
      { id: 3, error_type: 'hardware' },
    ]

    const hardwareErrors = store.errorsByType('hardware')

    expect(hardwareErrors).toHaveLength(2)
  })

  it('criticalCount returns correct count', () => {
    store.errors = [
      { id: 1, severity: 'critical', resolved_at: null },
      { id: 2, severity: 'high', resolved_at: null },
      { id: 3, severity: 'critical', resolved_at: new Date() },
    ]

    expect(store.criticalCount).toBe(1)
  })

  it('unresolvedCount returns correct count', () => {
    store.errors = [
      { id: 1, resolved_at: null },
      { id: 2, resolved_at: null },
      { id: 3, resolved_at: new Date() },
    ]

    expect(store.unresolvedCount).toBe(2)
  })

  it('addErrorRealtime adds to beginning', () => {
    const newError = {
      id: 1,
      node_id: 'climate_001',
      severity: 'high',
      message: 'Test error',
      occurred_at: new Date(),
    }

    store.addErrorRealtime(newError)

    expect(store.errors).toHaveLength(1)
    expect(store.errors[0]).toEqual(newError)
  })

  it('addErrorRealtime limits to 500 items', () => {
    // Fill with 500 errors
    store.errors = Array.from({ length: 500 }, (_, i) => ({
      id: i,
      node_id: 'test',
      message: `Error ${i}`,
    }))

    const newError = {
      id: 500,
      node_id: 'test',
      message: 'New error',
    }

    store.addErrorRealtime(newError)

    expect(store.errors).toHaveLength(500)
    expect(store.errors[0].id).toBe(500)
  })

  it('clearErrors clears all data', () => {
    store.errors = [{ id: 1 }]
    store.selectedError = { id: 1 }

    store.clearErrors()

    expect(store.errors).toEqual([])
    expect(store.selectedError).toBeNull()
  })
})

