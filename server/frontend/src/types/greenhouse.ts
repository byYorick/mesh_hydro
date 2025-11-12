export type GreenhouseStatus = 'draft' | 'active' | 'maintenance' | 'offline'

export interface GreenhouseSummary {
  id: number
  name: string
  code: string
  location?: string | null
  description?: string | null
  timezone?: string | null
  status: GreenhouseStatus
  root_node_id?: string | null
  root_node_mac?: string | null
  mesh_group?: string | null
  zone_count: number
  node_count: number
  active_cycle_count: number
  alert_count?: number
  image_url?: string | null
  created_at?: string
  updated_at?: string
}

export interface GreenhouseSettings {
  climate_profile?: string | null
  default_zone_type?: string | null
  preferred_units?: 'metric' | 'imperial'
  watering_rules?: Record<string, any> | null
  metadata?: Record<string, any> | null
}

export interface GreenhouseDetail extends GreenhouseSummary {
  tags?: string[]
  owner?: string | null
  notes?: string | null
  settings?: GreenhouseSettings | null
}

export interface GreenhouseZoneLink {
  id: number
  zone_id: number
  zone_name: string
  zone_mesh_id?: string | null
  zone_type?: string | null
  is_active: boolean
  is_available?: boolean
  current_cycle_id?: number | null
  assigned_at?: string
  metadata?: Record<string, any> | null
}

export interface GreenhouseNodeLink {
  node_id: string
  mac_address: string
  node_type: string
  is_root: boolean
  zone_id?: number | null
  zone_name?: string | null
  last_seen_at?: string | null
  online?: boolean
  metadata?: Record<string, any> | null
}

export type AutomationTriggerType =
  | 'schedule'
  | 'sensor_threshold'
  | 'manual_confirm'
  | 'webhook'

export interface GreenhouseAutomationTrigger {
  type: AutomationTriggerType
  config: Record<string, any>
}

export interface GreenhouseAutomationAction {
  action: string
  params?: Record<string, any>
  target_node_id?: string
  target_zone_id?: number
}

export interface GreenhouseAutomationRule {
  id: number
  greenhouse_id: number
  name: string
  description?: string | null
  enabled: boolean
  trigger: GreenhouseAutomationTrigger
  actions: GreenhouseAutomationAction[]
  created_at?: string
  updated_at?: string
}

export interface CreateGreenhousePayload {
  name: string
  code: string
  location?: string | null
  description?: string | null
  timezone?: string | null
  status?: GreenhouseStatus
  root_node_id?: string | null
  mesh_group?: string | null
  tags?: string[]
  settings?: GreenhouseSettings | null
}

export type UpdateGreenhousePayload = Partial<CreateGreenhousePayload>

export interface AttachZonePayload {
  zone_id: number
  metadata?: Record<string, any>
}

export interface AttachNodePayload {
  node_id: string
  as_root?: boolean
  metadata?: Record<string, any>
}


