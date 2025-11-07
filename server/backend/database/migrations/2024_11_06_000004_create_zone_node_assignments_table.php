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
        Schema::create('zone_node_assignments', function (Blueprint $table) {
            $table->id();
            
            // Зона и узел
            $table->unsignedBigInteger('zone_id');
            $table->string('node_id', 50);
            $table->string('node_role', 50);  // ph_node, climate_node, relay_node, etc.
            
            // Период назначения
            $table->timestamp('assigned_at');
            $table->timestamp('unassigned_at')->nullable();
            
            // Связано с циклом? (NULL если общее назначение зоны)
            $table->bigInteger('cycle_id')->nullable();
            
            // Статус
            $table->boolean('is_active')->default(true);
            
            $table->timestamps();
            
            // Внешние ключи
            $table->foreign('zone_id')->references('id')->on('zones')->onDelete('cascade');
            $table->foreign('node_id')->references('node_id')->on('nodes')->onDelete('cascade');
            
            // Индексы
            $table->index('zone_id');
            $table->index('node_id');
            $table->index('is_active');
            $table->index(['zone_id', 'node_id', 'is_active']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('zone_node_assignments');
    }
};

