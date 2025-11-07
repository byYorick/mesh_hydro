<?php

namespace Tests\Feature;

use Tests\TestCase;
use App\Models\GrowthCycle;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\Zone;
use App\Models\CycleParameterLog;
use App\Models\CycleStageHistory;
use App\Models\GrowthStage;
use Illuminate\Foundation\Testing\RefreshDatabase;

class GrowthAnalyticsTest extends TestCase
{
    use RefreshDatabase;

    /** @test */
    public function it_can_get_parameter_chart_via_api()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        $stage = GrowthStage::factory()->create(['preset_id' => $preset->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage->id,
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage->id,
            'started_at' => now()->subDays(5),
        ]);

        foreach (range(1, 3) as $day) {
            CycleParameterLog::factory()->create([
                'cycle_id' => $cycle->id,
                'log_date' => now()->subDays(3 - $day)->startOfDay(),
                'day_number' => $day,
                'avg_ph' => 6.0,
                'avg_ec' => 1.2,
            ]);
        }

        $response = $this->getJson("/api/growth/cycles/{$cycle->id}/analytics/chart");

        $response->assertStatus(200)
            ->assertJsonStructure([
                'stages',
                'data',
                'parameters',
            ]);
    }

    /** @test */
    public function it_can_get_parameter_statistics_via_api()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
        ]);

        foreach (range(1, 5) as $day) {
            CycleParameterLog::factory()->create([
                'cycle_id' => $cycle->id,
                'log_date' => now()->subDays(5 - $day)->startOfDay(),
                'day_number' => $day,
                'avg_ph' => 6.0,
                'avg_ec' => 1.2,
            ]);
        }

        $response = $this->getJson("/api/growth/cycles/{$cycle->id}/analytics/statistics");

        $response->assertStatus(200)
            ->assertJsonStructure([
                'ph' => ['count', 'avg', 'min', 'max'],
                'ec' => ['count', 'avg', 'min', 'max'],
            ]);
    }

    /** @test */
    public function it_can_get_cycle_report_via_api()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        $stage = GrowthStage::factory()->create(['preset_id' => $preset->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage->id,
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $stage->id,
            'started_at' => now()->subDays(10),
        ]);

        $response = $this->getJson("/api/growth/cycles/{$cycle->id}/analytics/report");

        $response->assertStatus(200)
            ->assertJsonStructure([
                'cycle',
                'stages',
                'parameter_statistics',
                'water_consumption',
                'deviations_summary',
            ]);
    }

    /** @test */
    public function it_can_compare_cycles_via_api()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle1 = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'harvest_weight_kg' => 10.0,
        ]);

        $cycle2 = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'harvest_weight_kg' => 12.0,
        ]);

        $response = $this->postJson('/api/growth/analytics/compare', [
            'cycle_ids' => [$cycle1->id, $cycle2->id],
        ]);

        $response->assertStatus(200)
            ->assertJsonStructure([
                'cycles',
                'summary',
            ]);
    }

    /** @test */
    public function it_can_create_snapshot_via_api()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'harvest_weight_kg' => 10.5,
        ]);

        foreach (range(1, 3) as $day) {
            CycleParameterLog::factory()->create([
                'cycle_id' => $cycle->id,
                'log_date' => now()->subDays(3 - $day)->startOfDay(),
                'day_number' => $day,
                'avg_ph' => 6.0,
                'water_consumed_liters' => 5.0,
            ]);
        }

        $response = $this->postJson("/api/growth/cycles/{$cycle->id}/analytics/snapshot");

        $response->assertStatus(201)
            ->assertJsonStructure([
                'id',
                'cycle_id',
                'avg_ph',
                'total_water_liters',
            ]);
    }
}


