<?php

namespace Database\Factories;

use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use Illuminate\Database\Eloquent\Factories\Factory;

class GrowthPresetFactory extends Factory
{
    protected $model = GrowthPreset::class;

    public function definition(): array
    {
        $culture = GrowthCulture::factory()->create();
        $uniqueId = $this->faker->unique()->numberBetween(1000, 9999);
        $name = $culture->name . ' - ' . $this->faker->randomElement(['Базовый', 'Продвинутый', 'Экспресс']) . ' ' . $uniqueId;
        $slug = \Illuminate\Support\Str::slug($name);

        return [
            'culture_id' => $culture->id,
            'name' => $name,
            'slug' => $slug,
            'description' => $this->faker->sentence(),
            'preset_type' => 'system',
            'total_days' => $this->faker->numberBetween(30, 90),
            'difficulty' => $this->faker->randomElement(['easy', 'medium', 'hard']),
            'recommended_system' => $this->faker->randomElement(['nft', 'dwc', 'drip', 'ebb_flow', 'any']),
            'usage_count' => 0,
            'is_active' => true,
        ];
    }
}

