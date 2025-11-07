<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: История смены стадий в цикле
     */
    public function up(): void
    {
        Schema::create('cycle_stage_history', function (Blueprint $table) {
            $table->id();
            $table->foreignId('cycle_id')->constrained('growth_cycles')->onDelete('cascade');
            $table->foreignId('stage_id')->constrained('growth_stages')->onDelete('cascade');
            
            $table->timestamp('started_at');
            $table->timestamp('ended_at')->nullable();
            $table->integer('actual_duration_days')->nullable();
            
            // Применённые параметры на этой стадии
            $table->json('applied_params')->nullable();
            
            // Заметки и наблюдения
            $table->text('notes')->nullable();
            
            $table->timestamps();
            
            $table->index(['cycle_id', 'started_at']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('cycle_stage_history');
    }
};

