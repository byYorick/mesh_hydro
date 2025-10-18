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
        Schema::create('telemetry', function (Blueprint $table) {
            $table->id();
            $table->string('node_id');            // "ph_ec_001"
            $table->string('node_type');          // "ph_ec"
            $table->jsonb('data');                // Данные телеметрии (зависят от типа узла)
            $table->timestamp('received_at');     // Время получения на сервере
            $table->timestamps();

            // Индексы для быстрых запросов
            $table->index('node_id');
            $table->index('node_type');
            $table->index('received_at');
            $table->index(['node_id', 'received_at']); // Составной индекс
        });

        // GIN индекс для поиска по JSONB полю
        DB::statement('CREATE INDEX telemetry_data_gin ON telemetry USING GIN (data)');
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('telemetry');
    }
};

