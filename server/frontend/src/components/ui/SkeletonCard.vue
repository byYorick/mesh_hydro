<template>
  <v-card class="skeleton-card" elevation="2">
    <v-card-title class="d-flex align-center">
      <v-skeleton-loader
        type="avatar"
        width="40"
        height="40"
        class="mr-3"
      ></v-skeleton-loader>
      <div class="flex-grow-1">
        <v-skeleton-loader
          type="text"
          width="60%"
          class="mb-1"
        ></v-skeleton-loader>
        <v-skeleton-loader
          type="text"
          width="40%"
          height="16"
        ></v-skeleton-loader>
      </div>
      <v-skeleton-loader
        type="chip"
        width="80"
        height="24"
      ></v-skeleton-loader>
    </v-card-title>

    <v-card-text>
      <v-row>
        <v-col cols="6">
          <v-skeleton-loader
            type="text"
            width="80%"
            class="mb-2"
          ></v-skeleton-loader>
          <v-skeleton-loader
            type="text"
            width="60%"
            height="20"
          ></v-skeleton-loader>
        </v-col>
        <v-col cols="6">
          <v-skeleton-loader
            type="text"
            width="80%"
            class="mb-2"
          ></v-skeleton-loader>
          <v-skeleton-loader
            type="text"
            width="60%"
            height="20"
          ></v-skeleton-loader>
        </v-col>
      </v-row>

      <v-divider class="my-3"></v-divider>

      <div class="mb-2">
        <v-skeleton-loader
          type="text"
          width="30%"
          class="mb-1"
        ></v-skeleton-loader>
        <v-skeleton-loader
          type="rect"
          height="6"
          class="rounded"
        ></v-skeleton-loader>
      </div>

      <div class="mb-2">
        <v-skeleton-loader
          type="text"
          width="25%"
          class="mb-1"
        ></v-skeleton-loader>
        <v-skeleton-loader
          type="rect"
          height="6"
          class="rounded"
        ></v-skeleton-loader>
      </div>

      <v-skeleton-loader
        type="text"
        width="50%"
        height="14"
        class="mt-2"
      ></v-skeleton-loader>
    </v-card-text>

    <v-card-actions>
      <v-skeleton-loader
        type="button"
        width="80"
        height="32"
      ></v-skeleton-loader>
      <v-spacer></v-spacer>
      <v-skeleton-loader
        type="button"
        width="100"
        height="32"
      ></v-skeleton-loader>
    </v-card-actions>
  </v-card>
</template>

<script setup>
// Skeleton card для загрузки узлов
</script>

<style scoped>
.skeleton-card {
  animation: pulse 1.5s ease-in-out infinite;
}

@keyframes pulse {
  0%, 100% {
    opacity: 1;
  }
  50% {
    opacity: 0.7;
  }
}
</style>