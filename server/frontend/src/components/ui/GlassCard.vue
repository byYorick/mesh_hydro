<template>
  <v-card
    class="glass-card"
    v-bind="$attrs"
  >
    <slot></slot>
  </v-card>
</template>

<style scoped>
.glass-card {
  background: rgba(255, 255, 255, 0.1);
  backdrop-filter: blur(10px);
  -webkit-backdrop-filter: blur(10px);
  border: 1px solid rgba(255, 255, 255, 0.2);
  box-shadow: 0 8px 32px 0 rgba(31, 38, 135, 0.15);
}

.v-theme--dark .glass-card {
  background: rgba(26, 31, 58, 0.7);
  border: 1px solid rgba(255, 255, 255, 0.1);
}
</style>

