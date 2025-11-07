<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Рекомендации перехода на следующую стадию (semi-auto)
     */
    public function up(): void
    {
        Schema::create('stage_transition_recommendations', function (Blueprint $table) {
            $table->id();
            $table->foreignId('cycle_id')->constrained('growth_cycles')->onDelete('cascade');
            $table->foreignId('current_stage_id')->constrained('growth_stages')->onDelete('cascade');
            $table->foreignId('recommended_stage_id')->constrained('growth_stages')->onDelete('cascade');
            
            $table->text('reason'); // Причина рекомендации
            $table->json('recommended_params')->nullable(); // Новые параметры для следующей стадии
            
            // Статус
            $table->enum('status', ['pending', 'accepted', 'rejected', 'expired'])->default('pending');
            $table->timestamp('recommended_at')->useCurrent();
            $table->timestamp('responded_at')->nullable();
            $table->unsignedBigInteger('responded_by_user_id')->nullable();
            
            $table->timestamps();
            
            $table->index(['cycle_id', 'status']);
            $table->index(['recommended_at', 'status']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('stage_transition_recommendations');
    }
};

