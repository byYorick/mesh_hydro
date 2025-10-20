import { defineStore } from 'pinia'
import api from '@/services/api'

export const useNodesStore = defineStore('nodes', {
  state: () => ({
    nodes: [],
    selectedNode: null,
    loading: false,
    error: null,
  }),

  getters: {
    // Get nodes by type
    nodesByType: (state) => (type) => {
      return state.nodes.filter(node => node.node_type === type)
    },

    // Get online nodes
    onlineNodes: (state) => {
      return state.nodes.filter(node => node.online || node.is_online)
    },

    // Get offline nodes
    offlineNodes: (state) => {
      return state.nodes.filter(node => !node.online && !node.is_online)
    },

    // Get node by ID
    getNodeById: (state) => (nodeId) => {
      return state.nodes.find(node => node.node_id === nodeId)
    },

    // Node count by type
    nodeCountByType: (state) => {
      const counts = {}
      state.nodes.forEach(node => {
        counts[node.node_type] = (counts[node.node_type] || 0) + 1
      })
      return counts
    },
  },

  actions: {
    async fetchNodes(params = {}) {
      this.loading = true
      this.error = null
      
      try {
        this.nodes = await api.getNodes(params)
        return this.nodes
      } catch (error) {
        this.error = error.message
        console.error('Error fetching nodes:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async fetchNode(nodeId) {
      this.loading = true
      this.error = null
      
      try {
        this.selectedNode = await api.getNode(nodeId)
        
        // Update in nodes array if exists
        const index = this.nodes.findIndex(n => n.node_id === nodeId)
        if (index !== -1) {
          this.nodes[index] = { ...this.nodes[index], ...this.selectedNode }
        }
        
        return this.selectedNode
      } catch (error) {
        this.error = error.message
        console.error('Error fetching node:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async sendCommand(nodeId, command, params = {}) {
      try {
        const result = await api.sendCommand(nodeId, command, params)
        return result
      } catch (error) {
        console.error('Error sending command:', error)
        throw error
      }
    },

    async updateConfig(nodeId, config) {
      try {
        const result = await api.updateNodeConfig(nodeId, config)
        
        // Update in store
        await this.fetchNode(nodeId)
        
        return result
      } catch (error) {
        console.error('Error updating config:', error)
        throw error
      }
    },

    async deleteNode(nodeId) {
      try {
        await api.deleteNode(nodeId)
        
        // Remove from store
        this.nodes = this.nodes.filter(n => n.node_id !== nodeId)
        
        if (this.selectedNode?.node_id === nodeId) {
          this.selectedNode = null
        }
      } catch (error) {
        console.error('Error deleting node:', error)
        throw error
      }
    },

    // Update node in real-time (from WebSocket or fallback polling)
    updateNodeRealtime(nodeData) {
      const index = this.nodes.findIndex(n => n.node_id === nodeData.node_id)
      
      if (index !== -1) {
        // Используем online из nodeData, если указано, иначе сохраняем текущее значение
        this.nodes[index] = {
          ...this.nodes[index],
          ...nodeData,
          // НЕ переопределяем online если он уже есть в nodeData
          online: nodeData.online !== undefined ? nodeData.online : this.nodes[index].online,
        }
      } else {
        // Add new node
        this.nodes.push(nodeData)
      }
      
      // Update selected node if it's the same
      if (this.selectedNode?.node_id === nodeData.node_id) {
        this.selectedNode = {
          ...this.selectedNode,
          ...nodeData,
        }
      }
    },
  },
})

