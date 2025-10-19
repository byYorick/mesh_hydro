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
        Schema::create('events', function (Blueprint $table) {
            $table->id();
            $table->string('node_id');
            $table->enum('level', ['info', 'warning', 'critical', 'emergency']);
            $table->text('message');
            $table->json('data')->nullable();
            $table->timestamp('resolved_at')->nullable();
            $table->string('resolved_by')->nullable(); // user_id или 'auto'
            $table->timestamps();

            // Индексы
            $table->index('node_id');
            $table->index('level');
            $table->index('resolved_at');
            $table->index('created_at');
            $table->index(['node_id', 'created_at']); // Составной индекс
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('events');
    }
};

