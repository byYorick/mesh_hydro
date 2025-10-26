/**
 * Safe array utilities to prevent runtime errors
 * Replaces all unsafe .includes() calls throughout the application
 */

import { logger } from './logger'

/**
 * Safely check if an array includes a value
 * @param arr - Array to search in (can be undefined/null)
 * @param item - Item to search for
 * @param context - Optional context for logging
 * @returns true if array includes item, false otherwise
 */
export function safeIncludes<T>(
  arr: T[] | undefined | null, 
  item: T, 
  context?: { component?: string; operation?: string }
): boolean {
  try {
    if (!arr) {
      logger.debug('safeIncludes: array is null/undefined', context)
      return false
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeIncludes: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return false
    }

    return arr.includes(item)
  } catch (error: any) {
    logger.error('safeIncludes: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return false
  }
}

/**
 * Safely filter an array
 * @param arr - Array to filter (can be undefined/null)
 * @param predicate - Filter function
 * @param context - Optional context for logging
 * @returns filtered array or empty array if input is invalid
 */
export function safeFilter<T>(
  arr: T[] | undefined | null,
  predicate: (item: T, index: number) => boolean,
  context?: { component?: string; operation?: string }
): T[] {
  try {
    if (!arr) {
      logger.debug('safeFilter: array is null/undefined', context)
      return []
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeFilter: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return []
    }

    return arr.filter(predicate)
  } catch (error: any) {
    logger.error('safeFilter: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return []
  }
}

/**
 * Safely map an array
 * @param arr - Array to map (can be undefined/null)
 * @param mapper - Map function
 * @param context - Optional context for logging
 * @returns mapped array or empty array if input is invalid
 */
export function safeMap<T, U>(
  arr: T[] | undefined | null,
  mapper: (item: T, index: number) => U,
  context?: { component?: string; operation?: string }
): U[] {
  try {
    if (!arr) {
      logger.debug('safeMap: array is null/undefined', context)
      return []
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeMap: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return []
    }

    return arr.map(mapper)
  } catch (error: any) {
    logger.error('safeMap: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return []
  }
}

/**
 * Safely find an item in an array
 * @param arr - Array to search in (can be undefined/null)
 * @param predicate - Find function
 * @param context - Optional context for logging
 * @returns found item or undefined
 */
export function safeFind<T>(
  arr: T[] | undefined | null,
  predicate: (item: T, index: number) => boolean,
  context?: { component?: string; operation?: string }
): T | undefined {
  try {
    if (!arr) {
      logger.debug('safeFind: array is null/undefined', context)
      return undefined
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeFind: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return undefined
    }

    return arr.find(predicate)
  } catch (error: any) {
    logger.error('safeFind: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return undefined
  }
}

/**
 * Safely get array length
 * @param arr - Array to get length of (can be undefined/null)
 * @param context - Optional context for logging
 * @returns array length or 0 if invalid
 */
export function safeLength<T>(
  arr: T[] | undefined | null,
  context?: { component?: string; operation?: string }
): number {
  try {
    if (!arr) {
      logger.debug('safeLength: array is null/undefined', context)
      return 0
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeLength: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return 0
    }

    return arr.length
  } catch (error: any) {
    logger.error('safeLength: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return 0
  }
}

/**
 * Safely check if array is empty
 * @param arr - Array to check (can be undefined/null)
 * @param context - Optional context for logging
 * @returns true if array is empty or invalid, false otherwise
 */
export function safeIsEmpty<T>(
  arr: T[] | undefined | null,
  context?: { component?: string; operation?: string }
): boolean {
  return safeLength(arr, context) === 0
}

/**
 * Safely get first item from array
 * @param arr - Array to get first item from (can be undefined/null)
 * @param context - Optional context for logging
 * @returns first item or undefined
 */
export function safeFirst<T>(
  arr: T[] | undefined | null,
  context?: { component?: string; operation?: string }
): T | undefined {
  try {
    if (!arr) {
      logger.debug('safeFirst: array is null/undefined', context)
      return undefined
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeFirst: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return undefined
    }

    return arr[0]
  } catch (error: any) {
    logger.error('safeFirst: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return undefined
  }
}

/**
 * Safely get last item from array
 * @param arr - Array to get last item from (can be undefined/null)
 * @param context - Optional context for logging
 * @returns last item or undefined
 */
export function safeLast<T>(
  arr: T[] | undefined | null,
  context?: { component?: string; operation?: string }
): T | undefined {
  try {
    if (!arr) {
      logger.debug('safeLast: array is null/undefined', context)
      return undefined
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeLast: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return undefined
    }

    return arr[arr.length - 1]
  } catch (error: any) {
    logger.error('safeLast: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return undefined
  }
}

/**
 * Safely slice an array
 * @param arr - Array to slice (can be undefined/null)
 * @param start - Start index
 * @param end - End index (optional)
 * @param context - Optional context for logging
 * @returns sliced array or empty array if invalid
 */
export function safeSlice<T>(
  arr: T[] | undefined | null,
  start: number,
  end?: number,
  context?: { component?: string; operation?: string }
): T[] {
  try {
    if (!arr) {
      logger.debug('safeSlice: array is null/undefined', context)
      return []
    }

    if (!Array.isArray(arr)) {
      logger.warn('safeSlice: provided value is not an array', { 
        ...(context as any), 
        actualType: typeof arr,
        value: arr 
      })
      return []
    }

    return arr.slice(start, end)
  } catch (error: any) {
    logger.error('safeSlice: unexpected error', { 
      ...(context as any), 
      error: error instanceof Error ? error.message : String(error) 
    })
    return []
  }
}

/**
 * Type guard to check if value is an array
 * @param value - Value to check
 * @returns true if value is an array
 */
export function isArray<T>(value: unknown): value is T[] {
  return Array.isArray(value)
}

/**
 * Type guard to check if value is a non-empty array
 * @param value - Value to check
 * @returns true if value is a non-empty array
 */
export function isNonEmptyArray<T>(value: unknown): value is T[] {
  return Array.isArray(value) && value.length > 0
}

export default {
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
}
