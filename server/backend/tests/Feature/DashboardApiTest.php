<?php

namespace Tests\Feature;

use App\Models\Command;
use App\Models\Event;
use App\Models\Node;
use App\Models\Telemetry;
use Carbon\Carbon;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\Cache;
use Tests\TestCase;

class DashboardApiTest extends TestCase
{
    use RefreshDatabase;

    protected function tearDown(): void
    {
        Carbon::setTestNow();
        parent::tearDown();
    }

    public function test_summary_returns_cached_dashboard_data(): void
    {
        Cache::flush();

        $onlineNode = Node::factory()->create([
            'node_id' => 'node-online',
            'node_type' => 'ph_ec',
            'last_seen_at' => now()->subSeconds(5),
            'online' => true,
            'zone' => 'Zone A',
        ]);

        $offlineNode = Node::factory()->create([
            'node_id' => 'node-offline',
            'node_type' => 'relay',
            'last_seen_at' => now()->subMinutes(5),
            'online' => false,
            'zone' => 'Zone B',
        ]);

        Telemetry::factory()->forNode($onlineNode->node_id, 'ph_ec')
            ->receivedAt(now()->subMinutes(10))
            ->withData(['ph' => 6.3])
            ->create();

        Telemetry::factory()->forNode($onlineNode->node_id, 'ph_ec')
            ->receivedAt(now()->subMinutes(2))
            ->withData(['ph' => 6.4])
            ->create();

        Event::factory()->forNode($onlineNode->node_id)
            ->critical()
            ->create([
                'message' => 'Critical pH deviation',
            ]);

        Event::factory()->forNode($offlineNode->node_id)
            ->resolved()
            ->create([
                'level' => Event::LEVEL_INFO,
            ]);

        Command::create([
            'node_id' => $onlineNode->node_id,
            'command' => 'calibrate',
            'params' => ['target' => 6.3],
            'status' => Command::STATUS_COMPLETED,
        ]);

        Command::create([
            'node_id' => $offlineNode->node_id,
            'command' => 'restart',
            'params' => [],
            'status' => Command::STATUS_PENDING,
        ]);

        Telemetry::factory()->forNode($offlineNode->node_id, 'relay')
            ->receivedAt(now()->subMinutes(30))
            ->withData(['state' => 'on'])
            ->create();

        Telemetry::factory()->forNode('node-other', 'water')
            ->receivedAt(now()->subHours(2))
            ->withData(['level' => 80])
            ->create();

        $response = $this->getJson('/api/dashboard/summary');

        $response->assertOk();

        $summary = $response->json();

        $this->assertSame(2, $summary['nodes']['total']);
        $this->assertSame(1, $summary['nodes']['online']);
        $this->assertSame(1, $summary['nodes']['offline']);

        $this->assertArrayHasKey('ph_ec', $summary['nodes']['by_type']);
        $this->assertArrayHasKey('relay', $summary['nodes']['by_type']);

        $this->assertSame(1, $summary['events']['active']);
        $this->assertSame(1, $summary['events']['critical']);
        $this->assertNotEmpty($summary['events']['recent']);

        $this->assertSame(2, $summary['commands']['today']);
        $this->assertSame(1, $summary['commands']['pending']);

        $this->assertSame(3, $summary['telemetry']['last_hour']);
        $this->assertNotEmpty($summary['telemetry']['latest']);

        $this->assertNotEmpty($summary['timestamp']);

        // Второй вызов должен отдавать тот же кэшированный результат
        $secondResponse = $this->getJson('/api/dashboard/summary');
        $secondResponse->assertOk()
            ->assertExactJson($summary);
    }

    public function test_status_reports_service_health(): void
    {
        Carbon::setTestNow(Carbon::create(2024, 7, 2, 15));

        Telemetry::factory()->forNode('node-status', 'ph_ec')
            ->receivedAt(now()->subMinute())
            ->withData(['ph' => 6.5])
            ->create();

        config([
            'telegram.enabled' => true,
            'telegram.bot_token' => 'test-token',
        ]);

        $response = $this->getJson('/api/status');

        $response->assertOk()
            ->assertJsonPath('status', 'running')
            ->assertJsonPath('database', 'ok')
            ->assertJsonPath('mqtt', 'connected')
            ->assertJsonPath('telegram', 'configured');

        $system = $response->json('system');
        $this->assertArrayHasKey('php_version', $system);
        $this->assertArrayHasKey('laravel_version', $system);
        $this->assertArrayHasKey('server_time', $system);
        $this->assertArrayHasKey('uptime', $system);
    }
}


