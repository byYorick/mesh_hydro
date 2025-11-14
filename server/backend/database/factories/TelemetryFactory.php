<?php

namespace Database\Factories;

use App\Models\Telemetry;
use Illuminate\Database\Eloquent\Factories\Factory;

class TelemetryFactory extends Factory
{
    protected $model = Telemetry::class;

    public function definition(): array
    {
        $receivedAt = $this->faker->dateTimeBetween('-2 days', '-2 hours');

        return [
            'node_id' => 'node_test_' . $this->faker->unique()->numberBetween(100, 999),
            'node_type' => $this->faker->randomElement(['ph_ec', 'climate', 'relay', 'water']),
            'data' => [
                'ph' => $this->faker->randomFloat(2, 5.0, 7.5),
                'ec' => $this->faker->randomFloat(2, 1.0, 2.5),
                'temp' => $this->faker->randomFloat(1, 18, 30),
            ],
            'received_at' => $receivedAt,
            'created_at' => $receivedAt,
            'updated_at' => $receivedAt,
        ];
    }

    public function forNode(string $nodeId, string $nodeType = 'ph_ec'): static
    {
        return $this->state(fn () => [
            'node_id' => $nodeId,
            'node_type' => $nodeType,
        ]);
    }

    public function withData(array $data): static
    {
        return $this->state(fn () => [
            'data' => $data,
        ]);
    }

    public function receivedAt($dateTime): static
    {
        return $this->state(fn () => [
            'received_at' => $dateTime,
            'created_at' => $dateTime,
            'updated_at' => $dateTime,
        ]);
    }
}


