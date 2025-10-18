<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Services\TelegramService;
use App\Services\MqttService;
use App\Models\Node;
use App\Models\Event;
use Illuminate\Support\Facades\Log;

class TelegramBotCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'telegram:bot';

    /**
     * The console command description.
     */
    protected $description = 'Run Telegram bot for system notifications and commands';

    /**
     * Execute the console command.
     */
    public function handle(TelegramService $telegram): int
    {
        $this->info('╔════════════════════════════════════════╗');
        $this->info('║   Telegram Bot for Hydro System       ║');
        $this->info('╚════════════════════════════════════════╝');
        $this->newLine();

        // Проверка подключения
        if (!$telegram->checkConnection()) {
            $this->error('❌ Cannot connect to Telegram API');
            $this->error('Please check your TELEGRAM_BOT_TOKEN in .env');
            return 1;
        }

        $this->info('✅ Connected to Telegram API');
        
        // Отправка стартового сообщения
        try {
            $telegram->sendMessage(
                "🤖 <b>Hydro System Bot Started</b>\n\n" .
                "Bot is now monitoring your hydroponic system.\n" .
                "You will receive alerts for critical events."
            );
        } catch (\Exception $e) {
            $this->error("Failed to send startup message: {$e->getMessage()}");
        }

        $this->info('🎧 Telegram bot is running...');
        $this->info('Press Ctrl+C to stop');
        $this->newLine();

        // TODO: Реализовать обработку входящих команд через long polling
        // Сейчас бот работает только для отправки уведомлений (через MqttService)
        
        $this->comment('Note: This bot currently only sends notifications.');
        $this->comment('Command handling will be implemented in future version.');

        // Держим процесс активным
        while (true) {
            sleep(3600); // Спим 1 час
        }

        return 0;
    }
}

