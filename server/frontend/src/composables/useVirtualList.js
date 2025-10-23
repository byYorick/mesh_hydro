import { ref, computed, onMounted, onUnmounted } from 'vue'

/**
 * Composable для виртуализации больших списков
 */
export function useVirtualList(items, options = {}) {
  const {
    itemHeight = 60,
    containerHeight = 400,
    overscan = 5,
    scrollElement = null
  } = options

  const scrollTop = ref(0)
  const containerRef = ref(null)

  // Вычисляем видимые элементы
  const visibleItems = computed(() => {
    if (!items.value || items.value.length === 0) return []

    const startIndex = Math.floor(scrollTop.value / itemHeight)
    const endIndex = Math.min(
      startIndex + Math.ceil(containerHeight / itemHeight) + overscan,
      items.value.length
    )

    const visibleStartIndex = Math.max(0, startIndex - overscan)
    const visibleEndIndex = Math.min(items.value.length, endIndex)

    return items.value.slice(visibleStartIndex, visibleEndIndex).map((item, index) => ({
      ...item,
      index: visibleStartIndex + index,
      top: (visibleStartIndex + index) * itemHeight
    }))
  })

  // Общая высота списка
  const totalHeight = computed(() => {
    return items.value ? items.value.length * itemHeight : 0
  })

  // Обработчик скролла
  const handleScroll = (event) => {
    scrollTop.value = event.target.scrollTop
  }

  onMounted(() => {
    const element = scrollElement || containerRef.value
    if (element) {
      element.addEventListener('scroll', handleScroll)
    }
  })

  onUnmounted(() => {
    const element = scrollElement || containerRef.value
    if (element) {
      element.removeEventListener('scroll', handleScroll)
    }
  })

  return {
    containerRef,
    visibleItems,
    totalHeight,
    scrollTop
  }
}
