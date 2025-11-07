<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Console\Commands\CheckStageTransitionsCommand;
use App\Models\GrowthCycle;
use App\Models\GrowthStage;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\Zone;
use App\Models\CycleStageHistory;
use App\Models\StageTransitionRecommendation;
use Illuminate\Foundation\Testing\RefreshDatabase;

class CheckStageTransitionsCommandTest extends TestCase
{
    use RefreshDatabase;

    #[Test]
    public function it_creates_recommendation_when_stage_duration_exceeded()
    {
        $zone = Zone::factory()->create();
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
            'started_at' => now()->subDays(10),
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(10),
        ]);

        // Запускаем команду
        $this->artisan('growth:check-transitions')
            ->assertExitCode(0);

        // Проверяем создание рекомендации
        $recommendation = StageTransitionRecommendation::where('cycle_id', $cycle->id)->first();

        $this->assertNotNull($recommendation);
        $this->assertEquals($stage1->id, $recommendation->current_stage_id);
        $this->assertEquals($stage2->id, $recommendation->recommended_stage_id);
        $this->assertEquals('pending', $recommendation->status);
    }

    #[Test]
    public function it_does_not_create_duplicate_recommendations()
    {
        $zone = Zone::factory()->create();
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
            'started_at' => now()->subDays(10),
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(10),
        ]);

        // Создаем существующую рекомендацию
        StageTransitionRecommendation::create([
            'cycle_id' => $cycle->id,
            'current_stage_id' => $stage1->id,
            'recommended_stage_id' => $stage2->id,
            'reason' => 'Существующая рекомендация',
            'status' => 'pending',
            'recommended_at' => now(),
        ]);

        // Запускаем команду
        $this->artisan('growth:check-transitions')
            ->assertExitCode(0);

        // Проверяем, что дубликат не создан
        $count = StageTransitionRecommendation::where('cycle_id', $cycle->id)
            ->where('status', 'pending')
            ->count();

        $this->assertEquals(1, $count);
    }

    #[Test]
    public function it_skips_cycles_at_last_stage()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $lastStage = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 5,
            'duration_days' => 21,
        ]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $lastStage->id,
            'started_at' => now()->subDays(30),
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $lastStage->id,
            'started_at' => now()->subDays(30),
        ]);

        // Запускаем команду
        $this->artisan('growth:check-transitions')
            ->assertExitCode(0);

        // Проверяем, что рекомендация не создана
        $count = StageTransitionRecommendation::where('cycle_id', $cycle->id)->count();
        $this->assertEquals(0, $count);
    }

    #[Test]
    public function it_processes_multiple_cycles_independently()
    {
        $zone1 = Zone::factory()->create();
        $zone2 = Zone::factory()->create();
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

        // Цикл 1: нужен переход
        $cycle1 = GrowthCycle::factory()->create([
            'zone_id' => $zone1->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage1->id,
            'started_at' => now()->subDays(10),
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle1->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(10),
        ]);

        // Цикл 2: переход не нужен
        $cycle2 = GrowthCycle::factory()->create([
            'zone_id' => $zone2->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage1->id,
            'started_at' => now()->subDays(3),
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle2->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(3),
        ]);

        // Запускаем команду
        $this->artisan('growth:check-transitions')
            ->assertExitCode(0);

        // Проверяем: рекомендация только для cycle1
        $this->assertDatabaseHas('stage_transition_recommendations', [
            'cycle_id' => $cycle1->id,
            'status' => 'pending',
        ]);

        $this->assertDatabaseMissing('stage_transition_recommendations', [
            'cycle_id' => $cycle2->id,
        ]);
    }
}

