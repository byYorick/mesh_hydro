<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;

class DatabaseSeeder extends Seeder
{
    /**
     * Seed the application's database.
     */
    public function run(): void
    {
        // ========================================
        // ТЕСТОВЫЕ СИДЫ ОТКЛЮЧЕНЫ
        // ========================================
        // Узлы будут добавляться автоматически через MQTT
        // при первом подключении устройств
        
        // Оставляем только PID пресеты для реальной работы
        $this->call([
            PidPresetSeeder::class,
            GrowthPresetsSeeder::class,
            ZoneSeeder::class,
            GrowthScenarioSeeder::class,
        ]);
        
        $this->command->info('✅ Базовые сиды загружены (presets, зоны, циклы)');
        $this->command->info('💡 Узлы из сидов имитируют реальное окружение для разработки');
    }
}

