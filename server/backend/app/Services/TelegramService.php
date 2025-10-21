<?php

namespace App\Services;

use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Http;

class TelegramService
{
    private string $botToken;
    private string $chatId;
    private bool $enabled;

    public function __construct()
    {
        // Попробовать загрузить из БД, если не удалось - из конфига
        try {
            $this->botToken = \App\Models\Setting::get('telegram.bot_token', config('telegram.bot_token', ''));
            $this->chatId = \App\Models\Setting::get('telegram.chat_id', config('telegram.chat_id', ''));
            $this->enabled = \App\Models\Setting::get('telegram.enabled', config('telegram.enabled', false));
        } catch (\Exception $e) {
            // Если БД недоступна, использовать конфиг
            $this->botToken = config('telegram.bot_token', '');
            $this->chatId = config('telegram.chat_id', '');
            $this->enabled = config('telegram.enabled', false);
        }
    }

    /**
     * Отправка алерта в Telegram
     */
    public function sendAlert(string $message, string $level = 'info'): bool
    {
        if (!$this->enabled || empty($this->botToken) || empty($this->chatId)) {
            Log::debug("Telegram disabled or not configured", [
                'enabled' => $this->enabled,
                'has_token' => !empty($this->botToken),
                'has_chat_id' => !empty($this->chatId)
            ]);
            return false;
        }

        $emoji = $this->getEmojiForLevel($level);
        $formattedMessage = $this->formatMessage($emoji, $message, $level);

        try {
            $response = Http::post("https://api.telegram.org/bot{$this->botToken}/sendMessage", [
                'chat_id' => $this->chatId,
                'text' => $formattedMessage,
                'parse_mode' => 'HTML',
                'disable_web_page_preview' => true,
            ]);

            if ($response->successful()) {
                Log::info("Telegram alert sent", [
                    'level' => $level,
                    'message' => $message
                ]);
                return true;
            } else {
                Log::error("Telegram API error", [
                    'status' => $response->status(),
                    'body' => $response->body()
                ]);
                return false;
            }
        } catch (\Exception $e) {
            Log::error("Failed to send Telegram alert", [
                'error' => $e->getMessage()
            ]);
            return false;
        }
    }

    /**
     * Отправка алерта о критичном событии
     */
    public function sendErrorAlert($error): bool
    {
        $level = $this->mapSeverityToLevel($error->level ?? $error->severity ?? 'info');
        
        $nodeId = $this->escapeHtml($error->node_id ?? 'unknown');
        $errorMessage = $this->escapeHtml($error->message ?? 'Unknown error');
        
        $message = sprintf(
            "<b>Узел:</b> %s\n<b>Событие:</b> %s\n<b>Время:</b> %s",
            $nodeId,
            $errorMessage,
            now()->format('d.m.Y H:i:s')
        );

        // Добавить данные если есть
        if (isset($error->data) && is_array($error->data)) {
            $message .= "\n\n<b>Данные:</b>\n";
            foreach ($error->data as $key => $value) {
                $formattedValue = $this->formatValue($value);
                $message .= "• {$key}: {$formattedValue}\n";
            }
        }

        return $this->sendAlert($message, $level);
    }

    /**
     * Отправка алерта о проблеме с нодой
     */
    public function sendNodeOfflineAlert(string $nodeId): bool
    {
        $message = sprintf(
            "<b>⚠️ Узел оффлайн</b>\n\nУзел: %s\nВремя: %s",
            $nodeId,
            now()->format('d.m.Y H:i:s')
        );

        return $this->sendAlert($message, 'warning');
    }

    /**
     * Отправка алерта об успешной калибровке
     */
    public function sendCalibrationAlert(string $nodeId, int $pumpId, float $mlPerSecond): bool
    {
        $message = sprintf(
            "<b>✅ Насос откалиброван</b>\n\nУзел: %s\nНасос: #%d\nПроизводительность: %.2f мл/с",
            $nodeId,
            $pumpId,
            $mlPerSecond
        );

        return $this->sendAlert($message, 'info');
    }

    /**
     * Получить emoji для уровня
     */
    private function getEmojiForLevel(string $level): string
    {
        return match($level) {
            'emergency' => '🚨',
            'critical' => '🔴',
            'warning' => '🟡',
            'info' => '🟢',
            default => 'ℹ️'
        };
    }

    /**
     * Форматирование сообщения
     */
    private function formatMessage(string $emoji, string $message, string $level): string
    {
        $levelName = strtoupper($level);
        $timestamp = now()->format('H:i:s');
        
        return "{$emoji} <b>[{$levelName}]</b> {$timestamp}\n\n{$message}";
    }

    /**
     * Маппинг severity в level
     */
    private function mapSeverityToLevel(string $severity): string
    {
        return match($severity) {
            'emergency' => 'emergency',
            'critical' => 'critical',
            'high' => 'critical',
            'warning', 'medium' => 'warning',
            default => 'info'
        };
    }

    /**
     * Проверка настройки Telegram
     */
    public function isConfigured(): bool
    {
        return $this->enabled && !empty($this->botToken) && !empty($this->chatId);
    }

    /**
     * Тестовое сообщение
     */
    public function sendTestMessage(): bool
    {
        return $this->sendAlert(
            "🌿 <b>Hydro Mesh System</b>\n\nTelegram уведомления настроены и работают!",
            'info'
        );
    }

    /**
     * Экранирование HTML спецсимволов
     */
    private function escapeHtml(string $text): string
    {
        return htmlspecialchars($text, ENT_QUOTES, 'UTF-8');
    }

    /**
     * Форматирование значения для вывода
     */
    private function formatValue($value): string
    {
        if (is_array($value) || is_object($value)) {
            return json_encode($value);
        }
        
        if (is_bool($value)) {
            return $value ? 'true' : 'false';
        }
        
        if (is_float($value)) {
            return number_format($value, 2, '.', '');
        }
        
        return (string)$value;
    }
}
