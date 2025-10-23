<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\Node;
use App\Models\Telemetry;
use App\Models\Event;
use App\Models\Command as NodeCommand;
use Illuminate\Support\Facades\DB;

class CleanTestDataCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'db:clean-test-data {--force : Force deletion without confirmation}';

    /**
     * The console command description.
     */
    protected $description = 'Clean all test/seed data from database';

    /**
     * Execute the console command.
     */
    public function handle(): int
    {
        $this->info('╔════════════════════════════════════════════════╗');
        $this->info('║   Очистка тестовых данных из базы данных      ║');
        $this->info('╚════════════════════════════════════════════════╝');
        $this->newLine();

        if (!$this->option('force')) {
            $this->warn('⚠️  Это действие удалит следующие данные:');
            $this->warn('   • Все тестовые узлы (nodes)');
            $this->warn('   • Всю телеметрию (telemetry)');
            $this->warn('   • Все события (events)');
            $this->warn('   • Все команды (commands)');
            $this->warn('   • Историю ошибок узлов (node_errors)');
            $this->newLine();

            if (!$this->confirm('Вы уверены, что хотите продолжить?')) {
                $this->info('Отменено.');
                return 0;
            }
        }

        $this->info('🧹 Начинаю очистку...');
        $this->newLine();

        DB::beginTransaction();

        try {
            // 1. Удаляем команды
            $commandsCount = NodeCommand::count();
            NodeCommand::truncate();
            $this->line("✓ Удалено команд: {$commandsCount}");

            // 2. Удаляем события
            $eventsCount = Event::count();
            Event::truncate();
            $this->line("✓ Удалено событий: {$eventsCount}");

            // 3. Удаляем телеметрию
            $telemetryCount = Telemetry::count();
            Telemetry::truncate();
            $this->line("✓ Удалено записей телеметрии: {$telemetryCount}");

            // 4. Удаляем ошибки узлов (если таблица существует)
            if (DB::getSchemaBuilder()->hasTable('node_errors')) {
                $errorsCount = DB::table('node_errors')->count();
                DB::table('node_errors')->truncate();
                $this->line("✓ Удалено ошибок узлов: {$errorsCount}");
            }

            // 5. Удаляем узлы
            $nodesCount = Node::count();
            Node::truncate();
            $this->line("✓ Удалено узлов: {$nodesCount}");

            DB::commit();

            $this->newLine();
            $this->info('✅ База данных успешно очищена от тестовых данных!');
            $this->info('💡 Узлы будут автоматически добавлены при первом подключении через MQTT.');
            $this->newLine();

            return 0;

        } catch (\Exception $e) {
            DB::rollBack();
            $this->error('❌ Ошибка при очистке: ' . $e->getMessage());
            return 1;
        }
    }
}

