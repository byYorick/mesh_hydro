<?php

require __DIR__ . '/vendor/autoload.php';

$app = require_once __DIR__ . '/bootstrap/app.php';
$kernel = $app->make(Illuminate\Contracts\Console\Kernel::class);
$kernel->bootstrap();

use App\Models\Node;
use Illuminate\Http\Request;

echo "=== Testing NodeController Query ===\n";

// Simulate NodeController::index logic
$query = Node::query();

echo "1. Node::query() count: " . $query->count() . "\n";

// Test with relation
try {
    $nodes = $query->with(['lastTelemetry'])->get();
    echo "2. with(['lastTelemetry'])->get() count: " . $nodes->count() . "\n";
} catch (Exception $e) {
    echo "2. ERROR: " . $e->getMessage() . "\n";
}

// Test without relation
$nodes = $query->get();
echo "3. get() without relation count: " . $nodes->count() . "\n";

// Test each
$nodes->each(function ($node) {
    try {
        $node->online = $node->isOnline();
        $node->status_color = $node->status_color;
        $node->icon = $node->icon;
        echo "4. Processed node: " . $node->node_id . ", online: " . ($node->online ? 'true' : 'false') . "\n";
    } catch (Exception $e) {
        echo "4. ERROR processing node: " . $e->getMessage() . "\n";
    }
});

// Test JSON encoding
try {
    $json = json_encode($nodes);
    echo "5. JSON encode: " . (strlen($json) > 0 ? 'OK (' . strlen($json) . ' bytes)' : 'EMPTY') . "\n";
    if (strlen($json) > 0 && strlen($json) < 1000) {
        echo "   JSON: " . $json . "\n";
    }
} catch (Exception $e) {
    echo "5. JSON encode ERROR: " . $e->getMessage() . "\n";
}

