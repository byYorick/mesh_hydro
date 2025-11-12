export type NewNodeStatus = 'discovered' | 'configuring' | 'confirmed' | 'failed'

export interface NewNodeMetadata {
  chip_model?: string | null
  firmware_version?: string | null
  ip_address?: string | null
  requested_zone?: string | null
  requested_mesh_id?: string | null
  [key: string]: any
}

export interface NewNode {
  mac_address: string
  node_type: string
  is_root: boolean
  pin: string
  temp_mesh_id?: string | null
  status: NewNodeStatus
  metadata?: NewNodeMetadata | null
  discovered_at?: string | null
  last_heartbeat_at?: string | null
  configured_at?: string | null
  is_online?: boolean
}

export interface ConfigureRootNodePayload {
  node_id: string
  zone: string
  mesh_id: string
  mqtt_broker_host: string
  mqtt_broker_port: number
  wifi_ssid: string
  wifi_password: string
  root_node_id?: string
  zone_location?: string
  greenhouse_id?: number | null
  [key: string]: any
}

export interface ConfigureRegularNodePayload {
  node_id: string
  zone: string
  mesh_id: string
  root_node_id?: string
  greenhouse_id?: number | null
  metadata?: Record<string, any>
  [key: string]: any
}

export type ConfigureNewNodePayload = ConfigureRootNodePayload | ConfigureRegularNodePayload

export interface SetupHistoryEntry {
  timestamp: string
  mac_address: string
  node_id?: string
  node_type?: string
  zone?: string
  action: 'discovered' | 'configured' | 'removed' | 'failed'
  message?: string
}


