<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Уведомления для циклов
     */
    public function up(): void
    {
        Schema::create('cycle_notifications', function (Blueprint $table) {
            $table->id();
            $table->foreignId('cycle_id')->constrained('growth_cycles')->onDelete('cascade');
            
            $table->enum('type', [
                'stage_transition',
                'parameter_deviation',
                'growth_delay',
                'harvest_approaching',
                'resource_low',
                'cycle_report',
                'weekly_report'
            ]);
            
            $table->string('title');
            $table->text('message');
            $table->json('data')->nullable(); // Дополнительные данные
            
            // Статус
            $table->boolean('is_read')->default(false);
            $table->boolean('is_actioned')->default(false); // Пользователь принял действие
            
            // Каналы
            $table->boolean('sent_web')->default(false);
            $table->boolean('sent_telegram')->default(false);
            $table->boolean('sent_sms')->default(false);
            
            $table->timestamps();
            
            $table->index(['cycle_id', 'type']);
            $table->index(['is_read', 'created_at']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('cycle_notifications');
    }
};

