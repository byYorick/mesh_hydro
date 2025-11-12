<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use Illuminate\Support\Arr;
use App\Models\Greenhouse;
use App\Models\Zone;
use App\Models\Node;
use App\Models\GrowthCycle;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\GrowthStage;
use Carbon\Carbon;

class GreenhouseZoneCycleSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $this->command->warn('🌱 Запуск демо-сидов теплиц, зон и циклов роста');

        $configurations = $this->greenhouseConfigurations();

        foreach ($configurations as $config) {
            $greenhouse = Greenhouse::updateOrCreate(
                ['code' => $config['greenhouse']['code']],
                $config['greenhouse']
            );

            $this->command->info("✓ Теплица создана/обновлена: {$greenhouse->name}");

            $rootNodes = [];
            foreach ($config['root_nodes'] as $rootData) {
                $root = $this->createRootNode($greenhouse, $rootData);
                $rootNodes[$root->node_id] = $root;
                $this->command->info("  • Root node {$root->node_id} → {$root->zone}");
            }

            foreach ($config['zones'] as $zoneDefinition) {
                $rootNode = $rootNodes[$zoneDefinition['root_node_id']] ?? null;
                if (!$rootNode) {
                    $this->command->warn("  ⚠️ Пропуск зоны {$zoneDefinition['name']} — root node {$zoneDefinition['root_node_id']} не найден");
                    continue;
                }

                $meshId = $zoneDefinition['mesh_network_id'];
                $assigned = [];
                foreach ($zoneDefinition['nodes'] as $nodeConfig) {
                    $node = $this->createChildNode($greenhouse, $rootNode, $meshId, $nodeConfig);
                    $assigned[$nodeConfig['role']] = $node->node_id;
                    $this->command->info("    · Узел {$node->node_id} ({$node->node_type}) привязан к {$meshId}");
                }

                $zoneAttributes = array_merge(
                    Arr::except($zoneDefinition, ['nodes', 'cycles']),
                    [
                        'greenhouse_id' => $greenhouse->id,
                        'assigned_nodes' => $assigned,
                        'mqtt_topic_prefix' => $zoneDefinition['mqtt_topic_prefix'] ?? "hydro/{$meshId}/",
                        'is_active' => $zoneDefinition['is_active'] ?? true,
                        'is_available' => $zoneDefinition['is_available'] ?? true,
                    ]
                );

                $zone = Zone::updateOrCreate(
                    ['mesh_network_id' => $meshId],
                    $zoneAttributes
                );

                foreach ($zoneDefinition['cycles'] ?? [] as $cycleConfig) {
                    $cycle = $this->createGrowthCycle($zone, $cycleConfig);
                    $this->command->info("      ↳ Цикл роста: {$cycle->name} ({$cycle->status})");
                }
            }
        }

        $this->command->info('🎉 Сидер теплиц успешно завершён');
    }

    private function greenhouseConfigurations(): array
    {
        $now = now();

        return [
            [
                'greenhouse' => [
                    'code' => 'GH-DEMO',
                    'name' => 'Демонстрационная теплица',
                    'location' => 'Санкт-Петербург, производственный комплекс',
                    'description' => 'Основная теплица для мультизонной демо-системы',
                    'timezone' => 'Europe/Moscow',
                    'status' => 'active',
                    'mesh_group' => 'demo_group',
                    'root_node_id' => 'root_demo_nft',
                    'settings' => [
                        'target_temperature' => 22,
                        'target_humidity' => 65,
                        'target_ph' => 6.0,
                        'target_ec' => 1.8,
                    ],
                ],
                'root_nodes' => [
                    [
                        'node_id' => 'root_demo_nft',
                        'mesh_network_id' => 'hydro_demo_nft',
                        'location' => 'Секция А',
                    ],
                    [
                        'node_id' => 'root_demo_dwc',
                        'mesh_network_id' => 'hydro_demo_dwc',
                        'location' => 'Секция Б',
                    ],
                ],
                'zones' => [
                    [
                        'name' => 'Demo NFT зона',
                        'description' => 'NFT контур для салатов и зелени. Полный набор датчиков и исполнительных устройств.',
                        'mesh_network_id' => 'hydro_demo_nft',
                        'root_node_id' => 'root_demo_nft',
                        'location' => 'Секция А',
                        'zone_type' => 'nft',
                        'reservoir_volume_liters' => 120,
                        'growing_area_m2' => 2.5,
                        'plant_capacity' => 24,
                        'notes' => 'Основная демонстрационная зона с NFT каналами',
                        'nodes' => [
                            ['role' => 'ph_node', 'node_id' => 'demo_ph_001', 'type' => 'ph_ec', 'config' => ['ph_target' => 6.1, 'ec_target' => 1.8, 'telemetry_interval' => 30]],
                            ['role' => 'climate_node', 'node_id' => 'demo_climate_001', 'type' => 'climate'],
                            ['role' => 'relay_node', 'node_id' => 'demo_relay_001', 'type' => 'relay'],
                            ['role' => 'water_node', 'node_id' => 'demo_water_001', 'type' => 'water'],
                        ],
                        'cycles' => [
                            [
                                'name' => 'Demo Salad 2025-01',
                                'preset_slug' => 'salat-nft-standart',
                                'status' => 'active',
                                'started_at' => $now->copy()->subDays(10),
                                'expected_harvest_at' => $now->copy()->addDays(20),
                                'plant_count' => 22,
                                'notes' => 'Показательный цикл для клиентов',
                            ],
                        ],
                    ],
                    [
                        'name' => 'Demo DWC зона',
                        'description' => 'DWC резервуар для томатов и огурцов.',
                        'mesh_network_id' => 'hydro_demo_dwc',
                        'root_node_id' => 'root_demo_dwc',
                        'location' => 'Секция Б',
                        'zone_type' => 'dwc',
                        'reservoir_volume_liters' => 180,
                        'growing_area_m2' => 3.6,
                        'plant_capacity' => 16,
                        'nodes' => [
                            ['role' => 'ph_node', 'node_id' => 'demo_ph_002', 'type' => 'ph_ec'],
                            ['role' => 'relay_node', 'node_id' => 'demo_relay_002', 'type' => 'relay'],
                            ['role' => 'water_node', 'node_id' => 'demo_water_002', 'type' => 'water'],
                        ],
                        'cycles' => [
                            [
                                'name' => 'Tomato Demo Early',
                                'preset_slug' => 'tomaty-drip',
                                'status' => 'planning',
                                'started_at' => $now->copy()->addDays(3),
                                'expected_harvest_at' => $now->copy()->addDays(95),
                                'plant_count' => 14,
                            ],
                        ],
                    ],
                ],
            ],
            [
                'greenhouse' => [
                    'code' => 'GH-NORTH',
                    'name' => 'Северный комплекс',
                    'location' => 'Новгородская область',
                    'description' => 'Опытно-промышленная теплица с тремя зонами.',
                    'timezone' => 'Europe/Moscow',
                    'status' => 'active',
                    'mesh_group' => 'north_group',
                    'root_node_id' => 'root_north_a',
                ],
                'root_nodes' => [
                    ['node_id' => 'root_north_a', 'mesh_network_id' => 'north_zone_a', 'location' => 'Корпус 1'],
                    ['node_id' => 'root_north_b', 'mesh_network_id' => 'north_zone_b', 'location' => 'Корпус 2'],
                    ['node_id' => 'root_north_c', 'mesh_network_id' => 'north_zone_c', 'location' => 'Корпус 3'],
                ],
                'zones' => [
                    [
                        'name' => 'Север A — Листовые',
                        'description' => 'Линия для зелени (руккола, шпинат).',
                        'mesh_network_id' => 'north_zone_a',
                        'root_node_id' => 'root_north_a',
                        'location' => 'Корпус 1',
                        'zone_type' => 'ebb_flow',
                        'reservoir_volume_liters' => 150,
                        'growing_area_m2' => 4.0,
                        'plant_capacity' => 60,
                        'nodes' => [
                            ['role' => 'ph_node', 'node_id' => 'north_ph_a', 'type' => 'ph_ec'],
                            ['role' => 'climate_node', 'node_id' => 'north_climate_a', 'type' => 'climate'],
                            ['role' => 'relay_node', 'node_id' => 'north_relay_a', 'type' => 'relay'],
                        ],
                        'cycles' => [
                            [
                                'name' => 'North Leafy Batch 12',
                                'preset_slug' => 'salat-nft-standart',
                                'status' => 'active',
                                'started_at' => $now->copy()->subDays(18),
                                'expected_harvest_at' => $now->copy()->addDays(12),
                                'plant_count' => 48,
                            ],
                        ],
                    ],
                    [
                        'name' => 'Север B — Ягодная',
                        'description' => 'Зона для круглогодичной клубники.',
                        'mesh_network_id' => 'north_zone_b',
                        'root_node_id' => 'root_north_b',
                        'location' => 'Корпус 2',
                        'zone_type' => 'dwc',
                        'reservoir_volume_liters' => 220,
                        'growing_area_m2' => 5.5,
                        'plant_capacity' => 18,
                        'nodes' => [
                            ['role' => 'ph_node', 'node_id' => 'north_ph_b', 'type' => 'ph_ec'],
                            ['role' => 'relay_node', 'node_id' => 'north_relay_b', 'type' => 'relay'],
                        ],
                        'cycles' => [
                            [
                                'name' => 'Strawberry Winter 2025',
                                'preset_slug' => 'klubnika-dwc-intensiv',
                                'status' => 'planning',
                                'started_at' => $now->copy()->addDays(14),
                                'expected_harvest_at' => $now->copy()->addDays(120),
                                'plant_count' => 18,
                            ],
                        ],
                    ],
                    [
                        'name' => 'Север C — Мониторинг',
                        'description' => 'Зона наблюдения с дисплей-узлом.',
                        'mesh_network_id' => 'north_zone_c',
                        'root_node_id' => 'root_north_c',
                        'location' => 'Корпус 3',
                        'zone_type' => 'other',
                        'reservoir_volume_liters' => 60,
                        'growing_area_m2' => 1.2,
                        'plant_capacity' => 12,
                        'nodes' => [
                            ['role' => 'display_node', 'node_id' => 'north_display_c', 'type' => 'display'],
                            ['role' => 'climate_node', 'node_id' => 'north_climate_c', 'type' => 'climate'],
                        ],
                    ],
                ],
            ],
            [
                'greenhouse' => [
                    'code' => 'GH-SOUTH',
                    'name' => 'Южный агрокластер',
                    'location' => 'Краснодарский край',
                    'description' => 'Площадка пилотных проектов с теплыми климатическими условиями.',
                    'timezone' => 'Europe/Moscow',
                    'status' => 'active',
                    'mesh_group' => 'south_group',
                    'root_node_id' => 'root_south_main',
                ],
                'root_nodes' => [
                    ['node_id' => 'root_south_main', 'mesh_network_id' => 'south_zone_main', 'location' => 'Главный павильон'],
                    ['node_id' => 'root_south_backup', 'mesh_network_id' => 'south_zone_backup', 'location' => 'Резервный павильон'],
                ],
                'zones' => [
                    [
                        'name' => 'Юг — Основная зона',
                        'description' => 'Микс культур (томаты, огурцы).',
                        'mesh_network_id' => 'south_zone_main',
                        'root_node_id' => 'root_south_main',
                        'location' => 'Главный павильон',
                        'zone_type' => 'drip',
                        'reservoir_volume_liters' => 260,
                        'growing_area_m2' => 7.2,
                        'plant_capacity' => 54,
                        'nodes' => [
                            ['role' => 'ph_node', 'node_id' => 'south_ph_main', 'type' => 'ph_ec'],
                            ['role' => 'water_node', 'node_id' => 'south_water_main', 'type' => 'water'],
                            ['role' => 'relay_node', 'node_id' => 'south_relay_main', 'type' => 'relay'],
                            ['role' => 'climate_node', 'node_id' => 'south_climate_main', 'type' => 'climate'],
                        ],
                        'cycles' => [
                            [
                                'name' => 'South Tomatoes Spring',
                                'preset_slug' => 'tomaty-drip',
                                'status' => 'active',
                                'started_at' => $now->copy()->subDays(28),
                                'expected_harvest_at' => $now->copy()->addDays(72),
                                'plant_count' => 40,
                                'notes' => 'Эксперимент с новым питательным раствором',
                            ],
                            [
                                'name' => 'South Cucumbers 2024-Q4',
                                'preset_slug' => 'ogurtsy-nft-fast',
                                'status' => 'harvested',
                                'started_at' => $now->copy()->subDays(160),
                                'expected_harvest_at' => $now->copy()->subDays(40),
                                'actual_harvest_at' => $now->copy()->subDays(42),
                                'ended_at' => $now->copy()->subDays(38),
                                'plant_count' => 30,
                                'harvest_weight_kg' => 118.4,
                                'rating' => 5,
                            ],
                        ],
                    ],
                    [
                        'name' => 'Юг — Резервная зона',
                        'description' => 'Площадка для новых сенсоров и алгоритмов.',
                        'mesh_network_id' => 'south_zone_backup',
                        'root_node_id' => 'root_south_backup',
                        'location' => 'Резервный павильон',
                        'zone_type' => 'other',
                        'reservoir_volume_liters' => 80,
                        'growing_area_m2' => 1.5,
                        'plant_capacity' => 8,
                        'nodes' => [
                            ['role' => 'sensor_node', 'node_id' => 'south_sensor_lab', 'type' => 'relay'],
                            ['role' => 'climate_node', 'node_id' => 'south_climate_backup', 'type' => 'climate'],
                        ],
                    ],
                ],
            ],
        ];
    }

    private function createRootNode(Greenhouse $greenhouse, array $data): Node
    {
        $metadata = array_merge([
            'mesh_network_id' => $data['mesh_network_id'],
            'firmware' => '2.0.0',
            'hardware' => 'ESP32-S3',
            'location' => $data['location'] ?? null,
        ], $data['metadata'] ?? []);

        return Node::updateOrCreate(
            ['node_id' => $data['node_id']],
            [
                'node_type' => 'root',
                'root_node_id' => $data['node_id'],
                'zone' => $data['mesh_network_id'],
                'greenhouse_id' => $greenhouse->id,
                'online' => true,
                'last_seen_at' => now()->subMinutes(rand(1, 5)),
                'metadata' => $metadata,
                'config' => [
                    'heartbeat_interval' => 10,
                    'mqtt_broker' => env('MQTT_BROKER_HOST', 'mosquitto'),
                    'mqtt_port' => 1883,
                ],
            ]
        );
    }

    private function createChildNode(Greenhouse $greenhouse, Node $rootNode, string $meshId, array $data): Node
    {
        $defaults = [
            'online' => $data['online'] ?? rand(0, 10) > 1,
            'last_seen_at' => $data['last_seen_at'] ?? now()->subMinutes(rand(5, 25)),
            'config' => $data['config'] ?? $this->defaultConfig($data['type']),
            'metadata' => array_merge([
                'firmware' => '2.0.0',
                'hardware' => 'ESP32',
            ], $data['metadata'] ?? []),
        ];

        return Node::updateOrCreate(
            ['node_id' => $data['node_id']],
            array_merge($defaults, [
                'node_type' => $data['type'],
                'root_node_id' => $rootNode->node_id,
                'zone' => $meshId,
                'greenhouse_id' => $greenhouse->id,
            ])
        );
    }

    private function createGrowthCycle(Zone $zone, array $config): GrowthCycle
    {
        $preset = GrowthPreset::where('slug', $config['preset_slug'])->with('stages')->first();

        if (!$preset) {
            throw new \RuntimeException('Growth preset not found for slug: ' . $config['preset_slug']);
        }

        $currentStage = $preset->stages->first()
            ?? GrowthStage::factory()->create([
                'preset_id' => $preset->id,
                'name' => 'Стартовая стадия',
                'order' => 1,
                'duration_days' => 7,
                'target_params' => [],
            ]);

        $cycle = GrowthCycle::updateOrCreate(
            ['zone_id' => $zone->id, 'name' => $config['name']],
            [
                'greenhouse_id' => $zone->greenhouse_id,
                'preset_id' => $preset->id,
                'culture_id' => $preset->culture_id,
                'current_stage_id' => $config['current_stage_id'] ?? $currentStage->id,
                'status' => $config['status'],
                'started_at' => $config['started_at'] ?? now(),
                'expected_harvest_at' => $config['expected_harvest_at'] ?? null,
                'actual_harvest_at' => $config['actual_harvest_at'] ?? null,
                'ended_at' => $config['ended_at'] ?? null,
                'plant_count' => $config['plant_count'] ?? 24,
                'harvest_weight_kg' => $config['harvest_weight_kg'] ?? null,
                'notes' => $config['notes'] ?? null,
                'rating' => $config['rating'] ?? null,
            ]
        );

        if ($cycle->status === 'active') {
            $zone->update([
                'current_cycle_id' => $cycle->id,
                'is_available' => false,
            ]);
        }

        return $cycle;
    }

    private function defaultConfig(string $type): array
    {
        return match ($type) {
            'ph_ec' => [
                'ph_target' => 6.0,
                'ec_target' => 1.8,
                'telemetry_interval' => 30,
            ],
            'climate' => [
                'temp_target' => 22,
                'humidity_target' => 65,
                'telemetry_interval' => 30,
            ],
            'relay' => [
                'relay_count' => 4,
                'telemetry_interval' => 60,
            ],
            'water' => [
                'min_level' => 20,
                'max_level' => 90,
                'telemetry_interval' => 30,
            ],
            'display' => [
                'refresh_interval' => 45,
                'pages' => ['overview', 'alerts'],
            ],
            default => ['telemetry_interval' => 30],
        };
    }
}

