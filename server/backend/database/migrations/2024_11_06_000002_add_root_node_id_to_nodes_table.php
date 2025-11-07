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
        Schema::table('nodes', function (Blueprint $table) {
            // ⭐ ЗОНИРОВАНИЕ: Добавляем привязку к Root Node
            $table->string('root_node_id', 50)->nullable()->after('node_type');
            
            // Индекс для быстрого поиска узлов зоны
            $table->index('root_node_id');
        });

        // Устанавливаем root_node_id для Root Nodes (сами себя)
        DB::statement("UPDATE nodes SET root_node_id = node_id WHERE node_type = 'root'");
        
        // Для остальных узлов устанавливаем root_001 (по умолчанию - существующая система)
        // Это временно, пока не настроим зоны
        DB::statement("UPDATE nodes SET root_node_id = 'root_001' WHERE node_type != 'root' AND root_node_id IS NULL");

        // Добавляем внешний ключ (после заполнения данных)
        Schema::table('nodes', function (Blueprint $table) {
            // Внешний ключ на себя (для связи узлов с Root Node)
            $table->foreign('root_node_id')
                  ->references('node_id')
                  ->on('nodes')
                  ->onDelete('set null');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::table('nodes', function (Blueprint $table) {
            // Удаляем внешний ключ
            $table->dropForeign(['root_node_id']);
            
            // Удаляем индекс
            $table->dropIndex(['root_node_id']);
            
            // Удаляем колонку
            $table->dropColumn('root_node_id');
        });
    }
};

