<?php

require_once 'vendor/autoload.php';

use App\Models\Node;

// Инициализация Laravel
$app = require_once 'bootstrap/app.php';
$app->make('Illuminate\Contracts\Console\Kernel')->bootstrap();

echo "🔍 Проверка узлов в базе данных:\n";
echo "================================\n";

$nodes = Node::all(['node_id', 'node_type', 'online', 'last_seen_at']);

foreach ($nodes as $node) {
    echo sprintf(
        "ID: %s | Тип: %s | Онлайн: %s | Последний контакт: %s\n",
        $node->node_id,
        $node->node_type,
        $node->online ? 'Да' : 'Нет',
        $node->last_seen_at ? $node->last_seen_at->format('Y-m-d H:i:s') : 'Никогда'
    );
}

echo "\n🔧 Исправление типа узла ph_3f0c00:\n";
echo "====================================\n";

$phNode = Node::where('node_id', 'ph_3f0c00')->first();

if ($phNode) {
    echo "Найден узел: {$phNode->node_id}\n";
    echo "Текущий тип: {$phNode->node_type}\n";
    
    // Исправляем тип на 'ph'
    $phNode->update(['node_type' => 'ph']);
    
    echo "✅ Тип узла обновлен на 'ph'\n";
    
    // Проверяем обновление
    $phNode->refresh();
    echo "Новый тип: {$phNode->node_type}\n";
} else {
    echo "❌ Узел ph_3f0c00 не найден\n";
}

echo "\n🎯 Проверка после исправления:\n";
echo "==============================\n";

$phNode = Node::where('node_id', 'ph_3f0c00')->first();
if ($phNode) {
    echo "ID: {$phNode->node_id} | Тип: {$phNode->node_type} | Онлайн: " . ($phNode->online ? 'Да' : 'Нет') . "\n";
}

echo "\n✅ Готово!\n";
