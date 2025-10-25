<?php

require __DIR__ . '/vendor/autoload.php';

use Illuminate\Support\Facades\DB;

$app = require_once __DIR__ . '/bootstrap/app.php';
$app->make('Illuminate\Contracts\Console\Kernel')->bootstrap();

echo "🔧 Fixing nodes with undefined sensors and capabilities...\n";

try {
    $nodes = DB::table('nodes')->get();
    
    $fixedCount = 0;
    
    foreach ($nodes as $node) {
        $metadata = json_decode($node->metadata, true);
        $needsUpdate = false;
        
        if (!is_array($metadata)) {
            $metadata = [];
            $needsUpdate = true;
        }
        
        // Fix undefined sensors
        if (!isset($metadata['sensors']) || $metadata['sensors'] === null) {
            $metadata['sensors'] = [];
            $needsUpdate = true;
            echo "  - Fixed sensors for node {$node->node_id}\n";
        }
        
        // Fix undefined capabilities
        if (!isset($metadata['capabilities']) || $metadata['capabilities'] === null) {
            $metadata['capabilities'] = [];
            $needsUpdate = true;
            echo "  - Fixed capabilities for node {$node->node_id}\n";
        }
        
        if ($needsUpdate) {
            DB::table('nodes')
                ->where('id', $node->id)
                ->update(['metadata' => json_encode($metadata)]);
            
            $fixedCount++;
        }
    }
    
    echo "\n✅ Fixed {$fixedCount} nodes\n";
    
} catch (Exception $e) {
    echo "❌ Error: " . $e->getMessage() . "\n";
    exit(1);
}
