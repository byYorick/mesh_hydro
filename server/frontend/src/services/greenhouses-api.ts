import { axios } from '@/services/api'
import type {
  AttachNodePayload,
  AttachZonePayload,
  CreateGreenhousePayload,
  GreenhouseAutomationRule,
  GreenhouseDetail,
  GreenhouseNodeLink,
  GreenhouseSummary,
  GreenhouseZoneLink,
  UpdateGreenhousePayload,
} from '@/types/greenhouse'

interface ListResponse {
  data?: GreenhouseSummary[]
  items?: GreenhouseSummary[]
  greenhouses?: GreenhouseSummary[]
  meta?: Record<string, any>
}

interface DetailResponse {
  data?: GreenhouseDetail
  greenhouse?: GreenhouseDetail
  meta?: Record<string, any>
}

interface ZonesResponse {
  data?: GreenhouseZoneLink[]
  zones?: GreenhouseZoneLink[]
  items?: GreenhouseZoneLink[]
}

interface NodesResponse {
  data?: GreenhouseNodeLink[]
  nodes?: GreenhouseNodeLink[]
  items?: GreenhouseNodeLink[]
}

interface AutomationResponse {
  data?: GreenhouseAutomationRule[]
  rules?: GreenhouseAutomationRule[]
  items?: GreenhouseAutomationRule[]
}

function unwrapList<T>(payload: { data?: T[]; items?: T[]; [key: string]: any } | T[] | undefined): T[] {
  if (!payload) {
    return []
  }

  if (Array.isArray(payload)) {
    return payload
  }

  if (Array.isArray(payload.data)) {
    return payload.data
  }

  if (Array.isArray(payload.items)) {
    return payload.items
  }

  return []
}

function unwrapDetail<T>(payload: { data?: T; [key: string]: any } | T | undefined): T | null {
  if (!payload) {
    return null
  }

  if (Array.isArray(payload)) {
    return payload[0] ?? null
  }

  if (typeof payload === 'object' && 'data' in payload) {
    return (payload as any).data ?? null
  }

  return payload as T
}

export const greenhousesApi = {
  async list(params: Record<string, any> = {}) {
    const response = (await axios.get('/greenhouses', { params })) as ListResponse | GreenhouseSummary[]
    const items = unwrapList<GreenhouseSummary>(response)
    const meta = (response as ListResponse)?.meta ?? {}
    return { greenhouses: items, meta }
  },

  async get(id: number, params: Record<string, any> = {}) {
    const response = (await axios.get(`/greenhouses/${id}`, { params })) as DetailResponse | GreenhouseDetail
    const greenhouse = unwrapDetail<GreenhouseDetail>(response)
    const meta = (response as DetailResponse)?.meta ?? {}
    return { greenhouse, meta }
  },

  async create(payload: CreateGreenhousePayload) {
    const response = (await axios.post('/greenhouses', payload)) as DetailResponse | GreenhouseDetail
    return unwrapDetail<GreenhouseDetail>(response)
  },

  async update(id: number, payload: UpdateGreenhousePayload) {
    const response = (await axios.put(`/greenhouses/${id}`, payload)) as DetailResponse | GreenhouseDetail
    return unwrapDetail<GreenhouseDetail>(response)
  },

  async remove(id: number) {
    await axios.delete(`/greenhouses/${id}`)
  },

  async listZones(id: number) {
    const response = (await axios.get(`/greenhouses/${id}/zones`)) as ZonesResponse | GreenhouseZoneLink[]
    return unwrapList<GreenhouseZoneLink>(response)
  },

  async attachZone(id: number, payload: AttachZonePayload) {
    const response = (await axios.post(`/greenhouses/${id}/zones`, payload)) as ZonesResponse | GreenhouseZoneLink
    const result = unwrapDetail<GreenhouseZoneLink>(response as any)
    return result
  },

  async detachZone(id: number, zoneId: number) {
    await axios.delete(`/greenhouses/${id}/zones/${zoneId}`)
  },

  async listNodes(id: number) {
    const response = (await axios.get(`/greenhouses/${id}/nodes`)) as NodesResponse | GreenhouseNodeLink[]
    return unwrapList<GreenhouseNodeLink>(response)
  },

  async attachNode(id: number, payload: AttachNodePayload) {
    const response = (await axios.post(`/greenhouses/${id}/nodes`, payload)) as NodesResponse | GreenhouseNodeLink
    return unwrapDetail<GreenhouseNodeLink>(response as any)
  },

  async detachNode(id: number, nodeId: string) {
    await axios.delete(`/greenhouses/${id}/nodes/${encodeURIComponent(nodeId)}`)
  },

  async listAutomationRules(id: number) {
    const response = (await axios.get(`/greenhouses/${id}/automation-rules`)) as AutomationResponse | GreenhouseAutomationRule[]
    return unwrapList<GreenhouseAutomationRule>(response)
  },

  async createAutomationRule(id: number, payload: Partial<GreenhouseAutomationRule>) {
    const response = (await axios.post(`/greenhouses/${id}/automation-rules`, payload)) as AutomationResponse | GreenhouseAutomationRule
    return unwrapDetail<GreenhouseAutomationRule>(response as any)
  },

  async updateAutomationRule(id: number, ruleId: number, payload: Partial<GreenhouseAutomationRule>) {
    const response = (await axios.put(`/greenhouses/${id}/automation-rules/${ruleId}`, payload)) as AutomationResponse | GreenhouseAutomationRule
    return unwrapDetail<GreenhouseAutomationRule>(response as any)
  },

  async deleteAutomationRule(id: number, ruleId: number) {
    await axios.delete(`/greenhouses/${id}/automation-rules/${ruleId}`)
  },
}

export default greenhousesApi


