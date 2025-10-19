<template>
  <v-menu>
    <template v-slot:activator="{ props }">
      <v-btn
        color="primary"
        v-bind="props"
        prepend-icon="mdi-plus-circle"
      >
        Быстрое добавление
      </v-btn>
    </template>

    <v-card min-width="300">
      <v-card-title class="text-subtitle-1">
        Быстрое добавление узла
      </v-card-title>
      <v-card-text>
        <v-form ref="form" v-model="valid" @submit.prevent="quickAdd">
          <v-select
            v-model="quickNode.node_type"
            :items="nodeTypes"
            item-title="label"
            item-value="value"
            label="Тип узла"
            variant="outlined"
            density="compact"
            :rules="[v => !!v || 'Выберите тип']"
          >
            <template v-slot:item="{ props, item }">
              <v-list-item v-bind="props">
                <template v-slot:prepend>
                  <v-icon :icon="item.raw.icon"></v-icon>
                </template>
              </v-list-item>
            </template>
          </v-select>

          <v-text-field
            v-model="quickNode.node_id"
            label="ID узла"
            variant="outlined"
            density="compact"
            :rules="[v => !!v || 'Введите ID', v => /^[a-z0-9_]+$/.test(v) || 'Только a-z, 0-9, _']"
            :prefix="quickNode.node_type ? quickNode.node_type + '_' : ''"
            class="mt-2"
          ></v-text-field>

          <v-select
            v-model="quickNode.zone"
            :items="['Zone 1', 'Zone 2', 'Main']"
            label="Зона"
            variant="outlined"
            density="compact"
            :rules="[v => !!v || 'Выберите зону']"
            class="mt-2"
          ></v-select>
        </v-form>
      </v-card-text>
      <v-card-actions>
        <v-spacer></v-spacer>
        <v-btn
          color="primary"
          size="small"
          :disabled="!valid"
          :loading="loading"
          @click="quickAdd"
        >
          Создать
        </v-btn>
      </v-card-actions>
    </v-card>
  </v-menu>
</template>

<script setup>
import { ref } from 'vue'

const emit = defineEmits(['node-created'])

const form = ref(null)
const valid = ref(false)
const loading = ref(false)

const quickNode = ref({
  node_type: null,
  node_id: '',
  zone: '',
})

const nodeTypes = [
  { value: 'ph_ec', label: 'pH/EC', icon: 'mdi-flask' },
  { value: 'climate', label: 'Climate', icon: 'mdi-thermometer' },
  { value: 'relay', label: 'Relay', icon: 'mdi-electric-switch' },
  { value: 'water', label: 'Water', icon: 'mdi-water' },
  { value: 'display', label: 'Display', icon: 'mdi-monitor' },
]

async function quickAdd() {
  if (!form.value.validate()) return

  loading.value = true

  try {
    const nodeData = {
      node_id: quickNode.value.node_id,
      node_type: quickNode.value.node_type,
      zone: quickNode.value.zone,
      config: {
        interval: 30,
      },
      metadata: {
        created_via: 'quick_add',
      },
    }

    emit('node-created', nodeData)
    
    // Reset form
    quickNode.value = {
      node_type: null,
      node_id: '',
      zone: '',
    }
  } finally {
    loading.value = false
  }
}
</script>

