<?php

require __DIR__ . '/vendor/autoload.php';

$app = require_once __DIR__ . '/bootstrap/app.php';
$kernel = $app->make(Illuminate\Contracts\Console\Kernel::class);
$kernel->bootstrap();

use App\Models\Node;

echo "=== Testing Nodes ===\n";

// Test 1: Simple count
echo "1. Node::count(): " . Node::count() . "\n";

// Test 2: Get all nodes
$nodes = Node::all();
echo "2. Node::all()->count(): " . $nodes->count() . "\n";

// Test 3: Get nodes without relations
$nodes = Node::query()->get();
echo "3. Node::query()->get()->count(): " . $nodes->count() . "\n";

// Test 4: Get nodes with lastTelemetry
$nodes = Node::with(['lastTelemetry'])->get();
echo "4. Node::with(['lastTelemetry'])->get()->count(): " . $nodes->count() . "\n";

// Test 5: Check first node
if ($nodes->count() > 0) {
    $node = $nodes->first();
    echo "5. First node:\n";
    echo "   - node_id: " . $node->node_id . "\n";
    echo "   - node_type: " . $node->node_type . "\n";
    echo "   - online: " . ($node->online ? 'true' : 'false') . "\n";
    echo "   - last_seen_at: " . ($node->last_seen_at ? $node->last_seen_at->toDateTimeString() : 'null') . "\n";
    echo "   - isOnline(): " . ($node->isOnline() ? 'true' : 'false') . "\n";
    
    // Test 6: Try to serialize
    try {
        $json = json_encode($node);
        echo "6. JSON encode: " . (strlen($json) > 0 ? 'OK (' . strlen($json) . ' bytes)' : 'EMPTY') . "\n";
    } catch (Exception $e) {
        echo "6. JSON encode ERROR: " . $e->getMessage() . "\n";
    }
} else {
    echo "5. No nodes found!\n";
}

