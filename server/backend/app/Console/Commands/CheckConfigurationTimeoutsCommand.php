<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Services\NodeConfigurationService;
use Illuminate\Support\Facades\Log;

class CheckConfigurationTimeoutsCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'growth:check-config-timeouts';

    /**
     * The console command description.
     */
    protected $description = 'Проверка таймаутов подтверждений конфигурации узлов';

    /**
     * Execute the console command.
     */
    public function handle(NodeConfigurationService $configService): int
    {
        $this->info('⏱️  Проверка таймаутов подтверждений конфигурации...');
        $this->newLine();

        $timeoutCount = $configService->checkTimeouts();

        if ($timeoutCount > 0) {
            $this->warn("⚠️  Найдено {$timeoutCount} подтверждений с таймаутом");
        } else {
            $this->info('✅ Нет подтверждений с таймаутом');
        }

        return 0;
    }
}

