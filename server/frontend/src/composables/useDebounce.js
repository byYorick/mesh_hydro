import { ref, watch } from 'vue'

/**
 * Composable для debounce функциональности
 */
export function useDebounce(value, delay = 300) {
  const debouncedValue = ref(value.value)

  let timeoutId = null

  watch(value, (newValue) => {
    if (timeoutId) {
      clearTimeout(timeoutId)
    }

    timeoutId = setTimeout(() => {
      debouncedValue.value = newValue
    }, delay)
  }, { immediate: true })

  return debouncedValue
}
