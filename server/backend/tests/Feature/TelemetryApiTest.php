<?php

namespace Tests\Feature;

use App\Models\Node;
use App\Models\Telemetry;
use Carbon\Carbon;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Tests\TestCase;

class TelemetryApiTest extends TestCase
{
    use RefreshDatabase;

    protected function tearDown(): void
    {
        Carbon::setTestNow();
        parent::tearDown();
    }

    public function test_index_returns_filtered_results(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 1, 1, 12));

        Telemetry::factory()->forNode('node-1', 'climate')
            ->receivedAt(now()->subHours(2))
            ->withData(['temp' => 23.5])
            ->create();

        Telemetry::factory()->forNode('node-1', 'climate')
            ->receivedAt(now()->subHours(30))
            ->withData(['temp' => 19.5])
            ->create();

        Telemetry::factory()->forNode('node-2', 'water')
            ->receivedAt(now()->subHours(1))
            ->withData(['temp' => 21.1])
            ->create();

        $response = $this->getJson('/api/telemetry?node_id=node-1&hours=4&limit=10');

        $response->assertStatus(200)
            ->assertJsonPath('count', 1)
            ->assertJsonCount(1, 'data')
            ->assertJsonPath('data.0.node_id', 'node-1')
            ->assertJsonPath('data.0.node_type', 'climate');
    }

    public function test_latest_returns_nodes_with_last_telemetry(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 2, 10, 9));

        $node = Node::factory()->create([
            'node_id' => 'node-latest-1',
            'node_type' => 'climate',
            'last_seen_at' => now()->subSeconds(5),
            'online' => true,
        ]);

        Telemetry::factory()->forNode('node-latest-1', 'climate')
            ->receivedAt(now()->subMinutes(3))
            ->withData(['temp' => 24.2])
            ->create();

        Telemetry::factory()->forNode('node-latest-1', 'climate')
            ->receivedAt(now()->subMinutes(10))
            ->withData(['temp' => 22.0])
            ->create();

        $response = $this->getJson('/api/telemetry/latest');

        $response->assertStatus(200)
            ->assertJsonFragment([
                'node_id' => 'node-latest-1',
                'node_type' => 'climate',
                'online' => true,
            ]);

        $data = collect($response->json());
        $nodeData = $data->firstWhere('node_id', 'node-latest-1');

        $this->assertNotNull($nodeData);
        $this->assertEquals(24.2, $nodeData['telemetry']['data']['temp']);
    }

    public function test_aggregate_returns_bucketed_metrics(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 3, 5, 18));

        $base = now()->startOfHour();

        Telemetry::factory()->forNode('node-agg', 'water')
            ->receivedAt($base->copy()->subHours(2))
            ->withData(['temp' => 20.0])
            ->create();

        Telemetry::factory()->forNode('node-agg', 'water')
            ->receivedAt($base->copy()->subHours(2)->addMinutes(10))
            ->withData(['temp' => 21.0])
            ->create();

        Telemetry::factory()->forNode('node-agg', 'water')
            ->receivedAt($base->copy()->subHours(1))
            ->withData(['temp' => 25.0])
            ->create();

        $response = $this->getJson('/api/telemetry/aggregate?node_id=node-agg&field=temp&hours=4&interval=1hour');

        $response->assertStatus(200)
            ->assertJsonPath('node_id', 'node-agg')
            ->assertJsonPath('field', 'temp')
            ->assertJsonPath('interval', '1hour');

        $this->assertSame(4, (int) $response->json('period_hours'));

        $data = $response->json('data');

        $this->assertGreaterThanOrEqual(2, count($data));

        $firstBucket = $data[0];
        $this->assertEquals(2, (int) $firstBucket['count']);
        $this->assertEqualsWithDelta(20.5, (float) $firstBucket['avg'], 0.001);
        $this->assertEqualsWithDelta(20.0, (float) $firstBucket['min'], 0.001);
        $this->assertEqualsWithDelta(21.0, (float) $firstBucket['max'], 0.001);
    }

    public function test_export_streams_csv_with_headers(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 4, 15, 14));

        Telemetry::factory()->forNode('node-export', 'ph_ec')
            ->receivedAt(now()->subMinutes(5))
            ->withData(['ph' => 6.5, 'ec' => 1.3])
            ->create();

        $response = $this->get('/api/telemetry/export?node_id=node-export&hours=6');

        $response->assertStatus(200);

        $contentType = $response->headers->get('Content-Type');
        $this->assertNotNull($contentType);
        $this->assertStringContainsString('text/csv', $contentType);

        $response->assertHeader('Content-Disposition', 'attachment; filename="telemetry_export.csv"');

        $content = $response->streamedContent();
        $lines = array_values(array_filter(explode("\n", trim($content))));

        $this->assertNotEmpty($lines);
        $this->assertEquals(['id', 'node_id', 'node_type', 'data', 'received_at'], str_getcsv($lines[0]));

        $row = str_getcsv($lines[1]);
        $this->assertSame('node-export', $row[1]);
        $this->assertSame('ph_ec', $row[2]);
    }
}


