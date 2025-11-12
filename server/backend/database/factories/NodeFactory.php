<?php

namespace Database\Factories;

use App\Models\Node;
use Illuminate\Database\Eloquent\Factories\Factory;

class NodeFactory extends Factory
{
    protected $model = Node::class;

    public function definition(): array
    {
        $nodeId = 'node_test_' . $this->faker->unique()->numberBetween(1000, 9999);
        $nodeType = $this->faker->randomElement(['ph_ec', 'climate', 'relay', 'water', 'display']);

        return [
            'node_id' => $nodeId,
            'node_type' => $nodeType,
            'root_node_id' => null,
            'zone' => 'mesh_' . $this->faker->numberBetween(1, 999),
            'greenhouse_id' => null,
            'online' => $this->faker->boolean(80),
            'last_seen_at' => $this->faker->optional()->dateTimeBetween('-1 hour', 'now'),
            'metadata' => [],
        ];
    }

    public function root(): static
    {
        return $this->state(function (array $attributes) {
            $rootNodeId = $attributes['node_id'] ?? 'root_test_' . $this->faker->unique()->numberBetween(1000, 9999);

            return [
                'node_id' => $rootNodeId,
                'node_type' => 'root',
                'root_node_id' => $rootNodeId,
                'zone' => $attributes['zone'] ?? 'mesh_' . $this->faker->numberBetween(1, 999),
                'online' => true,
            ];
        })
        ->afterMaking(function (Node $node) {
            $node->root_node_id = $node->node_id;
        })
        ->afterCreating(function (Node $node) {
            $node->update(['root_node_id' => $node->node_id]);
        });
    }

    public function online(): static
    {
        return $this->state(fn (array $attributes) => [
            'online' => true,
            'last_seen_at' => now(),
        ]);
    }

    public function offline(): static
    {
        return $this->state(fn (array $attributes) => [
            'online' => false,
            'last_seen_at' => now()->subMinutes(10),
        ]);
    }
}

