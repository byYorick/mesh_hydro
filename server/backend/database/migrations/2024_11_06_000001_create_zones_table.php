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
        Schema::create('zones', function (Blueprint $table) {
            $table->id();
            
            // Основная информация
            $table->string('name', 100);
            $table->text('description')->nullable();
            
            // ⭐ ЗОНИРОВАНИЕ: Привязка к Root Node
            $table->string('root_node_id', 50)->unique();  // "root_001"
            $table->string('mesh_network_id', 50)->unique(); // "HYDRO1_ZONE1"
            $table->string('mqtt_topic_prefix', 100)->nullable(); // "hydro/zone1/"
            
            // Физическое расположение
            $table->string('location', 100)->nullable();
            
            // Тип зоны
            $table->enum('zone_type', ['nft', 'dwc', 'ebb_flow', 'drip', 'other'])->default('other');
            
            // Объем системы
            $table->decimal('reservoir_volume_liters', 10, 2)->nullable();
            $table->decimal('growing_area_m2', 10, 2)->nullable();
            $table->integer('plant_capacity')->nullable();
            
            // Узлы, закрепленные за зоной
            // JSONB в PostgreSQL для быстрого поиска
            $table->jsonb('assigned_nodes')->default('{}');
            /*
            Структура assigned_nodes:
            {
              "ph_node": "ph_001",
              "climate_node": "climate_001",
              "relay_node": "relay_001",
              "water_node": "water_001",
              "display_node": "display_001"
            }
            */
            
            // Статус
            $table->boolean('is_active')->default(true);
            $table->boolean('is_available')->default(true);
            
            // Текущий активный цикл (будет заполнено после создания growth_cycles)
            $table->bigInteger('current_cycle_id')->nullable();
            
            // Метаданные
            $table->string('image_url', 255)->nullable();
            $table->text('notes')->nullable();
            
            $table->timestamps();
            
            // Индексы
            $table->index('root_node_id');
            $table->index('mesh_network_id');
            $table->index('is_active');
            $table->index('is_available');
            $table->index('current_cycle_id');
            $table->index('zone_type');
            
            // Внешний ключ на nodes (Root Node)
            // Будет добавлен после того как обновим nodes table
            // $table->foreign('root_node_id')->references('node_id')->on('nodes')->onDelete('restrict');
        });

        // GIN индекс для JSONB поля assigned_nodes (только PostgreSQL)
        if (config('database.default') === 'pgsql') {
            DB::statement('CREATE INDEX zones_assigned_nodes_gin ON zones USING GIN (assigned_nodes)');
        }
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('zones');
    }
};

