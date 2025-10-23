<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\Node;

class NodeSeeder extends Seeder
{
    public function run(): void
    {
        $nodes = [
            [
                'node_id' => 'root_001',
                'node_type' => 'root',
                'zone' => 'Main',
                'mac_address' => 'AA:BB:CC:DD:EE:00',
                'online' => true,
                'last_seen_at' => now(),
                'config' => [
                    'mqtt_broker' => 'localhost',
                    'mqtt_port' => 1883,
                    'interval' => 60,
                ],
                'metadata' => [
                    'firmware' => '2.0.0',
                    'hardware' => 'ESP32-WROOM-32',
                    'heap_total' => 320000,
                    'heap_used' => 128000,
                    'free_heap' => 192000,
                    'flash_total' => 4194304,
                    'flash_used' => 2097152,
                    'psram_total' => 4194304,
                    'psram_used' => 1048576,
                    'uptime' => 86400 * 7, // 7 days
                    'cpu_freq' => 240,
                    'wifi_rssi' => -45,
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.100',
                    'boot_count' => 5,
                    'created_via' => 'seeder',
                    'created_at' => now()->subDays(30)->toDateTimeString(),
                ],
            ],
            [
                'node_id' => 'ph_001',
                'node_type' => 'ph',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:01',
                'online' => true,
                'last_seen_at' => now(),
                'config' => [
                    'ph_target' => 6.0,
                    'ph_min' => 5.5,
                    'ph_max' => 6.5,
                    'ph_cal_offset' => 0.0,
                    'autonomous_enabled' => true,
                    'pump_pid' => [
                        ['kp' => 0.5, 'ki' => 0.01, 'kd' => 0.05, 'setpoint' => 6.0],
                        ['kp' => 0.5, 'ki' => 0.01, 'kd' => 0.05, 'setpoint' => 6.0],
                    ],
                ],
                'metadata' => [
                    'firmware' => '2.0.0',
                    'hardware' => 'ESP32-C3',
                    'sensors' => ['pH Trema'],
                    'actuators' => ['pump_up', 'pump_down'],
                    'heap_total' => 320000,
                    'heap_used' => 145000,
                    'free_heap' => 175000,
                    'flash_total' => 4194304,
                    'flash_used' => 1310720,
                    'uptime' => 86400 * 2, // 2 days
                    'cpu_freq' => 160,
                    'wifi_rssi' => -55,
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.101',
                    'boot_count' => 5,
                    'created_via' => 'mqtt',
                    'created_at' => now()->subDays(10)->toDateTimeString(),
                ],
            ],
            [
                'node_id' => 'ec_001',
                'node_type' => 'ec',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:06',
                'online' => true,
                'last_seen_at' => now(),
                'config' => [
                    'ec_target' => 2.0,
                    'ec_min' => 1.5,
                    'ec_max' => 2.5,
                    'ec_cal_offset' => 0.0,
                    'autonomous_enabled' => true,
                    'pump_pid' => [
                        ['kp' => 0.3, 'ki' => 0.01, 'kd' => 0.02, 'setpoint' => 2.0],
                        ['kp' => 0.3, 'ki' => 0.01, 'kd' => 0.02, 'setpoint' => 2.0],
                        ['kp' => 0.3, 'ki' => 0.01, 'kd' => 0.02, 'setpoint' => 2.0],
                    ],
                ],
                'metadata' => [
                    'firmware' => '2.0.0',
                    'hardware' => 'ESP32-C3',
                    'sensors' => ['EC Trema'],
                    'actuators' => ['pump_a', 'pump_b', 'pump_c'],
                    'heap_total' => 320000,
                    'heap_used' => 152000,
                    'free_heap' => 168000,
                    'flash_total' => 4194304,
                    'flash_used' => 1376256,
                    'uptime' => 86400 * 2, // 2 days
                    'cpu_freq' => 160,
                    'wifi_rssi' => -52,
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.106',
                    'boot_count' => 4,
                    'created_via' => 'mqtt',
                    'created_at' => now()->subDays(10)->toDateTimeString(),
                ],
            ],
            [
                'node_id' => 'climate_001',
                'node_type' => 'climate',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:02',
                'online' => true,
                'last_seen_at' => now(),
                'config' => [
                    'interval' => 30,
                    'temp_threshold' => [18.0, 28.0],
                    'humidity_threshold' => [40.0, 80.0],
                    'co2_threshold' => [400, 1200],
                ],
                'metadata' => [
                    'firmware' => '1.0.0',
                    'hardware' => 'ESP32-WROVER',
                    'sensors' => ['DHT22', 'MHZ19'],
                    'heap_total' => 320000,
                    'heap_used' => 142000,
                    'free_heap' => 178000,
                    'flash_total' => 4194304,
                    'flash_used' => 1310720,
                    'psram_total' => 4194304,
                    'psram_used' => 524288,
                    'uptime' => 86400 * 5, // 5 days
                    'cpu_freq' => 240,
                    'wifi_rssi' => -52,
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.102',
                    'boot_count' => 8,
                    'created_via' => 'mqtt',
                    'created_at' => now()->subDays(20)->toDateTimeString(),
                ],
            ],
            [
                'node_id' => 'relay_001',
                'node_type' => 'relay',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:03',
                'online' => false,
                'last_seen_at' => now()->subMinutes(2),
                'config' => [
                    'relays' => [
                        ['id' => 1, 'name' => 'Window 1', 'gpio' => 16],
                        ['id' => 2, 'name' => 'Window 2', 'gpio' => 17],
                        ['id' => 3, 'name' => 'Fan', 'gpio' => 18],
                        ['id' => 4, 'name' => 'Heater', 'gpio' => 19],
                    ],
                ],
                'metadata' => [
                    'firmware' => '1.0.0',
                    'hardware' => 'ESP32-WROOM-32',
                    'heap_total' => 320000,
                    'heap_used' => 98000,
                    'free_heap' => 222000,
                    'flash_total' => 4194304,
                    'flash_used' => 1048576,
                    'uptime' => 86400 * 10, // 10 days
                    'cpu_freq' => 240,
                    'wifi_rssi' => -48,
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.103',
                    'boot_count' => 3,
                    'created_via' => 'web_ui',
                    'created_at' => now()->subDays(25)->toDateTimeString(),
                ],
            ],
            [
                'node_id' => 'water_001',
                'node_type' => 'water',
                'zone' => 'Zone 2',
                'mac_address' => 'AA:BB:CC:DD:EE:04',
                'online' => false,
                'last_seen_at' => now()->subMinutes(10),
                'config' => [
                    'interval' => 60,
                    'level_threshold' => [20.0, 90.0],
                ],
                'metadata' => [
                    'firmware' => '0.9.5',
                    'hardware' => 'ESP32-DevKitC',
                    'heap_total' => 320000,
                    'heap_used' => 265000, // High memory usage
                    'free_heap' => 55000,
                    'flash_total' => 4194304,
                    'flash_used' => 1835008,
                    'uptime' => 86400 * 1, // 1 day
                    'cpu_freq' => 160, // Lower freq
                    'wifi_rssi' => -78, // Weak signal
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.104',
                    'boot_count' => 25, // Many reboots
                    'created_via' => 'api',
                    'created_at' => now()->subDays(10)->toDateTimeString(),
                ],
            ],
            [
                'node_id' => 'display_001',
                'node_type' => 'display',
                'zone' => 'Zone 1',
                'mac_address' => 'AA:BB:CC:DD:EE:05',
                'online' => false,
                'last_seen_at' => now()->subMinutes(1),
                'config' => [
                    'brightness' => 80,
                    'pages' => ['overview', 'ph_ec', 'climate'],
                ],
                'metadata' => [
                    'firmware' => '1.0.1',
                    'hardware' => 'ESP32-WROVER',
                    'heap_total' => 320000,
                    'heap_used' => 198000,
                    'free_heap' => 122000,
                    'flash_total' => 4194304,
                    'flash_used' => 2621440,
                    'psram_total' => 4194304,
                    'psram_used' => 2097152,
                    'uptime' => 86400 * 15, // 15 days
                    'cpu_freq' => 240,
                    'wifi_rssi' => -55,
                    'wifi_ssid' => 'HydroMesh',
                    'ip_address' => '192.168.1.105',
                    'boot_count' => 4,
                    'created_via' => 'quick_add',
                    'created_at' => now()->subDays(18)->toDateTimeString(),
                ],
            ],
        ];

        foreach ($nodes as $nodeData) {
            Node::updateOrCreate(
                ['node_id' => $nodeData['node_id']],
                $nodeData
            );
        }

        $this->command->info('Created ' . count($nodes) . ' test nodes');
    }
}

