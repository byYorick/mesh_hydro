<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\Zone;
use App\Models\Node;
use App\Models\Greenhouse;

class ZoneSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $greenhouse = Greenhouse::firstOrCreate(
            ['code' => 'GH-LEGACY'],
            [
                'name' => 'Legacy Demo Greenhouse',
                'location' => 'Основной комплекс',
                'description' => 'Базовый набор зон и узлов для dev окружения',
                'timezone' => 'Europe/Moscow',
                'status' => 'active',
                'mesh_group' => 'legacy_group',
                'root_node_id' => 'root_001',
            ]
        );

        // ⚠️ Важно: Сначала создаем Root Nodes, потом зоны

        $rootNodes = [
            [
                'node_id' => 'root_001',
                'mesh_id' => 'HYDRO1_ZONE1',
                'zone_name' => 'Зона 1 - NFT',
            ],
            [
                'node_id' => 'root_002',
                'mesh_id' => 'HYDRO1_ZONE2',
                'zone_name' => 'Зона 2 - DWC',
            ],
            [
                'node_id' => 'root_003',
                'mesh_id' => 'HYDRO1_ZONE3',
                'zone_name' => 'Теплица А - Капельный полив',
            ],
        ];

        foreach ($rootNodes as $rootData) {
            // Создаем или обновляем Root Node
            Node::updateOrCreate(
                ['node_id' => $rootData['node_id']],
                [
                    'node_type' => 'root',
                    'root_node_id' => $rootData['node_id'], // Root сам себя
                    'zone' => $rootData['zone_name'], // Deprecated field
                    'greenhouse_id' => $greenhouse->id,
                    'online' => true,
                    'last_seen_at' => now(),
                    'config' => [
                        'mqtt_broker' => env('MQTT_BROKER_HOST', '192.168.1.100'),
                        'mqtt_port' => 1883,
                        'heartbeat_interval' => 10,
                    ],
                    'metadata' => [
                        'mesh_network_id' => $rootData['mesh_id'],
                        'firmware' => '2.0.0',
                        'hardware' => 'ESP32-S3',
                        'heap_total' => 320000,
                        'free_heap' => 192000,
                        'uptime' => 86400,
                        'cpu_freq' => 240,
                        'wifi_ssid' => 'Yorick',
                        'wifi_rssi' => -45,
                    ],
                ]
            );
        }

        // Создаем зоны
        $zones = [
            [
                'name' => 'Зона 1 - NFT',
                'description' => 'NFT (Nutrient Film Technique) система для выращивания зелени и салатов. Проточная система с тонким слоем питательного раствора.',
                'root_node_id' => 'root_001',
                'mesh_network_id' => 'HYDRO1_ZONE1',
                'greenhouse_id' => $greenhouse->id,
                'mqtt_topic_prefix' => 'hydro/zone1/',
                'location' => 'Комната 1',
                'zone_type' => 'nft',
                'reservoir_volume_liters' => 100.0,
                'growing_area_m2' => 2.0,
                'plant_capacity' => 20,
                'assigned_nodes' => [
                    'ph_node' => 'ph_001',
                    'climate_node' => 'climate_001',
                    'relay_node' => 'relay_001',
                    'water_node' => 'water_001',
                ],
                'is_active' => true,
                'is_available' => true,
                'notes' => 'Основная зона для салатов и зелени',
            ],
            [
                'name' => 'Зона 2 - DWC',
                'description' => 'DWC (Deep Water Culture) система для выращивания крупных растений. Корни погружены в насыщенный кислородом питательный раствор.',
                'root_node_id' => 'root_002',
                'mesh_network_id' => 'HYDRO1_ZONE2',
                'greenhouse_id' => $greenhouse->id,
                'mqtt_topic_prefix' => 'hydro/zone2/',
                'location' => 'Комната 2',
                'zone_type' => 'dwc',
                'reservoir_volume_liters' => 50.0,
                'growing_area_m2' => 1.0,
                'plant_capacity' => 10,
                'assigned_nodes' => [
                    'ph_node' => 'ph_002',
                    'climate_node' => 'climate_002',
                    'relay_node' => 'relay_002',
                ],
                'is_active' => true,
                'is_available' => true,
                'notes' => 'Зона для экспериментов с DWC',
            ],
            [
                'name' => 'Теплица А - Капельный полив',
                'description' => 'Система капельного полива для теплицы. Большой объем для промышленного выращивания томатов, огурцов и клубники.',
                'root_node_id' => 'root_003',
                'mesh_network_id' => 'HYDRO1_ZONE3',
                'greenhouse_id' => $greenhouse->id,
                'mqtt_topic_prefix' => 'hydro/zone3/',
                'location' => 'Теплица секция A',
                'zone_type' => 'drip',
                'reservoir_volume_liters' => 200.0,
                'growing_area_m2' => 5.0,
                'plant_capacity' => 50,
                'assigned_nodes' => [
                    'ph_node' => 'ph_003',
                    'climate_node' => 'climate_003',
                    'relay_node' => 'relay_003',
                    'water_node' => 'water_003',
                ],
                'is_active' => true,
                'is_available' => true,
                'notes' => 'Производственная теплица',
            ],
        ];

        foreach ($zones as $zoneData) {
            // Создаем зону
            $zone = Zone::updateOrCreate(
                ['root_node_id' => $zoneData['root_node_id']],
                $zoneData
            );

            $this->command->info("✓ Зона создана: {$zone->name} ({$zone->root_node_id})");

            // Создаем узлы для зоны
            foreach ($zoneData['assigned_nodes'] as $role => $nodeId) {
                // Определяем тип узла из роли
                $nodeType = str_replace('_node', '', $role); // ph_node -> ph
                $nodeType = str_replace('_', '_', $nodeType); // для ph_ec

                // Если это ph_node, то type = ph_ec (полный узел pH+EC)
                if ($role === 'ph_node') {
                    $nodeType = 'ph_ec';
                }

                Node::updateOrCreate(
                    ['node_id' => $nodeId],
                    [
                        'node_type' => $nodeType,
                        'root_node_id' => $zoneData['root_node_id'],
                        'greenhouse_id' => $greenhouse->id,
                        'zone' => $zoneData['name'], // Deprecated field
                        'online' => rand(0, 10) > 2, // 80% онлайн
                        'last_seen_at' => now()->subMinutes(rand(1, 15)),
                        'config' => $this->getNodeConfig($nodeType),
                        'metadata' => [
                            'firmware' => '2.0.0',
                            'hardware' => 'ESP32',
                            'uptime' => rand(3600, 86400),
                            'free_heap' => rand(100000, 200000),
                        ],
                    ]
                );

                $this->command->info("  ✓ Узел: {$nodeId} ({$nodeType})");
            }
        }

        $this->command->info("\n🎉 ZoneSeeder завершен! Создано зон: " . count($zones));
        $this->command->info("📊 Root Nodes: " . Node::rootNodes()->count());
        $this->command->info("📊 Обычных узлов: " . Node::where('node_type', '!=', 'root')->count());
    }

    /**
     * Получить конфигурацию для типа узла
     */
    private function getNodeConfig(string $nodeType): array
    {
        return match($nodeType) {
            'ph_ec' => [
                'ph_target' => 6.0,
                'ph_min' => 5.5,
                'ph_max' => 6.5,
                'ec_target' => 1.8,
                'ec_min' => 1.5,
                'ec_max' => 2.1,
                'pump_ph_down_pin' => 25,
                'pump_ph_up_pin' => 26,
                'pump_ec_pin' => 27,
                'telemetry_interval' => 30,
            ],
            'climate' => [
                'temp_target' => 22.0,
                'temp_min' => 18.0,
                'temp_max' => 26.0,
                'humidity_target' => 65.0,
                'humidity_min' => 50.0,
                'humidity_max' => 80.0,
                'telemetry_interval' => 30,
            ],
            'relay' => [
                'light_pin' => 23,
                'fan_pin' => 22,
                'pump_pin' => 21,
                'heater_pin' => 19,
            ],
            'water' => [
                'level_sensor_pin' => 34,
                'flow_sensor_pin' => 35,
                'pump_pin' => 18,
                'min_level' => 20.0,
                'max_level' => 90.0,
            ],
            default => [],
        };
    }
}

