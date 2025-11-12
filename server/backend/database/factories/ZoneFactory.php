<?php

namespace Database\Factories;

use App\Models\Zone;
use App\Models\Node;
use Illuminate\Database\Eloquent\Factories\Factory;

class ZoneFactory extends Factory
{
    protected $model = Zone::class;

    public function definition(): array
    {
        // Создаем уникальный root_node_id для каждой зоны
        $rootNodeId = 'root_test_' . $this->faker->unique()->numberBetween(1000, 9999);
        $zoneNumber = $this->faker->unique()->numberBetween(1, 999);

        // Создаем Root Node перед созданием зоны
        Node::firstOrCreate(
            ['node_id' => $rootNodeId],
            [
                'node_type' => 'root',
                'root_node_id' => $rootNodeId,
                'zone' => 'TEST_MESH_' . $zoneNumber,
                'online' => true,
                'last_seen_at' => now(),
                'metadata' => [],
            ]
        );

        return [
            'name' => $this->faker->words(3, true) . ' Zone',
            'description' => $this->faker->sentence(),
            'root_node_id' => $rootNodeId,
            'mesh_network_id' => 'TEST_MESH_' . $zoneNumber,
            'mqtt_topic_prefix' => 'test/zone' . $zoneNumber . '/',
            'location' => $this->faker->optional()->words(2, true),
            'zone_type' => $this->faker->randomElement(['nft', 'dwc', 'ebb_flow', 'drip', 'other']),
            'reservoir_volume_liters' => $this->faker->randomFloat(2, 10, 500),
            'growing_area_m2' => $this->faker->randomFloat(2, 0.5, 20),
            'plant_capacity' => $this->faker->numberBetween(5, 100),
            'assigned_nodes' => [
                'ph_node' => 'ph_test_' . $zoneNumber,
                'climate_node' => 'climate_test_' . $zoneNumber,
            ],
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => null,
            'image_url' => null,
            'notes' => $this->faker->optional()->sentence(),
            'greenhouse_id' => null,
        ];
    }

    /**
     * Зона типа NFT
     */
    public function nft(): static
    {
        return $this->state(fn (array $attributes) => [
            'zone_type' => 'nft',
            'reservoir_volume_liters' => $this->faker->randomFloat(2, 50, 150),
            'growing_area_m2' => $this->faker->randomFloat(2, 1, 5),
            'plant_capacity' => $this->faker->numberBetween(10, 30),
        ]);
    }

    /**
     * Зона типа DWC
     */
    public function dwc(): static
    {
        return $this->state(fn (array $attributes) => [
            'zone_type' => 'dwc',
            'reservoir_volume_liters' => $this->faker->randomFloat(2, 20, 100),
            'growing_area_m2' => $this->faker->randomFloat(2, 0.5, 3),
            'plant_capacity' => $this->faker->numberBetween(5, 15),
        ]);
    }

    /**
     * Зона с капельным поливом
     */
    public function drip(): static
    {
        return $this->state(fn (array $attributes) => [
            'zone_type' => 'drip',
            'reservoir_volume_liters' => $this->faker->randomFloat(2, 100, 500),
            'growing_area_m2' => $this->faker->randomFloat(2, 5, 20),
            'plant_capacity' => $this->faker->numberBetween(30, 100),
        ]);
    }

    /**
     * Активная зона
     */
    public function active(): static
    {
        return $this->state(fn (array $attributes) => [
            'is_active' => true,
        ]);
    }

    /**
     * Неактивная зона
     */
    public function inactive(): static
    {
        return $this->state(fn (array $attributes) => [
            'is_active' => false,
        ]);
    }

    /**
     * Доступная зона (для новых циклов)
     */
    public function available(): static
    {
        return $this->state(fn (array $attributes) => [
            'is_available' => true,
            'is_active' => true,
            'current_cycle_id' => null,
        ]);
    }

    /**
     * Занятая зона (есть активный цикл)
     */
    public function busy(): static
    {
        return $this->state(fn (array $attributes) => [
            'current_cycle_id' => $this->faker->numberBetween(1, 100),
        ]);
    }

    /**
     * Зона с Root Node (создаёт Root Node автоматически)
     */
    public function withRootNode(): static
    {
        return $this->afterCreating(function (Zone $zone) {
            // Проверяем существует ли Root Node
            $rootNode = Node::where('node_id', $zone->root_node_id)->first();
            
            if (!$rootNode) {
                Node::create([
                    'node_id' => $zone->root_node_id,
                    'node_type' => 'root',
                    'root_node_id' => $zone->root_node_id,
                    'online' => true,
                    'last_seen_at' => now(),
                    'metadata' => [
                        'mesh_network_id' => $zone->mesh_network_id,
                    ],
                ]);
            }
        });
    }

    /**
     * Зона с узлами (создаёт Root Node + дочерние узлы)
     */
    public function withNodes(): static
    {
        return $this->withRootNode()->afterCreating(function (Zone $zone) {
            foreach ($zone->assigned_nodes as $role => $nodeId) {
                $nodeType = str_replace('_node', '', $role);
                
                if ($role === 'ph_node') {
                    $nodeType = 'ph_ec';
                }

                // Используем firstOrCreate чтобы избежать дубликатов
                Node::firstOrCreate(
                    ['node_id' => $nodeId],
                    [
                        'node_type' => $nodeType,
                        'root_node_id' => $zone->root_node_id,
                        'zone' => $zone->mesh_network_id,
                        'greenhouse_id' => $zone->greenhouse_id,
                        'online' => $this->faker->boolean(80), // 80% онлайн
                        'last_seen_at' => now()->subMinutes($this->faker->numberBetween(1, 30)),
                        'metadata' => [],
                    ]
                );
            }
        });
    }

    public function forGreenhouse(int $greenhouseId): static
    {
        return $this->state(fn () => ['greenhouse_id' => $greenhouseId]);
    }
}

