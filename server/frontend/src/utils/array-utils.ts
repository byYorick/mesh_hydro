/**
 * Safe Array Utilities
 * Предотвращает ошибки при работе с undefined/null в массивах
 */

/**
 * Безопасная проверка наличия элемента в массиве
 * @param array - Массив (может быть undefined/null)
 * @param element - Элемент для поиска
 * @returns true если элемент найден, false в противном случае
 */
export function safeIncludes<T>(array: T[] | null | undefined, element: T): boolean {
  return Array.isArray(array) && array.includes(element)
}

/**
 * Безопасное получение длины массива
 * @param array - Массив (может быть undefined/null)
 * @returns Длина массива или 0
 */
export function safeLength<T>(array: T[] | null | undefined): number {
  return Array.isArray(array) ? array.length : 0
}

/**
 * Безопасное получение элемента по индексу
 * @param array - Массив (может быть undefined/null)
 * @param index - Индекс элемента
 * @returns Элемент или undefined
 */
export function safeGet<T>(array: T[] | null | undefined, index: number): T | undefined {
  return Array.isArray(array) && index >= 0 && index < array.length ? array[index] : undefined
}

/**
 * Безопасный поиск элемента
 * @param array - Массив (может быть undefined/null)
 * @param predicate - Функция поиска
 * @returns Найденный элемент или undefined
 */
export function safeFind<T>(
  array: T[] | null | undefined,
  predicate: (value: T, index: number) => boolean
): T | undefined {
  return Array.isArray(array) ? array.find(predicate) : undefined
}

/**
 * Безопасный фильтр массива
 * @param array - Массив (может быть undefined/null)
 * @param predicate - Функция фильтрации
 * @returns Новый массив с отфильтрованными элементами или пустой массив
 */
export function safeFilter<T>(
  array: T[] | null | undefined,
  predicate: (value: T, index: number) => boolean
): T[] {
  return Array.isArray(array) ? array.filter(predicate) : []
}

/**
 * Безопасное преобразование массива
 * @param array - Массив (может быть undefined/null)
 * @param mapper - Функция преобразования
 * @returns Новый массив с преобразованными элементами или пустой массив
 */
export function safeMap<T, U>(
  array: T[] | null | undefined,
  mapper: (value: T, index: number) => U
): U[] {
  return Array.isArray(array) ? array.map(mapper) : []
}

/**
 * Безопасное получение слайса массива
 * @param array - Массив (может быть undefined/null)
 * @param start - Начальный индекс
 * @param end - Конечный индекс
 * @returns Новый массив или пустой массив
 */
export function safeSlice<T>(
  array: T[] | null | undefined,
  start?: number,
  end?: number
): T[] {
  return Array.isArray(array) ? array.slice(start, end) : []
}

/**
 * Проверка что массив пустой
 * @param array - Массив (может быть undefined/null)
 * @returns true если массив пустой или undefined/null
 */
export function isEmpty<T>(array: T[] | null | undefined): boolean {
  return !Array.isArray(array) || array.length === 0
}

/**
 * Проверка что массив не пустой
 * @param array - Массив (может быть undefined/null)
 * @returns true если массив существует и не пустой
 */
export function isNotEmpty<T>(array: T[] | null | undefined): boolean {
  return Array.isArray(array) && array.length > 0
}

/**
 * Безопасное получение первого элемента
 * @param array - Массив (может быть undefined/null)
 * @returns Первый элемент или undefined
 */
export function first<T>(array: T[] | null | undefined): T | undefined {
  return safeGet(array, 0)
}

/**
 * Безопасное получение последнего элемента
 * @param array - Массив (может быть undefined/null)
 * @returns Последний элемент или undefined
 */
export function last<T>(array: T[] | null | undefined): T | undefined {
  if (!Array.isArray(array) || array.length === 0) return undefined
  return array[array.length - 1]
}
