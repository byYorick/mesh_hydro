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
        Schema::create('greenhouses', function (Blueprint $table) {
            $table->id();
            $table->string('name', 150);
            $table->string('code', 100)->unique();
            $table->string('location', 150)->nullable();
            $table->string('description', 1024)->nullable();
            $table->string('timezone', 64)->nullable();
            $table->string('status', 32)->default('active');
            $table->string('mesh_group', 100)->nullable();
            $table->string('root_node_id', 50)->nullable();
            $table->string('root_node_mac', 17)->nullable();
            $table->string('image_url', 255)->nullable();
            $table->json('tags')->nullable();
            $table->json('settings')->nullable();
            $table->timestamps();

            $table->index('status');
            $table->index('root_node_id');
        });

        Schema::table('zones', function (Blueprint $table) {
            $table->foreignId('greenhouse_id')
                ->nullable()
                ->after('id')
                ->constrained('greenhouses')
                ->nullOnDelete();
        });

        Schema::table('nodes', function (Blueprint $table) {
            $table->foreignId('greenhouse_id')
                ->nullable()
                ->after('root_node_id')
                ->constrained('greenhouses')
                ->nullOnDelete();
        });

        Schema::table('growth_cycles', function (Blueprint $table) {
            $table->foreignId('greenhouse_id')
                ->nullable()
                ->after('zone_id')
                ->constrained('greenhouses')
                ->nullOnDelete();
        });

        Schema::create('greenhouse_automation_rules', function (Blueprint $table) {
            $table->id();
            $table->foreignId('greenhouse_id')
                ->constrained('greenhouses')
                ->cascadeOnDelete();
            $table->string('name', 150);
            $table->string('description', 512)->nullable();
            $table->boolean('enabled')->default(true);
            $table->string('trigger_type', 64);
            $table->json('trigger_config');
            $table->json('actions');
            $table->timestamps();

            $table->index(['greenhouse_id', 'enabled']);
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('greenhouse_automation_rules');

        Schema::table('growth_cycles', function (Blueprint $table) {
            $table->dropForeign(['greenhouse_id']);
            $table->dropColumn('greenhouse_id');
        });

        Schema::table('nodes', function (Blueprint $table) {
            $table->dropForeign(['greenhouse_id']);
            $table->dropColumn('greenhouse_id');
        });

        Schema::table('zones', function (Blueprint $table) {
            $table->dropForeign(['greenhouse_id']);
            $table->dropColumn('greenhouse_id');
        });

        Schema::dropIfExists('greenhouses');
    }
};


