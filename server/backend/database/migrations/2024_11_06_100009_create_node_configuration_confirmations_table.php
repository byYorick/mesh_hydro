<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     * 
     * ⭐ GROWTH PLANNER: Подтверждения конфигурации от узлов
     */
    public function up(): void
    {
        Schema::create('node_configuration_confirmations', function (Blueprint $table) {
            $table->id();
            $table->foreignId('cycle_id')->nullable()->constrained('growth_cycles')->onDelete('cascade');
            $table->string('node_id');
            
            $table->json('sent_config'); // Конфигурация, которую отправили
            $table->json('confirmed_config')->nullable(); // Конфигурация, которую узел подтвердил
            
            $table->timestamp('sent_at')->useCurrent();
            $table->timestamp('confirmed_at')->nullable();
            $table->timestamp('failed_at')->nullable();
            
            $table->enum('status', ['pending', 'confirmed', 'failed', 'timeout'])->default('pending');
            $table->text('error_message')->nullable();
            
            $table->timestamps();
            
            $table->foreign('node_id')->references('node_id')->on('nodes')->onDelete('cascade');
            
            $table->index(['cycle_id', 'status']);
            $table->index(['node_id', 'sent_at']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('node_configuration_confirmations');
    }
};

