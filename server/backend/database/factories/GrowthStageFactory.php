<?php

namespace Database\Factories;

use App\Models\GrowthStage;
use App\Models\GrowthPreset;
use Illuminate\Database\Eloquent\Factories\Factory;

class GrowthStageFactory extends Factory
{
    protected $model = GrowthStage::class;

    public function definition(): array
    {
        $preset = GrowthPreset::factory()->create();
        $stageNames = ['Проращивание', 'Вегетация', 'Цветение', 'Плодоношение', 'Созревание'];
        $name = $this->faker->randomElement($stageNames);

        $targetParams = [
            'ph' => $this->faker->randomFloat(1, 5.5, 6.5),
            'ec' => $this->faker->randomFloat(1, 1.0, 2.0),
            'temp' => $this->faker->randomFloat(1, 20, 25),
            'humidity' => $this->faker->numberBetween(50, 70),
            'co2' => $this->faker->numberBetween(400, 1000),
            'lux' => $this->faker->numberBetween(10000, 50000),
        ];

        return [
            'preset_id' => $preset->id,
            'name' => $name,
            'order' => $this->faker->numberBetween(1, 5),
            'duration_days' => $this->faker->numberBetween(7, 21),
            'target_params' => $targetParams, // Laravel автоматически закодирует в JSON благодаря cast
            'description' => $this->faker->sentence(),
        ];
    }
}

