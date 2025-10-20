<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\PidPreset;

class PidPresetSeeder extends Seeder
{
    public function run(): void
    {
        $defaultPresets = [
            [
                'name' => 'Салат NFT (100L)',
                'description' => 'Оптимальные настройки для выращивания салата в NFT системе объемом 100 литров',
                'is_default' => true,
                'ph_config' => [
                    'enabled' => true,
                    'setpoint' => 5.8,
                    'kp' => 2.0,
                    'ki' => 0.005,
                    'kd' => 0.5,
                    'deadband' => 0.1,
                    'doseMinInterval' => 300,
                    'mixDelay' => 120,
                    'integralMax' => 5.0,
                    'integralMin' => -5.0,
                    'outputMax' => 10,
                    'outputMin' => 1,
                    'filterAlpha' => 0.8
                ],
                'ec_config' => [
                    'enabled' => true,
                    'setpoint' => 1.4,
                    'kp' => 5.0,
                    'ki' => 0.01,
                    'kd' => 1.0,
                    'deadband' => 0.1,
                    'doseMinInterval' => 600,
                    'mixDelay' => 180,
                    'componentABDelay' => 60,
                    'integralMax' => 10.0,
                    'integralMin' => -10.0,
                    'outputMax' => 20,
                    'outputMin' => 2,
                    'filterAlpha' => 0.7
                ]
            ],
            [
                'name' => 'Томаты капельный (300L)',
                'description' => 'Настройки для выращивания томатов в капельной системе объемом 300 литров',
                'is_default' => true,
                'ph_config' => [
                    'enabled' => true,
                    'setpoint' => 6.0,
                    'kp' => 1.5,
                    'ki' => 0.003,
                    'kd' => 0.3,
                    'deadband' => 0.15,
                    'doseMinInterval' => 600,
                    'mixDelay' => 240,
                    'integralMax' => 8.0,
                    'integralMin' => -8.0,
                    'outputMax' => 15,
                    'outputMin' => 2,
                    'filterAlpha' => 0.6
                ],
                'ec_config' => [
                    'enabled' => true,
                    'setpoint' => 2.2,
                    'kp' => 3.0,
                    'ki' => 0.005,
                    'kd' => 0.5,
                    'deadband' => 0.15,
                    'doseMinInterval' => 900,
                    'mixDelay' => 300,
                    'componentABDelay' => 120,
                    'integralMax' => 15.0,
                    'integralMin' => -15.0,
                    'outputMax' => 30,
                    'outputMin' => 3,
                    'filterAlpha' => 0.6
                ]
            ],
            [
                'name' => 'DWC малая (50L)',
                'description' => 'Настройки для малой DWC системы объемом 50 литров',
                'is_default' => true,
                'ph_config' => [
                    'enabled' => true,
                    'setpoint' => 5.5,
                    'kp' => 3.0,
                    'ki' => 0.01,
                    'kd' => 1.0,
                    'deadband' => 0.05,
                    'doseMinInterval' => 180,
                    'mixDelay' => 60,
                    'integralMax' => 3.0,
                    'integralMin' => -3.0,
                    'outputMax' => 5,
                    'outputMin' => 0.5,
                    'filterAlpha' => 0.9
                ],
                'ec_config' => [
                    'enabled' => true,
                    'setpoint' => 1.2,
                    'kp' => 8.0,
                    'ki' => 0.02,
                    'kd' => 2.0,
                    'deadband' => 0.05,
                    'doseMinInterval' => 300,
                    'mixDelay' => 90,
                    'componentABDelay' => 30,
                    'integralMax' => 5.0,
                    'integralMin' => -5.0,
                    'outputMax' => 10,
                    'outputMin' => 1,
                    'filterAlpha' => 0.85
                ]
            ],
            [
                'name' => 'Консервативный (безопасный)',
                'description' => 'Безопасные настройки с медленной реакцией для тестирования системы',
                'is_default' => true,
                'ph_config' => [
                    'enabled' => true,
                    'setpoint' => 6.0,
                    'kp' => 1.0,
                    'ki' => 0.001,
                    'kd' => 0.1,
                    'deadband' => 0.2,
                    'doseMinInterval' => 900,
                    'mixDelay' => 300,
                    'integralMax' => 3.0,
                    'integralMin' => -3.0,
                    'outputMax' => 5,
                    'outputMin' => 1,
                    'filterAlpha' => 0.5
                ],
                'ec_config' => [
                    'enabled' => true,
                    'setpoint' => 1.5,
                    'kp' => 2.0,
                    'ki' => 0.002,
                    'kd' => 0.2,
                    'deadband' => 0.2,
                    'doseMinInterval' => 1200,
                    'mixDelay' => 360,
                    'componentABDelay' => 180,
                    'integralMax' => 5.0,
                    'integralMin' => -5.0,
                    'outputMax' => 10,
                    'outputMin' => 2,
                    'filterAlpha' => 0.5
                ]
            ]
        ];

        foreach ($defaultPresets as $preset) {
            PidPreset::updateOrCreate(
                ['name' => $preset['name'], 'is_default' => true],
                $preset
            );
        }

        $this->command->info('Default PID presets seeded successfully!');
    }
}

