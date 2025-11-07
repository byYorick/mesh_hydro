<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Console\Commands\LogCycleParametersCommand;
use App\Models\GrowthCycle;
use App\Models\Zone;
use App\Models\Node;
use App\Models\CycleParameterLog;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\DB;

class LogCycleParametersCommandTest extends TestCase
{
    use RefreshDatabase;

    #[Test]
    public function it_logs_parameters_for_active_cycles()
    {
        // Создаем зону с узлами
        $zone = Zone::factory()->withNodes()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'status' => 'active',
            'started_at' => now()->subDays(5),
        ]);

        // Добавляем телеметрию для узлов зоны
        $nodes = $zone->getAllNodes();
        foreach ($nodes as $node) {
            DB::table('telemetry')->insert([
                'node_id' => $node->node_id,
                'node_type' => $node->node_type,
                'data' => json_encode([
                    'ph' => 6.0,
                    'ec' => 1.5,
                    'temp' => 22.5,
                    'humidity' => 65,
                ]),
                'received_at' => now()->subHours(2),
                'created_at' => now(),
                'updated_at' => now(),
            ]);
        }

        // Запускаем команду
        $this->artisan('growth:log-parameters')
            ->assertExitCode(0);

        // Проверяем создание лога
        $log = CycleParameterLog::where('cycle_id', $cycle->id)
            ->where('log_date', now()->startOfDay())
            ->first();

        $this->assertNotNull($log);
        $this->assertEquals($cycle->id, $log->cycle_id);
        $this->assertNotNull($log->avg_ph);
    }

    #[Test]
    public function it_skips_cycles_without_telemetry()
    {
        $zone = Zone::factory()->withRootNode()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'status' => 'active',
        ]);

        // Запускаем команду без телеметрии
        $this->artisan('growth:log-parameters')
            ->assertExitCode(0);

        // Проверяем, что лог не создан
        $logCount = CycleParameterLog::where('cycle_id', $cycle->id)->count();
        $this->assertEquals(0, $logCount);
    }

    #[Test]
    public function it_can_log_parameters_for_specific_cycle()
    {
        $zone = Zone::factory()->withNodes()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle1 = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'status' => 'active',
        ]);

        $zone2 = Zone::factory()->withNodes()->create();
        $cycle2 = GrowthCycle::factory()->create([
            'zone_id' => $zone2->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'status' => 'active',
        ]);

        // Добавляем телеметрию
        $nodes = $zone->getAllNodes();
        foreach ($nodes as $node) {
            DB::table('telemetry')->insert([
                'node_id' => $node->node_id,
                'node_type' => $node->node_type,
                'data' => json_encode(['ph' => 6.0]),
                'received_at' => now()->subHours(1),
                'created_at' => now(),
                'updated_at' => now(),
            ]);
        }

        // Запускаем команду только для cycle1
        $this->artisan('growth:log-parameters', ['--cycle-id' => $cycle1->id])
            ->assertExitCode(0);

        // Проверяем, что лог создан только для cycle1
        $this->assertDatabaseHas('cycle_parameter_logs', ['cycle_id' => $cycle1->id]);
        $this->assertDatabaseMissing('cycle_parameter_logs', ['cycle_id' => $cycle2->id]);
    }
}

