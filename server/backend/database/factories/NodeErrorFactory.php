<?php

namespace Database\Factories;

use App\Models\NodeError;
use Illuminate\Database\Eloquent\Factories\Factory;

class NodeErrorFactory extends Factory
{
    protected $model = NodeError::class;

    public function definition(): array
    {
        $occurredAt = $this->faker->dateTimeBetween('-2 days', 'now');

        return [
            'node_id' => 'node_' . $this->faker->numberBetween(100, 999),
            'error_code' => $this->faker->randomElement(['SENSOR_FAIL', 'NETWORK_TIMEOUT', 'HEAP_LOW']),
            'error_type' => $this->faker->randomElement([
                NodeError::TYPE_HARDWARE,
                NodeError::TYPE_SOFTWARE,
                NodeError::TYPE_NETWORK,
                NodeError::TYPE_SENSOR,
            ]),
            'severity' => $this->faker->randomElement([
                NodeError::SEVERITY_LOW,
                NodeError::SEVERITY_MEDIUM,
                NodeError::SEVERITY_HIGH,
            ]),
            'message' => $this->faker->sentence(4),
            'stack_trace' => null,
            'diagnostics' => ['detail' => $this->faker->numberBetween(1, 100)],
            'occurred_at' => $occurredAt,
            'resolved_at' => null,
            'resolution_notes' => null,
            'resolved_by' => null,
            'created_at' => $occurredAt,
            'updated_at' => $occurredAt,
        ];
    }

    public function critical(): static
    {
        return $this->state(fn () => ['severity' => NodeError::SEVERITY_CRITICAL]);
    }

    public function resolved(string $resolvedBy = 'technician', ?string $notes = null): static
    {
        return $this->state(function () use ($resolvedBy, $notes) {
            $resolvedAt = $this->faker->dateTimeBetween('-1 day', 'now');

            return [
                'resolved_at' => $resolvedAt,
                'resolved_by' => $resolvedBy,
                'resolution_notes' => $notes,
                'updated_at' => $resolvedAt,
            ];
        });
    }

    public function forNode(string $nodeId): static
    {
        return $this->state(fn () => ['node_id' => $nodeId]);
    }

    public function occurredAt($dateTime): static
    {
        return $this->state(fn () => [
            'occurred_at' => $dateTime,
            'created_at' => $dateTime,
            'updated_at' => $dateTime,
        ]);
    }
}


