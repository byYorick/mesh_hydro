<?php

namespace Tests\Feature;

use App\Models\Event;
use App\Models\Node;
use Carbon\Carbon;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Tests\TestCase;

class EventApiTest extends TestCase
{
    use RefreshDatabase;

    protected function tearDown(): void
    {
        Carbon::setTestNow();
        parent::tearDown();
    }

    public function test_index_filters_and_enriches_events(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 1, 12, 0));

        $nodeA = Node::factory()->create(['node_id' => 'node-A']);
        $nodeB = Node::factory()->create(['node_id' => 'node-B']);

        Event::factory()->forNode($nodeA->node_id)
            ->state([
                'level' => Event::LEVEL_WARNING,
                'created_at' => now()->subHours(2),
                'updated_at' => now()->subHours(2),
            ])->create();

        Event::factory()->forNode($nodeB->node_id)
            ->critical()
            ->state([
                'created_at' => now()->subHours(5),
                'updated_at' => now()->subHours(5),
            ])->create();

        Event::factory()->forNode($nodeB->node_id)
            ->state([
                'level' => Event::LEVEL_INFO,
                'created_at' => now()->subHours(10),
                'updated_at' => now()->subHours(10),
            ])->create();

        $response = $this->getJson('/api/events?node_id=node-B&critical_only=1&hours=6&limit=5');

        $response->assertOk();

        $events = $response->json();
        $this->assertCount(1, $events);
        $this->assertSame('node-B', $events[0]['node_id']);
        $this->assertSame(Event::LEVEL_CRITICAL, $events[0]['level']);
        $this->assertSame('red', $events[0]['level_color']);
        $this->assertSame('mdi-alert-circle', $events[0]['level_icon']);
    }

    public function test_index_can_paginate_results(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 2, 10));

        Event::factory()->count(3)->create();

        $response = $this->getJson('/api/events?paginate=1&per_page=2');

        $response->assertOk()
            ->assertJsonPath('per_page', 2)
            ->assertJsonPath('total', 3);
    }

    public function test_show_returns_event_with_node_and_icons(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 3, 9));

        $node = Node::factory()->create(['node_id' => 'node-show']);

        $event = Event::factory()->forNode($node->node_id)
            ->critical()
            ->create([
                'message' => 'Reservoir empty',
            ]);

        $response = $this->getJson('/api/events/' . $event->id);

        $response->assertOk()
            ->assertJsonPath('id', $event->id)
            ->assertJsonPath('node.node_id', 'node-show')
            ->assertJsonPath('level_color', 'red')
            ->assertJsonPath('level_icon', 'mdi-alert-circle')
            ->assertJsonPath('message', 'Reservoir empty');
    }

    public function test_resolve_marks_event_and_prevents_duplicate_resolution(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 4, 8));

        $event = Event::factory()->create();

        $resolveResponse = $this->postJson('/api/events/' . $event->id . '/resolve', [
            'resolved_by' => 'operator-1',
        ]);

        $resolveResponse->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('event.resolved_by', 'operator-1');

        $this->assertNotNull($event->fresh()->resolved_at);

        $secondAttempt = $this->postJson('/api/events/' . $event->id . '/resolve');
        $secondAttempt->assertStatus(400)
            ->assertJsonPath('error', 'Event is already resolved');
    }

    public function test_resolve_bulk_updates_unresolved_events(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 5, 7));

        $events = Event::factory()->count(3)->create();
        $alreadyResolved = Event::factory()->resolved()->create();

        $ids = $events->pluck('id')->merge([$alreadyResolved->id])->all();

        $response = $this->postJson('/api/events/resolve-bulk', [
            'event_ids' => $ids,
            'resolved_by' => 'bulk-operator',
        ]);

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('count', 3);

        $this->assertEquals(3, Event::resolved()->whereIn('id', $events->pluck('id'))->count());
        $this->assertEquals('bulk-operator', Event::find($events[0]->id)->resolved_by);
        $this->assertNotNull(Event::find($events[1]->id)->resolved_at);
    }

    public function test_statistics_returns_counts_and_breakdowns(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 6, 12));

        Event::factory()->count(2)->create([
            'level' => Event::LEVEL_WARNING,
            'created_at' => now()->subHours(3),
            'updated_at' => now()->subHours(3),
        ]);

        Event::factory()->create([
            'level' => Event::LEVEL_INFO,
            'created_at' => now()->subHours(2),
            'updated_at' => now()->subHours(2),
        ]);

        Event::factory()->resolved()->create([
            'level' => Event::LEVEL_CRITICAL,
            'created_at' => now()->subHours(1),
            'updated_at' => now()->subHours(1),
        ]);

        Event::factory()->create([
            'created_at' => now()->subHours(30),
            'updated_at' => now()->subHours(30),
        ]);

        $response = $this->getJson('/api/events/statistics?hours=24');

        $response->assertOk();

        $this->assertSame(24, (int) $response->json('period_hours'));

        $response
            ->assertJsonPath('total', 4)
            ->assertJsonPath('resolved', 1)
            ->assertJsonPath('active', 3);

        $byLevel = $response->json('by_level');
        $this->assertEquals(2, $byLevel[Event::LEVEL_WARNING]);
        $this->assertEquals(1, $byLevel[Event::LEVEL_INFO]);

        $topNodes = $response->json('top_nodes');
        $this->assertNotEmpty($topNodes);
        $this->assertArrayHasKey('node_id', $topNodes[0]);
        $this->assertArrayHasKey('count', $topNodes[0]);
    }

    public function test_destroy_requires_resolved_event(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 5, 7, 15));

        $event = Event::factory()->create();

        $this->deleteJson('/api/events/' . $event->id)
            ->assertStatus(400)
            ->assertJsonPath('error', 'Can only delete resolved events');

        $event->resolve('operator');

        $this->deleteJson('/api/events/' . $event->id)
            ->assertOk()
            ->assertJsonPath('success', true);

        $this->assertDatabaseMissing('events', ['id' => $event->id]);
    }
}


