<?php

namespace Database\Factories;

use App\Models\GrowthCulture;
use Illuminate\Database\Eloquent\Factories\Factory;

class GrowthCultureFactory extends Factory
{
    protected $model = GrowthCulture::class;

    public function definition(): array
    {
        $baseName = $this->faker->randomElement(['Салат', 'Клубника', 'Томат', 'Огурец', 'Базилик']);
        $uniqueId = $this->faker->unique()->numberBetween(1000, 9999);
        $name = $baseName . ' ' . $uniqueId;
        $slug = \Illuminate\Support\Str::slug($name);

        return [
            'name' => $name,
            'slug' => $slug,
            'category' => $this->faker->randomElement(['leafy_greens', 'berries', 'vegetables', 'herbs']),
            'description' => $this->faker->sentence(),
            'typical_cycle_days' => $this->faker->numberBetween(30, 90),
            'optimal_temp_min' => $this->faker->randomFloat(1, 18, 22),
            'optimal_temp_max' => $this->faker->randomFloat(1, 22, 28),
            'optimal_humidity_min' => $this->faker->numberBetween(40, 50),
            'optimal_humidity_max' => $this->faker->numberBetween(60, 70),
            'is_system' => true,
            'is_active' => true,
        ];
    }
}

