import { createRouter, createWebHistory } from 'vue-router'

const routes = [
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
    path: '/settings',
    name: 'Settings',
    component: () => import('@/views/Settings.vue'),
    meta: {
      title: 'Настройки',
      icon: 'mdi-cog',
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

