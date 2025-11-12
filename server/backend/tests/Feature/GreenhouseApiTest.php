<?php

namespace Tests\Feature;

use App\Models\Greenhouse;
use App\Models\Node;
use App\Models\Zone;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Testing\Fluent\AssertableJson;
use Tests\TestCase;

class GreenhouseApiTest extends TestCase
{
    use RefreshDatabase;

    public function test_can_create_greenhouse_with_root_node(): void
    {
        $rootNode = Node::factory()->root()->create([
            'node_id' => 'root_test_1001',
        ]);

        $payload = [
            'name' => 'Test Greenhouse',
            'code' => 'GH_TEST_CREATE',
            'root_node_id' => $rootNode->node_id,
        ];

        $response = $this->postJson('/api/greenhouses', $payload);

        $response->assertCreated()
            ->assertJsonPath('data.name', 'Test Greenhouse')
            ->assertJsonPath('data.root_node_id', 'root_test_1001');

        $greenhouseId = $response->json('data.id');
        $this->assertDatabaseHas('greenhouses', [
            'id' => $greenhouseId,
            'code' => 'GH_TEST_CREATE',
            'root_node_id' => 'root_test_1001',
        ]);

        $this->assertDatabaseHas('nodes', [
            'node_id' => 'root_test_1001',
            'greenhouse_id' => $greenhouseId,
        ]);
    }

    public function test_can_list_greenhouses(): void
    {
        Greenhouse::factory()->count(2)->active()->create();

        $response = $this->getJson('/api/greenhouses');

        $response->assertOk()
            ->assertJsonCount(2, 'data');
    }

    public function test_show_greenhouse_returns_details(): void
    {
        $greenhouse = Greenhouse::factory()->active()->create([
            'name' => 'Detail Greenhouse',
            'code' => 'GH_DETAIL_01',
        ]);

        $zone = Zone::factory()->forGreenhouse($greenhouse->id)->create();
        $node = Node::factory()->create([
            'node_id' => 'node_detail_01',
            'root_node_id' => $zone->root_node_id,
            'greenhouse_id' => $greenhouse->id,
        ]);

        $response = $this->getJson("/api/greenhouses/{$greenhouse->id}");

        $response->assertOk()
            ->assertJson(fn (AssertableJson $json) => $json
                ->where('data.name', 'Detail Greenhouse')
                ->where('data.zone_count', 1)
                ->where('data.node_count', 1)
                ->where('data.zones.0.id', $zone->id)
                ->where('data.nodes.0.node_id', 'node_detail_01')
                ->has('data.automation_rules')
                ->etc()
            );
    }

    public function test_can_attach_and_detach_zone(): void
    {
        $greenhouse = Greenhouse::factory()->create();
        $zone = Zone::factory()->create(['greenhouse_id' => null]);

        $attachResponse = $this->postJson("/api/greenhouses/{$greenhouse->id}/zones", [
            'zone_id' => $zone->id,
        ]);

        $attachResponse->assertOk()
            ->assertJsonPath('data.zone_id', $zone->id);

        $this->assertDatabaseHas('zones', [
            'id' => $zone->id,
            'greenhouse_id' => $greenhouse->id,
        ]);

        $detachResponse = $this->deleteJson("/api/greenhouses/{$greenhouse->id}/zones/{$zone->id}");

        $detachResponse->assertOk()
            ->assertJsonPath('success', true);

        $this->assertDatabaseHas('zones', [
            'id' => $zone->id,
            'greenhouse_id' => null,
        ]);
    }

    public function test_cannot_detach_zone_with_active_cycle(): void
    {
        $greenhouse = Greenhouse::factory()->create();
        $zone = Zone::factory()->forGreenhouse($greenhouse->id)->create([
            'current_cycle_id' => 123,
        ]);

        $response = $this->deleteJson("/api/greenhouses/{$greenhouse->id}/zones/{$zone->id}");

        $response->assertStatus(422);

        $this->assertDatabaseHas('zones', [
            'id' => $zone->id,
            'greenhouse_id' => $greenhouse->id,
        ]);
    }

    public function test_can_attach_and_detach_node(): void
    {
        $greenhouse = Greenhouse::factory()->create();
        $node = Node::factory()->create([
            'node_id' => 'node_attach_1',
        ]);

        $attachResponse = $this->postJson("/api/greenhouses/{$greenhouse->id}/nodes", [
            'node_id' => $node->node_id,
        ]);

        $attachResponse->assertOk()
            ->assertJsonPath('data.node_id', $node->node_id);

        $this->assertDatabaseHas('nodes', [
            'node_id' => $node->node_id,
            'greenhouse_id' => $greenhouse->id,
        ]);

        $detachResponse = $this->deleteJson("/api/greenhouses/{$greenhouse->id}/nodes/{$node->node_id}");

        $detachResponse->assertOk();

        $this->assertDatabaseHas('nodes', [
            'node_id' => $node->node_id,
            'greenhouse_id' => null,
        ]);
    }

    public function test_can_manage_automation_rules(): void
    {
        $greenhouse = Greenhouse::factory()->create();

        $createResponse = $this->postJson("/api/greenhouses/{$greenhouse->id}/automation-rules", [
            'name' => 'Morning irrigation',
            'description' => 'Run pumps every morning at 08:00',
            'trigger_type' => 'schedule',
            'trigger_config' => ['time' => '08:00', 'days' => ['mon', 'wed', 'fri']],
            'actions' => [
                ['action' => 'run_pump', 'params' => ['pump_id' => 1, 'duration_sec' => 10]],
            ],
        ]);

        $createResponse->assertCreated()
            ->assertJsonPath('name', 'Morning irrigation');

        $ruleId = $createResponse->json('id');

        $updateResponse = $this->putJson("/api/greenhouses/{$greenhouse->id}/automation-rules/{$ruleId}", [
            'enabled' => false,
            'description' => 'Disabled for maintenance',
        ]);

        $updateResponse->assertOk()
            ->assertJsonPath('enabled', false)
            ->assertJsonPath('description', 'Disabled for maintenance');

        $deleteResponse = $this->deleteJson("/api/greenhouses/{$greenhouse->id}/automation-rules/{$ruleId}");

        $deleteResponse->assertOk()
            ->assertJsonPath('success', true);

        $this->assertDatabaseMissing('greenhouse_automation_rules', [
            'id' => $ruleId,
        ]);
    }
}


