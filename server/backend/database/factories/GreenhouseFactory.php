<?php

namespace Database\Factories;

use App\Models\Greenhouse;
use Illuminate\Database\Eloquent\Factories\Factory;

class GreenhouseFactory extends Factory
{
    protected $model = Greenhouse::class;

    public function definition(): array
    {
        $code = strtoupper($this->faker->unique()->lexify('GH????'));

        return [
            'name' => $this->faker->words(3, true) . ' Greenhouse',
            'code' => $code,
            'location' => null,
            'description' => $this->faker->sentence(),
            'timezone' => null,
            'status' => $this->faker->randomElement(['active', 'draft', 'maintenance']),
            'mesh_group' => null,
            'root_node_id' => null,
            'root_node_mac' => null,
            'image_url' => null,
            'tags' => [],
            'settings' => [],
        ];
    }

    public function active(): static
    {
        return $this->state(fn () => ['status' => 'active']);
    }
}


