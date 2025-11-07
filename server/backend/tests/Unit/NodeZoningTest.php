<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Models\Node;
use App\Models\Zone;
use Illuminate\Foundation\Testing\RefreshDatabase;

class NodeZoningTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();
        
        // Очищаем все узлы перед каждым тестом
        Node::query()->delete();
        Zone::query()->delete();
        
        // Создаем Root Node
        Node::create([
            'node_id' => 'test_root_001',
            'node_type' => 'root',
            'root_node_id' => 'test_root_001',
            'online' => true,
            'last_seen_at' => now(),
        ]);
    }

    #[Test]
    public function root_node_has_root_node_id_pointing_to_itself()
    {
        $rootNode = Node::where('node_id', 'test_root_001')->first();

        $this->assertEquals('test_root_001', $rootNode->root_node_id);
        $this->assertTrue($rootNode->isRootNode());
    }

    #[Test]
    public function regular_node_belongs_to_root_node()
    {
        $childNode = Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
            'online' => true,
        ]);

        $rootNode = $childNode->rootNode;

        $this->assertInstanceOf(Node::class, $rootNode);
        $this->assertEquals('test_root_001', $rootNode->node_id);
        $this->assertEquals('root', $rootNode->node_type);
    }

    #[Test]
    public function root_node_has_many_child_nodes()
    {
        // Очищаем все узлы кроме root node
        Node::where('node_id', '!=', 'test_root_001')->delete();
        
        // Создаем дочерние узлы
        Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        Node::create([
            'node_id' => 'climate_test_001',
            'node_type' => 'climate',
            'root_node_id' => 'test_root_001',
        ]);

        Node::create([
            'node_id' => 'relay_test_001',
            'node_type' => 'relay',
            'root_node_id' => 'test_root_001',
        ]);

        $rootNode = Node::where('node_id', 'test_root_001')->first();
        $childNodes = $rootNode->childNodes;

        $this->assertCount(3, $childNodes);
        $this->assertTrue($childNodes->contains('node_id', 'ph_test_001'));
        $this->assertTrue($childNodes->contains('node_id', 'climate_test_001'));
        $this->assertTrue($childNodes->contains('node_id', 'relay_test_001'));
    }

    #[Test]
    public function node_can_access_its_zone()
    {
        // Создаем зону
        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        // Создаем узел в этой зоне
        $node = Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        $nodeZone = $node->zone;

        $this->assertInstanceOf(Zone::class, $nodeZone);
        $this->assertEquals('Test Zone', $nodeZone->name);
        $this->assertEquals('test_root_001', $nodeZone->root_node_id);
    }

    #[Test]
    public function it_checks_if_node_is_root()
    {
        $rootNode = Node::where('node_id', 'test_root_001')->first();
        $this->assertTrue($rootNode->isRootNode());

        $regularNode = Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);
        $this->assertFalse($regularNode->isRootNode());
    }

    #[Test]
    public function it_gets_mesh_nodes_for_root()
    {
        // Очищаем все узлы кроме root node
        Node::where('node_id', '!=', 'test_root_001')->delete();
        
        Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        Node::create([
            'node_id' => 'climate_test_001',
            'node_type' => 'climate',
            'root_node_id' => 'test_root_001',
        ]);

        $rootNode = Node::where('node_id', 'test_root_001')->first();
        $meshNodes = $rootNode->getMeshNodes();

        $this->assertCount(2, $meshNodes);
    }

    #[Test]
    public function it_gets_mesh_nodes_for_regular_node()
    {
        // Очищаем все узлы кроме root node
        Node::where('node_id', '!=', 'test_root_001')->delete();
        
        Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        $regularNode = Node::create([
            'node_id' => 'climate_test_001',
            'node_type' => 'climate',
            'root_node_id' => 'test_root_001',
        ]);

        // Обычный узел должен получить все узлы своего Root
        $meshNodes = $regularNode->getMeshNodes();

        $this->assertCount(2, $meshNodes); // ph_test_001 + climate_test_001
    }

    #[Test]
    public function it_filters_nodes_in_zone()
    {
        // Создаем второй Root Node
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
        ]);

        // Узлы зоны 1
        Node::create([
            'node_id' => 'ph_zone1_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        Node::create([
            'node_id' => 'climate_zone1_001',
            'node_type' => 'climate',
            'root_node_id' => 'test_root_001',
        ]);

        // Узлы зоны 2
        Node::create([
            'node_id' => 'ph_zone2_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_002',
        ]);

        $zone1Nodes = Node::inZone('test_root_001')->get();
        $zone2Nodes = Node::inZone('test_root_002')->get();

        $this->assertCount(3, $zone1Nodes); // root + 2 узла
        $this->assertCount(2, $zone2Nodes); // root + 1 узел

        $this->assertTrue($zone1Nodes->contains('node_id', 'ph_zone1_001'));
        $this->assertTrue($zone1Nodes->contains('node_id', 'climate_zone1_001'));
        $this->assertFalse($zone1Nodes->contains('node_id', 'ph_zone2_001'));

        $this->assertTrue($zone2Nodes->contains('node_id', 'ph_zone2_001'));
        $this->assertFalse($zone2Nodes->contains('node_id', 'ph_zone1_001'));
    }

    #[Test]
    public function it_filters_only_root_nodes()
    {
        // Создаем дополнительные Root Nodes
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
        ]);

        Node::create([
            'node_id' => 'test_root_003',
            'node_type' => 'root',
            'root_node_id' => 'test_root_003',
        ]);

        // Создаем обычные узлы
        Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        Node::create([
            'node_id' => 'climate_test_001',
            'node_type' => 'climate',
            'root_node_id' => 'test_root_001',
        ]);

        $rootNodes = Node::rootNodes()->get();

        $this->assertCount(3, $rootNodes);
        $this->assertTrue($rootNodes->every(fn($node) => $node->node_type === 'root'));
        $this->assertTrue($rootNodes->contains('node_id', 'test_root_001'));
        $this->assertTrue($rootNodes->contains('node_id', 'test_root_002'));
        $this->assertTrue($rootNodes->contains('node_id', 'test_root_003'));
    }

    #[Test]
    public function nodes_in_different_zones_are_isolated()
    {
        // Создаем второй Root Node и зону
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
        ]);

        Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'MESH_ZONE_001',
            'zone_type' => 'nft',
        ]);

        Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'MESH_ZONE_002',
            'zone_type' => 'dwc',
        ]);

        // Узлы зоны 1
        $node1 = Node::create([
            'node_id' => 'ph_zone1',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        // Узлы зоны 2
        $node2 = Node::create([
            'node_id' => 'ph_zone2',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_002',
        ]);

        // Проверяем что узлы знают свою зону
        $this->assertEquals('Zone 1', $node1->zone->name);
        $this->assertEquals('Zone 2', $node2->zone->name);

        // Проверяем что mesh узлы не пересекаются
        $meshNodes1 = $node1->getMeshNodes();
        $meshNodes2 = $node2->getMeshNodes();

        $this->assertFalse($meshNodes1->contains('node_id', 'ph_zone2'));
        $this->assertFalse($meshNodes2->contains('node_id', 'ph_zone1'));
    }

    #[Test]
    public function node_with_null_root_node_id_can_exist()
    {
        // Для обратной совместимости: узлы без root_node_id
        $legacyNode = Node::create([
            'node_id' => 'legacy_node_001',
            'node_type' => 'ph_ec',
            'root_node_id' => null,
            'zone' => 'Legacy Zone', // Старый способ
        ]);

        $this->assertNull($legacyNode->root_node_id);
        $this->assertEquals('Legacy Zone', $legacyNode->zone);
    }

    #[Test]
    public function root_node_fillable_includes_root_node_id()
    {
        $node = new Node();
        $fillable = $node->getFillable();

        $this->assertContains('root_node_id', $fillable);
    }
}

