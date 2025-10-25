import api from './api'

export default {
  // Update node basic fields
  async updateNode(nodeId, data) {
    return api.put(`/nodes/${nodeId}`, data)
  },

  // Get available zones from existing nodes
  async getZones() {
    const nodes = await api.getNodes()
    const zones = [...new Set(nodes.map(n => n.zone).filter(Boolean))]
    return zones.length > 0 ? zones : ['Zone 1', 'Zone 2', 'Main']
  },

  // Get node count by type
  async getNodeStats() {
    const nodes = await api.getNodes()
    
    const stats = {
      total: nodes.length,
      online: nodes.filter(n => n.online).length,
      offline: nodes.filter(n => !n.online).length,
      byType: {},
    }

    nodes.forEach(node => {
      stats.byType[node.node_type] = (stats.byType[node.node_type] || 0) + 1
    })

    return stats
  },

  // Validate node_id uniqueness
  async validateNodeId(nodeId) {
    try {
      await api.getNode(nodeId)
      return false // Node exists
    } catch (error) {
      if (error.response?.status === 404) {
        return true // Node doesn't exist - ID is available
      }
      throw error
    }
  },
}

