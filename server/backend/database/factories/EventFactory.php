<?php

namespace Database\Factories;

use App\Models\Event;
use Illuminate\Database\Eloquent\Factories\Factory;

class EventFactory extends Factory
{
    protected $model = Event::class;

    public function definition(): array
    {
        $createdAt = $this->faker->dateTimeBetween('-3 days', 'now');

        return [
            'node_id' => 'node_' . $this->faker->numberBetween(100, 999),
            'level' => $this->faker->randomElement([
                Event::LEVEL_INFO,
                Event::LEVEL_WARNING,
                Event::LEVEL_CRITICAL,
            ]),
            'message' => $this->faker->sentence(3),
            'data' => ['value' => $this->faker->randomFloat(2, 0, 100)],
            'resolved_at' => null,
            'resolved_by' => null,
            'created_at' => $createdAt,
            'updated_at' => $createdAt,
        ];
    }

    public function critical(): static
    {
        return $this->state(fn () => ['level' => Event::LEVEL_CRITICAL]);
    }

    public function resolved(string $resolvedBy = 'system'): static
    {
        return $this->state(function () use ($resolvedBy) {
            $resolvedAt = $this->faker->dateTimeBetween('-1 day', 'now');

            return [
                'resolved_at' => $resolvedAt,
                'resolved_by' => $resolvedBy,
                'updated_at' => $resolvedAt,
            ];
        });
    }

    public function forNode(string $nodeId): static
    {
        return $this->state(fn () => ['node_id' => $nodeId]);
    }
}


