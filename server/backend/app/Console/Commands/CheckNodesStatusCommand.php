<?php

namespace App\Console\Commands;

use Illuminate\Console\Command;
use App\Models\Node;
use App\Models\Event;
use App\Services\TelegramService;

class CheckNodesStatusCommand extends Command
{
    /**
     * The name and signature of the console command.
     */
    protected $signature = 'nodes:check-status
                            {--notify : Send notifications for offline nodes}';

    /**
     * The console command description.
     */
    protected $description = 'Check all nodes status and mark offline nodes';

    /**
     * Execute the console command.
     */
    public function handle(): int
    {
        $this->info('Checking nodes status...');
        
        $timeout = config('hydro.node_offline_timeout', 30);
        $offlineThreshold = now()->subSeconds($timeout);
        
        $nodes = Node::all();
        $offlineCount = 0;
        $onlineCount = 0;

        foreach ($nodes as $node) {
            $wasOnline = $node->online;
            $isOnline = $node->isOnline();

            // Обновление статуса
            if ($node->online !== $isOnline) {
                $node->update(['online' => $isOnline]);
            }

            if ($isOnline) {
                $onlineCount++;
                $this->line("✅ {$node->node_id} - ONLINE");
            } else {
                $offlineCount++;
                $this->line("❌ {$node->node_id} - OFFLINE");

                // Если узел только что стал offline - создать событие
                if ($wasOnline && !$isOnline) {
                    $event = Event::create([
                        'node_id' => $node->node_id,
                        'level' => Event::LEVEL_WARNING,
                        'message' => "Node {$node->node_id} went offline",
                        'data' => [
                            'last_seen' => $node->last_seen_at?->toDateTimeString(),
                        ],
                    ]);

                    // Отправка уведомления если включено
                    if ($this->option('notify')) {
                        try {
                            app(TelegramService::class)->sendNodeStatus($node, 'offline');
                        } catch (\Exception $e) {
                            $this->error("Failed to send notification: {$e->getMessage()}");
                        }
                    }
                }
            }
        }

        $this->newLine();
        $this->info("Summary:");
        $this->info("  Online:  {$onlineCount}");
        $this->info("  Offline: {$offlineCount}");
        $this->info("  Total:   {$nodes->count()}");

        return 0;
    }
}

