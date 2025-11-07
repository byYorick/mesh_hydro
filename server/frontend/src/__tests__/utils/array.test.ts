import { describe, expect, it, vi } from 'vitest'
import {
  safeIncludes,
  safeFilter,
  safeMap,
  safeFind,
  safeLength,
  safeIsEmpty,
  safeFirst,
  safeLast,
  safeSlice,
  isArray,
  isNonEmptyArray
} from '@/utils/array'

vi.mock('@/utils/logger', () => {
  const logger = {
    debug: vi.fn(),
    warn: vi.fn(),
    error: vi.fn()
  }
  return { logger }
})

const logger = (await import('@/utils/logger')).logger

describe('array utils', () => {
  it('handles safeIncludes with invalid input', () => {
    expect(safeIncludes(null, 1)).toBe(false)
    expect(logger.debug).toHaveBeenCalled()

    expect(safeIncludes(123 as any, 1)).toBe(false)
    expect(logger.warn).toHaveBeenCalled()
  })

  it('returns includes result when array is valid', () => {
    expect(safeIncludes([1, 2, 3], 2)).toBe(true)
  })

  it('filters, maps and finds safely', () => {
    expect(safeFilter<number>(null, () => true)).toEqual([])
    expect(safeMap<number, number>(null, x => x * 2)).toEqual([])
    expect(safeFind<number>(null, () => true)).toBeUndefined()

    const arr = [1, 2, 3]
    expect(safeFilter(arr, n => n > 1)).toEqual([2, 3])
    expect(safeMap(arr, n => n * 2)).toEqual([2, 4, 6])
    expect(safeFind(arr, n => n === 3)).toBe(3)
  })

  it('computes length, emptiness and boundary elements', () => {
    expect(safeLength(null)).toBe(0)
    expect(safeIsEmpty(null)).toBe(true)

    const arr = [10, 20]
    expect(safeLength(arr)).toBe(2)
    expect(safeIsEmpty(arr)).toBe(false)
    expect(safeFirst(arr)).toBe(10)
    expect(safeLast(arr)).toBe(20)
  })

  it('slices arrays safely', () => {
    expect(safeSlice(null, 0, 1)).toEqual([])
    expect(safeSlice([1, 2, 3, 4], 1, 3)).toEqual([2, 3])
  })

  it('checks array type guards', () => {
    expect(isArray([1, 2])).toBe(true)
    expect(isArray('str')).toBe(false)
    expect(isNonEmptyArray([5])).toBe(true)
    expect(isNonEmptyArray([])).toBe(false)
  })
})
