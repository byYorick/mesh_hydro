import { defineStore } from 'pinia'
import api from '@/services/api'
import { nodeStatusManager } from '@/services/NodeStatusManager'
import type { Node } from '@/types/nodeStatus'

export const useNodesStore = defineStore('nodes', {
  state: () => ({
    nodes: [] as Node[],
    selectedNode: null as Node | null,
    loading: false,
    error: null as string | null,
  }),

  getters: {
    // Get nodes by type
    nodesByType: (state) => (type: string) => {
      return state.nodes.filter(node => node.node_type === type)
    },

    // Get online nodes
    onlineNodes: (state) => {
      return state.nodes.filter(node => node.online)
    },

    // Get offline nodes
    offlineNodes: (state) => {
      return state.nodes.filter(node => !node.online)
    },

    // Get node by ID
    getNodeById: (state) => (nodeId: string) => {
      return state.nodes.find(node => node.node_id === nodeId)
    },

    // Node count by type
    nodeCountByType: (state) => {
      const counts: Record<string, number> = {}
      state.nodes.forEach(node => {
        counts[node.node_type] = (counts[node.node_type] || 0) + 1
      })
      return counts
    },

    // Централизованная статистика через менеджер
    statusStats: (state) => {
      const stats = { online: 0, offline: 0, warning: 0, total: state.nodes.length }
      state.nodes.forEach(node => {
        const status = nodeStatusManager.calculateStatus(node)
        if (status.isOnline) {
          if (status.quality === 'poor') stats.warning++
          else stats.online++
        } else {
          stats.offline++
        }
      })
      return stats
    },
  },

  actions: {
    async fetchNodes(params = {}) {
      this.loading = true
      this.error = null
      
      try {
        this.nodes = await api.getNodes(params)
        
        // Обновить статусы через менеджер
        this.nodes.forEach(node => {
          nodeStatusManager.updateNodeStatus(node.node_id, node)
        })
        
        return this.nodes
      } catch (error: any) {
        this.error = error.message
        console.error('Error fetching nodes:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async fetchNode(nodeId) {
      if (!nodeId) {
        throw new Error('Node ID is required')
      }
      
      this.loading = true
      this.error = null
      
      try {
        this.selectedNode = await api.getNode(nodeId)
        
        // Update in nodes array if exists
        const index = this.nodes.findIndex(n => n.node_id === nodeId)
        if (index !== -1) {
          this.nodes[index] = { ...(this as any).nodes[index], ...(this as any).selectedNode }
        }
        
        return this.selectedNode
      } catch (error: any) {
        this.error = error.message
        console.error('Error fetching node:', error)
        throw error
      } finally {
        this.loading = false
      }
    },

    async sendCommand(nodeId, command, params = {}) {
      if (!nodeId) {
        throw new Error('Node ID is required')
      }
      
      try {
        const result = await api.sendCommand(nodeId, command, params)
        return result
      } catch (error: any) {
        console.error('Error sending command:', error)
        throw error
      }
    },

    async updateConfig(nodeId, config) {
      if (!nodeId) {
        throw new Error('Node ID is required')
      }
      
      try {
        const result = await api.updateNodeConfig(nodeId, config)
        
        // Update in store
        await this.fetchNode(nodeId)
        
        return result
      } catch (error: any) {
        console.error('Error updating config:', error)
        throw error
      }
    },

    async deleteNode(nodeId) {
      if (!nodeId) {
        throw new Error('Node ID is required')
      }
      
      try {
        await api.deleteNode(nodeId)
        
        // Remove from store
        this.nodes = this.nodes.filter(n => n.node_id !== nodeId)
        
        if (this.selectedNode?.node_id === nodeId) {
          this.selectedNode = null
        }
      } catch (error: any) {
        console.error('Error deleting node:', error)
        throw error
      }
    },

    // Update node in real-time (from WebSocket or fallback polling)
    updateNodeRealtime(nodeData: Partial<Node>) {
      if (!nodeData || !nodeData.node_id) {
        console.warn('Invalid node data for real-time update:', nodeData)
        return
      }
      
      const index = this.nodes.findIndex(n => n.node_id === nodeData.node_id)
      
      if (index !== -1) {
        // Обновляем узел с данными из nodeData
        this.nodes[index] = {
          ...this.nodes[index],
          ...nodeData,
        }
        
        // Уведомить менеджер об обновлении
        nodeStatusManager.updateNodeStatus(
          nodeData.node_id!,
          this.nodes[index]
        )
      } else {
        // Add new node
        this.nodes.push(nodeData as Node)
        if (nodeData.node_id) {
          nodeStatusManager.updateNodeStatus(nodeData.node_id, nodeData as Node)
        }
      }
      
      // Update selected node if it's the same
      if (this.selectedNode?.node_id === nodeData.node_id) {
        this.selectedNode = {
          ...this.selectedNode,
          ...nodeData,
        }
      }
    },

    // Update node status (for WebSocket events)
    updateNodeStatus(nodeId, isOnline) {
      if (!nodeId) {
        console.warn('Node ID is required for status update')
        return
      }
      
      const node = this.nodes.find(n => n.node_id === nodeId)
      if (node) {
        node.online = isOnline
        node.is_online = isOnline
        // Обновляем last_seen_at если узел онлайн
        if (isOnline) {
          node.last_seen_at = new Date().toISOString()
        }
      } else {
        console.warn(`Node ${nodeId} not found for status update`)
      }
    },
  },
})



