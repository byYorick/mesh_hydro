<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Models\Zone;
use App\Models\Node;
use App\Models\GrowthCulture;
use App\Models\GrowthPreset;
use App\Models\GrowthStage;
use App\Models\GrowthCycle;
use App\Models\CycleStageHistory;
use Illuminate\Foundation\Testing\RefreshDatabase;

/**
 * ⭐ GROWTH PLANNER: Unit тесты для GrowthCycle (с зонами!)
 */
class GrowthCycleModelTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();

        // Создать Root Node
        $this->rootNode = Node::create([
            'node_id' => 'root_test',
            'node_type' => 'root',
            'mac_address' => 'AA:BB:CC:DD:EE:FF',
            'online' => true,
        ]);

        // Создать зону
        $this->zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => $this->rootNode->node_id,
            'mesh_network_id' => 'TEST_MESH',
            'mqtt_topic_prefix' => 'hydro/test/',
            'zone_type' => 'nft',
            'is_active' => true,
            'is_available' => true,
        ]);

        // Создать культуру
        $this->culture = GrowthCulture::create([
            'name' => 'Test Culture',
            'slug' => 'test-culture',
            'category' => 'leafy_greens',
        ]);

        // Создать пресет со стадиями
        $this->preset = GrowthPreset::create([
            'culture_id' => $this->culture->id,
            'name' => 'Test Preset',
            'slug' => 'test-preset',
            'preset_type' => 'custom',
            'total_days' => 30,
            'difficulty' => 'easy',
            'recommended_system' => 'nft',
        ]);

        $this->stage1 = GrowthStage::create([
            'preset_id' => $this->preset->id,
            'name' => 'Stage 1',
            'order' => 1,
            'duration_days' => 15,
            'target_params' => ['ph_min' => 5.5, 'ph_max' => 6.5],
        ]);

        $this->stage2 = GrowthStage::create([
            'preset_id' => $this->preset->id,
            'name' => 'Stage 2',
            'order' => 2,
            'duration_days' => 15,
            'target_params' => ['ph_min' => 5.5, 'ph_max' => 6.5],
        ]);
    }

    #[Test]
    public function it_can_create_a_cycle_with_zone()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now(),
            'expected_harvest_at' => now()->addDays(30),
            'status' => 'active',
        ]);

        $this->assertDatabaseHas('growth_cycles', [
            'zone_id' => $this->zone->id,
            'status' => 'active',
        ]);
    }

    #[Test]
    public function it_belongs_to_zone()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now(),
            'status' => 'active',
        ]);

        $this->assertInstanceOf(Zone::class, $cycle->zone);
        $this->assertEquals('Test Zone', $cycle->zone->name);
    }

    #[Test]
    public function it_calculates_progress_percent()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now()->subDays(15),
            'expected_harvest_at' => now()->addDays(15),
            'status' => 'active',
        ]);

        $this->assertEquals(50, round($cycle->progress));
    }

    #[Test]
    public function it_gets_current_day()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now()->subDays(10),
            'status' => 'active',
        ]);

        $this->assertEquals(11, $cycle->current_day);
    }

    #[Test]
    public function it_gets_next_stage()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now(),
            'status' => 'active',
        ]);

        $nextStage = $cycle->getNextStage();

        $this->assertNotNull($nextStage);
        $this->assertEquals($this->stage2->id, $nextStage->id);
    }

    #[Test]
    public function it_can_transition_to_next_stage()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now(),
            'status' => 'active',
        ]);

        CycleStageHistory::create([
            'cycle_id' => $cycle->id,
            'stage_id' => $this->stage1->id,
            'started_at' => now(),
        ]);

        $result = $cycle->transitionToNextStage();

        $this->assertTrue($result);
        $cycle->refresh();
        $this->assertEquals($this->stage2->id, $cycle->current_stage_id);
    }

    #[Test]
    public function it_has_status_color_attribute()
    {
        $cycle = GrowthCycle::create([
            'zone_id' => $this->zone->id,
            'preset_id' => $this->preset->id,
            'culture_id' => $this->culture->id,
            'current_stage_id' => $this->stage1->id,
            'started_at' => now(),
            'status' => 'active',
        ]);

        $this->assertEquals('success', $cycle->status_color);
    }
}

