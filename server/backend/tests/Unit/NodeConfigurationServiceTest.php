<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Services\NodeConfigurationService;
use App\Models\Node;
use App\Models\GrowthCycle;
use App\Models\NodeConfigurationConfirmation;
use App\Models\Zone;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\Cache;

class NodeConfigurationServiceTest extends TestCase
{
    use RefreshDatabase;

    private NodeConfigurationService $service;

    protected function setUp(): void
    {
        parent::setUp();
        $this->service = app(NodeConfigurationService::class);
    }

    #[Test]
    public function it_can_send_configuration_with_confirmation_required()
    {
        $node = Node::factory()->create(['node_id' => 'ph_test_001']);
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
        ]);

        $config = [
            'target_ph' => 6.0,
            'target_ec' => 1.5,
        ];

        $confirmation = $this->service->sendConfigurationWithConfirmation(
            $node->node_id,  // Используем node_id вместо объекта
            $config,
            $cycle->id
        );

        $this->assertInstanceOf(NodeConfigurationConfirmation::class, $confirmation);
        $this->assertEquals('pending', $confirmation->status);
        $this->assertEquals($node->node_id, $confirmation->node_id);  // Сравниваем node_id (string)
        $this->assertEquals($cycle->id, $confirmation->cycle_id);
    }

    #[Test]
    public function it_can_handle_configuration_response()
    {
        $node = Node::factory()->create(['node_id' => 'ph_test_001']);
        
        $confirmation = NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,  // Используем node_id (string)
            'sent_config' => ['target_ph' => 6.0],
            'status' => 'pending',
            'sent_at' => now(),
        ]);

        $receivedConfig = ['target_ph' => 6.0];

        $this->service->handleConfigurationResponse(
            $confirmation->id,
            $receivedConfig
        );

        $confirmation->refresh();
        
        $this->assertEquals('confirmed', $confirmation->status);
        $this->assertNotNull($confirmation->confirmed_at);
        $this->assertEquals($receivedConfig, $confirmation->confirmed_config);  // confirmed_config не received_config
    }

    #[Test]
    public function it_can_mark_configuration_as_failed()
    {
        $node = Node::factory()->create(['node_id' => 'ph_test_001']);
        
        $confirmation = NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,  // Используем node_id (string)
            'sent_config' => ['target_ph' => 6.0],
            'status' => 'pending',
            'sent_at' => now(),
        ]);

        $this->service->markAsFailed($confirmation->id, 'Timeout');

        $confirmation->refresh();
        
        $this->assertEquals('failed', $confirmation->status);
        $this->assertNotNull($confirmation->failed_at);
        $this->assertEquals('Timeout', $confirmation->error_message);  // error_message не failure_reason
    }

    #[Test]
    public function it_can_check_for_timeouts()
    {
        $node = Node::factory()->create(['node_id' => 'ph_test_001']);
        
        // Создаем старое подтверждение (70 секунд назад - превышает таймаут 60 секунд)
        $oldConfirmation = NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,  // Используем node_id (string)
            'sent_config' => ['target_ph' => 6.0],
            'status' => 'pending',
            'sent_at' => now()->subSeconds(70),
        ]);

        // Создаем свежее подтверждение (30 секунд назад - в пределах таймаута)
        $newConfirmation = NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,  // Используем node_id (string)
            'sent_config' => ['target_ec' => 1.5],
            'status' => 'pending',
            'sent_at' => now()->subSeconds(30),
        ]);

        $timedOut = $this->service->checkTimeouts();

        $this->assertGreaterThan(0, $timedOut);

        $oldConfirmation->refresh();
        $newConfirmation->refresh();

        $this->assertEquals('failed', $oldConfirmation->status);
        $this->assertEquals('pending', $newConfirmation->status);
    }

    #[Test]
    public function it_can_get_pending_confirmations_for_node()
    {
        $node = Node::factory()->create(['node_id' => 'ph_test_001']);
        
        NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,  // Используем node_id (string)
            'sent_config' => ['target_ph' => 6.0],
            'status' => 'pending',
            'sent_at' => now(),
        ]);

        NodeConfigurationConfirmation::create([
            'node_id' => $node->node_id,  // Используем node_id (string)
            'sent_config' => ['target_ec' => 1.5],
            'status' => 'confirmed',
            'sent_at' => now(),
            'confirmed_at' => now(),
        ]);

        $pending = $this->service->getPendingConfirmations($node->node_id);  // Передаем node_id

        $this->assertCount(1, $pending);
        $this->assertEquals('pending', $pending->first()->status);
    }
}

