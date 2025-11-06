<?php

namespace App\Http\Controllers;

use App\Models\Node;
use App\Models\Event;
use App\Models\Telemetry;
use App\Models\Command;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Cache;

class DashboardController extends Controller
{
    /**
     * Общая сводка для дашборда (с кэшированием)
     */
    public function summary(): JsonResponse
    {
        // Кэшируем данные на 15 секунд для производительности (больше чем scheduler интервал)
        $summary = Cache::remember('dashboard.summary', 15, function () {
            // Статистика узлов - оптимизированно через один запрос
            $nodesStats = DB::table('nodes')
                ->selectRaw('
                    COUNT(*) as total,
                    COUNT(*) FILTER (WHERE last_seen_at > NOW() - INTERVAL \'30 seconds\') as online,
                    COUNT(*) FILTER (WHERE last_seen_at IS NULL OR last_seen_at <= NOW() - INTERVAL \'30 seconds\') as offline
                ')
                ->first();
            
            $totalNodes = $nodesStats->total ?? 0;
            $onlineNodes = $nodesStats->online ?? 0;
            $offlineNodes = $nodesStats->offline ?? 0;

            // Статистика по типам узлов - оптимизированный запрос
            $nodesByType = DB::table('nodes')
                ->select('node_type', DB::raw('COUNT(*) as count'))
                ->groupBy('node_type')
                ->pluck('count', 'node_type');

            // Статистика событий - оптимизированные запросы
            $activeEvents = DB::table('events')
                ->whereNull('resolved_at')
                ->count();
            
            $criticalEvents = DB::table('events')
                ->whereNull('resolved_at')
                ->whereIn('level', ['critical', 'emergency'])
                ->count();

            // Последние события - с eager loading для избежания N+1
            $recentEvents = Event::with('node')
                ->latest()
                ->limit(10)
                ->get();

            // Статистика команд - оптимизированные запросы
            $commandsToday = DB::table('commands')
                ->where('created_at', '>', now()->subDay())
                ->count();
            
            $commandsPending = DB::table('commands')
                ->where('status', 'pending')
                ->count();

            // Телеметрия за последний час - оптимизированный запрос
            $telemetryLastHour = DB::table('telemetry')
                ->where('received_at', '>', now()->subHour())
                ->count();

            // Последняя телеметрия от каждого узла
            $latestTelemetry = Node::with('lastTelemetry')
                ->online()
                ->get()
                ->map(function ($node) {
                    return [
                        'node_id' => $node->node_id,
                        'node_type' => $node->node_type,
                        'zone' => $node->zone,
                        'icon' => $node->icon,
                        'data' => $node->lastTelemetry?->data,
                        'received_at' => $node->lastTelemetry?->received_at,
                    ];
                });

            return [
                'nodes' => [
                    'total' => $totalNodes,
                    'online' => $onlineNodes,
                    'offline' => $offlineNodes,
                    'by_type' => $nodesByType,
                ],
                'events' => [
                    'active' => $activeEvents,
                    'critical' => $criticalEvents,
                    'recent' => $recentEvents,
                ],
                'commands' => [
                    'today' => $commandsToday,
                    'pending' => $commandsPending,
                ],
                'telemetry' => [
                    'last_hour' => $telemetryLastHour,
                    'latest' => $latestTelemetry,
                ],
                'timestamp' => now()->toIso8601String(),
            ];
        });

        return response()->json($summary);
    }

    /**
     * Системный статус
     */
    public function status(): JsonResponse
    {
        // Проверка подключения к БД
        $dbStatus = 'ok';
        try {
            DB::connection()->getPdo();
        } catch (\Exception $e) {
            $dbStatus = 'error: ' . $e->getMessage();
        }

        // Проверка MQTT - через последнюю активность (быстро, без socket проверки)
        $mqttStatus = 'disconnected';
        try {
            // Проверяем, есть ли хотя бы одна телеметрия за последние 2 минуты (limit(1) быстрее чем count())
            $recentTelemetry = Telemetry::where('received_at', '>', now()->subMinutes(2))->limit(1)->exists();
            
            // Если есть свежая телеметрия - MQTT работает
            if ($recentTelemetry) {
                $mqttStatus = 'connected';
            }
            // Если нет свежей телеметрии - считаем disconnected (без медленной socket проверки)
        } catch (\Exception $e) {
            $mqttStatus = 'error';
        }

        // Проверка Telegram (быстрая - только проверка конфигурации, без внешних запросов)
        $telegramStatus = 'disabled';
        if (config('telegram.enabled', false)) {
            // Проверяем только наличие токена (быстро), без реальной проверки соединения
            $telegramToken = config('telegram.bot_token');
            $telegramStatus = !empty($telegramToken) ? 'configured' : 'not_configured';
        }

        // Информация о системе
        $systemInfo = [
            'php_version' => PHP_VERSION,
            'laravel_version' => app()->version(),
            'server_time' => now()->toDateTimeString(),
            'uptime' => $this->getServerUptime(),
        ];

        return response()->json([
            'status' => 'running',
            'database' => $dbStatus,
            'mqtt' => $mqttStatus,
            'telegram' => $telegramStatus,
            'system' => $systemInfo,
        ]);
    }

    /**
     * Получить uptime сервера (если доступно)
     */
    private function getServerUptime(): ?string
    {
        if (PHP_OS_FAMILY === 'Linux') {
            try {
                // Читаем /proc/uptime напрямую (быстрее и надежнее для Alpine/BusyBox)
                $uptimeFile = '/proc/uptime';
                if (file_exists($uptimeFile)) {
                    $content = file_get_contents($uptimeFile);
                    if ($content !== false) {
                        $uptimeSeconds = (float)explode(' ', trim($content))[0];
                        return $this->formatUptime($uptimeSeconds);
                    }
                }
                return null;
            } catch (\Exception $e) {
                return null;
            }
        }
        return null;
    }

    /**
     * Форматировать секунды в читаемый uptime
     */
    private function formatUptime(float $seconds): string
    {
        $days = floor($seconds / 86400);
        $hours = floor(($seconds % 86400) / 3600);
        $minutes = floor(($seconds % 3600) / 60);
        
        $parts = [];
        if ($days > 0) $parts[] = "{$days} day" . ($days > 1 ? 's' : '');
        if ($hours > 0) $parts[] = "{$hours} hour" . ($hours > 1 ? 's' : '');
        if ($minutes > 0 && $days === 0) $parts[] = "{$minutes} minute" . ($minutes > 1 ? 's' : '');
        
        return 'up ' . implode(', ', $parts);
    }
}

