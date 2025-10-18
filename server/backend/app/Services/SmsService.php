<?php

namespace App\Services;

use App\Models\Event;
use Illuminate\Support\Facades\Http;
use Illuminate\Support\Facades\Log;
use Exception;

class SmsService
{
    private string $gatewayUrl;
    private string $apiKey;
    private string $phone;
    private bool $enabled;

    public function __construct()
    {
        $this->gatewayUrl = config('sms.gateway_url', '');
        $this->apiKey = config('sms.api_key', '');
        $this->phone = config('sms.phone', '');
        $this->enabled = config('sms.enabled', false) && !empty($this->apiKey);
    }

    /**
     * Отправка SMS алерта о критичном событии
     */
    public function sendAlert(Event $event): void
    {
        if (!$this->enabled) {
            Log::debug('SMS disabled, skipping alert');
            return;
        }

        // SMS отправляем только для EMERGENCY событий
        if ($event->level !== Event::LEVEL_EMERGENCY) {
            Log::debug('SMS only for emergency events', [
                'event_level' => $event->level
            ]);
            return;
        }

        try {
            $message = $this->formatAlertMessage($event);
            $this->sendSms($this->phone, $message);
            
            Log::info('SMS alert sent', [
                'event_id' => $event->id,
                'level' => $event->level
            ]);
        } catch (Exception $e) {
            Log::error('SMS alert failed', [
                'event_id' => $event->id,
                'error' => $e->getMessage()
            ]);
        }
    }

    /**
     * Отправка SMS
     */
    public function sendSms(string $phone, string $message): void
    {
        if (!$this->enabled) {
            return;
        }

        if (empty($this->phone)) {
            Log::warning('SMS phone not configured');
            return;
        }

        try {
            // Пример для SMS.RU API (можно адаптировать под другие сервисы)
            $response = Http::post($this->gatewayUrl, [
                'api_id' => $this->apiKey,
                'to' => $phone,
                'msg' => $message,
                'json' => 1,
            ]);

            if (!$response->successful()) {
                throw new Exception('SMS API error: ' . $response->body());
            }

            $result = $response->json();
            
            if (isset($result['status']) && $result['status'] !== 'OK') {
                throw new Exception('SMS sending failed: ' . ($result['status_text'] ?? 'Unknown error'));
            }

            Log::info('SMS sent', [
                'phone' => $phone,
                'length' => strlen($message)
            ]);
        } catch (Exception $e) {
            Log::error('SMS send failed', [
                'phone' => $phone,
                'error' => $e->getMessage()
            ]);
            throw $e;
        }
    }

    /**
     * Форматирование SMS сообщения
     * SMS должны быть короткими!
     */
    private function formatAlertMessage(Event $event): string
    {
        // Максимум 160 символов для SMS
        $message = "HYDRO ALERT!\n";
        $message .= "{$event->level}: {$event->node_id}\n";
        $message .= substr($event->message, 0, 100);
        
        return $message;
    }

    /**
     * Проверка доступности SMS шлюза
     */
    public function checkConnection(): bool
    {
        if (!$this->enabled) {
            return false;
        }

        try {
            // Проверка баланса (для SMS.RU)
            $response = Http::get($this->gatewayUrl . '/balance', [
                'api_id' => $this->apiKey,
            ]);

            return $response->successful();
        } catch (Exception $e) {
            Log::error('SMS connection check failed', [
                'error' => $e->getMessage()
            ]);
            return false;
        }
    }
}

