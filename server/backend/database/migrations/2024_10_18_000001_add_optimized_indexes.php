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
        // Составные индексы для telemetry (самая нагруженная таблица)
        Schema::table('telemetry', function (Blueprint $table) {
            // Индекс для запросов типа: WHERE node_id = X ORDER BY received_at DESC
            $table->index(['node_id', 'received_at'], 'idx_telemetry_node_time');
            
            // Индекс для запросов по типу узла
            $table->index(['node_type', 'received_at'], 'idx_telemetry_type_time');
        });

        // Partial index для активных событий (только для PostgreSQL)
        if (DB::connection()->getDriverName() === 'pgsql') {
            DB::statement('CREATE INDEX idx_events_active ON events(created_at DESC) WHERE resolved_at IS NULL');
            DB::statement('CREATE INDEX idx_events_active_critical ON events(level, created_at DESC) WHERE resolved_at IS NULL AND level IN (\'critical\', \'emergency\')');
        }

        // Составной индекс для events
        Schema::table('events', function (Blueprint $table) {
            $table->index(['node_id', 'created_at'], 'idx_events_node_time');
            $table->index(['level', 'created_at'], 'idx_events_level_time');
        });

        // Составные индексы для nodes
        Schema::table('nodes', function (Blueprint $table) {
            $table->index(['online', 'node_type'], 'idx_nodes_online_type');
            $table->index(['node_type', 'last_seen_at'], 'idx_nodes_type_lastseen');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        // Удаление составных индексов telemetry
        Schema::table('telemetry', function (Blueprint $table) {
            $table->dropIndex('idx_telemetry_node_time');
            $table->dropIndex('idx_telemetry_type_time');
        });

        // Удаление partial indexes (только PostgreSQL)
        if (DB::connection()->getDriverName() === 'pgsql') {
            DB::statement('DROP INDEX IF EXISTS idx_events_active');
            DB::statement('DROP INDEX IF EXISTS idx_events_active_critical');
        }

        // Удаление составных индексов events
        Schema::table('events', function (Blueprint $table) {
            $table->dropIndex('idx_events_node_time');
            $table->dropIndex('idx_events_level_time');
        });

        // Удаление составных индексов nodes
        Schema::table('nodes', function (Blueprint $table) {
            $table->dropIndex('idx_nodes_online_type');
            $table->dropIndex('idx_nodes_type_lastseen');
        });
    }
};

