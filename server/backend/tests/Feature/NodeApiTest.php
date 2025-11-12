<?php

namespace Tests\Feature;

use App\Models\Command;
use App\Models\ConfigHistory;
use App\Models\Event;
use App\Models\Node;
use App\Models\NodeConfigurationConfirmation;
use App\Models\PumpCalibration;
use App\Models\Telemetry;
use App\Services\MqttService;
use App\Services\NodeConfigurationService;
use Carbon\Carbon;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\Config;
use Mockery;
use Tests\TestCase;

class NodeApiTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();
        Config::set('hydro.node_offline_timeout', 300);
        Config::set('hydro.heartbeat_interval', 10);
    }

    protected function tearDown(): void
    {
        Mockery::close();
        parent::tearDown();
    }

    public function test_index_returns_filtered_nodes_with_last_telemetry(): void
    {
        $onlineNode = Node::factory()->online()->create([
            'node_id' => 'node_online',
            'node_type' => 'ph_ec',
        ]);

        Telemetry::create([
            'node_id' => $onlineNode->node_id,
            'node_type' => $onlineNode->node_type,
            'data' => ['ph' => 6.2],
            'received_at' => now(),
        ]);

        Node::factory()->offline()->create([
            'node_id' => 'node_offline',
            'node_type' => 'climate',
            'last_seen_at' => now()->subMinutes(20),
        ]);

        $response = $this->getJson('/api/nodes?status=online');

        $response->assertOk();
        $data = $response->json();
        $this->assertCount(1, $data);
        $this->assertSame('node_online', $data[0]['node_id']);
        $this->assertArrayHasKey('last_telemetry', $data[0]);
        $this->assertTrue($data[0]['online']);
    }

    public function test_show_returns_node_with_relations(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'node_show',
            'node_type' => 'relay',
        ]);

        Telemetry::create([
            'node_id' => $node->node_id,
            'node_type' => $node->node_type,
            'data' => ['relay_state' => true],
            'received_at' => now(),
        ]);

        Event::create([
            'node_id' => $node->node_id,
            'level' => Event::LEVEL_INFO,
            'message' => 'Test event',
            'data' => ['value' => 1],
        ]);

        Command::create([
            'node_id' => $node->node_id,
            'command' => 'toggle',
            'params' => ['state' => true],
            'status' => Command::STATUS_COMPLETED,
        ]);

        $response = $this->getJson('/api/nodes/' . $node->node_id);

        $response->assertOk()
            ->assertJsonPath('node_id', $node->node_id)
            ->assertJsonPath('node_type', 'relay')
            ->assertJsonPath('online', true)
            ->assertJsonPath('commands.0.command', 'toggle')
            ->assertJsonPath('events.0.message', 'Test event');
    }

    public function test_store_creates_node_and_handles_duplicate(): void
    {
        $payload = [
            'node_id' => 'ph_node_1',
            'node_type' => 'ph_ec',
            'zone' => 'Zone 1',
            'config' => ['target_ph' => 6.2],
        ];

        $this->postJson('/api/nodes', $payload)
            ->assertCreated()
            ->assertJsonPath('node.node_id', 'ph_node_1');

        $this->assertDatabaseHas('nodes', ['node_id' => 'ph_node_1']);

        $this->postJson('/api/nodes', $payload)
            ->assertStatus(422)
            ->assertJsonValidationErrorFor('node_id');
    }

    public function test_update_modifies_node_fields(): void
    {
        $node = Node::factory()->create([
            'node_id' => 'node_update',
            'node_type' => 'climate',
            'zone' => 'UNCONFIGURED',
        ]);

        $response = $this->putJson('/api/nodes/' . $node->node_id, [
            'zone' => 'Grow Room',
            'metadata' => ['note' => 'updated'],
        ]);

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('node.zone', 'Grow Room')
            ->assertJsonPath('node.metadata.note', 'updated');

        $this->assertDatabaseHas('nodes', [
            'node_id' => 'node_update',
            'zone' => 'Grow Room',
        ]);
    }

    public function test_destroy_removes_node_and_related_records(): void
    {
        $node = Node::factory()->create([
            'node_id' => 'node_delete',
            'node_type' => 'water',
        ]);

        Telemetry::create([
            'node_id' => $node->node_id,
            'node_type' => 'water',
            'data' => ['level' => 80],
            'received_at' => now(),
        ]);

        Event::create([
            'node_id' => $node->node_id,
            'level' => Event::LEVEL_WARNING,
            'message' => 'Water level low',
            'data' => ['threshold' => 30],
        ]);

        Command::create([
            'node_id' => $node->node_id,
            'command' => 'refill',
            'params' => ['amount' => 10],
            'status' => Command::STATUS_PENDING,
        ]);

        $this->deleteJson('/api/nodes/' . $node->node_id)
            ->assertOk()
            ->assertJsonPath('success', true);

        $this->assertDatabaseMissing('nodes', ['node_id' => 'node_delete']);
        $this->assertDatabaseMissing('telemetry', ['node_id' => 'node_delete']);
        $this->assertDatabaseMissing('events', ['node_id' => 'node_delete']);
        $this->assertDatabaseMissing('commands', ['node_id' => 'node_delete']);
    }

    public function test_statistics_returns_data_within_period(): void
    {
        $node = Node::factory()->create([
            'node_id' => 'node_stats',
            'node_type' => 'ph_ec',
        ]);

        Telemetry::create([
            'node_id' => $node->node_id,
            'node_type' => 'ph_ec',
            'data' => ['ph' => 6.0],
            'received_at' => now()->subHours(2),
        ]);

        Telemetry::create([
            'node_id' => $node->node_id,
            'node_type' => 'ph_ec',
            'data' => ['ph' => 5.8],
            'received_at' => now()->subHours(5),
        ]);

        $this->getJson('/api/nodes/' . $node->node_id . '/statistics?hours=3')
            ->assertOk()
            ->assertJsonPath('node_id', 'node_stats')
            ->assertJsonPath('data_points', 1);
    }

    public function test_update_config_with_confirmation_returns_confirmation_id(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'node_config',
            'config' => ['mode' => 'auto'],
            'last_seen_at' => now(),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldNotReceive('sendConfig');
        $this->app->instance(MqttService::class, $mqttMock);

        $confirmation = NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,
            'sent_config' => ['target_ph' => 6.1],
            'status' => 'pending',
            'sent_at' => now(),
        ]);

        $serviceMock = Mockery::mock(NodeConfigurationService::class);
        $serviceMock->shouldReceive('sendConfigurationWithConfirmation')
            ->once()
            ->with($node->node_id, ['target_ph' => 6.1], null)
            ->andReturn($confirmation);

        $this->app->instance(NodeConfigurationService::class, $serviceMock);

        $response = $this->postJson('/api/nodes/' . $node->node_id . '/config', [
            'config' => ['target_ph' => 6.1],
            'require_confirmation' => true,
        ]);

        $response->assertOk()
            ->assertJsonPath('status', 'awaiting_confirmation')
            ->assertJsonPath('confirmation_id', $confirmation->id);

        $this->assertDatabaseHas('config_history', [
            'node_id' => $node->node_id,
            'change_type' => 'update_config',
        ]);
    }

    public function test_update_config_offline_queues_message_and_logs_history(): void
    {
        Config::set('hydro.node_offline_timeout', 10);

        $node = Node::factory()->create([
            'node_id' => 'node_offline_config',
            'node_type' => 'ph_ec',
            'last_seen_at' => Carbon::now()->subMinutes(5),
            'config' => ['mode' => 'manual'],
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldNotReceive('sendConfig');
        $this->app->instance(MqttService::class, $mqttMock);

        $response = $this->postJson('/api/nodes/' . $node->node_id . '/config', [
            'config' => ['mode' => 'auto'],
        ]);

        $response->assertOk()
            ->assertJsonPath('status', 'queued')
            ->assertJsonPath('message', 'Config updated but node is offline');

        $this->assertDatabaseHas('nodes', [
            'node_id' => 'node_offline_config',
            'config->mode' => 'auto',
        ]);
        $this->assertDatabaseHas('config_history', [
            'node_id' => 'node_offline_config',
            'change_type' => 'update_config',
        ]);
    }

    public function test_send_command_online_marks_as_sent(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'node_command',
            'last_seen_at' => now(),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldReceive('sendCommand')->once();
        $this->app->instance(MqttService::class, $mqttMock);

        $response = $this->postJson('/api/nodes/' . $node->node_id . '/command', [
            'command' => 'reboot',
            'params' => ['delay' => 5],
        ]);

        $response->assertOk()
            ->assertJsonPath('status', 'sent')
            ->assertJsonPath('command.command', 'reboot');

        $this->assertDatabaseHas('commands', [
            'node_id' => 'node_command',
            'command' => 'reboot',
            'status' => Command::STATUS_SENT,
        ]);
    }

    public function test_send_command_offline_queues_command_without_mqtt_call(): void
    {
        Config::set('hydro.node_offline_timeout', 10);

        $node = Node::factory()->create([
            'node_id' => 'node_command_offline',
            'last_seen_at' => Carbon::now()->subMinutes(5),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldNotReceive('sendCommand');
        $this->app->instance(MqttService::class, $mqttMock);

        $response = $this->postJson('/api/nodes/' . $node->node_id . '/command', [
            'command' => 'reboot',
        ]);

        $response->assertOk()
            ->assertJsonPath('status', 'queued')
            ->assertJsonPath('message', 'Node is offline, command queued for delivery');

        $this->assertDatabaseHas('commands', [
            'node_id' => 'node_command_offline',
            'command' => 'reboot',
            'status' => Command::STATUS_PENDING,
        ]);
    }

    public function test_send_command_failure_returns_error(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'node_command_fail',
            'last_seen_at' => now(),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldReceive('sendCommand')->andThrow(new \Exception('MQTT failure'));
        $this->app->instance(MqttService::class, $mqttMock);

        $response = $this->postJson('/api/nodes/' . $node->node_id . '/command', [
            'command' => 'reboot',
        ]);

        $response->assertStatus(500)
            ->assertJsonPath('status', 'failed')
            ->assertJsonPath('error', 'Failed to send command: MQTT failure');

        $this->assertDatabaseHas('commands', [
            'node_id' => 'node_command_fail',
            'command' => 'reboot',
            'status' => Command::STATUS_FAILED,
        ]);
    }

    public function test_run_pump_sends_command_when_online(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'pump_node',
            'last_seen_at' => now(),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldReceive('sendCommand')
            ->once()
            ->with('pump_node', 'run_pump_manual', ['pump_id' => 2, 'duration_sec' => 5.5]);

        $this->app->instance(MqttService::class, $mqttMock);

        $this->postJson('/api/nodes/pump_node/pump/run', [
            'pump_id' => 2,
            'duration_sec' => 5.5,
        ])
            ->assertOk()
            ->assertJsonPath('message', 'Pump 2 started for 5.5 seconds');
    }

    public function test_run_pump_returns_error_when_offline(): void
    {
        Config::set('hydro.node_offline_timeout', 10);

        $node = Node::factory()->create([
            'node_id' => 'pump_node_offline',
            'last_seen_at' => now()->subMinutes(5),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldNotReceive('sendCommand');
        $this->app->instance(MqttService::class, $mqttMock);

        $this->postJson('/api/nodes/pump_node_offline/pump/run', [
            'pump_id' => 1,
            'duration_sec' => 3,
        ])
            ->assertStatus(400)
            ->assertJsonPath('error', 'Node is offline');
    }

    public function test_calibrate_pump_updates_calibration_and_sends_command(): void
    {
        Config::set('telegram.enabled', false);

        $node = Node::factory()->online()->create([
            'node_id' => 'pump_calibration',
            'last_seen_at' => now(),
        ]);

        PumpCalibration::create([
            'node_id' => 'pump_calibration',
            'pump_id' => 1,
            'ml_per_second' => 1.5,
            'is_calibrated' => true,
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldReceive('sendCommand')->once()->with(
            'pump_calibration',
            'set_config',
            Mockery::on(function ($params) {
                return isset($params['pump_1_ml_per_sec'], $params['pump_1_calibration_volume'], $params['pump_1_calibration_time'])
                    && abs($params['pump_1_ml_per_sec'] - 4.0) < 0.0001
                    && (float) $params['pump_1_calibration_volume'] === 8.0
                    && (int) $params['pump_1_calibration_time'] === 2;
            })
        );
        $this->app->instance(MqttService::class, $mqttMock);

        $response = $this->postJson('/api/nodes/pump_calibration/pump/calibrate', [
            'pump_id' => 1,
            'duration_sec' => 2,
            'volume_ml' => 8,
        ])
            ->assertOk();

        $this->assertEqualsWithDelta(4.0, (float) $response->json('calibration.ml_per_second'), 0.0001);

        $this->assertDatabaseHas('pump_calibrations', [
            'node_id' => 'pump_calibration',
            'pump_id' => 1,
            'ml_per_second' => 4.0,
            'is_calibrated' => true,
        ]);

        $this->assertDatabaseHas('config_history', [
            'node_id' => 'pump_calibration',
            'change_type' => 'calibrate_pump',
        ]);
    }

    public function test_calibrate_pump_detects_offline_node(): void
    {
        $node = Node::factory()->create([
            'node_id' => 'pump_calibration_offline',
            'last_seen_at' => now()->subMinutes(20),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldNotReceive('sendCommand');
        $this->app->instance(MqttService::class, $mqttMock);

        $this->postJson('/api/nodes/pump_calibration_offline/pump/calibrate', [
            'pump_id' => 0,
            'duration_sec' => 5,
            'volume_ml' => 50,
        ])
            ->assertStatus(400)
            ->assertJsonPath('error', 'Node is offline');

        $this->assertDatabaseMissing('pump_calibrations', [
            'node_id' => 'pump_calibration_offline',
        ]);
    }

    public function test_calibrate_pump_reports_mqtt_failure(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'pump_calibration_fail',
            'last_seen_at' => now(),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldReceive('sendCommand')->andThrow(new \Exception('MQTT error'));
        $this->app->instance(MqttService::class, $mqttMock);

        $response = $this->postJson('/api/nodes/pump_calibration_fail/pump/calibrate', [
            'pump_id' => 2,
            'duration_sec' => 4,
            'volume_ml' => 20,
        ]);

        $response->assertStatus(500)
            ->assertJsonPath('error', 'Calibration saved to DB but failed to send to node: MQTT error');

        $this->assertDatabaseHas('pump_calibrations', [
            'node_id' => 'pump_calibration_fail',
            'pump_id' => 2,
        ]);
    }

    public function test_request_config_sends_command_when_online(): void
    {
        $node = Node::factory()->online()->create([
            'node_id' => 'config_node',
            'last_seen_at' => now(),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldReceive('sendCommand')
            ->once()
            ->with('config_node', 'get_config', []);
        $this->app->instance(MqttService::class, $mqttMock);

        $this->getJson('/api/nodes/config_node/config/request')
            ->assertOk()
            ->assertJsonPath('message', 'Config request sent to node. Check WebSocket for response.');
    }

    public function test_request_config_returns_error_when_offline(): void
    {
        Config::set('hydro.node_offline_timeout', 10);

        $node = Node::factory()->create([
            'node_id' => 'config_node_offline',
            'last_seen_at' => now()->subMinutes(10),
        ]);

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldNotReceive('sendCommand');
        $this->app->instance(MqttService::class, $mqttMock);

        $this->getJson('/api/nodes/config_node_offline/config/request')
            ->assertStatus(400)
            ->assertJsonPath('error', 'Node is offline');
    }

    public function test_get_pump_calibrations_returns_sorted_list(): void
    {
        Node::factory()->create(['node_id' => 'calibration_list']);

        PumpCalibration::create([
            'node_id' => 'calibration_list',
            'pump_id' => 1,
            'ml_per_second' => 1.2,
            'is_calibrated' => true,
        ]);

        PumpCalibration::create([
            'node_id' => 'calibration_list',
            'pump_id' => 0,
            'ml_per_second' => 0.8,
            'is_calibrated' => true,
        ]);

        $this->getJson('/api/nodes/calibration_list/pump/calibrations')
            ->assertOk()
            ->assertJsonPath('calibrations.0.pump_id', 0)
            ->assertJsonCount(2, 'calibrations');
    }

    public function test_get_config_history_returns_recent_records(): void
    {
        Node::factory()->create(['node_id' => 'history_node']);

        ConfigHistory::create([
            'node_id' => 'history_node',
            'user_id' => 'api',
            'old_config' => ['mode' => 'manual'],
            'new_config' => ['mode' => 'auto'],
            'changes' => ['mode' => ['old' => 'manual', 'new' => 'auto']],
            'change_type' => 'update_config',
            'comment' => 'Initial change',
        ]);

        $this->getJson('/api/nodes/history_node/config/history')
            ->assertOk()
            ->assertJsonPath('history.0.change_type', 'update_config');
    }
}
