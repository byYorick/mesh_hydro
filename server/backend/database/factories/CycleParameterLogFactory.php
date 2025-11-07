<?php

namespace Database\Factories;

use App\Models\CycleParameterLog;
use App\Models\GrowthCycle;
use Illuminate\Database\Eloquent\Factories\Factory;

class CycleParameterLogFactory extends Factory
{
    protected $model = CycleParameterLog::class;

    public function definition(): array
    {
        $cycle = GrowthCycle::factory()->create();
        $dayNumber = $this->faker->numberBetween(1, 30);

        return [
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(30 - $dayNumber),
            'day_number' => $dayNumber,
            'avg_ph' => $this->faker->randomFloat(2, 5.5, 6.5),
            'avg_ec' => $this->faker->randomFloat(2, 1.0, 2.0),
            'avg_temp' => $this->faker->randomFloat(2, 20, 25),
            'avg_humidity' => $this->faker->randomFloat(2, 50, 70),
            'avg_co2' => $this->faker->numberBetween(400, 1000),
            'avg_lux' => $this->faker->numberBetween(10000, 50000),
            'min_ph' => $this->faker->randomFloat(2, 5.5, 6.0),
            'max_ph' => $this->faker->randomFloat(2, 6.0, 6.5),
            'min_temp' => $this->faker->randomFloat(2, 20, 22),
            'max_temp' => $this->faker->randomFloat(2, 23, 25),
            'water_consumed_liters' => $this->faker->randomFloat(2, 1, 10),
            'nutrient_a_ml' => $this->faker->randomFloat(2, 10, 50),
            'nutrient_b_ml' => $this->faker->randomFloat(2, 10, 50),
            'ph_down_ml' => $this->faker->randomFloat(2, 0, 5),
            'ph_up_ml' => $this->faker->randomFloat(2, 0, 5),
        ];
    }
}


