<?php

namespace App\Services;

use App\Models\Event;
use App\Models\Node;
use Illuminate\Support\Facades\Http;
use Illuminate\Support\Facades\Log;
use Exception;

class TelegramService
{
    private string $botToken;
    private string $chatId;
    private bool $enabled;

    public function __construct()
    {
        $this->botToken = config('telegram.bot_token', '');
        $this->chatId = config('telegram.chat_id', '');
        $this->enabled = config('telegram.enabled', true) && !empty($this->botToken);
    }

    /**
     * Отправка алерта о критичном событии
     */
    public function sendAlert(Event $event): void
    {
        if (!$this->enabled) {
            Log::debug('Telegram disabled, skipping alert');
            return;
        }

        try {
            $message = $this->formatAlertMessage($event);
            $this->sendMessage($message, true);
            
            Log::info('Telegram alert sent', [
                'event_id' => $event->id,
                'level' => $event->level
            ]);
        } catch (Exception $e) {
            Log::error('Telegram alert failed', [
                'event_id' => $event->id,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Отправка уведомления о состоянии узла
     */
    public function sendNodeStatus(Node $node, string $status): void
    {
        if (!$this->enabled) {
            return;
        }

        try {
            $emoji = $status === 'online' ? '✅' : '❌';
            $message = "{$emoji} <b>{$node->node_id}</b>\n";
            $message .= "Status: <b>{$status}</b>\n";
            $message .= "Type: {$node->node_type}\n";
            $message .= "Zone: {$node->zone}\n";
            
            $this->sendMessage($message);
            
            Log::info('Telegram node status sent', [
                'node_id' => $node->node_id,
                'status' => $status
            ]);
        } catch (Exception $e) {
            Log::error('Telegram node status failed', [
                'node_id' => $node->node_id,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Отправка обычного сообщения
     */
    public function sendMessage(string $text, bool $disableNotification = false): void
    {
        if (!$this->enabled) {
            return;
        }

        if (empty($this->chatId)) {
            Log::warning('Telegram chat_id not configured');
            return;
        }

        try {
            $response = Http::post($this->getApiUrl('sendMessage'), [
                'chat_id' => $this->chatId,
                'text' => $text,
                'parse_mode' => 'HTML',
                'disable_notification' => $disableNotification,
            ]);

            if (!$response->successful()) {
                throw new Exception('Telegram API error: ' . $response->body());
            }

            Log::debug('Telegram message sent', [
                'chat_id' => $this->chatId,
                'length' => strlen($text)
            ]);
        } catch (Exception $e) {
            Log::error('Telegram send message failed', [
                'error' => $e->getMessage()
            ]);
            throw $e;
        }
    }

    /**
     * Форматирование сообщения об алерте
     */
    private function formatAlertMessage(Event $event): string
    {
        $emoji = $this->getLevelEmoji($event->level);
        $node = $event->node;
        
        $message = "{$emoji} <b>ALERT: {$event->level}</b>\n\n";
        $message .= "Node: <b>{$event->node_id}</b>\n";
        
        if ($node) {
            $message .= "Type: {$node->node_type}\n";
            $message .= "Zone: {$node->zone}\n";
        }
        
        $message .= "\nMessage:\n<i>{$event->message}</i>\n";
        $message .= "\nTime: " . $event->created_at->format('Y-m-d H:i:s');
        
        // Добавление данных события если есть
        if (!empty($event->data)) {
            $message .= "\n\nData:\n";
            foreach ($event->data as $key => $value) {
                $message .= "• {$key}: {$value}\n";
            }
        }
        
        return $message;
    }

    /**
     * Получить эмодзи для уровня события
     */
    private function getLevelEmoji(string $level): string
    {
        return match($level) {
            Event::LEVEL_INFO => 'ℹ️',
            Event::LEVEL_WARNING => '⚠️',
            Event::LEVEL_CRITICAL => '🚨',
            Event::LEVEL_EMERGENCY => '🔥',
            default => '❓',
        };
    }

    /**
     * Получить URL API метода
     */
    private function getApiUrl(string $method): string
    {
        return "https://api.telegram.org/bot{$this->botToken}/{$method}";
    }

    /**
     * Проверка доступности Telegram API
     */
    public function checkConnection(): bool
    {
        if (!$this->enabled) {
            return false;
        }

        try {
            $response = Http::get($this->getApiUrl('getMe'));
            return $response->successful();
        } catch (Exception $e) {
            Log::error('Telegram connection check failed', [
                'error' => $e->getMessage()
            ]);
            return false;
        }
    }
}

