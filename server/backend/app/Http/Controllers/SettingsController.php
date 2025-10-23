<?php

namespace App\Http\Controllers;

use App\Models\Setting;
use App\Services\TelegramService;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Validator;

class SettingsController extends Controller
{
    /**
     * Получить все настройки (или по группе)
     */
    public function index(Request $request): JsonResponse
    {
        $group = $request->query('group');

        if ($group) {
            $settings = Setting::where('group', $group)->get();
        } else {
            $settings = Setting::all();
        }

        // Скрыть чувствительные данные
        $settings = $settings->map(function ($setting) {
            if ($setting->is_sensitive && !empty($setting->value)) {
                $setting->value = '***';
            }
            return $setting;
        });

        return response()->json([
            'success' => true,
            'settings' => $settings,
        ]);
    }

    /**
     * Получить настройки Telegram
     */
    public function getTelegram(): JsonResponse
    {
        $settings = [
            'enabled' => Setting::get('telegram.enabled', false),
            'bot_token' => Setting::get('telegram.bot_token', ''),
            'chat_id' => Setting::get('telegram.chat_id', ''),
            'notify_critical' => Setting::get('telegram.notify_critical', true),
            'notify_warnings' => Setting::get('telegram.notify_warnings', true),
            'notify_info' => Setting::get('telegram.notify_info', false),
        ];

        // Замаскировать токен
        if (!empty($settings['bot_token'])) {
            $token = $settings['bot_token'];
            $settings['bot_token_masked'] = substr($token, 0, 10) . '...' . substr($token, -5);
            $settings['has_token'] = true;
        } else {
            $settings['bot_token_masked'] = '';
            $settings['has_token'] = false;
        }

        // Не отправлять реальный токен
        unset($settings['bot_token']);

        return response()->json([
            'success' => true,
            'telegram' => $settings,
        ]);
    }

    /**
     * Сохранить настройки Telegram
     */
    public function saveTelegram(Request $request): JsonResponse
    {
        $validator = Validator::make($request->all(), [
            'bot_token' => 'nullable|string|min:30',
            'chat_id' => 'nullable|string',
            'enabled' => 'boolean',
            'notify_critical' => 'boolean',
            'notify_warnings' => 'boolean',
            'notify_info' => 'boolean',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'success' => false,
                'error' => 'Validation failed',
                'errors' => $validator->errors(),
            ], 422);
        }

        $data = $validator->validated();

        try {
            // Сохранить настройки
            if (isset($data['bot_token'])) {
                Setting::set('telegram.bot_token', $data['bot_token']);
            }

            if (isset($data['chat_id'])) {
                Setting::set('telegram.chat_id', $data['chat_id']);
            }

            if (isset($data['enabled'])) {
                Setting::set('telegram.enabled', $data['enabled']);
            }

            if (isset($data['notify_critical'])) {
                Setting::set('telegram.notify_critical', $data['notify_critical']);
            }

            if (isset($data['notify_warnings'])) {
                Setting::set('telegram.notify_warnings', $data['notify_warnings']);
            }

            if (isset($data['notify_info'])) {
                Setting::set('telegram.notify_info', $data['notify_info']);
            }

            // Обновить конфиг в памяти
            config([
                'telegram.enabled' => Setting::get('telegram.enabled', false),
                'telegram.bot_token' => Setting::get('telegram.bot_token', ''),
                'telegram.chat_id' => Setting::get('telegram.chat_id', ''),
            ]);

            Log::info("Telegram settings updated", [
                'enabled' => $data['enabled'] ?? null,
                'has_token' => isset($data['bot_token']) && !empty($data['bot_token']),
                'has_chat_id' => isset($data['chat_id']) && !empty($data['chat_id']),
            ]);

            return response()->json([
                'success' => true,
                'message' => 'Настройки Telegram сохранены',
            ]);
        } catch (\Exception $e) {
            Log::error("Failed to save Telegram settings", [
                'error' => $e->getMessage()
            ]);

            return response()->json([
                'success' => false,
                'error' => 'Failed to save settings',
                'message' => $e->getMessage(),
            ], 500);
        }
    }

    /**
     * Тестовое сообщение в Telegram
     */
    public function testTelegram(TelegramService $telegram): JsonResponse
    {
        // Обновить конфиг из БД
        config([
            'telegram.enabled' => Setting::get('telegram.enabled', false),
            'telegram.bot_token' => Setting::get('telegram.bot_token', ''),
            'telegram.chat_id' => Setting::get('telegram.chat_id', ''),
        ]);

        if (!Setting::get('telegram.enabled', false)) {
            return response()->json([
                'success' => false,
                'error' => 'Telegram отключен в настройках',
            ], 400);
        }

        $botToken = Setting::get('telegram.bot_token', '');
        $chatId = Setting::get('telegram.chat_id', '');

        if (empty($botToken) || empty($chatId)) {
            return response()->json([
                'success' => false,
                'error' => 'Bot Token или Chat ID не настроены',
            ], 400);
        }

        try {
            $result = $telegram->sendTestMessage();

            if ($result) {
                Log::info("Telegram test message sent successfully");

                return response()->json([
                    'success' => true,
                    'message' => 'Тестовое сообщение отправлено! Проверьте Telegram.',
                ]);
            } else {
                return response()->json([
                    'success' => false,
                    'error' => 'Не удалось отправить сообщение. Проверьте токен и chat_id.',
                ], 500);
            }
        } catch (\Exception $e) {
            Log::error("Telegram test failed", ['error' => $e->getMessage()]);

            return response()->json([
                'success' => false,
                'error' => 'Ошибка отправки: ' . $e->getMessage(),
            ], 500);
        }
    }

    /**
     * Получить Chat ID через GetUpdates
     */
    public function getChatId(): JsonResponse
    {
        $botToken = Setting::get('telegram.bot_token', '');

        if (empty($botToken)) {
            return response()->json([
                'success' => false,
                'error' => 'Bot Token не настроен',
            ], 400);
        }

        try {
            $response = \Illuminate\Support\Facades\Http::get(
                "https://api.telegram.org/bot{$botToken}/getUpdates"
            );

            if ($response->successful()) {
                $data = $response->json();

                if (!empty($data['result'])) {
                    $updates = $data['result'];
                    $chatIds = [];

                    foreach ($updates as $update) {
                        if (isset($update['message']['chat']['id'])) {
                            $chatId = $update['message']['chat']['id'];
                            $chatType = $update['message']['chat']['type'] ?? 'unknown';
                            $chatTitle = $update['message']['chat']['title'] ?? 
                                        $update['message']['chat']['first_name'] ?? 
                                        'Unknown';

                            $chatIds[] = [
                                'chat_id' => $chatId,
                                'type' => $chatType,
                                'title' => $chatTitle,
                            ];
                        }
                    }

                    // Удалить дубликаты
                    $chatIds = array_unique($chatIds, SORT_REGULAR);

                    return response()->json([
                        'success' => true,
                        'chats' => $chatIds,
                        'count' => count($chatIds),
                    ]);
                } else {
                    return response()->json([
                        'success' => false,
                        'error' => 'Нет сообщений от бота. Отправьте любое сообщение боту и попробуйте снова.',
                    ], 404);
                }
            } else {
                return response()->json([
                    'success' => false,
                    'error' => 'Ошибка Telegram API: ' . $response->body(),
                ], 500);
            }
        } catch (\Exception $e) {
            return response()->json([
                'success' => false,
                'error' => 'Ошибка: ' . $e->getMessage(),
            ], 500);
        }
    }
}

