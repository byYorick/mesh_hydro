<?php

namespace Tests\Unit;

use Tests\TestCase;
use App\Services\GrowthAnalyticsService;
use App\Models\GrowthCycle;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\Zone;
use App\Models\CycleParameterLog;
use App\Models\CycleStageHistory;
use App\Models\GrowthStage;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Carbon\Carbon;

class GrowthAnalyticsServiceTest extends TestCase
{
    use RefreshDatabase;

    private GrowthAnalyticsService $analytics;

    protected function setUp(): void
    {
        parent::setUp();
        $this->analytics = app(GrowthAnalyticsService::class);
    }

    /** @test */
    public function it_can_get_parameter_chart_by_stages()
    {
        // Создаем зону
        $zone = Zone::factory()->create();
        
        // Создаем культуру и пресет
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        // Создаем стадии
        $stage1 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 1,
            'duration_days' => 7,
            'target_params' => ['ph' => 6.0, 'ec' => 1.2, 'temp' => 22.0],
        ]);
        
        $stage2 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 2,
            'duration_days' => 14,
            'target_params' => ['ph' => 6.2, 'ec' => 1.5, 'temp' => 24.0],
        ]);

        // Создаем цикл
        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage1->id,
            'started_at' => now()->subDays(5),
        ]);

        // Создаем историю стадий
        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage1->id,
            'started_at' => now()->subDays(5),
        ]);

        // Создаем логи параметров
        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(3),
            'day_number' => 3,
            'avg_ph' => 6.1,
            'avg_ec' => 1.3,
            'avg_temp' => 22.5,
        ]);

        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(1),
            'day_number' => 5,
            'avg_ph' => 6.0,
            'avg_ec' => 1.2,
            'avg_temp' => 22.0,
        ]);

        // Получаем график
        $chart = $this->analytics->getParameterChartByStages($cycle->id, ['ph', 'ec']);

        $this->assertIsArray($chart);
        $this->assertArrayHasKey('stages', $chart);
        $this->assertArrayHasKey('data', $chart);
        $this->assertArrayHasKey('parameters', $chart);
        $this->assertCount(1, $chart['stages']); // Только одна стадия начата
        $this->assertGreaterThan(0, count($chart['data']));
    }

    /** @test */
    public function it_can_get_parameter_statistics()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
        ]);

        // Создаем логи с разными значениями
        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(3),
            'day_number' => 3,
            'avg_ph' => 6.0,
            'avg_ec' => 1.2,
            'avg_temp' => 22.0,
        ]);

        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(2),
            'day_number' => 4,
            'avg_ph' => 6.2,
            'avg_ec' => 1.4,
            'avg_temp' => 23.0,
        ]);

        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(1),
            'day_number' => 5,
            'avg_ph' => 6.1,
            'avg_ec' => 1.3,
            'avg_temp' => 22.5,
        ]);

        $statistics = $this->analytics->getParameterStatistics($cycle->id);

        $this->assertIsArray($statistics);
        $this->assertArrayHasKey('ph', $statistics);
        $this->assertEquals(3, $statistics['ph']['count']);
        $this->assertEquals(6.1, $statistics['ph']['avg']); // (6.0 + 6.2 + 6.1) / 3
        $this->assertEquals(6.0, $statistics['ph']['min']);
        $this->assertEquals(6.2, $statistics['ph']['max']);
    }

    /** @test */
    public function it_can_compare_cycles()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle1 = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'started_at' => now()->subDays(30),
            'ended_at' => now()->subDays(5),
            'harvest_weight_kg' => 10.5,
            'plant_count' => 20,
            'rating' => 4,
        ]);

        $cycle2 = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'started_at' => now()->subDays(25),
            'ended_at' => now()->subDays(2),
            'harvest_weight_kg' => 12.0,
            'plant_count' => 20,
            'rating' => 5,
        ]);

        $comparison = $this->analytics->compareCycles([$cycle1->id, $cycle2->id]);

        $this->assertIsArray($comparison);
        $this->assertArrayHasKey('cycles', $comparison);
        $this->assertArrayHasKey('summary', $comparison);
        $this->assertCount(2, $comparison['cycles']);
        $this->assertEquals(2, $comparison['summary']['total_cycles']);
    }

    /** @test */
    public function it_can_create_comparison_snapshot()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'harvest_weight_kg' => 10.5,
            'plant_count' => 20,
        ]);

        // Создаем логи параметров
        CycleParameterLog::factory()->count(3)->create([
            'cycle_id' => $cycle->id,
            'avg_ph' => 6.0,
            'avg_ec' => 1.2,
            'water_consumed_liters' => 5.0,
        ]);

        $snapshot = $this->analytics->createComparisonSnapshot($cycle->id);

        $this->assertNotNull($snapshot);
        $this->assertEquals($cycle->id, $snapshot->cycle_id);
        $this->assertNotNull($snapshot->avg_ph);
        $this->assertNotNull($snapshot->total_water_liters);
    }

    /** @test */
    public function it_can_get_cycle_report()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $stage = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 1,
        ]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage->id,
            'started_at' => now()->subDays(10),
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage->id,
            'started_at' => now()->subDays(10),
        ]);

        CycleParameterLog::factory()->count(5)->create([
            'cycle_id' => $cycle->id,
            'avg_ph' => 6.0,
            'avg_ec' => 1.2,
        ]);

        $report = $this->analytics->getCycleReport($cycle->id);

        $this->assertIsArray($report);
        $this->assertArrayHasKey('cycle', $report);
        $this->assertArrayHasKey('stages', $report);
        $this->assertArrayHasKey('parameter_statistics', $report);
        $this->assertArrayHasKey('water_consumption', $report);
        $this->assertArrayHasKey('deviations_summary', $report);
    }

    /** @test */
    public function it_can_calculate_water_consumption()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
        ]);

        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(3),
            'day_number' => 3,
            'water_consumed_liters' => 5.0,
        ]);

        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(2),
            'day_number' => 4,
            'water_consumed_liters' => 6.0,
        ]);

        CycleParameterLog::create([
            'cycle_id' => $cycle->id,
            'log_date' => now()->subDays(1),
            'day_number' => 5,
            'water_consumed_liters' => 7.0,
        ]);

        $report = $this->analytics->getCycleReport($cycle->id);
        $consumption = $report['water_consumption'];

        $this->assertEquals(18.0, $consumption['total_liters']);
        $this->assertEquals(6.0, $consumption['avg_per_day']);
        $this->assertArrayHasKey('daily', $consumption);
    }
}

