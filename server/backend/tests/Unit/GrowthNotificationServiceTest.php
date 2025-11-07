<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Services\GrowthNotificationService;
use App\Models\GrowthCycle;
use App\Models\StageTransitionRecommendation;
use App\Models\CycleNotification;
use App\Models\GrowthPreset;
use App\Models\GrowthCulture;
use App\Models\GrowthStage;
use App\Models\Zone;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Mockery;

class GrowthNotificationServiceTest extends TestCase
{
    use RefreshDatabase;

    private GrowthNotificationService $notificationService;

    protected function setUp(): void
    {
        parent::setUp();
        
        // Мокаем зависимости
        $telegram = Mockery::mock(\App\Services\TelegramService::class);
        $sms = Mockery::mock(\App\Services\SmsService::class);
        
        $this->notificationService = new GrowthNotificationService($telegram, $sms);
    }

    #[Test]
    public function it_can_send_stage_transition_recommendation_notification()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        
        $stage1 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 1,
            'name' => 'Проращивание',
        ]);
        
        $stage2 = GrowthStage::factory()->create([
            'preset_id' => $preset->id,
            'order' => 2,
            'name' => 'Вегетация',
        ]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage1->id,
        ]);

        $recommendation = StageTransitionRecommendation::create([
            'cycle_id' => $cycle->id,
            'current_stage_id' => $stage1->id,
            'recommended_stage_id' => $stage2->id,
            'status' => 'pending',
            'reason' => 'Минимальная длительность стадии достигнута',
            'recommended_params' => ['ph' => 6.2, 'ec' => 1.5],
            'recommended_at' => now(),
        ]);

        $this->notificationService->sendStageTransitionRecommendation($recommendation);

        // Проверяем, что уведомление создано
        $notification = CycleNotification::where('cycle_id', $cycle->id)
            ->where('type', 'stage_transition')
            ->first();

        $this->assertNotNull($notification);
        $this->assertEquals('Рекомендация перехода стадии', $notification->title);
        $this->assertFalse($notification->is_read);
        $this->assertArrayHasKey('recommendation_id', $notification->data);
    }

    #[Test]
    public function it_can_send_parameter_deviation_notification()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        $stage = GrowthStage::factory()->create(['preset_id' => $preset->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage->id,
        ]);

        $deviations = [
            'ph' => [
                'value' => 7.0,
                'target' => 6.0,
                'status' => 'high',
            ],
            'ec' => [
                'value' => 0.8,
                'target' => 1.2,
                'status' => 'low',
            ],
        ];

        $this->notificationService->sendParameterDeviationNotification($cycle, $deviations);

        $notification = CycleNotification::where('cycle_id', $cycle->id)
            ->where('type', 'parameter_deviation')
            ->first();

        $this->assertNotNull($notification);
        $this->assertEquals('Параметры вне целевых значений', $notification->title);
        $this->assertArrayHasKey('deviations', $notification->data);
    }

    #[Test]
    public function it_can_send_growth_delay_notification()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
        ]);

        $this->notificationService->sendGrowthDelayNotification($cycle, 5);

        $notification = CycleNotification::where('cycle_id', $cycle->id)
            ->where('type', 'growth_delay')
            ->first();

        $this->assertNotNull($notification);
        $this->assertEquals('Задержка роста', $notification->title);
        $this->assertEquals(5, $notification->data['days_behind']);
    }

    #[Test]
    public function it_can_send_harvest_approaching_notification()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'expected_harvest_at' => now()->addDays(3),
        ]);

        $this->notificationService->sendHarvestApproachingNotification($cycle, 3);

        $notification = CycleNotification::where('cycle_id', $cycle->id)
            ->where('type', 'harvest_approaching')
            ->first();

        $this->assertNotNull($notification);
        $this->assertEquals('Приближается сбор урожая', $notification->title);
        $this->assertEquals(3, $notification->data['days_until_harvest']);
    }

    #[Test]
    public function it_can_send_weekly_cycle_report()
    {
        $zone = Zone::factory()->create();
        $culture = GrowthCulture::factory()->create();
        $preset = GrowthPreset::factory()->create(['culture_id' => $culture->id]);
        $stage = GrowthStage::factory()->create(['preset_id' => $preset->id]);

        $cycle = GrowthCycle::factory()->create([
            'zone_id' => $zone->id,
            'preset_id' => $preset->id,
            'culture_id' => $culture->id,
            'current_stage_id' => $stage->id,
            'started_at' => now()->subDays(7),
        ]);

        $this->notificationService->sendWeeklyCycleReport($cycle);

        $notification = CycleNotification::where('cycle_id', $cycle->id)
            ->where('type', 'weekly_report')
            ->first();

        $this->assertNotNull($notification);
        $this->assertEquals('Еженедельный отчет', $notification->title);
        $this->assertArrayHasKey('progress', $notification->data);
    }
}


