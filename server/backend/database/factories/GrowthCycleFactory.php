<?php

namespace Database\Factories;

use App\Models\GrowthCycle;
use App\Models\Zone;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\GrowthStage;
use Illuminate\Database\Eloquent\Factories\Factory;

class GrowthCycleFactory extends Factory
{
    protected $model = GrowthCycle::class;

    public function definition(): array
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        $stage = GrowthStage::factory()->create(['preset_id' => $preset->id]);

        return [
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage->id,
            'started_at' => now()->subDays($this->faker->numberBetween(1, 30)),
            'expected_harvest_at' => now()->addDays($this->faker->numberBetween(10, 60)),
            'status' => 'active',
            'plant_count' => $this->faker->numberBetween(10, 50),
        ];
    }

    public function completed(): static
    {
        return $this->state(fn (array $attributes) => [
            'status' => 'harvested',
            'ended_at' => now(),
            'actual_harvest_at' => now(),
            'harvest_weight_kg' => $this->faker->randomFloat(2, 5, 20),
        ]);
    }
}


