<?php

namespace Tests\Feature;

use App\Services\DockerService;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Mockery;
use Tests\TestCase;

class DockerApiTest extends TestCase
{
    use RefreshDatabase;

    protected function tearDown(): void
    {
        Mockery::close();
        parent::tearDown();
    }

    private function mockDockerService(array $expectations): void
    {
        $mock = Mockery::mock(DockerService::class);

        foreach ($expectations as $method => $return) {
            $mock->shouldReceive($method)->andReturn($return);
        }

        $this->app->instance(DockerService::class, $mock);
    }

    public function test_status_returns_service_payload(): void
    {
        $payload = [
            'containers' => [
                ['name' => 'backend', 'status' => 'running'],
            ],
        ];

        $this->mockDockerService(['getStatus' => $payload]);

        $response = $this->getJson('/api/docker/status');

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('docker.containers.0.name', 'backend');
    }

    public function test_restart_all_respects_config_toggle(): void
    {
        config(['docker.allowed_operations.restart' => false]);

        $response = $this->postJson('/api/docker/restart/all');
        $response->assertStatus(403)
            ->assertJsonPath('success', false);

        config(['docker.allowed_operations.restart' => true]);

        $this->mockDockerService(['restartAll' => ['success' => true, 'message' => 'restarted']]);

        $okResponse = $this->postJson('/api/docker/restart/all');
        $okResponse->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('message', 'restarted');

        $this->mockDockerService(['restartAll' => ['success' => false, 'message' => 'failed']]);

        $failResponse = $this->postJson('/api/docker/restart/all');
        $failResponse->assertStatus(500)
            ->assertJsonPath('success', false)
            ->assertJsonPath('message', 'failed');
    }

    public function test_restart_container_validates_and_handles_service_result(): void
    {
        config(['docker.allowed_operations.restart' => true]);

        $invalid = $this->postJson('/api/docker/restart/container', ['container' => 'unknown']);
        $invalid->assertStatus(422)
            ->assertJsonValidationErrors('container');

        $this->mockDockerService(['restartContainer' => ['success' => true, 'message' => 'ok']]);

        $ok = $this->postJson('/api/docker/restart/container', ['container' => 'backend']);
        $ok->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('message', 'ok');

        $this->mockDockerService(['restartContainer' => ['success' => false, 'message' => 'boom']]);

        $fail = $this->postJson('/api/docker/restart/container', ['container' => 'backend']);
        $fail->assertStatus(500)
            ->assertJsonPath('success', false)
            ->assertJsonPath('message', 'boom');
    }

    public function test_start_all_and_stop_all_obey_config(): void
    {
        config(['docker.allowed_operations.start' => false]);
        $startDisabled = $this->postJson('/api/docker/start/all');
        $startDisabled->assertStatus(403)
            ->assertJsonPath('success', false);

        config(['docker.allowed_operations.start' => true]);
        $this->mockDockerService(['startAll' => ['success' => true, 'message' => 'started']]);
        $start = $this->postJson('/api/docker/start/all');
        $start->assertOk()
            ->assertJsonPath('success', true);

        config(['docker.allowed_operations.stop' => false]);
        $stopDisabled = $this->postJson('/api/docker/stop/all');
        $stopDisabled->assertStatus(403)
            ->assertJsonPath('success', false);

        config(['docker.allowed_operations.stop' => true]);
        $this->mockDockerService(['stopAll' => ['success' => false, 'message' => 'not allowed']]);
        $stop = $this->postJson('/api/docker/stop/all');
        $stop->assertStatus(500)
            ->assertJsonPath('success', false)
            ->assertJsonPath('message', 'not allowed');
    }

    public function test_get_logs_returns_service_output(): void
    {
        config(['docker.allowed_operations.logs' => true]);

        $this->mockDockerService([
            'getLogs' => [
                'success' => true,
                'container' => 'backend',
                'lines' => ['line1', 'line2'],
            ],
        ]);

        $response = $this->getJson('/api/docker/logs?container=backend&lines=50');

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('lines.0', 'line1');
    }
}


