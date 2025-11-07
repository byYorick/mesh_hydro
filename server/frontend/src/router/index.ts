import { createRouter, createWebHistory } from 'vue-router'

const routes: any[] = [
  {
    path: '/',
    name: 'Dashboard',
    component: () => import('@/views/Dashboard.vue'),
    meta: {
      title: 'Dashboard',
      icon: 'mdi-view-dashboard',
    },
  },
  {
    path: '/nodes',
    name: 'Nodes',
    component: () => import('@/views/Nodes.vue'),
    meta: {
      title: 'Узлы',
      icon: 'mdi-access-point-network',
    },
  },
  {
    path: '/nodes/:nodeId',
    name: 'NodeDetail',
    component: () => import('@/views/NodeDetail.vue'),
    meta: {
      title: 'Детали узла',
      showInMenu: false,
    },
  },
  {
    path: '/telemetry',
    name: 'Telemetry',
    component: () => import('@/views/Telemetry.vue'),
    meta: {
      title: 'Телеметрия',
      icon: 'mdi-chart-line',
    },
  },
  {
    path: '/events',
    name: 'Events',
    component: () => import('@/views/Events.vue'),
    meta: {
      title: 'События',
      icon: 'mdi-alert-circle',
    },
  },
  {
    path: '/analytics',
    name: 'Analytics',
    component: () => import('@/views/Analytics.vue'),
    meta: {
      title: 'Аналитика',
      icon: 'mdi-chart-box',
    },
  },
  {
    path: '/errors',
    name: 'Errors',
    component: () => import('@/views/Errors.vue'),
    meta: {
      title: 'Ошибки',
      icon: 'mdi-bug',
    },
  },
  {
    path: '/documentation',
    name: 'Documentation',
    component: () => import('@/views/Documentation.vue'),
    meta: {
      title: 'Документация',
      icon: 'mdi-book-open-variant',
    },
  },
  {
    path: '/settings',
    name: 'Settings',
    component: () => import('@/views/Settings.vue'),
    meta: {
      title: 'Настройки',
      icon: 'mdi-cog',
    },
  },
  {
    path: '/zones',
    name: 'Zones',
    component: () => import('@/views/Zones.vue'),
    meta: {
      title: 'Зоны',
      icon: 'mdi-map-marker',
    },
  },
  {
    path: '/growth',
    name: 'GrowthPlanner',
    component: () => import('@/views/GrowthPlanner.vue'),
    meta: {
      title: 'Планировщик циклов',
      icon: 'mdi-sprout',
    },
  },
  {
    path: '/growth/presets',
    name: 'PresetLibrary',
    component: () => import('@/views/PresetLibrary.vue'),
    meta: {
      title: 'Библиотека пресетов',
      icon: 'mdi-book-open-variant',
    },
  },
  {
    path: '/growth/cycles/:id',
    name: 'CycleDetail',
    component: () => import('@/views/CycleDetail.vue'),
    meta: {
      title: 'Детали цикла',
      showInMenu: false,
    },
  },
]

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes,
})

// Navigation guard
router.beforeEach((to, from, next) => {
  // Set page title
  document.title = to.meta.title 
    ? `${to.meta.title} - Mesh Hydro System` 
    : 'Mesh Hydro System'
  
  next()
})

export default router

