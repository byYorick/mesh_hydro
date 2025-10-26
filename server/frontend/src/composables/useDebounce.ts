import { Ref, ComputedRef } from 'vue'
import { ref, watch } from 'vue'

/**
 * Composable для debounce функциональности
 */
export function useDebounce(value: any, delay: any = 300): any {
  const debouncedValue = ref<any>(value.value)

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
