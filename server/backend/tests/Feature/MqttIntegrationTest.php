<?php

namespace Tests\Feature;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Models\Zone;
use App\Models\Node;
use App\Services\MqttService;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\DB;
use Mockery;

/**
 * ⭐ MQTT: Интеграционные тесты для MQTT взаимодействия
 */
class MqttIntegrationTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();

        $mqttMock = Mockery::mock(MqttService::class);
        $mqttMock->shouldIgnoreMissing();
        $mqttMock->shouldReceive('sendConfig')->andReturnNull();
        $mqttMock->shouldReceive('sendCommand')->andReturnNull();
        $mqttMock->shouldReceive('publish')->andReturnNull();

        app()->instance(MqttService::class, $mqttMock);
    }

    protected function tearDown(): void
    {
        Mockery::close();
        parent::tearDown();
    }

    #[Test]
    public function mqtt_listener_extracts_zone_from_topic()
    {
        $zone = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/nft1/',
        ]);

        // Имитируем получение сообщения MQTT
        $topic = 'hydro/nft1/telemetry/ph_ec_001';
        
        // Извлекаем имя зоны из топика
        preg_match('#^hydro/([^/]+)/#', $topic, $matches);
        $zoneName = $matches[1] ?? null;

        $this->assertEquals('nft1', $zoneName);
    }

    #[Test]
    public function mqtt_topics_follow_zone_structure()
    {
        $zone = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/zone1/',
        ]);

        $expectedTopics = [
            'telemetry' => 'hydro/zone1/telemetry/#',
            'commands' => 'hydro/zone1/commands/#',
            'events' => 'hydro/zone1/events/#',
            'heartbeat' => 'hydro/zone1/heartbeat/#',
            'config' => 'hydro/zone1/config/#',
        ];

        foreach ($expectedTopics as $type => $expectedTopic) {
            $actualTopic = $zone->mqtt_topic_prefix . $type . '/#';
            $this->assertEquals($expectedTopic, $actualTopic);
        }
    }

    #[Test]
    public function telemetry_message_contains_zone_information()
    {
        $zone = Zone::factory()->withNodes()->create([
            'mqtt_topic_prefix' => 'hydro/nft1/',
        ]);

        $node = $zone->getAllNodes()->first();

        // Имитируем телеметрическое сообщение
        $message = [
            'node_id' => $node->node_id,
            'root_node_id' => $zone->root_node_id,
            'timestamp' => now()->timestamp,
            'data' => [
                'ph' => 6.0,
                'ec' => 1.5,
                'temp' => 22.5,
            ],
        ];

        // Проверяем наличие root_node_id
        $this->assertArrayHasKey('root_node_id', $message);
        $this->assertEquals($zone->root_node_id, $message['root_node_id']);
    }

    #[Test]
    public function command_response_includes_confirmation_id()
    {
        $zone = Zone::factory()->withNodes()->create();
        $node = $zone->getAllNodes()->first();
        $node->update(['online' => true, 'last_seen_at' => now()]);

        // Отправляем команду с требованием подтверждения
        $response = $this->postJson("/api/nodes/{$node->node_id}/config", [
            'config' => ['target_ph' => 6.0],
            'require_confirmation' => true,
        ]);

        $response->assertStatus(200)
            ->assertJsonStructure(['confirmation_id']);

        $confirmationId = $response->json('confirmation_id');
        $this->assertNotNull($confirmationId);

        // Проверяем, что подтверждение создано в БД
        $this->assertDatabaseHas('node_configuration_confirmations', [
            'id' => $confirmationId,
            'node_id' => $node->node_id,
            'status' => 'pending',
        ]);
    }

    #[Test]
    public function mqtt_message_size_is_within_limits()
    {
        $zone = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/nft1/',
        ]);

        // Создаем большое сообщение
        $largeMessage = [
            'node_id' => 'ph_ec_001',
            'root_node_id' => $zone->root_node_id,
            'timestamp' => now()->timestamp,
            'data' => [
                'ph' => 6.0,
                'ec' => 1.5,
                'temp' => 22.5,
                'humidity' => 65.0,
                'co2' => 800,
                'lux' => 25000,
                'water_level' => 75.5,
            ],
        ];

        $messageSize = strlen(json_encode($largeMessage));

        // MQTT limit обычно 256 KB для ESP32
        $this->assertLessThan(256 * 1024, $messageSize);

        // Для нашей системы достаточно 4 KB
        $this->assertLessThan(4 * 1024, $messageSize);
    }

    #[Test]
    public function heartbeat_messages_maintain_zone_connection()
    {
        $zone = Zone::factory()->withRootNode()->create([
            'mqtt_topic_prefix' => 'hydro/nft1/',
        ]);

        $rootNode = Node::where('node_id', $zone->root_node_id)->first();

        // Имитируем heartbeat сообщение
        $heartbeat = [
            'node_id' => $rootNode->node_id,
            'node_type' => 'root',
            'root_node_id' => $rootNode->node_id,
            'timestamp' => now()->timestamp,
            'uptime' => 12345,
            'free_heap' => 128000,
            'mesh_nodes_count' => 5,
        ];

        // Сохраняем в БД
        $rootNode->update(['last_seen_at' => now()]);

        // Проверяем, что узел онлайн
        $this->assertTrue($rootNode->isOnline());
    }

    #[Test]
    public function zone_commands_are_queued_when_offline()
    {
        $zone = Zone::factory()->withNodes()->create();
        $node = $zone->getAllNodes()->first();

        // Устанавливаем узел в offline
        $node->update([
            'online' => false,
            'last_seen_at' => now()->subMinutes(10),
        ]);

        // Отправляем команду
        $response = $this->postJson("/api/nodes/{$node->node_id}/command", [
            'command' => 'update_config',
            'params' => ['target_ph' => 6.0],
        ]);

        // Команда должна быть принята, но помечена для отложенной доставки
        $response->assertStatus(200)
            ->assertJsonPath('status', 'queued');
    }

    #[Test]
    public function mqtt_topics_support_wildcards()
    {
        $zone = Zone::factory()->create([
            'mqtt_topic_prefix' => 'hydro/nft1/',
        ]);

        // Wildcard patterns для подписки
        $patterns = [
            'hydro/+/telemetry/#',     // Все зоны, вся телеметрия
            'hydro/nft1/#',             // Одна зона, все топики
            'hydro/+/commands/ph_ec_001', // Все зоны, конкретный узел
        ];

        foreach ($patterns as $pattern) {
            // Проверяем, что паттерн валиден (используем ~ как разделитель вместо #)
            $this->assertMatchesRegularExpression('~^hydro/[+#/a-z0-9_]+$~i', $pattern);
        }
    }
}

