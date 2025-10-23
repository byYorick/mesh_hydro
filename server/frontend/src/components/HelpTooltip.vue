<template>
  <v-tooltip location="top" :text="text">
    <template #activator="{ props }">
      <v-icon v-bind="props" size="small" color="primary" class="ml-1">mdi-help-circle</v-icon>
    </template>
  </v-tooltip>
</template>

<script setup>
defineProps({
  text: {
    type: String,
    required: true,
  },
})
</script>


