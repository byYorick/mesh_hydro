<?php

require __DIR__ . '/vendor/autoload.php';

$app = require_once __DIR__ . '/bootstrap/app.php';
$kernel = $app->make(Illuminate\Contracts\Console\Kernel::class);
$kernel->bootstrap();

use App\Models\Node;
use Illuminate\Support\Facades\DB;

echo "=== Database Connection Check ===\n";

// Check DB connection
echo "1. DB Connection:\n";
echo "   - Database: " . DB::connection()->getDatabaseName() . "\n";
echo "   - Host: " . DB::connection()->getConfig('host') . "\n";
echo "   - Username: " . DB::connection()->getConfig('username') . "\n";

// Check if connection works
try {
    DB::connection()->getPdo();
    echo "   - Status: Connected\n";
} catch (Exception $e) {
    echo "   - Status: ERROR - " . $e->getMessage() . "\n";
    exit(1);
}

// Check nodes table
echo "\n2. Nodes Table:\n";
echo "   - Node::count(): " . Node::count() . "\n";
echo "   - DB::table('nodes')->count(): " . DB::table('nodes')->count() . "\n";

// Get all nodes
$nodes = Node::all();
echo "   - Node::all()->count(): " . $nodes->count() . "\n";

if ($nodes->count() > 0) {
    echo "\n3. First Node:\n";
    $node = $nodes->first();
    echo "   - node_id: " . $node->node_id . "\n";
    echo "   - node_type: " . $node->node_type . "\n";
    echo "   - online: " . ($node->online ? 'true' : 'false') . "\n";
}

// Direct SQL query
echo "\n4. Direct SQL Query:\n";
$result = DB::select("SELECT node_id, node_type, online FROM nodes LIMIT 5");
echo "   - Result count: " . count($result) . "\n";
foreach ($result as $row) {
    echo "   - " . $row->node_id . " (" . $row->node_type . ", online: " . ($row->online ? 'true' : 'false') . ")\n";
}

