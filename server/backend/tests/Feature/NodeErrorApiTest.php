<?php

namespace Tests\Feature;

use App\Models\Node;
use App\Models\NodeError;
use Carbon\Carbon;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Tests\TestCase;

class NodeErrorApiTest extends TestCase
{
    use RefreshDatabase;

    protected function tearDown(): void
    {
        Carbon::setTestNow();
        parent::tearDown();
    }

    public function test_index_filters_and_enriches_errors(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 1, 12));

        $nodeA = Node::factory()->create(['node_id' => 'node-A']);
        $nodeB = Node::factory()->create(['node_id' => 'node-B']);

        NodeError::factory()->forNode($nodeA->node_id)
            ->state([
                'severity' => NodeError::SEVERITY_HIGH,
                'error_type' => NodeError::TYPE_SENSOR,
            ])->occurredAt(now()->subHours(2))->create();

        NodeError::factory()->forNode($nodeB->node_id)
            ->critical()
            ->state([
                'error_type' => NodeError::TYPE_NETWORK,
            ])->occurredAt(now()->subHours(3))->create();

        NodeError::factory()->forNode($nodeB->node_id)
            ->state([
                'severity' => NodeError::SEVERITY_LOW,
                'error_type' => NodeError::TYPE_SOFTWARE,
            ])->occurredAt(now()->subHours(30))->create();

        $response = $this->getJson('/api/errors?node_id=node-B&critical_only=1&hours=6&limit=10');

        $response->assertOk();
        $errors = $response->json();

        $this->assertCount(1, $errors);
        $this->assertSame('node-B', $errors[0]['node_id']);
        $this->assertSame(NodeError::SEVERITY_CRITICAL, $errors[0]['severity']);
        $this->assertSame('error', $errors[0]['severity_color']);
        $this->assertSame('mdi-wifi-alert', $errors[0]['type_icon']);
    }

    public function test_index_can_paginate(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 2, 8));

        NodeError::factory()->count(3)->create();

        $response = $this->getJson('/api/errors?paginate=1&per_page=2');

        $response->assertOk()
            ->assertJsonPath('per_page', 2)
            ->assertJsonPath('total', 3);
    }

    public function test_show_returns_error_with_node_and_icons(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 3, 9));

        $node = Node::factory()->create(['node_id' => 'node-show']);

        $error = NodeError::factory()->forNode($node->node_id)
            ->critical()
            ->create([
                'error_type' => NodeError::TYPE_HARDWARE,
                'message' => 'Pump failure',
            ]);

        $response = $this->getJson('/api/errors/' . $error->id);

        $response->assertOk()
            ->assertJsonPath('id', $error->id)
            ->assertJsonPath('node.node_id', 'node-show')
            ->assertJsonPath('severity_color', 'error')
            ->assertJsonPath('type_icon', 'mdi-chip')
            ->assertJsonPath('message', 'Pump failure');
    }

    public function test_for_node_applies_filters(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 4, 10));

        $node = Node::factory()->create(['node_id' => 'node-specific']);

        NodeError::factory()->forNode($node->node_id)
            ->occurredAt(now()->subHours(2))->create();

        NodeError::factory()->forNode($node->node_id)
            ->resolved()->occurredAt(now()->subHours(3))->create();

        NodeError::factory()->forNode($node->node_id)
            ->occurredAt(now()->subHours(10))->create();

        $response = $this->getJson('/api/nodes/' . $node->node_id . '/errors?status=active&hours=6&limit=10');

        $response->assertOk()
            ->assertJsonPath('node_id', 'node-specific')
            ->assertJsonPath('count', 1);

        $errors = $response->json('errors');
        $this->assertCount(1, $errors);
        $this->assertNull($errors[0]['resolved_at']);
    }

    public function test_statistics_returns_breakdowns(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 5, 11));

        NodeError::factory()->count(2)->create([
            'severity' => NodeError::SEVERITY_HIGH,
            'error_type' => NodeError::TYPE_SENSOR,
        ]);

        NodeError::factory()->create([
            'severity' => NodeError::SEVERITY_LOW,
            'error_type' => NodeError::TYPE_SOFTWARE,
        ]);

        NodeError::factory()->critical()->resolved()->create([
            'error_type' => NodeError::TYPE_NETWORK,
        ]);

        NodeError::factory()->occurredAt(now()->subHours(30))->create();

        $response = $this->getJson('/api/errors/statistics?hours=24');

        $response->assertOk();

        $this->assertSame(24, (int) $response->json('period_hours'));

        $response
            ->assertJsonPath('total', 4)
            ->assertJsonPath('critical', 1);

        $bySeverity = $response->json('by_severity');
        $this->assertEquals(2, $bySeverity[NodeError::SEVERITY_HIGH]);

        $byType = $response->json('by_type');
        $this->assertEquals(2, $byType[NodeError::TYPE_SENSOR]);

        $this->assertNotEmpty($response->json('top_nodes'));
        $this->assertNotEmpty($response->json('top_error_codes'));
    }

    public function test_resolve_marks_error_and_prevents_duplicate(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 6, 14));

        $error = NodeError::factory()->create();

        $resolveResponse = $this->postJson('/api/errors/' . $error->id . '/resolve', [
            'resolved_by' => 'tech-1',
            'resolution_notes' => 'Replaced sensor',
        ]);

        $resolveResponse->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('error.resolution_notes', 'Replaced sensor');

        $this->assertNotNull($error->fresh()->resolved_at);

        $secondAttempt = $this->postJson('/api/errors/' . $error->id . '/resolve');
        $secondAttempt->assertStatus(400)
            ->assertJsonPath('error', 'Error is already resolved');
    }

    public function test_resolve_bulk_updates_unresolved_errors(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 7, 16));

        $errors = NodeError::factory()->count(3)->create();
        $resolved = NodeError::factory()->resolved()->create();

        $ids = $errors->pluck('id')->merge([$resolved->id])->all();

        $response = $this->postJson('/api/errors/resolve-bulk', [
            'error_ids' => $ids,
            'resolved_by' => 'bulk-tech',
            'resolution_notes' => 'Bulk resolution',
        ]);

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('count', 3);

        $this->assertEquals(3, NodeError::resolved()->whereIn('id', $errors->pluck('id'))->count());
        $this->assertEquals('bulk-tech', NodeError::find($errors[0]->id)->resolved_by);
    }

    public function test_destroy_requires_resolved_error(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 6, 8, 18));

        $error = NodeError::factory()->create();

        $this->deleteJson('/api/errors/' . $error->id)
            ->assertStatus(400)
            ->assertJsonPath('error', 'Can only delete resolved errors');

        $error->resolve('tech');

        $this->deleteJson('/api/errors/' . $error->id)
            ->assertOk()
            ->assertJsonPath('success', true);

        $this->assertDatabaseMissing('node_errors', ['id' => $error->id]);
    }
}


