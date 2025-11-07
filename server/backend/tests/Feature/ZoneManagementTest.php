<?php

namespace Tests\Feature;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Models\Zone;
use App\Models\Node;
use Illuminate\Foundation\Testing\RefreshDatabase;

class ZoneManagementTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();
        
        // Создаем Root Node для тестов
        Node::create([
            'node_id' => 'test_root_001',
            'node_type' => 'root',
            'root_node_id' => 'test_root_001',
            'online' => true,
            'last_seen_at' => now(),
        ]);
    }

    #[Test]
    public function it_can_create_zone_with_root_node()
    {
        $zoneData = [
            'name' => 'Test NFT Zone',
            'description' => 'NFT system for testing',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'mqtt_topic_prefix' => 'test/zone1/',
            'zone_type' => 'nft',
            'reservoir_volume_liters' => 100.0,
            'growing_area_m2' => 2.0,
            'plant_capacity' => 20,
            'assigned_nodes' => [
                'ph_node' => 'ph_test_001',
                'climate_node' => 'climate_test_001',
            ],
        ];

        $zone = Zone::create($zoneData);

        $this->assertDatabaseHas('zones', [
            'name' => 'Test NFT Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
        ]);

        $this->assertEquals('test_root_001', $zone->root_node_id);
        $this->assertInstanceOf(Node::class, $zone->rootNode);
    }

    #[Test]
    public function it_prevents_duplicate_root_node_in_zones()
    {
        // Создаем первую зону
        Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        // Попытка создать вторую зону с тем же root_node_id должна провалиться
        $this->expectException(\Illuminate\Database\QueryException::class);

        Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_001', // Дубликат!
            'mesh_network_id' => 'TEST_MESH_002',
            'zone_type' => 'dwc',
        ]);
    }

    #[Test]
    public function it_prevents_duplicate_mesh_network_id()
    {
        // Создаем второй Root Node
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
        ]);

        // Создаем первую зону
        Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        // Попытка создать зону с тем же mesh_network_id
        $this->expectException(\Illuminate\Database\QueryException::class);

        Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'TEST_MESH_001', // Дубликат!
            'zone_type' => 'dwc',
        ]);
    }

    #[Test]
    public function it_creates_nodes_for_zone()
    {
        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
            'assigned_nodes' => [
                'ph_node' => 'ph_test_001',
                'climate_node' => 'climate_test_001',
                'relay_node' => 'relay_test_001',
            ],
        ]);

        // Создаем узлы для зоны
        foreach ($zone->assigned_nodes as $role => $nodeId) {
            $nodeType = str_replace('_node', '', $role);
            if ($role === 'ph_node') $nodeType = 'ph_ec';

            Node::create([
                'node_id' => $nodeId,
                'node_type' => $nodeType,
                'root_node_id' => $zone->root_node_id,
                'online' => true,
            ]);
        }

        // Проверяем что узлы созданы
        $this->assertDatabaseHas('nodes', ['node_id' => 'ph_test_001']);
        $this->assertDatabaseHas('nodes', ['node_id' => 'climate_test_001']);
        $this->assertDatabaseHas('nodes', ['node_id' => 'relay_test_001']);

        // Проверяем что узлы привязаны к Root Node
        $nodes = Node::where('root_node_id', 'test_root_001')
                     ->where('node_type', '!=', 'root')
                     ->get();

        $this->assertCount(3, $nodes);
    }

    #[Test]
    public function it_identifies_available_zones()
    {
        // Доступная зона
        $availableZone = Zone::create([
            'name' => 'Available Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => null,
        ]);

        // Создаем второй Root Node для других зон
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

        // Занятая зона
        $busyZone = Zone::create([
            'name' => 'Busy Zone',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'TEST_MESH_002',
            'zone_type' => 'dwc',
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => 1,
        ]);

        // Неактивная зона
        $inactiveZone = Zone::create([
            'name' => 'Inactive Zone',
            'root_node_id' => 'test_root_003',
            'mesh_network_id' => 'TEST_MESH_003',
            'zone_type' => 'nft',
            'is_active' => false,
            'is_available' => true,
            'current_cycle_id' => null,
        ]);

        $this->assertTrue($availableZone->isAvailableForCycle());
        $this->assertFalse($busyZone->isAvailableForCycle());
        $this->assertFalse($inactiveZone->isAvailableForCycle());

        // Проверяем scope
        $availableZones = Zone::available()->get();
        $this->assertCount(1, $availableZones);
        $this->assertEquals('Available Zone', $availableZones->first()->name);
    }

    #[Test]
    public function zones_maintain_node_isolation()
    {
        // Создаем второй и третий Root Node
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

        // Создаем 3 зоны
        $zone1 = Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'MESH_001',
            'zone_type' => 'nft',
        ]);

        $zone2 = Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'MESH_002',
            'zone_type' => 'dwc',
        ]);

        $zone3 = Zone::create([
            'name' => 'Zone 3',
            'root_node_id' => 'test_root_003',
            'mesh_network_id' => 'MESH_003',
            'zone_type' => 'drip',
        ]);

        // Создаем узлы для каждой зоны
        $nodesZone1 = collect(['ph_001', 'climate_001', 'relay_001'])->map(function($id) {
            return Node::create([
                'node_id' => $id,
                'node_type' => explode('_', $id)[0] === 'ph' ? 'ph_ec' : explode('_', $id)[0],
                'root_node_id' => 'test_root_001',
            ]);
        });

        $nodesZone2 = collect(['ph_002', 'climate_002'])->map(function($id) {
            return Node::create([
                'node_id' => $id,
                'node_type' => explode('_', $id)[0] === 'ph' ? 'ph_ec' : explode('_', $id)[0],
                'root_node_id' => 'test_root_002',
            ]);
        });

        $nodesZone3 = collect(['ph_003', 'climate_003', 'relay_003', 'water_003'])->map(function($id) {
            return Node::create([
                'node_id' => $id,
                'node_type' => explode('_', $id)[0] === 'ph' ? 'ph_ec' : explode('_', $id)[0],
                'root_node_id' => 'test_root_003',
            ]);
        });

        // Проверяем изоляцию: узлы каждой зоны
        $zone1Nodes = Node::inZone('test_root_001')->where('node_type', '!=', 'root')->get();
        $zone2Nodes = Node::inZone('test_root_002')->where('node_type', '!=', 'root')->get();
        $zone3Nodes = Node::inZone('test_root_003')->where('node_type', '!=', 'root')->get();

        $this->assertCount(3, $zone1Nodes);
        $this->assertCount(2, $zone2Nodes);
        $this->assertCount(4, $zone3Nodes);

        // Проверяем что нет пересечений
        $this->assertFalse($zone1Nodes->contains('node_id', 'ph_002'));
        $this->assertFalse($zone2Nodes->contains('node_id', 'ph_001'));
        $this->assertFalse($zone3Nodes->contains('node_id', 'ph_001'));
    }

    #[Test]
    public function it_tracks_zone_node_assignments()
    {
        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        $node = Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
        ]);

        // Создаем запись о назначении
        $assignment = $zone->nodeAssignments()->create([
            'node_id' => $node->node_id,
            'node_role' => 'ph_node',
            'assigned_at' => now(),
            'is_active' => true,
        ]);

        $this->assertDatabaseHas('zone_node_assignments', [
            'zone_id' => $zone->id,
            'node_id' => 'ph_test_001',
            'node_role' => 'ph_node',
            'is_active' => true,
        ]);

        $this->assertCount(1, $zone->nodeAssignments);
    }

    #[Test]
    public function it_generates_correct_mqtt_topic_prefix()
    {
        $zone1 = Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'MESH_001',
            'zone_type' => 'nft',
            'mqtt_topic_prefix' => 'hydro/zone1/',
        ]);

        $this->assertEquals('hydro/zone1/', $zone1->getMqttTopicPrefix());

        // Создаем второй Root Node
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
        ]);

        $zone2 = Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'MESH_002',
            'zone_type' => 'dwc',
            // Без mqtt_topic_prefix - должен генерироваться автоматически
        ]);

        $expectedPrefix = "hydro/zone{$zone2->id}/";
        $this->assertEquals($expectedPrefix, $zone2->getMqttTopicPrefix());
    }
}

