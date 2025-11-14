<?php

namespace App\Console\Commands;

use App\Services\Mqtt\Enum\MqttMessageType;
use App\Services\MqttService;
use Illuminate\Console\Command;
use Illuminate\Support\Arr;

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

        $topics = config('mqtt.topics', []);
        $subscriptionMap = config('mqtt.subscriptions', []);

        foreach ($subscriptionMap as $topicKey => $type) {
            $topicPattern = Arr::get($topics, $topicKey);
            if (!$topicPattern) {
                $this->warn("Topic pattern not found for subscription key [{$topicKey}]");
                continue;
            }

            $messageType = $type instanceof MqttMessageType ? $type : MqttMessageType::from($type);

            $this->info(sprintf('📡 Subscribing to: %s (%s)', $topicPattern, strtoupper($messageType->value)));

            $mqtt->subscribe($topicPattern, $messageType);
        }

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

}

