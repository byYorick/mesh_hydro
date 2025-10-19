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
        // Кэшируем данные на 10 секунд для производительности
        $summary = Cache::remember('dashboard.summary', 10, function () {
            // Статистика узлов
            $totalNodes = Node::count();
            $onlineNodes = Node::online()->count();
            $offlineNodes = Node::offline()->count();

            $nodesByType = Node::select('node_type', DB::raw('COUNT(*) as count'))
                ->groupBy('node_type')
                ->pluck('count', 'node_type');

            // Статистика событий
            $activeEvents = Event::active()->count();
            $criticalEvents = Event::active()->critical()->count();

            // Оптимизировано: убрали map() - accessors работают автоматически
            $recentEvents = Event::with('node')
                ->latest()
                ->limit(10)
                ->get();

            // Статистика команд за последние 24 часа
            $commandsToday = Command::where('created_at', '>', now()->subDay())->count();
            $commandsPending = Command::pending()->count();

            // Телеметрия за последний час
            $telemetryLastHour = Telemetry::where('received_at', '>', now()->subHour())->count();

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

        // Проверка MQTT - через последнюю активность
        $mqttStatus = 'disconnected';
        try {
            // Проверяем, есть ли телеметрия за последние 2 минуты
            $recentTelemetry = Telemetry::where('received_at', '>', now()->subMinutes(2))->count();
            
            // Если есть свежая телеметрия - MQTT работает
            if ($recentTelemetry > 0) {
                $mqttStatus = 'connected';
            } else {
                // Проверяем через socket напрямую
                $mqttHost = config('mqtt.host', '127.0.0.1');
                $mqttPort = config('mqtt.port', 1883);
                
                $socket = @fsockopen($mqttHost, $mqttPort, $errno, $errstr, 1);
                if ($socket) {
                    $mqttStatus = 'connected';
                    fclose($socket);
                } else {
                    $mqttStatus = 'disconnected';
                }
            }
        } catch (\Exception $e) {
            $mqttStatus = 'error';
        }

        // Проверка Telegram
        $telegramStatus = 'disabled';
        if (config('telegram.enabled', true)) {
            try {
                $telegram = app(\App\Services\TelegramService::class);
                $telegramStatus = $telegram->checkConnection() ? 'ok' : 'error';
            } catch (\Exception $e) {
                $telegramStatus = 'error: ' . $e->getMessage();
            }
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
                $uptime = shell_exec('uptime -p');
                return trim($uptime);
            } catch (\Exception $e) {
                return null;
            }
        }
        return null;
    }
}

