import { axios } from '@/services/api'
import type { Node } from '@/types/nodeStatus'

export interface ZoneSummary {
  zone: string
  root_node_id?: string | null
  nodes_total: number
  nodes_online: number
  nodes_offline: number
  last_activity: string | null
}

export interface ZoneNode extends Node {
  last_telemetry?: {
    data: Record<string, any> | null
    received_at: string | null
  } | null
  last_data?: Record<string, any> | null
  metadata?: Record<string, any> | null
}

export interface ZoneStats {
  zone: string
  nodes_total: number
  nodes_online: number
  nodes_offline: number
  last_activity: string | null
  root_node: {
    node_id: string
    online: boolean
    last_seen_at: string | null
    metadata: Record<string, any> | null
  } | null
  nodes_by_type: Array<{
    type: string
    total: number
    online: number
  }>
  telemetry: {
    averages: Record<string, number>
    latest: Array<{
      node_id: string
      data: Record<string, any>
      received_at: string
    }>
  }
}

interface ZonesListResponse {
  data?: ZoneSummary[]
  zones?: ZoneSummary[]
  meta?: Record<string, any>
  [key: string]: any
}

interface ZoneNodesResponse {
  data?: ZoneNode[]
  nodes?: ZoneNode[]
  meta?: Record<string, any>
  success?: boolean
  message?: string
  [key: string]: any
}

interface ZoneStatsResponse {
  data?: ZoneStats
  success?: boolean
  message?: string
  [key: string]: any
}

function sanitizeZones(payload: ZonesListResponse | ZoneSummary[] | undefined): ZoneSummary[] {
  if (!payload) {
    return []
  }

  if (Array.isArray(payload)) {
    return payload
  }

  if (Array.isArray(payload.data)) {
    return payload.data
  }

  if (Array.isArray(payload.zones)) {
    return payload.zones
  }

  return []
}

function sanitizeZoneNodes(payload: ZoneNodesResponse | ZoneNode[] | undefined): ZoneNode[] {
  if (!payload) {
    return []
  }

  if (Array.isArray(payload)) {
    return payload
  }

  if (Array.isArray(payload.data)) {
    return payload.data
  }

  if (Array.isArray(payload.nodes)) {
    return payload.nodes
  }

  return []
}

export const zonesApi = {
  async list() {
    const response = (await axios.get('/zones')) as ZonesListResponse | ZoneSummary[]
    const zones = sanitizeZones(response)
    const meta = (response as ZonesListResponse)?.meta ?? {}

    return { zones, meta }
  },

  async getZoneNodes(zone: string) {
    const response = (await axios.get(`/zones/${encodeURIComponent(zone)}/nodes`)) as ZoneNodesResponse | ZoneNode[]
    const nodes = sanitizeZoneNodes(response)
    const meta = (response as ZoneNodesResponse)?.meta ?? {}

    return { nodes, meta }
  },

  async getZoneStats(zone: string) {
    const response = (await axios.get(`/zones/${encodeURIComponent(zone)}/stats`)) as ZoneStatsResponse | ZoneStats

    if (!response) {
      return null
    }

    if (!('data' in (response as ZoneStatsResponse)) && !('zone' in (response as ZoneStats))) {
      return null
    }

    if ((response as ZoneStatsResponse).data) {
      return (response as ZoneStatsResponse).data as ZoneStats
    }

    return response as ZoneStats
  },
}

export default zonesApi

