<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Services\MqttService;
use Illuminate\Support\Facades\Log;

class MqttListenerCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'mqtt:listen 
                            {--reconnect-delay=5 : Delay in seconds before reconnecting}
                            {--max-retries=0 : Maximum reconnection attempts (0 = infinite)}';

    /**
     * The console command description.
     */
    protected $description = '⭐ Listen for MQTT messages from ALL zones (multi-zone support)';

    private int $reconnectAttempts = 0;

    /**
     * Execute the console command.
     */
    public function handle(MqttService $mqtt): int
    {
        $this->info('╔═══════════════════════════════════════════╗');
        $this->info('║  ⭐ MQTT Listener (MULTI-ZONE SUPPORT)   ║');
        $this->info('║     Hydroponic Management System         ║');
        $this->info('╚═══════════════════════════════════════════╝');
        $this->newLine();
        
        $maxRetries = (int) $this->option('max-retries');
        $reconnectDelay = (int) $this->option('reconnect-delay');

        while (true) {
            try {
                $this->connectAndListen($mqtt);
            } catch (\Exception $e) {
                $this->error("❌ MQTT Error: {$e->getMessage()}");
                
                $this->reconnectAttempts++;
                
                if ($maxRetries > 0 && $this->reconnectAttempts >= $maxRetries) {
                    $this->error("Maximum reconnection attempts reached ({$maxRetries})");
                    return 1;
                }
                
                $this->warn("Reconnecting in {$reconnectDelay} seconds... (attempt {$this->reconnectAttempts})");
                sleep($reconnectDelay);
            }
        }

        return 0;
    }

    /**
     * Подключение и прослушивание MQTT
     */
    private function connectAndListen(MqttService $mqtt): void
    {
        $this->info('🔌 Connecting to MQTT broker...');
        $mqtt->connect();
        $this->info('✅ Connected successfully!');
        $this->newLine();

        // Сброс счетчика попыток переподключения
        $this->reconnectAttempts = 0;

        // ⭐ ЗОНИРОВАНИЕ: Подписка на телеметрию ВСЕХ зон
        // Формат: hydro/+/telemetry/# где + = любая зона (zone1, zone2, zone3...)
        $this->info('📡 Subscribing to: hydro/+/telemetry/# (ALL ZONES)');
        $mqtt->subscribe('hydro/+/telemetry/#', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("📊 [TELEMETRY] [{$zoneName}] {$topic}");
            $mqtt->handleTelemetry($topic, $message);
        });

        // ⭐ ЗОНИРОВАНИЕ: Подписка на события ВСЕХ зон
        $this->info('📡 Subscribing to: hydro/+/event/# (ALL ZONES)');
        $mqtt->subscribe('hydro/+/event/#', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("🔔 [EVENT] [{$zoneName}] {$topic}");
            $mqtt->handleEvent($topic, $message);
        });

        // ⭐ ЗОНИРОВАНИЕ: Подписка на heartbeat ВСЕХ зон
        $this->info('📡 Subscribing to: hydro/+/heartbeat/# (ALL ZONES)');
        $mqtt->subscribe('hydro/+/heartbeat/#', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("💓 [HEARTBEAT] [{$zoneName}] {$topic}");
            $mqtt->handleHeartbeat($topic, $message);
        });

        // ⭐ ЗОНИРОВАНИЕ: Подписка на ответы команд ВСЕХ зон
        $this->info('📡 Subscribing to: hydro/+/response/# (ALL ZONES)');
        $mqtt->subscribe('hydro/+/response/#', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("📥 [RESPONSE] [{$zoneName}] {$topic}");
            $mqtt->handleCommandResponse($topic, $message);
        });

        // Подписка на discovery (автопоиск узлов)
        $this->info('📡 Subscribing to: hydro/+/discovery');
        $mqtt->subscribe('hydro/+/discovery', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("🔍 [DISCOVERY] [{$zoneName}] {$topic}");
            $mqtt->handleDiscovery($topic, $message);
        });

        // ⭐ ЗОНИРОВАНИЕ: Подписка на config_response ВСЕХ зон
        $this->info('📡 Subscribing to: hydro/+/config_response/# (ALL ZONES)');
        $mqtt->subscribe('hydro/+/config_response/#', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("📋 [CONFIG_RESPONSE] [{$zoneName}] {$topic}");
            $mqtt->handleConfigResponse($topic, $message);
        });

        // ⭐ ЗОНИРОВАНИЕ: Подписка на ошибки узлов ВСЕХ зон
        $this->info('📡 Subscribing to: hydro/+/error/# (ALL ZONES)');
        $mqtt->subscribe('hydro/+/error/#', function ($topic, $message) use ($mqtt) {
            $zoneName = $this->extractZoneFromTopic($topic);
            $this->line("❌ [ERROR] [{$zoneName}] {$topic}");
            $mqtt->handleError($topic, $message);
        });

        $this->newLine();
        $this->info('🎧 ⭐ MQTT Listener is running (MULTI-ZONE + AUTO-DISCOVERY)...');
        $this->info('   Listening to ALL zones: zone1, zone2, zone3...');
        $this->info('   Press Ctrl+C to stop');
        $this->newLine();

        // Обработка Ctrl+C для корректного закрытия (только для Unix)
        if (function_exists('pcntl_async_signals')) {
            pcntl_async_signals(true);
            pcntl_signal(SIGINT, function () use ($mqtt) {
                $this->newLine();
                $this->warn('⚠️  Shutting down...');
                $mqtt->disconnect();
                $this->info('👋 Goodbye!');
                exit(0);
            });

            pcntl_signal(SIGTERM, function () use ($mqtt) {
                $this->newLine();
                $this->warn('⚠️  Shutting down...');
                $mqtt->disconnect();
                $this->info('👋 Goodbye!');
                exit(0);
            });
        }

        // Для Windows: используем try-catch для graceful shutdown
        try {
            // Бесконечный цикл прослушивания с обработкой сообщений
            while (true) {
                $mqtt->loop(true); // blocking = true, exitWhenQueuesEmpty = false (default)
            }
        } catch (\Exception $e) {
            $this->newLine();
            $this->warn('⚠️  Shutting down...');
            $mqtt->disconnect();
            throw $e;
        }
    }

    /**
     * ⭐ ЗОНИРОВАНИЕ: Извлечение имени зоны из MQTT топика
     * 
     * Примеры:
     * - hydro/zone1/telemetry/ph_001 → zone1
     * - hydro/zone2/event/critical → zone2
     * - hydro/zone123/heartbeat/root_123 → zone123
     * 
     * @param string $topic MQTT топик
     * @return string Имя зоны (например "zone1") или "unknown"
     */
    private function extractZoneFromTopic(string $topic): string
    {
        $parts = explode('/', $topic);

        if (count($parts) >= 2 && $parts[0] === 'hydro') {
            $zone = $parts[1];
            if ($zone === 'setup') {
                return 'setup';
            }

            return $zone;
        }

        return 'unknown';
    }
}

