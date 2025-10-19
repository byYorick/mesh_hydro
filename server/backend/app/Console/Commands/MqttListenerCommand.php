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
    protected $description = 'Listen for MQTT messages from ROOT node and mesh network';

    private int $reconnectAttempts = 0;

    /**
     * Execute the console command.
     */
    public function handle(MqttService $mqtt): int
    {
        $this->info('╔════════════════════════════════════════╗');
        $this->info('║   MQTT Listener for Hydro System      ║');
        $this->info('╚════════════════════════════════════════╝');
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

        // Подписка на телеметрию
        $this->info('📡 Subscribing to: hydro/telemetry/#');
        $mqtt->subscribe('hydro/telemetry/#', function ($topic, $message) use ($mqtt) {
            $this->line("📊 [TELEMETRY] {$topic}");
            $mqtt->handleTelemetry($topic, $message);
        });

        // Подписка на события
        $this->info('📡 Subscribing to: hydro/event/#');
        $mqtt->subscribe('hydro/event/#', function ($topic, $message) use ($mqtt) {
            $this->line("🔔 [EVENT] {$topic}");
            $mqtt->handleEvent($topic, $message);
        });

        // Подписка на heartbeat
        $this->info('📡 Subscribing to: hydro/heartbeat/#');
        $mqtt->subscribe('hydro/heartbeat/#', function ($topic, $message) use ($mqtt) {
            $this->line("💓 [HEARTBEAT] {$topic}");
            $mqtt->handleHeartbeat($topic, $message);
        });

        // Подписка на ответы команд
        $this->info('📡 Subscribing to: hydro/response/#');
        $mqtt->subscribe('hydro/response/#', function ($topic, $message) use ($mqtt) {
            $this->line("📥 [RESPONSE] {$topic}");
            $mqtt->handleCommandResponse($topic, $message);
        });

        // Подписка на discovery (автопоиск узлов)
        // Подписываемся БЕЗ wildcard, т.к. ESP32 публикует в "hydro/discovery"
        $this->info('📡 Subscribing to: hydro/discovery');
        $mqtt->subscribe('hydro/discovery', function ($topic, $message) use ($mqtt) {
            $this->line("🔍 [DISCOVERY] {$topic}");
            $mqtt->handleDiscovery($topic, $message);
        });

        $this->newLine();
        $this->info('🎧 MQTT Listener is running (AUTO-DISCOVERY enabled)...');
        $this->info('Press Ctrl+C to stop');
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
}

