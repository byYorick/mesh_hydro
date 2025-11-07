<?php

namespace Tests\Feature;

use Tests\TestCase;
use App\Models\Zone;
use App\Models\Node;
use App\Models\GrowthCycle;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\DB;

/**
 * ⭐ ZONING: Тесты изоляции зон и MQTT топиков
 */
class ZoneIsolationTest extends TestCase
{
    use RefreshDatabase;

    /** @test */
    public function zones_use_different_mqtt_topics()
    {
        $zone1 = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/nft1/',
        ]);

        $zone2 = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/dwc1/',
        ]);

        $zone3 = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/drip1/',
        ]);

        $allZones = Zone::all();
        $prefixes = $allZones->pluck('mqtt_topic_prefix')->toArray();

        // Проверяем, что все префиксы уникальны
        $this->assertEquals(count($prefixes), count(array_unique($prefixes)));

        // Проверяем формат префиксов
        foreach ($prefixes as $prefix) {
            $this->assertStringStartsWith('hydro/', $prefix);
            $this->assertStringEndsWith('/', $prefix);
        }
    }

    /** @test */
    public function zones_use_different_mesh_network_ids()
    {
        $zone1 = Zone::factory()->create([
            'mesh_network_id' => 'HYDRO_MESH_NFT_001',
        ]);

        $zone2 = Zone::factory()->create([
            'mesh_network_id' => 'HYDRO_MESH_DWC_001',
        ]);

        $zone3 = Zone::factory()->create([
            'mesh_network_id' => 'HYDRO_MESH_DRIP_001',
        ]);

        $allZones = Zone::all();
        $meshIds = $allZones->pluck('mesh_network_id')->toArray();

        // Проверяем уникальность
        $this->assertEquals(count($meshIds), count(array_unique($meshIds)));
    }

    /** @test */
    public function node_can_only_belong_to_one_zone_at_a_time()
    {
        $zone1 = Zone::factory()->withRootNode()->create();
        $zone2 = Zone::factory()->withRootNode()->create();

        $node = Node::factory()->create([
            'node_id' => 'ph_test_001',
            'root_node_id' => $zone1->root_node_id,
        ]);

        // Узел принадлежит zone1
        $this->assertEquals($zone1->root_node_id, $node->root_node_id);

        // Пытаемся назначить узел zone2 через API
        $response = $this->postJson("/api/zones/{$zone2->id}/assign-node", [
            'node_id' => $node->node_id,
            'role' => 'ph_sensor',
        ]);

        // Должна быть ошибка валидации
        $response->assertStatus(422);
    }

    /** @test */
    public function telemetry_is_routed_by_zone_topic()
    {
        $zone1 = Zone::factory()->withNodes()->create([
            'mqtt_topic_prefix' => 'hydro/zone1/',
        ]);

        $zone2 = Zone::factory()->withNodes()->create([
            'mqtt_topic_prefix' => 'hydro/zone2/',
        ]);

        // Добавляем телеметрию для zone1
        $node1 = $zone1->getAllNodes()->first();
        DB::table('telemetry')->insert([
            'node_id' => $node1->node_id,
            'node_type' => $node1->node_type,
            'data' => json_encode(['ph' => 6.0, 'zone_marker' => 'zone1']),
            'received_at' => now(),
            'created_at' => now(),
            'updated_at' => now(),
        ]);

        // Добавляем телеметрию для zone2
        $node2 = $zone2->getAllNodes()->first();
        DB::table('telemetry')->insert([
            'node_id' => $node2->node_id,
            'node_type' => $node2->node_type,
            'data' => json_encode(['ph' => 6.5, 'zone_marker' => 'zone2']),
            'received_at' => now(),
            'created_at' => now(),
            'updated_at' => now(),
        ]);

        // Получаем телеметрию через API
        $response1 = $this->getJson("/api/zones/{$zone1->id}/telemetry");
        $response2 = $this->getJson("/api/zones/{$zone2->id}/telemetry");

        $response1->assertStatus(200);
        $response2->assertStatus(200);

        // Телеметрия зон не пересекается
        $data1 = $response1->json();
        $data2 = $response2->json();

        $this->assertNotEquals($data1, $data2);
    }

    /** @test */
    public function commands_are_routed_to_correct_zone()
    {
        $zone1 = Zone::factory()->withNodes()->create([
            'mqtt_topic_prefix' => 'hydro/zone1/',
        ]);

        $zone2 = Zone::factory()->withNodes()->create([
            'mqtt_topic_prefix' => 'hydro/zone2/',
        ]);

        $node1 = $zone1->getAllNodes()->first();
        $node2 = $zone2->getAllNodes()->first();

        // Отправляем команду узлу в zone1
        $response = $this->postJson("/api/zones/{$zone1->id}/command", [
            'node_id' => $node1->node_id,
            'command' => 'update_config',
            'params' => ['target_ph' => 6.0],
        ]);

        $response->assertStatus(200)
            ->assertJsonPath('mqtt_topic', "hydro/zone1/commands/{$node1->node_id}");
    }

    /** @test */
    public function multiple_cycles_run_independently_in_different_zones()
    {
        // Создаем 3 зоны
        $zones = Zone::factory()->withRootNode()->count(3)->create();

        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        // Создаем цикл в каждой зоне
        $cycles = [];
        foreach ($zones as $zone) {
            $cycle = GrowthCycle::factory()->create([
                'zone_id' => $zone->id,
                'preset_id' => $preset->id,
                'culture_id' => $culture->id,
                'status' => 'active',
                'name' => "Цикл в {$zone->name}",
            ]);
            $cycles[] = $cycle;

            // Обновляем зону
            $zone->update(['current_cycle_id' => $cycle->id]);
        }

        // Проверяем, что все циклы активны
        $this->assertEquals(3, GrowthCycle::where('status', 'active')->count());

        // Завершаем один цикл
        $cycles[0]->update(['status' => 'harvested']);
        $zones[0]->update(['current_cycle_id' => null]);

        // Другие циклы остаются активными
        $this->assertEquals(2, GrowthCycle::where('status', 'active')->count());

        // Зона 1 становится доступной
        $zones[0]->refresh();
        $this->assertNull($zones[0]->current_cycle_id);
        $this->assertTrue($zones[0]->isAvailableForCycle());

        // Зоны 2 и 3 заняты
        $zones[1]->refresh();
        $zones[2]->refresh();
        $this->assertNotNull($zones[1]->current_cycle_id);
        $this->assertNotNull($zones[2]->current_cycle_id);
        $this->assertFalse($zones[1]->isAvailableForCycle());
        $this->assertFalse($zones[2]->isAvailableForCycle());
    }

    /** @test */
    public function zone_statistics_are_calculated_independently()
    {
        $zone1 = Zone::factory()->withNodes()->create();
        $zone2 = Zone::factory()->withNodes()->create();

        // Добавляем разную телеметрию для каждой зоны
        $node1 = $zone1->getAllNodes()->first();
        $node2 = $zone2->getAllNodes()->first();

        // Zone 1: pH 6.0
        for ($i = 0; $i < 10; $i++) {
            DB::table('telemetry')->insert([
                'node_id' => $node1->node_id,
                'node_type' => $node1->node_type,
                'data' => json_encode(['ph' => 6.0]),
                'received_at' => now()->subHours($i),
                'created_at' => now(),
                'updated_at' => now(),
            ]);
        }

        // Zone 2: pH 6.5
        for ($i = 0; $i < 10; $i++) {
            DB::table('telemetry')->insert([
                'node_id' => $node2->node_id,
                'node_type' => $node2->node_type,
                'data' => json_encode(['ph' => 6.5]),
                'received_at' => now()->subHours($i),
                'created_at' => now(),
                'updated_at' => now(),
            ]);
        }

        // Получаем статистику для каждой зоны
        $response1 = $this->getJson("/api/zones/{$zone1->id}/statistics?field=ph");
        $response2 = $this->getJson("/api/zones/{$zone2->id}/statistics?field=ph");

        $response1->assertStatus(200);
        $response2->assertStatus(200);

        // Статистика различается
        $avg1 = $response1->json('avg_ph');
        $avg2 = $response2->json('avg_ph');

        $this->assertEquals(6.0, round($avg1, 1));
        $this->assertEquals(6.5, round($avg2, 1));
    }

    /** @test */
    public function root_node_cannot_be_shared_between_zones()
    {
        $rootNode = Node::factory()->create([
            'node_id' => 'root_shared_001',
            'node_type' => 'root',
            'root_node_id' => 'root_shared_001',
        ]);

        $zone1 = Zone::factory()->create([
            'root_node_id' => $rootNode->node_id,
        ]);

        // Попытка создать вторую зону с тем же Root Node
        $response = $this->postJson('/api/zones', [
            'name' => 'Zone 2',
            'root_node_id' => $rootNode->node_id,
            'mesh_network_id' => 'HYDRO_MESH_002',
            'mqtt_topic_prefix' => 'hydro/zone2/',
            'zone_type' => 'dwc',
        ]);

        $response->assertStatus(422)
            ->assertJsonValidationErrors(['root_node_id']);
    }

    /** @test */
    public function zone_can_be_disabled_without_affecting_other_zones()
    {
        $zone1 = Zone::factory()->withRootNode()->create(['is_active' => true]);
        $zone2 = Zone::factory()->withRootNode()->create(['is_active' => true]);
        $zone3 = Zone::factory()->withRootNode()->create(['is_active' => true]);

        // Отключаем zone1
        $response = $this->patchJson("/api/zones/{$zone1->id}", [
            'is_active' => false,
        ]);

        $response->assertStatus(200);

        // Проверяем статус зон
        $zone1->refresh();
        $zone2->refresh();
        $zone3->refresh();

        $this->assertFalse($zone1->is_active);
        $this->assertTrue($zone2->is_active);
        $this->assertTrue($zone3->is_active);

        // Проверяем количество активных зон
        $this->assertEquals(2, Zone::where('is_active', true)->count());
    }
}

