<?php

namespace Database\Seeders;

use Illuminate\Database\Seeder;
use App\Models\GrowthCulture;
use App\Models\GrowthPreset;
use App\Models\GrowthStage;
use Illuminate\Support\Str;

class GrowthPresetsSeeder extends Seeder
{
    /**
     * ⭐ GROWTH PLANNER: Заполнение культур и пресетов
     */
    public function run(): void
    {
        // ========= САЛАТ И ЗЕЛЕНЬ =========
        $saladCulture = GrowthCulture::create([
            'name' => 'Салат листовой',
            'slug' => 'salat-listovoy',
            'category' => 'leafy_greens',
            'description' => 'Листовой салат для NFT систем',
            'typical_cycle_days' => 35,
            'optimal_temp_min' => 18.0,
            'optimal_temp_max' => 24.0,
            'optimal_humidity_min' => 50.0,
            'optimal_humidity_max' => 70.0,
            'image_url' => null,
            'is_system' => true,
            'is_active' => true,
        ]);

        $saladPreset = GrowthPreset::create([
            'culture_id' => $saladCulture->id,
            'name' => 'Салат NFT - Стандартный',
            'slug' => 'salat-nft-standart',
            'description' => 'Оптимизированный пресет для выращивания салата в NFT системе',
            'preset_type' => 'system',
            'total_days' => 35,
            'difficulty' => 'easy',
            'recommended_system' => 'nft',
            'is_public' => true,
            'is_active' => true,
        ]);

        // Стадия 1: Прорастание (7 дней)
        GrowthStage::create([
            'preset_id' => $saladPreset->id,
            'name' => 'Прорастание',
            'order' => 1,
            'duration_days' => 7,
            'target_params' => [
                'ph_min' => 5.8,
                'ph_max' => 6.2,
                'ec_min' => 0.8,
                'ec_max' => 1.2,
                'temp_min' => 20.0,
                'temp_max' => 22.0,
                'humidity_min' => 60.0,
                'humidity_max' => 70.0,
                'light_intensity' => 50,
                'light_hours' => 16,
            ],
            'description' => 'Прорастание семян и формирование корневой системы',
            'care_instructions' => 'Поддерживайте высокую влажность. Следите за чистотой раствора.',
        ]);

        // Стадия 2: Вегетация (21 день)
        GrowthStage::create([
            'preset_id' => $saladPreset->id,
            'name' => 'Вегетативный рост',
            'order' => 2,
            'duration_days' => 21,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.0,
                'ec_min' => 1.2,
                'ec_max' => 1.8,
                'temp_min' => 18.0,
                'temp_max' => 24.0,
                'humidity_min' => 50.0,
                'humidity_max' => 65.0,
                'light_intensity' => 80,
                'light_hours' => 16,
            ],
            'description' => 'Активный рост листьев',
            'care_instructions' => 'Постепенно увеличивайте EC. Следите за освещением.',
        ]);

        // Стадия 3: Сбор урожая (7 дней)
        GrowthStage::create([
            'preset_id' => $saladPreset->id,
            'name' => 'Подготовка к сбору',
            'order' => 3,
            'duration_days' => 7,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.0,
                'ec_min' => 1.0,
                'ec_max' => 1.5,
                'temp_min' => 18.0,
                'temp_max' => 22.0,
                'humidity_min' => 50.0,
                'humidity_max' => 60.0,
                'light_intensity' => 70,
                'light_hours' => 14,
            ],
            'description' => 'Финальная стадия роста перед сбором',
            'care_instructions' => 'Снизьте EC для улучшения вкуса',
        ]);

        $this->command->info("✅ Салат: 1 культура, 1 пресет, 3 стадии");

        // ========= КЛУБНИКА =========
        $strawberryCulture = GrowthCulture::create([
            'name' => 'Клубника',
            'slug' => 'klubnika',
            'category' => 'berries',
            'description' => 'Клубника для DWC систем',
            'typical_cycle_days' => 90,
            'optimal_temp_min' => 18.0,
            'optimal_temp_max' => 26.0,
            'optimal_humidity_min' => 60.0,
            'optimal_humidity_max' => 80.0,
            'is_system' => true,
            'is_active' => true,
        ]);

        $strawberryPreset = GrowthPreset::create([
            'culture_id' => $strawberryCulture->id,
            'name' => 'Клубника DWC - Интенсив',
            'slug' => 'klubnika-dwc-intensiv',
            'description' => 'Интенсивное выращивание клубники в DWC',
            'preset_type' => 'system',
            'total_days' => 90,
            'difficulty' => 'medium',
            'recommended_system' => 'dwc',
            'is_public' => true,
            'is_active' => true,
        ]);

        // Стадия 1: Укоренение (14 дней)
        GrowthStage::create([
            'preset_id' => $strawberryPreset->id,
            'name' => 'Укоренение',
            'order' => 1,
            'duration_days' => 14,
            'target_params' => [
                'ph_min' => 5.8,
                'ph_max' => 6.2,
                'ec_min' => 1.0,
                'ec_max' => 1.4,
                'temp_min' => 20.0,
                'temp_max' => 24.0,
                'humidity_min' => 70.0,
                'humidity_max' => 80.0,
                'light_intensity' => 60,
                'light_hours' => 14,
            ],
            'description' => 'Развитие корневой системы',
        ]);

        // Стадия 2: Вегетация (35 дней)
        GrowthStage::create([
            'preset_id' => $strawberryPreset->id,
            'name' => 'Вегетация',
            'order' => 2,
            'duration_days' => 35,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.0,
                'ec_min' => 1.4,
                'ec_max' => 2.0,
                'temp_min' => 18.0,
                'temp_max' => 26.0,
                'humidity_min' => 60.0,
                'humidity_max' => 70.0,
                'light_intensity' => 85,
                'light_hours' => 16,
            ],
            'description' => 'Рост листвы и усов',
        ]);

        // Стадия 3: Цветение (21 день)
        GrowthStage::create([
            'preset_id' => $strawberryPreset->id,
            'name' => 'Цветение',
            'order' => 3,
            'duration_days' => 21,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.0,
                'ec_min' => 1.6,
                'ec_max' => 2.2,
                'temp_min' => 20.0,
                'temp_max' => 24.0,
                'humidity_min' => 55.0,
                'humidity_max' => 65.0,
                'light_intensity' => 90,
                'light_hours' => 14,
            ],
            'description' => 'Формирование цветов',
        ]);

        // Стадия 4: Плодоношение (20 дней)
        GrowthStage::create([
            'preset_id' => $strawberryPreset->id,
            'name' => 'Плодоношение',
            'order' => 4,
            'duration_days' => 20,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.2,
                'ec_min' => 1.8,
                'ec_max' => 2.4,
                'temp_min' => 18.0,
                'temp_max' => 24.0,
                'humidity_min' => 50.0,
                'humidity_max' => 65.0,
                'light_intensity' => 85,
                'light_hours' => 12,
            ],
            'description' => 'Созревание ягод',
        ]);

        $this->command->info("✅ Клубника: 1 культура, 1 пресет, 4 стадии");

        // ========= ТОМАТЫ =========
        $tomatoCulture = GrowthCulture::create([
            'name' => 'Томаты черри',
            'slug' => 'tomaty-cherry',
            'category' => 'vegetables',
            'description' => 'Томаты черри для капельного полива',
            'typical_cycle_days' => 110,
            'optimal_temp_min' => 20.0,
            'optimal_temp_max' => 28.0,
            'optimal_humidity_min' => 50.0,
            'optimal_humidity_max' => 70.0,
            'is_system' => true,
            'is_active' => true,
        ]);

        $tomatoPreset = GrowthPreset::create([
            'culture_id' => $tomatoCulture->id,
            'name' => 'Томаты - Капельный полив',
            'slug' => 'tomaty-drip',
            'description' => 'Полный цикл выращивания томатов черри',
            'preset_type' => 'system',
            'total_days' => 110,
            'difficulty' => 'hard',
            'recommended_system' => 'drip',
            'is_public' => true,
            'is_active' => true,
        ]);

        GrowthStage::create([
            'preset_id' => $tomatoPreset->id,
            'name' => 'Рассада',
            'order' => 1,
            'duration_days' => 21,
            'target_params' => [
                'ph_min' => 5.8,
                'ph_max' => 6.3,
                'ec_min' => 1.0,
                'ec_max' => 1.5,
                'temp_min' => 22.0,
                'temp_max' => 26.0,
                'humidity_min' => 60.0,
                'humidity_max' => 70.0,
                'light_intensity' => 70,
                'light_hours' => 16,
            ],
        ]);

        GrowthStage::create([
            'preset_id' => $tomatoPreset->id,
            'name' => 'Вегетация',
            'order' => 2,
            'duration_days' => 35,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.5,
                'ec_min' => 2.0,
                'ec_max' => 2.8,
                'temp_min' => 20.0,
                'temp_max' => 28.0,
                'humidity_min' => 55.0,
                'humidity_max' => 65.0,
                'light_intensity' => 90,
                'light_hours' => 16,
            ],
        ]);

        GrowthStage::create([
            'preset_id' => $tomatoPreset->id,
            'name' => 'Цветение',
            'order' => 3,
            'duration_days' => 28,
            'target_params' => [
                'ph_min' => 6.0,
                'ph_max' => 6.5,
                'ec_min' => 2.5,
                'ec_max' => 3.5,
                'temp_min' => 22.0,
                'temp_max' => 26.0,
                'humidity_min' => 50.0,
                'humidity_max' => 60.0,
                'light_intensity' => 95,
                'light_hours' => 14,
            ],
        ]);

        GrowthStage::create([
            'preset_id' => $tomatoPreset->id,
            'name' => 'Плодоношение',
            'order' => 4,
            'duration_days' => 26,
            'target_params' => [
                'ph_min' => 6.0,
                'ph_max' => 6.8,
                'ec_min' => 2.8,
                'ec_max' => 3.8,
                'temp_min' => 20.0,
                'temp_max' => 28.0,
                'humidity_min' => 50.0,
                'humidity_max' => 65.0,
                'light_intensity' => 90,
                'light_hours' => 14,
            ],
        ]);

        $this->command->info("✅ Томаты: 1 культура, 1 пресет, 4 стадии");

        // ========= ОГУРЦЫ =========
        $cucumberCulture = GrowthCulture::create([
            'name' => 'Огурцы',
            'slug' => 'ogurtsy',
            'category' => 'vegetables',
            'description' => 'Партенокарпические огурцы для NFT',
            'typical_cycle_days' => 70,
            'optimal_temp_min' => 22.0,
            'optimal_temp_max' => 28.0,
            'optimal_humidity_min' => 60.0,
            'optimal_humidity_max' => 80.0,
            'is_system' => true,
            'is_active' => true,
        ]);

        $cucumberPreset = GrowthPreset::create([
            'culture_id' => $cucumberCulture->id,
            'name' => 'Огурцы NFT - Быстрый',
            'slug' => 'ogurtsy-nft-fast',
            'description' => 'Ускоренный цикл выращивания огурцов',
            'preset_type' => 'system',
            'total_days' => 70,
            'difficulty' => 'medium',
            'recommended_system' => 'nft',
            'is_public' => true,
            'is_active' => true,
        ]);

        GrowthStage::create([
            'preset_id' => $cucumberPreset->id,
            'name' => 'Прорастание',
            'order' => 1,
            'duration_days' => 10,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.0,
                'ec_min' => 1.2,
                'ec_max' => 1.6,
                'temp_min' => 24.0,
                'temp_max' => 28.0,
                'humidity_min' => 70.0,
                'humidity_max' => 80.0,
                'light_intensity' => 60,
                'light_hours' => 16,
            ],
        ]);

        GrowthStage::create([
            'preset_id' => $cucumberPreset->id,
            'name' => 'Вегетация',
            'order' => 2,
            'duration_days' => 25,
            'target_params' => [
                'ph_min' => 5.5,
                'ph_max' => 6.5,
                'ec_min' => 1.8,
                'ec_max' => 2.4,
                'temp_min' => 22.0,
                'temp_max' => 28.0,
                'humidity_min' => 65.0,
                'humidity_max' => 75.0,
                'light_intensity' => 85,
                'light_hours' => 16,
            ],
        ]);

        GrowthStage::create([
            'preset_id' => $cucumberPreset->id,
            'name' => 'Цветение и плодоношение',
            'order' => 3,
            'duration_days' => 35,
            'target_params' => [
                'ph_min' => 5.8,
                'ph_max' => 6.5,
                'ec_min' => 2.2,
                'ec_max' => 3.0,
                'temp_min' => 24.0,
                'temp_max' => 28.0,
                'humidity_min' => 60.0,
                'humidity_max' => 75.0,
                'light_intensity' => 90,
                'light_hours' => 14,
            ],
        ]);

        $this->command->info("✅ Огурцы: 1 культура, 1 пресет, 3 стадии");
        $this->command->newLine();
        $this->command->info("🎉 GrowthPresetsSeeder завершен!");
        $this->command->info("📊 Всего: 4 культуры, 4 пресета, 14 стадий");
    }
}

