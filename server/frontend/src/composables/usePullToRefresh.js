import { ref, onMounted, onUnmounted } from 'vue'

/**
 * Composable для Pull-to-Refresh на mobile
 */
export function usePullToRefresh(onRefresh, options = {}) {
  const {
    threshold = 80,
    resistance = 2.5,
    maxDistance = 120,
  } = options

  const isPulling = ref(false)
  const pullDistance = ref(0)
  const isRefreshing = ref(false)

  let startY = 0
  let currentY = 0
  let scrollable = null

  function onTouchStart(e) {
    if (window.scrollY > 0) return
    
    startY = e.touches[0].clientY
    scrollable = e.target.closest('.v-main__scroller') || window
  }

  function onTouchMove(e) {
    if (window.scrollY > 0 || isRefreshing.value) return
    
    currentY = e.touches[0].clientY
    const distance = (currentY - startY) / resistance
    
    if (distance > 0) {
      isPulling.value = true
      pullDistance.value = Math.min(distance, maxDistance)
      
      // Prevent default scroll if pulling
      if (pullDistance.value > 10) {
        e.preventDefault()
      }
    }
  }

  async function onTouchEnd() {
    if (!isPulling.value) return
    
    if (pullDistance.value >= threshold && !isRefreshing.value) {
      isRefreshing.value = true
      
      try {
        await onRefresh()
      } catch (error) {
        console.error('Refresh failed:', error)
      } finally {
        setTimeout(() => {
          isRefreshing.value = false
        }, 500)
      }
    }
    
    isPulling.value = false
    pullDistance.value = 0
  }

  onMounted(() => {
    document.addEventListener('touchstart', onTouchStart, { passive: true })
    document.addEventListener('touchmove', onTouchMove, { passive: false })
    document.addEventListener('touchend', onTouchEnd, { passive: true })
  })

  onUnmounted(() => {
    document.removeEventListener('touchstart', onTouchStart)
    document.removeEventListener('touchmove', onTouchMove)
    document.removeEventListener('touchend', onTouchEnd)
  })

  return {
    isPulling,
    pullDistance,
    isRefreshing,
  }
}

