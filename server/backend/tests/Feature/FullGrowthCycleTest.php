<?php

namespace Tests\Feature;

use Tests\TestCase;
use App\Models\GrowthCycle;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\GrowthStage;
use App\Models\Zone;
use App\Models\CycleStageHistory;
use App\Models\StageTransitionRecommendation;
use App\Services\GrowthNotificationService;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Mockery;

/**
 * ⭐ GROWTH PLANNER: Полные интеграционные тесты для циклов роста
 */
class FullGrowthCycleTest extends TestCase
{
    use RefreshDatabase;

    /** @test */
    public function it_can_create_and_manage_full_growth_cycle()
    {
        // Создаем зону
        $zone = Zone::factory()->withRootNode()->create();
        
        // Создаем культуру и пресет
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        // Создаем стадии
        $stage1 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'name' => 'Проращивание',
            'order' => 1,
            'duration_days' => 7,
        ]);
        
        $stage2 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'name' => 'Вегетация',
            'order' => 2,
            'duration_days' => 14,
        ]);

        // Создаем цикл через API
        $response = $this->postJson('/api/growth/cycles', [
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'name' => 'Тестовый цикл',
            'plant_count' => 20,
        ]);

        $response->assertStatus(201)
            ->assertJsonStructure(['id', 'zone_id', 'preset_id', 'status']);

        $cycleId = $response->json('id');
        $cycle = GrowthCycle::find($cycleId);

        // Проверяем создание цикла
        $this->assertEquals('active', $cycle->status);
        $this->assertEquals($stage1->id, $cycle->current_stage_id);
        $this->assertNotNull($cycle->started_at);

        // Переход на следующую стадию
        $response = $this->postJson("/api/growth/cycles/{$cycleId}/transition", [
            'to_stage_id' => $stage2->id,
        ]);

        if ($response->status() !== 200) {
            dump($response->json());
        }

        $response->assertStatus(200);
        
        $cycle->refresh();
        $this->assertEquals($stage2->id, $cycle->current_stage_id);

        // Завершение цикла (сбор урожая)
        $response = $this->postJson("/api/growth/cycles/{$cycleId}/harvest", [
            'harvest_weight_kg' => 15.5,
            'rating' => 5,
            'notes' => 'Отличный урожай',
        ]);

        $response->assertStatus(200);
        
        $cycle->refresh();
        $this->assertEquals('harvested', $cycle->status);
        $this->assertEquals(15.5, $cycle->harvest_weight_kg);
        $this->assertEquals(5, $cycle->rating);
    }

    /** @test */
    public function it_can_run_multiple_cycles_in_different_zones_simultaneously()
    {
        // Создаем 3 зоны
        $zone1 = Zone::factory()->withRootNode()->create(['name' => 'NFT Zone 1']);
        $zone2 = Zone::factory()->withRootNode()->create(['name' => 'DWC Zone 2']);
        $zone3 = Zone::factory()->withRootNode()->create(['name' => 'Drip Zone 3']);

        // Создаем культуры и пресеты
        $culture1 = GrowthCulture::factory()->create();
        $preset1 = GrowthPreset::factory()->create(['culture_id' => $culture1->id]);
        GrowthStage::factory()->create(['preset_id' => $preset1->id, 'order' => 1]);

        $culture2 = GrowthCulture::factory()->create();
        $preset2 = GrowthPreset::factory()->create(['culture_id' => $culture2->id]);
        GrowthStage::factory()->create(['preset_id' => $preset2->id, 'order' => 1]);

        $culture3 = GrowthCulture::factory()->create();
        $preset3 = GrowthPreset::factory()->create(['culture_id' => $culture3->id]);
        GrowthStage::factory()->create(['preset_id' => $preset3->id, 'order' => 1]);

        // Создаем 3 параллельных цикла
        $cycle1 = GrowthCycle::factory()->create([
            'zone_id' => $zone1->id,
            'preset_id' => $preset1->id,
            'culture_id' => $culture1->id,
            'status' => 'active',
        ]);

        $cycle2 = GrowthCycle::factory()->create([
            'zone_id' => $zone2->id,
            'preset_id' => $preset2->id,
            'culture_id' => $culture2->id,
            'status' => 'active',
        ]);

        $cycle3 = GrowthCycle::factory()->create([
            'zone_id' => $zone3->id,
            'preset_id' => $preset3->id,
            'culture_id' => $culture3->id,
            'status' => 'active',
        ]);

        // Проверяем изоляцию зон
        $this->assertNotEquals($cycle1->zone_id, $cycle2->zone_id);
        $this->assertNotEquals($cycle2->zone_id, $cycle3->zone_id);
        $this->assertNotEquals($cycle1->zone_id, $cycle3->zone_id);

        // Проверяем, что все циклы активны
        $activeCycles = GrowthCycle::where('status', 'active')->count();
        $this->assertEquals(3, $activeCycles);

        // Проверяем, что зоны заняты
        $zones[0]->refresh();
        $zones[1]->refresh();
        $zones[2]->refresh();

        $this->assertEquals($cycle1->id, $zones[0]->current_cycle_id);
        $this->assertEquals($cycle2->id, $zones[1]->current_cycle_id);
        $this->assertEquals($cycle3->id, $zones[2]->current_cycle_id);
    }

    /** @test */
    public function it_prevents_starting_cycle_in_occupied_zone()
    {
        $zone = Zone::factory()->withRootNode()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        GrowthStage::factory()->create(['preset_id' => $preset->id, 'order' => 1]);

        // Создаем первый цикл
        $cycle1 = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'status' => 'active',
        ]);

        $zone->update(['current_cycle_id' => $cycle1->id]);

        // Пытаемся создать второй цикл в той же зоне
        $response = $this->postJson('/api/growth/cycles', [
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'name' => 'Второй цикл',
            'plant_count' => 20,
        ]);

        $response->assertStatus(409);  // 409 Conflict вместо 422 Validation Error
    }

    /** @test */
    public function it_tracks_stage_history_throughout_cycle()
    {
        $zone = Zone::factory()->withRootNode()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $stage1 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 1,
            'duration_days' => 7,
        ]);
        
        $stage2 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 2,
            'duration_days' => 14,
        ]);
        
        $stage3 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 3,
            'duration_days' => 21,
        ]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage1->id,
        ]);

        // Начальная стадия
        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(10),
        ]);

        // Переход на стадию 2 (через API)
        $this->postJson("/api/growth/cycles/{$cycle->id}/transition", [
            'to_stage_id' => $stage2->id,
        ])->assertStatus(200);

        // Переход на стадию 3
        $this->postJson("/api/growth/cycles/{$cycle->id}/transition", [
            'to_stage_id' => $stage3->id,
        ])->assertStatus(200);

        $cycle->refresh();

        // Проверяем историю
        $history = $cycle->stageHistory()->orderBy('started_at')->get();
        
        $this->assertCount(3, $history);
        $this->assertEquals($stage1->id, $history[0]->stage_id);
        $this->assertEquals($stage2->id, $history[1]->stage_id);
        $this->assertEquals($stage3->id, $history[2]->stage_id);
        
        // Проверяем, что предыдущие стадии завершены
        $this->assertNotNull($history[0]->ended_at);
        $this->assertNotNull($history[1]->ended_at);
        $this->assertNull($history[2]->ended_at); // Текущая стадия
    }

    /** @test */
    public function it_creates_stage_transition_recommendations()
    {
        $zone = Zone::factory()->withRootNode()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $stage1 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 1,
            'duration_days' => 7,
        ]);
        
        $stage2 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 2,
            'duration_days' => 14,
        ]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage1->id,
            'started_at' => now()->subDays(8), // Превысили длительность стадии
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(8),
        ]);

        // Создаем рекомендацию
        $recommendation = StageTransitionRecommendation::create([
            'cycle_id' => $cycle->id,
            'current_stage_id' => $stage1->id,
            'recommended_stage_id' => $stage2->id,
            'reason' => 'Минимальная длительность стадии достигнута',
            'status' => 'pending',
            'recommended_at' => now(),
        ]);

        $this->assertDatabaseHas('stage_transition_recommendations', [
            'cycle_id' => $cycle->id,
            'current_stage_id' => $stage1->id,
            'recommended_stage_id' => $stage2->id,
            'status' => 'pending',
        ]);

        // Принимаем рекомендацию
        $response = $this->postJson("/api/growth/cycles/{$cycle->id}/accept-transition/{$recommendation->id}");

        $response->assertStatus(200);

        $recommendation->refresh();
        $this->assertEquals('accepted', $recommendation->status);
        
        $cycle->refresh();
        $this->assertEquals($stage2->id, $cycle->current_stage_id);
    }
}

