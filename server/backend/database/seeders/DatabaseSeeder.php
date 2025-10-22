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
        ]);
        
        $this->command->info('✅ Настройки PID пресетов загружены');
        $this->command->info('💡 Узлы будут автоматически добавлены при подключении через MQTT');
    }
}

