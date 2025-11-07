<?php

namespace Tests\Feature;

use App\Http\Controllers\ZoneController;
use App\Models\GrowthCycle;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Zone;
use App\Models\ZoneNodeAssignment;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\Config;
use Tests\TestCase;

class ZoneApiTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();
        Config::set('hydro.node_offline_timeout', 300);
    }

    public function test_index_applies_filters_and_returns_meta(): void
    {
        $availableZone = Zone::factory()->available()->create([
            'is_available' => true,
            'is_active' => true,
        ]);

        $busyZone = Zone::factory()->create([
            'is_available' => false,
            'is_active' => true,
            'current_cycle_id' => 42,
        ]);

        $response = $this->getJson('/api/zones?available=true');

        $response->assertOk()
            ->assertJsonPath('meta.total', 1)
            ->assertJsonPath('meta.available', 1)
            ->assertJsonPath('data.0.id', $availableZone->id);

        // Ensure busy zone excluded
        $this->assertNotEquals($busyZone->id, $response->json('data.0.id'));
    }

    public function test_store_creates_zone_with_root_node_validation(): void
    {
        $rootNode = Node::create([
            'node_id' => 'root_api',
            'node_type' => 'root',
            'root_node_id' => 'root_api',
            'online' => true,
            'last_seen_at' => now(),
        ]);

        $payload = [
            'name' => 'API Zone',
            'description' => 'Test description',
            'location' => 'Rack 1',
            'root_node_id' => $rootNode->node_id,
            'mesh_network_id' => 'MESH_API_1',
            'mqtt_topic_prefix' => 'hydro/api_zone/',
            'zone_type' => 'nft',
            'reservoir_volume_liters' => 120,
            'growing_area_m2' => 4.5,
            'plant_capacity' => 24,
            'is_active' => true,
            'is_available' => true,
        ];

        $this->postJson('/api/zones', $payload)
            ->assertCreated()
            ->assertJsonPath('data.root_node.node_id', 'root_api');

        $this->assertDatabaseHas('zones', [
            'name' => 'API Zone',
            'root_node_id' => 'root_api',
        ]);

        // Attempt to use non-root node should fail
        $nonRoot = Node::factory()->create([
            'node_id' => 'non_root',
            'node_type' => 'ph_ec',
        ]);

        $payload['name'] = 'Invalid Zone';
        $payload['mesh_network_id'] = 'MESH_API_2';
        $payload['root_node_id'] = $nonRoot->node_id;

        $this->postJson('/api/zones', $payload)
            ->assertStatus(422)
            ->assertJsonPath('message', 'Указанный узел не является Root Node');
    }

    public function test_get_nodes_returns_root_and_child_nodes(): void
    {
        $zone = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_1',
                'climate_node' => 'climate_zone_1',
            ],
        ]);

        $response = $this->getJson('/api/zones/' . $zone->id . '/nodes');

        $response->assertOk()
            ->assertJsonPath('data.root_node.node_id', $zone->root_node_id)
            ->assertJsonPath('data.total_nodes', 3);
    }

    public function test_send_command_validates_node_belongs_to_zone(): void
    {
        $zoneA = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_a',
                'climate_node' => 'climate_zone_a',
            ],
            'mqtt_topic_prefix' => 'hydro/zoneA/',
        ]);

        $zoneB = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_b',
            ],
        ]);

        $this->postJson('/api/zones/' . $zoneA->id . '/command', [
            'node_id' => 'ph_zone_a',
            'command' => 'sync_config',
            'params' => ['mode' => 'auto'],
        ])
            ->assertOk()
            ->assertJsonPath('data.mqtt_topic', 'hydro/zoneA/commands/ph_zone_a');

        $this->postJson('/api/zones/' . $zoneA->id . '/command', [
            'node_id' => 'ph_zone_b',
            'command' => 'sync_config',
        ])
            ->assertStatus(422)
            ->assertJsonPath('message', 'Узел не принадлежит этой зоне');
    }

    public function test_update_zone_applies_changes(): void
    {
        $zone = Zone::factory()->create([
            'name' => 'Original Zone',
            'zone_type' => 'nft',
            'is_active' => true,
        ]);

        $this->putJson('/api/zones/' . $zone->id, [
            'name' => 'Updated Zone',
            'zone_type' => 'dwc',
            'is_active' => false,
        ])
            ->assertOk()
            ->assertJsonPath('data.name', 'Updated Zone')
            ->assertJsonPath('data.zone_type', 'dwc')
            ->assertJsonPath('data.is_active', false);

        $this->assertDatabaseHas('zones', [
            'id' => $zone->id,
            'name' => 'Updated Zone',
            'zone_type' => 'dwc',
            'is_active' => false,
        ]);
    }

    public function test_destroy_zone_requires_no_active_cycle(): void
    {
        $zone = Zone::factory()->create([
            'current_cycle_id' => null,
        ]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
        ]);

        $zone->update(['current_cycle_id' => $cycle->id]);

        $this->deleteJson('/api/zones/' . $zone->id)
            ->assertStatus(422)
            ->assertJsonPath('message', 'Невозможно удалить зону с активным циклом роста');

        $zone->update(['current_cycle_id' => null]);

        $this->deleteJson('/api/zones/' . $zone->id)
            ->assertOk()
            ->assertJsonPath('message', 'Зона успешно удалена');

        $this->assertDatabaseMissing('zones', ['id' => $zone->id]);
    }

    public function test_check_availability_returns_zone_status(): void
    {
        $zone = Zone::factory()->create([
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => null,
        ]);

        $response = app(ZoneController::class)->checkAvailability($zone->id);
        $payload = $response->getData(true);

        $this->assertTrue($payload['success']);
        $this->assertTrue($payload['data']['is_available']);
        $this->assertTrue($payload['data']['zone_status']['is_active']);
        $this->assertFalse($payload['data']['zone_status']['has_current_cycle']);
    }

    public function test_get_root_node_returns_node_details(): void
    {
        $zone = Zone::factory()->withRootNode()->create();

        $this->getJson('/api/zones/' . $zone->id . '/root-node')
            ->assertOk()
            ->assertJsonPath('data.node_id', $zone->root_node_id)
            ->assertJsonPath('data.node_type', 'root');
    }

    public function test_assign_node_updates_zone_and_history(): void
    {
        $zone = Zone::factory()->withRootNode()->create([
            'assigned_nodes' => [],
        ]);

        $freeNode = Node::factory()->create([
            'node_id' => 'free_ph_node',
            'node_type' => 'ph_ec',
            'root_node_id' => null,
        ]);

        $this->postJson('/api/zones/' . $zone->id . '/assign-node', [
            'node_id' => $freeNode->node_id,
            'role' => 'ph_node',
        ])
            ->assertOk()
            ->assertJsonPath('data.role', 'ph_node');

        $this->assertDatabaseHas('nodes', [
            'node_id' => 'free_ph_node',
            'root_node_id' => $zone->root_node_id,
        ]);

        $this->assertDatabaseHas('zones', [
            'id' => $zone->id,
            'assigned_nodes->ph_node' => 'free_ph_node',
        ]);

        $this->assertDatabaseHas('zone_node_assignments', [
            'zone_id' => $zone->id,
            'node_id' => 'free_ph_node',
            'node_role' => 'ph_node',
        ]);
    }

    public function test_check_node_availability_returns_statuses(): void
    {
        $zone = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_status',
            ],
        ]);

        $response = $this->postJson('/api/zones/check-nodes-availability', [
            'node_ids' => ['ph_zone_status', 'unknown_node'],
        ]);

        $response->assertStatus(422)
            ->assertJsonValidationErrorFor('node_ids.1');

        $response = $this->postJson('/api/zones/check-nodes-availability', [
            'node_ids' => ['ph_zone_status'],
        ]);

        $response->assertOk()
            ->assertJsonPath('data.ph_zone_status.available', false)
            ->assertJsonPath('data.ph_zone_status.used_in_zone', $zone->name);
    }

    public function test_get_telemetry_returns_data_for_zone_nodes(): void
    {
        $zone = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_t',
                'climate_node' => 'climate_zone_t',
            ],
        ]);

        $otherZone = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_other',
            ],
        ]);

        Telemetry::create([
            'node_id' => 'ph_zone_t',
            'node_type' => 'ph_ec',
            'data' => ['ph' => 6.3, 'zone_marker' => 'A'],
            'received_at' => now(),
        ]);

        Telemetry::create([
            'node_id' => 'ph_zone_other',
            'node_type' => 'ph_ec',
            'data' => ['ph' => 5.9, 'zone_marker' => 'B'],
            'received_at' => now(),
        ]);

        $response = $this->getJson('/api/zones/' . $zone->id . '/telemetry');

        $response->assertOk()
            ->assertJsonCount(1, 'data')
            ->assertJsonPath('data.0.node_id', 'ph_zone_t');
    }

    public function test_get_statistics_returns_aggregated_metrics(): void
    {
        $zone = Zone::factory()->withNodes()->create([
            'assigned_nodes' => [
                'ph_node' => 'ph_zone_stats',
            ],
        ]);

        Telemetry::create([
            'node_id' => 'ph_zone_stats',
            'node_type' => 'ph_ec',
            'data' => ['ph' => 6.0],
            'received_at' => now()->subHours(1),
        ]);

        Telemetry::create([
            'node_id' => 'ph_zone_stats',
            'node_type' => 'ph_ec',
            'data' => ['ph' => 6.4],
            'received_at' => now()->subHours(2),
        ]);

        $statsResponse = $this->getJson('/api/zones/' . $zone->id . '/statistics?field=ph')
            ->assertOk()
            ->assertJsonPath('meta.field', 'ph');

        $avgPh = (float) $statsResponse->json('data.avg_ph');
        $this->assertEqualsWithDelta(6.2, $avgPh, 0.0001);
    }
}
