<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Активные циклы роста (привязаны к зонам!)
     */
    public function up(): void
    {
        Schema::create('growth_cycles', function (Blueprint $table) {
            $table->id();
            
            // ⭐ ЗОНИРОВАНИЕ: Привязка к зоне
            $table->foreignId('zone_id')->constrained('zones')->onDelete('cascade');
            
            // Связь с пресетом и культурой
            $table->foreignId('preset_id')->constrained('growth_presets')->onDelete('restrict');
            $table->foreignId('culture_id')->constrained('growth_cultures')->onDelete('restrict');
            
            // Текущая стадия
            $table->foreignId('current_stage_id')->nullable()->constrained('growth_stages')->onDelete('set null');
            
            // Даты
            $table->timestamp('started_at');
            $table->timestamp('expected_harvest_at')->nullable();
            $table->timestamp('actual_harvest_at')->nullable();
            $table->timestamp('ended_at')->nullable();
            
            // Статус
            $table->enum('status', ['planning', 'active', 'paused', 'harvested', 'failed', 'cancelled'])->default('planning');
            
            // Название цикла (опционально)
            $table->string('name')->nullable();
            
            // Количество растений
            $table->integer('plant_count')->nullable();
            
            // Результаты
            $table->decimal('harvest_weight_kg', 10, 3)->nullable();
            $table->text('notes')->nullable();
            $table->integer('rating')->nullable(); // 1-5
            
            // Кто создал
            $table->unsignedBigInteger('created_by_user_id')->nullable(); // TODO: FK когда будет таблица users
            
            $table->timestamps();
            
            $table->index(['zone_id', 'status']);
            $table->index('status');
            $table->index('started_at');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('growth_cycles');
    }
};

