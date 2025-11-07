<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use Illuminate\Support\Arr;
use Illuminate\Support\Facades\DB;
use App\Models\Node;
use App\Models\Zone;
use App\Models\GrowthPreset;
use App\Models\GrowthStage;
use App\Models\GrowthCycle;
use App\Models\CycleStageHistory;

class GrowthScenarioSeeder extends Seeder
{
    public function run(): void
    {
        $zones = $this->scenarios();

        foreach ($zones as $scenario) {
            DB::transaction(function () use ($scenario) {
                $root = $this->upsertRootNode($scenario['root']);

                $assigned = [];
                foreach ($scenario['nodes'] as $node) {
                    $created = $this->upsertChildNode($root->node_id, $node);
                    $assigned[$node['role']] = $created->node_id;
                }

                $zone = Zone::updateOrCreate(
                    ['root_node_id' => $root->node_id],
                    array_merge(
                        Arr::except($scenario['zone'], ['assigned_nodes']),
                        [
                            'assigned_nodes' => $assigned,
                            'mesh_network_id' => $scenario['zone']['mesh_network_id'] ?? $root->metadata['mesh_network_id'] ?? 'DEV_MESH_' . strtoupper($root->node_id),
                            'mqtt_topic_prefix' => $scenario['zone']['mqtt_topic_prefix'] ?? 'dev/' . strtolower($root->node_id) . '/',
                            'is_active' => $scenario['zone']['is_active'] ?? true,
                            'is_available' => $scenario['zone']['is_available'] ?? true,
                        ]
                    )
                );

                foreach ($scenario['cycles'] as $cycleConfig) {
                    $cycle = $this->upsertCycle($zone, $cycleConfig);

                    if ($cycleConfig['status'] === 'active') {
                        $zone->update([
                            'current_cycle_id' => $cycle->id,
                            'is_available' => false,
                        ]);
                    }
                }
            });
        }

        $this->command?->info('🌱 GrowthScenarioSeeder: подготовлено ' . count($zones) . ' зон с циклами.');
    }

    private function scenarios(): array
    {
        $now = now();

        return [
            [
                'root' => [
                    'node_id' => 'root_dev_alpha',
                    'mesh_network_id' => 'DEV_ALPHA_MESH',
                    'location' => 'Лаборатория А',
                ],
                'zone' => [
                    'name' => 'Dev Alpha NFT',
                    'description' => 'Стенд NFT для отладки алгоритмов',
                    'zone_type' => 'nft',
                    'reservoir_volume_liters' => 120,
                    'growing_area_m2' => 2.4,
                    'plant_capacity' => 32,
                    'location' => 'Лаборатория А',
                    'notes' => 'Основная зона для QA',
                ],
                'nodes' => [
                    [
                        'role' => 'ph_node',
                        'node_id' => 'dev_alpha_ph',
                        'type' => 'ph_ec',
                        'config' => [
                            'ph_target' => 6.1,
                            'ec_target' => 1.9,
                            'telemetry_interval' => 45,
                        ],
                    ],
                    [
                        'role' => 'climate_node',
                        'node_id' => 'dev_alpha_climate',
                        'type' => 'climate',
                    ],
                    [
                        'role' => 'relay_node',
                        'node_id' => 'dev_alpha_relay',
                        'type' => 'relay',
                    ],
                    [
                        'role' => 'water_node',
                        'node_id' => 'dev_alpha_water',
                        'type' => 'water',
                    ],
                ],
                'cycles' => [
                    [
                        'name' => 'Alpha Salad Batch',
                        'preset_slug' => 'salat-nft-standart',
                        'status' => 'active',
                        'started_at' => $now->copy()->subDays(15),
                        'expected_harvest_at' => $now->copy()->addDays(12),
                        'plant_count' => 28,
                        'notes' => 'Тест стабильности pH',
                    ],
                    [
                        'name' => 'Alpha Archive Harvest',
                        'preset_slug' => 'salat-nft-standart',
                        'status' => 'harvested',
                        'started_at' => $now->copy()->subDays(65),
                        'expected_harvest_at' => $now->copy()->subDays(25),
                        'actual_harvest_at' => $now->copy()->subDays(22),
                        'ended_at' => $now->copy()->subDays(21),
                        'plant_count' => 26,
                        'harvest_weight_kg' => 13.8,
                        'rating' => 5,
                    ],
                ],
            ],
            [
                'root' => [
                    'node_id' => 'root_dev_beta',
                    'mesh_network_id' => 'DEV_BETA_MESH',
                    'location' => 'Лаборатория B',
                ],
                'zone' => [
                    'name' => 'Hydro Beta DWC',
                    'description' => 'DWC резервуар для клубники',
                    'zone_type' => 'dwc',
                    'reservoir_volume_liters' => 80,
                    'growing_area_m2' => 1.8,
                    'plant_capacity' => 18,
                    'location' => 'Лаборатория B',
                    'notes' => 'Испытания новых насосов',
                ],
                'nodes' => [
                    [
                        'role' => 'ph_node',
                        'node_id' => 'dev_beta_ph',
                        'type' => 'ph_ec',
                        'config' => [
                            'ph_target' => 5.9,
                            'ec_target' => 2.1,
                            'telemetry_interval' => 60,
                        ],
                    ],
                    [
                        'role' => 'climate_node',
                        'node_id' => 'dev_beta_climate',
                        'type' => 'climate',
                    ],
                    [
                        'role' => 'relay_node',
                        'node_id' => 'dev_beta_relay',
                        'type' => 'relay',
                        'online' => false,
                    ],
                ],
                'cycles' => [
                    [
                        'name' => 'Beta Strawberry Winter',
                        'preset_slug' => 'klubnika-dwc-intensiv',
                        'status' => 'planning',
                        'started_at' => $now->copy()->addDays(5),
                        'expected_harvest_at' => $now->copy()->addDays(95),
                        'plant_count' => 16,
                        'notes' => 'Запуск запланирован на следующую неделю',
                    ],
                ],
            ],
            [
                'root' => [
                    'node_id' => 'root_dev_gamma',
                    'mesh_network_id' => 'DEV_GAMMA_MESH',
                    'location' => 'Теплица демонстрационная',
                ],
                'zone' => [
                    'name' => 'Gamma Drip Greenhouse',
                    'description' => 'Капельное орошение для томатов черри',
                    'zone_type' => 'drip',
                    'reservoir_volume_liters' => 220,
                    'growing_area_m2' => 6.3,
                    'plant_capacity' => 48,
                    'location' => 'Теплица демонстрационная',
                    'notes' => 'Демо зона для гостей',
                ],
                'nodes' => [
                    [
                        'role' => 'ph_node',
                        'node_id' => 'dev_gamma_ph',
                        'type' => 'ph_ec',
                    ],
                    [
                        'role' => 'climate_node',
                        'node_id' => 'dev_gamma_climate',
                        'type' => 'climate',
                    ],
                    [
                        'role' => 'relay_node',
                        'node_id' => 'dev_gamma_relay',
                        'type' => 'relay',
                    ],
                    [
                        'role' => 'display_node',
                        'node_id' => 'dev_gamma_display',
                        'type' => 'display',
                    ],
                ],
                'cycles' => [
                    [
                        'name' => 'Gamma Cherry Tomatoes',
                        'preset_slug' => 'tomaty-drip',
                        'status' => 'active',
                        'started_at' => $now->copy()->subDays(40),
                        'expected_harvest_at' => $now->copy()->addDays(55),
                        'plant_count' => 40,
                        'notes' => 'Большой цикл для демонстраций',
                    ],
                    [
                        'name' => 'Gamma Early Harvest',
                        'preset_slug' => 'tomaty-drip',
                        'status' => 'harvested',
                        'started_at' => $now->copy()->subDays(140),
                        'expected_harvest_at' => $now->copy()->subDays(50),
                        'actual_harvest_at' => $now->copy()->subDays(52),
                        'ended_at' => $now->copy()->subDays(48),
                        'plant_count' => 42,
                        'harvest_weight_kg' => 62.4,
                        'rating' => 4,
                    ],
                ],
            ],
        ];
    }

    private function upsertRootNode(array $rootData): Node
    {
        $metadata = array_merge([
            'mesh_network_id' => $rootData['mesh_network_id'],
            'firmware' => '2.1.0-dev',
            'hardware' => 'ESP32-S3',
            'network' => [
                'ssid' => 'HydroMeshDev',
                'rssi' => -42,
            ],
        ], $rootData['metadata'] ?? []);

        return Node::updateOrCreate(
            ['node_id' => $rootData['node_id']],
            [
                'node_type' => 'root',
                'root_node_id' => $rootData['node_id'],
                'zone' => $rootData['location'] ?? null,
                'online' => true,
                'last_seen_at' => now()->subMinutes(rand(1, 5)),
                'config' => [
                    'mqtt_broker' => env('MQTT_BROKER_HOST', 'mosquitto'),
                    'mqtt_port' => 1883,
                    'heartbeat_interval' => 10,
                ],
                'metadata' => $metadata,
            ]
        );
    }

    private function upsertChildNode(string $rootNodeId, array $nodeData): Node
    {
        $defaults = [
            'online' => $nodeData['online'] ?? true,
            'last_seen_at' => $nodeData['last_seen_at'] ?? now()->subMinutes(rand(2, 25)),
            'config' => $nodeData['config'] ?? $this->defaultConfigForType($nodeData['type']),
            'metadata' => array_merge([
                'firmware' => '2.1.0-dev',
                'hardware' => 'ESP32',
            ], $nodeData['metadata'] ?? []),
        ];

        return Node::updateOrCreate(
            ['node_id' => $nodeData['node_id']],
            array_merge($defaults, [
                'node_type' => $nodeData['type'],
                'root_node_id' => $rootNodeId,
                'zone' => $nodeData['zone_label'] ?? null,
            ])
        );
    }

    private function upsertCycle(Zone $zone, array $cycleConfig): GrowthCycle
    {
        $preset = GrowthPreset::where('slug', $cycleConfig['preset_slug'])->with('stages')->first();

        if (!$preset) {
            throw new \RuntimeException('Growth preset not found for slug: ' . $cycleConfig['preset_slug']);
        }

        $currentStage = $preset->stages->first() ?? GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'name' => 'Стартовая стадия',
            'order' => 1,
            'duration_days' => 7,
            'target_params' => [],
        ]);

        $cycle = GrowthCycle::updateOrCreate(
            ['zone_id' => $zone->id, 'name' => $cycleConfig['name']],
            [
                'preset_id' => $preset->id,
                'culture_id' => $preset->culture_id,
                'current_stage_id' => $cycleConfig['current_stage_id'] ?? $currentStage->id,
                'status' => $cycleConfig['status'],
                'started_at' => $cycleConfig['started_at'] ?? now(),
                'expected_harvest_at' => $cycleConfig['expected_harvest_at'] ?? null,
                'actual_harvest_at' => $cycleConfig['actual_harvest_at'] ?? null,
                'ended_at' => $cycleConfig['ended_at'] ?? null,
                'plant_count' => $cycleConfig['plant_count'] ?? 24,
                'harvest_weight_kg' => $cycleConfig['harvest_weight_kg'] ?? null,
                'notes' => $cycleConfig['notes'] ?? null,
                'rating' => $cycleConfig['rating'] ?? null,
            ]
        );

        CycleStageHistory::updateOrCreate(
            ['cycle_id' => $cycle->id, 'stage_id' => $cycle->current_stage_id],
            [
                'started_at' => $cycle->started_at,
                'ended_at' => in_array($cycle->status, ['harvested', 'ended']) ? $cycle->ended_at ?? $cycle->actual_harvest_at : null,
                'actual_duration_days' => $cycle->started_at && $cycle->ended_at
                    ? $cycle->started_at->diffInDays($cycle->ended_at)
                    : null,
            ]
        );

        return $cycle;
    }

    private function defaultConfigForType(string $type): array
    {
        return match ($type) {
            'ph_ec' => [
                'ph_target' => 6.0,
                'ec_target' => 1.8,
                'telemetry_interval' => 60,
            ],
            'climate' => [
                'temp_target' => 22,
                'humidity_target' => 65,
            ],
            'relay' => [
                'channels' => 4,
                'default_mode' => 'auto',
            ],
            'water' => [
                'interval' => 120,
                'level_threshold' => [25, 85],
            ],
            'display' => [
                'brightness' => 75,
                'pages' => ['overview', 'zones'],
            ],
            default => [],
        };
    }
}


