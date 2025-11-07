<?php

namespace Tests\Feature;

use App\Models\Setting;
use App\Services\TelegramService;
use Carbon\Carbon;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\Cache;
use Illuminate\Support\Facades\Http;
use Mockery;
use Tests\TestCase;

class SettingsApiTest extends TestCase
{
    use RefreshDatabase;

    protected function tearDown(): void
    {
        Carbon::setTestNow();
        Mockery::close();
        parent::tearDown();
    }

    public function test_index_masks_sensitive_settings(): void
    {
        Setting::query()->delete();
        Cache::flush();

        Setting::create([
            'key' => 'general.site_name',
            'value' => 'Mesh Hydro',
            'group' => 'general',
            'type' => 'string',
            'is_sensitive' => false,
        ]);

        Setting::create([
            'key' => 'telegram.bot_token',
            'value' => 'secret-token-value',
            'group' => 'telegram',
            'type' => 'string',
            'is_sensitive' => true,
        ]);

        $response = $this->getJson('/api/settings');

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonCount(2, 'settings');

        $settings = collect($response->json('settings'));

        $this->assertSame('Mesh Hydro', $settings->firstWhere('key', 'general.site_name')['value']);
        $this->assertSame('***', $settings->firstWhere('key', 'telegram.bot_token')['value']);
    }

    public function test_index_can_filter_by_group(): void
    {
        Setting::query()->delete();

        Setting::create([
            'key' => 'general.site_name',
            'value' => 'Mesh Hydro',
            'group' => 'general',
            'type' => 'string',
        ]);

        Setting::create([
            'key' => 'telegram.enabled',
            'value' => '1',
            'group' => 'telegram',
            'type' => 'boolean',
        ]);

        $response = $this->getJson('/api/settings?group=telegram');

        $response->assertOk()
            ->assertJsonCount(1, 'settings')
            ->assertJsonPath('settings.0.key', 'telegram.enabled');
    }

    public function test_get_telegram_masks_token_and_flags(): void
    {
        Setting::set('telegram.enabled', true);
        Setting::set('telegram.bot_token', '1234567890abcdefghijklmnopqrstuv');
        Setting::set('telegram.chat_id', '987654321');
        Setting::set('telegram.notify_critical', true);
        Setting::set('telegram.notify_warnings', false);
        Setting::set('telegram.notify_info', false);

        $response = $this->getJson('/api/settings/telegram');

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('telegram.enabled', true)
            ->assertJsonPath('telegram.has_token', true)
            ->assertJsonMissingPath('telegram.bot_token')
            ->assertJsonPath('telegram.bot_token_masked', '1234567890...rstuv');
    }

    public function test_save_telegram_validates_and_persists_settings(): void
    {
        $payload = [
            'bot_token' => '1234567890abcdefghijklmnopqrstuv',
            'chat_id' => '987654321',
            'enabled' => true,
            'notify_critical' => true,
            'notify_warnings' => false,
            'notify_info' => true,
        ];

        $response = $this->postJson('/api/settings/telegram', $payload);

        $response->assertOk()
            ->assertJsonPath('success', true);

        $this->assertEquals('1234567890abcdefghijklmnopqrstuv', Setting::get('telegram.bot_token'));
        $this->assertEquals('987654321', Setting::get('telegram.chat_id'));
        $this->assertTrue(Setting::get('telegram.enabled'));
        $this->assertFalse(Setting::get('telegram.notify_warnings'));
    }

    public function test_save_telegram_returns_validation_errors(): void
    {
        $response = $this->postJson('/api/settings/telegram', [
            'bot_token' => 'short',
            'enabled' => 'not_bool',
        ]);

        $response->assertStatus(422)
            ->assertJsonPath('success', false)
            ->assertJsonValidationErrors(['bot_token', 'enabled']);
    }

    public function test_test_telegram_checks_requirements_and_sends_message(): void
    {
        $telegramMock = Mockery::mock(TelegramService::class);
        $this->app->instance(TelegramService::class, $telegramMock);

        Setting::set('telegram.enabled', true);
        Setting::set('telegram.bot_token', '1234567890abcdefghijklmnopqrstuv');
        Setting::set('telegram.chat_id', '987654321');

        $telegramMock->shouldReceive('sendTestMessage')
            ->once()
            ->andReturn(true);

        $response = $this->postJson('/api/settings/telegram/test');

        $response->assertOk()
            ->assertJsonPath('success', true);
    }

    public function test_test_telegram_fails_when_disabled_or_missing_data(): void
    {
        Setting::set('telegram.enabled', false);

        $disabledResponse = $this->postJson('/api/settings/telegram/test');
        $disabledResponse->assertStatus(400)
            ->assertJsonPath('success', false);

        Setting::set('telegram.enabled', true);
        Setting::set('telegram.bot_token', '');
        Setting::set('telegram.chat_id', '');

        $missingResponse = $this->postJson('/api/settings/telegram/test');
        $missingResponse->assertStatus(400)
            ->assertJsonPath('error', 'Bot Token или Chat ID не настроены');
    }

    public function test_get_chat_id_fetches_updates_and_returns_chats(): void
    {
        Setting::set('telegram.bot_token', '1234567890abcdefghijklmnopqrstuv');

        Http::fake([
            'https://api.telegram.org/*' => Http::response([
                'ok' => true,
                'result' => [
                    [
                        'message' => [
                            'chat' => [
                                'id' => 111,
                                'type' => 'private',
                                'first_name' => 'Alice',
                            ],
                        ],
                    ],
                    [
                        'message' => [
                            'chat' => [
                                'id' => 222,
                                'type' => 'group',
                                'title' => 'Hydro Operators',
                            ],
                        ],
                    ],
                ],
            ], 200),
        ]);

        $response = $this->getJson('/api/settings/telegram/chat-id');

        $response->assertOk()
            ->assertJsonPath('success', true)
            ->assertJsonPath('count', 2)
            ->assertJsonCount(2, 'chats');
    }

    public function test_get_chat_id_handles_missing_token_and_api_errors(): void
    {
        Setting::set('telegram.bot_token', '');

        $missingToken = $this->getJson('/api/settings/telegram/chat-id');
        $missingToken->assertStatus(400)
            ->assertJsonPath('success', false);

        Setting::set('telegram.bot_token', '1234567890abcdefghijklmnopqrstuv');

        Http::fake([
            'https://api.telegram.org/*' => Http::response([], 500),
        ]);

        $apiError = $this->getJson('/api/settings/telegram/chat-id');
        $apiError->assertStatus(500)
            ->assertJsonPath('success', false);
    }
}


