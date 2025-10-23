<?php

namespace App\Services;

use Illuminate\Support\Facades\Cache;
use Illuminate\Support\Facades\Log;

class NotificationThrottleService
{
    /**
     * Ключи для кэширования
     */
    private const THROTTLE_KEY_PREFIX = 'notification_throttle:';
    private const CRITICAL_THROTTLE_KEY = 'critical_notifications:';
    
    /**
     * Настройки throttling
     */
    private const THROTTLE_SETTINGS = [
        'critical' => [
            'max_per_hour' => 5,
            'min_interval' => 300, // 5 минут между критичными уведомлениями
        ],
        'warning' => [
            'max_per_hour' => 10,
            'min_interval' => 180, // 3 минуты между предупреждениями
        ],
        'info' => [
            'max_per_hour' => 20,
            'min_interval' => 60, // 1 минута между информационными
        ],
    ];

    /**
     * Проверить можно ли отправить уведомление
     */
    public function canSendNotification(string $type, string $nodeId, string $message): bool
    {
        $settings = self::THROTTLE_SETTINGS[$type] ?? self::THROTTLE_SETTINGS['info'];
        
        // Проверяем общий лимит по типу
        if (!$this->checkHourlyLimit($type, $settings['max_per_hour'])) {
            Log::warning("Notification throttled: hourly limit exceeded", [
                'type' => $type,
                'node_id' => $nodeId,
                'message' => substr($message, 0, 100)
            ]);
            return false;
        }
        
        // Проверяем минимальный интервал между уведомлениями
        if (!$this->checkMinInterval($type, $nodeId, $settings['min_interval'])) {
            Log::debug("Notification throttled: min interval not reached", [
                'type' => $type,
                'node_id' => $nodeId,
                'message' => substr($message, 0, 100)
            ]);
            return false;
        }
        
        // Проверяем дубликаты сообщений
        if (!$this->checkDuplicateMessage($type, $nodeId, $message)) {
            Log::debug("Notification throttled: duplicate message", [
                'type' => $type,
                'node_id' => $nodeId,
                'message' => substr($message, 0, 100)
            ]);
            return false;
        }
        
        return true;
    }

    /**
     * Зарегистрировать отправку уведомления
     */
    public function markNotificationSent(string $type, string $nodeId, string $message): void
    {
        $now = now();
        
        // Обновляем счетчик за час
        $hourlyKey = self::THROTTLE_KEY_PREFIX . $type . ':' . $now->format('Y-m-d-H');
        Cache::increment($hourlyKey, 1);
        Cache::put($hourlyKey, Cache::get($hourlyKey, 0), 3600); // TTL 1 час
        
        // Записываем время последнего уведомления
        $lastSentKey = self::THROTTLE_KEY_PREFIX . 'last_sent:' . $type . ':' . $nodeId;
        Cache::put($lastSentKey, $now->timestamp, 3600);
        
        // Записываем хеш сообщения для проверки дубликатов
        $messageHash = md5($message);
        $duplicateKey = self::THROTTLE_KEY_PREFIX . 'duplicate:' . $type . ':' . $nodeId . ':' . $messageHash;
        Cache::put($duplicateKey, true, 1800); // TTL 30 минут
    }

    /**
     * Проверить лимит уведомлений в час
     */
    private function checkHourlyLimit(string $type, int $maxPerHour): bool
    {
        $hourlyKey = self::THROTTLE_KEY_PREFIX . $type . ':' . now()->format('Y-m-d-H');
        $count = Cache::get($hourlyKey, 0);
        
        return $count < $maxPerHour;
    }

    /**
     * Проверить минимальный интервал между уведомлениями
     */
    private function checkMinInterval(string $type, string $nodeId, int $minIntervalSeconds): bool
    {
        $lastSentKey = self::THROTTLE_KEY_PREFIX . 'last_sent:' . $type . ':' . $nodeId;
        $lastSent = Cache::get($lastSentKey);
        
        if (!$lastSent) {
            return true;
        }
        
        $timeSinceLastSent = now()->timestamp - $lastSent;
        return $timeSinceLastSent >= $minIntervalSeconds;
    }

    /**
     * Проверить дубликаты сообщений
     */
    private function checkDuplicateMessage(string $type, string $nodeId, string $message): bool
    {
        $messageHash = md5($message);
        $duplicateKey = self::THROTTLE_KEY_PREFIX . 'duplicate:' . $type . ':' . $nodeId . ':' . $messageHash;
        
        return !Cache::has($duplicateKey);
    }

    /**
     * Получить статистику throttling
     */
    public function getThrottleStats(): array
    {
        $stats = [];
        $now = now();
        
        foreach (array_keys(self::THROTTLE_SETTINGS) as $type) {
            $hourlyKey = self::THROTTLE_KEY_PREFIX . $type . ':' . $now->format('Y-m-d-H');
            $stats[$type] = [
                'sent_this_hour' => Cache::get($hourlyKey, 0),
                'max_per_hour' => self::THROTTLE_SETTINGS[$type]['max_per_hour'],
                'min_interval' => self::THROTTLE_SETTINGS[$type]['min_interval'],
            ];
        }
        
        return $stats;
    }

    /**
     * Сбросить все throttling данные
     */
    public function resetThrottling(): void
    {
        $pattern = self::THROTTLE_KEY_PREFIX . '*';
        
        // В реальном приложении нужно использовать Redis SCAN или аналогичный механизм
        // Для простоты используем известные ключи
        $keys = [
            self::THROTTLE_KEY_PREFIX . 'critical:' . now()->format('Y-m-d-H'),
            self::THROTTLE_KEY_PREFIX . 'warning:' . now()->format('Y-m-d-H'),
            self::THROTTLE_KEY_PREFIX . 'info:' . now()->format('Y-m-d-H'),
        ];
        
        foreach ($keys as $key) {
            Cache::forget($key);
        }
        
        Log::info('Notification throttling data reset');
    }
}
