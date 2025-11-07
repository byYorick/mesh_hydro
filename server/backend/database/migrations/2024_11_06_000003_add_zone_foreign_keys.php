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
        // Добавляем внешний ключ от zones к nodes (Root Node)
        Schema::table('zones', function (Blueprint $table) {
            $table->foreign('root_node_id')
                  ->references('node_id')
                  ->on('nodes')
                  ->onDelete('restrict');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::table('zones', function (Blueprint $table) {
            $table->dropForeign(['root_node_id']);
        });
    }
};

