<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        Schema::create('nodes', function (Blueprint $table) {
            $table->id();
            $table->string('node_id')->unique();  // "ph_ec_001", "climate_001"
            $table->string('node_type');          // "ph_ec", "climate", "relay", "water", "display"
            $table->string('zone')->nullable();   // "Zone 1", "Zone 2"
            $table->string('mac_address', 17)->nullable(); // "AA:BB:CC:DD:EE:FF"
            $table->boolean('online')->default(false);
            $table->timestamp('last_seen_at')->nullable();
            $table->jsonb('config')->nullable();   // Конфигурация узла (JSONB в PostgreSQL)
            $table->jsonb('metadata')->nullable(); // Метаданные (версия прошивки и т.д., JSONB в PostgreSQL)
            $table->timestamps();

            // Индексы для быстрого поиска
            $table->index('node_id');
            $table->index('node_type');
            $table->index('online');
            $table->index('last_seen_at');
        });

        // GIN индексы для JSONB полей (только PostgreSQL)
        if (config('database.default') === 'pgsql') {
            DB::statement('CREATE INDEX nodes_config_gin ON nodes USING GIN (config)');
            DB::statement('CREATE INDEX nodes_metadata_gin ON nodes USING GIN (metadata)');
        }
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('nodes');
    }
};

