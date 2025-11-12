<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;
use Illuminate\Support\Facades\DB;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        // 1. Обновить существующие узлы с zone='Auto-discovered' или NULL на 'UNCONFIGURED'
        DB::table('nodes')
            ->where(function ($query) {
                $query->where('zone', 'Auto-discovered')
                      ->orWhereNull('zone')
                      ->orWhere('zone', '');
            })
            ->update(['zone' => 'UNCONFIGURED']);

        // 2. Сделать zone NOT NULL
        Schema::table('nodes', function (Blueprint $table) {
            $table->string('zone')->nullable(false)->default('UNCONFIGURED')->change();
        });

        // 3. Добавить индексы для быстрых запросов по зонам
        Schema::table('nodes', function (Blueprint $table) {
            // Комбинированные индексы для частых запросов
            $table->index(['zone', 'node_type'], 'idx_nodes_zone_type');
            $table->index(['zone', 'online'], 'idx_nodes_zone_online');
            $table->index(['root_node_id', 'online'], 'idx_nodes_root_online');
            
            // Индекс для поиска по zone
            $table->index('zone', 'idx_nodes_zone');
        });

        // 4. Создать таблицу для метрик по зонам (опционально)
        if (!Schema::hasTable('zone_metrics')) {
            Schema::create('zone_metrics', function (Blueprint $table) {
                $table->id();
                $table->string('zone')->unique();
                $table->integer('nodes_total')->default(0);
                $table->integer('nodes_online')->default(0);
                $table->timestamp('last_updated_at')->nullable();
                $table->timestamps();
                
                $table->index('zone');
                $table->index('last_updated_at');
            });
        }
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        // Удалить таблицу метрик
        Schema::dropIfExists('zone_metrics');
        
        // Удалить индексы
        Schema::table('nodes', function (Blueprint $table) {
            $table->dropIndex('idx_nodes_zone_type');
            $table->dropIndex('idx_nodes_zone_online');
            $table->dropIndex('idx_nodes_root_online');
            $table->dropIndex('idx_nodes_zone');
        });
        
        // Вернуть zone как nullable
        Schema::table('nodes', function (Blueprint $table) {
            $table->string('zone')->nullable()->change();
        });
    }
};


