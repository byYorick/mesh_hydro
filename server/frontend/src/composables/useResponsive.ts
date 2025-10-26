import { Ref, ComputedRef } from 'vue'
import { computed } from 'vue'
import { useDisplay } from 'vuetify'

/**
 * Composable для работы с responsive дизайном
 */
export function useResponsive(): any {
  const { xs, sm, md, lg, xl, mobile, tablet, desktop } = useDisplay()

  return {
    // Breakpoints
    xs,
    sm,
    md,
    lg,
    xl,
    
    // Device types
    isMobile: mobile,
    isTablet: tablet,
    isDesktop: desktop,
    
    // Вычисляемые значения
    columns: computed(() => {
      if (mobile.value) return 1
      if (sm.value) return 2
      if (md.value) return 3
      if (lg.value) return 4
      return 4
    }),
    
    cardCols: computed(() => {
      if (xs.value) return 12  // 1 колонка на mobile
      if (sm.value) return 6   // 2 колонки на tablet
      if (md.value) return 4   // 3 колонки на laptop
      if (lg.value) return 3   // 4 колонки на desktop
      return 3                  // 4 колонки на wide
    }),
    
    chartHeight: computed(() => {
      if (mobile.value) return 250
      if (tablet.value) return 300
      return 350
    }),
    
    dialogMaxWidth: computed(() => {
      if (mobile.value) return '100%'
      if (sm.value) return '600px'
      return '800px'
    }),
    
    dialogFullscreen: computed(() => mobile.value),
    
    // Spacing
    containerPadding: computed(() => {
      if (mobile.value) return 2  // 16px
      if (tablet.value) return 3  // 24px
      return 4                     // 32px
    }),
    
    cardPadding: computed(() => {
      if (mobile.value) return 3  // 24px
      return 4                     // 32px
    }),
  }
}

