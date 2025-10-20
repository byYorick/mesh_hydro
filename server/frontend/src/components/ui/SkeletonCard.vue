<template>
  <v-card>
    <v-card-title>
      <v-skeleton-loader type="heading" width="60%"></v-skeleton-loader>
    </v-card-title>

    <v-card-subtitle>
      <v-skeleton-loader type="text" width="40%"></v-skeleton-loader>
    </v-card-subtitle>

    <v-card-text>
      <v-skeleton-loader type="paragraph"></v-skeleton-loader>
      
      <v-row dense class="mt-4">
        <v-col cols="6">
          <v-skeleton-loader type="chip"></v-skeleton-loader>
        </v-col>
        <v-col cols="6">
          <v-skeleton-loader type="chip"></v-skeleton-loader>
        </v-col>
      </v-row>
    </v-card-text>

    <v-card-actions>
      <v-skeleton-loader type="button"></v-skeleton-loader>
      <v-spacer></v-spacer>
      <v-skeleton-loader type="button"></v-skeleton-loader>
    </v-card-actions>
  </v-card>
</template>

<script setup>
defineProps({
  loading: {
    type: Boolean,
    default: true,
  },
})
</script>

<style scoped>
.v-skeleton-loader {
  background: transparent !important;
}
</style>

