<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    public function up(): void
    {
        Schema::create('new_nodes', function (Blueprint $table) {
            $table->id();
            $table->string('mac_address', 17)->unique();
            $table->string('node_type');
            $table->boolean('is_root')->default(false);
            $table->string('pin', 6); // PIN из MAC (например, DDEEFF)
            $table->string('temp_mesh_id', 32)->nullable();
            $table->enum('status', ['discovered', 'configuring', 'confirmed', 'failed'])
                ->default('discovered');
            $table->jsonb('metadata')->nullable();
            $table->timestamp('discovered_at');
            $table->timestamp('last_heartbeat_at')->nullable();
            $table->timestamp('configured_at')->nullable();
            $table->timestamps();

            $table->index('mac_address');
            $table->index('node_type');
            $table->index('status');
            $table->index('last_heartbeat_at');
        });
    }

    public function down(): void
    {
        Schema::dropIfExists('new_nodes');
    }
};
