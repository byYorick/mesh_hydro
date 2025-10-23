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
        Schema::create('node_errors', function (Blueprint $table) {
            $table->id();
            $table->string('node_id');
            $table->string('error_code');       // SENSOR_FAIL, HEAP_LOW, NETWORK_TIMEOUT
            $table->string('error_type');       // hardware, software, network, sensor
            $table->enum('severity', ['low', 'medium', 'high', 'critical']);
            $table->text('message');
            $table->text('stack_trace')->nullable();
            $table->jsonb('diagnostics');        // JSONB в PostgreSQL
            $table->timestamp('occurred_at');
            $table->timestamp('resolved_at')->nullable();
            $table->text('resolution_notes')->nullable();
            $table->string('resolved_by')->nullable(); // user_id или 'auto'
            $table->timestamps();

            // Индексы
            $table->index('node_id');
            $table->index('error_code');
            $table->index('error_type');
            $table->index('severity');
            $table->index('occurred_at');
            $table->index(['node_id', 'occurred_at']); // Составной индекс
            $table->index(['error_code', 'occurred_at']);
            $table->index(['severity', 'occurred_at']);
        });

        // GIN индекс для JSONB поля diagnostics (только PostgreSQL)
        if (config('database.default') === 'pgsql') {
            DB::statement('CREATE INDEX node_errors_diagnostics_gin ON node_errors USING GIN (diagnostics)');
        }
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('node_errors');
    }
};

