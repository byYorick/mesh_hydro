<?php

namespace Tests\Unit;

use Tests\TestCase;
use PHPUnit\Framework\Attributes\Test;
use App\Models\Zone;
use App\Models\Node;
use App\Models\ZoneNodeAssignment;
use Illuminate\Foundation\Testing\RefreshDatabase;

class ZoneModelTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();
        
        // Очищаем все данные перед каждым тестом
        Node::query()->delete();
        Zone::query()->delete();
        
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
    public function it_can_create_a_zone()
    {
        $zone = Zone::create([
            'name' => 'Test Zone',
            'description' => 'Test zone description',
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
            'is_active' => true,
            'is_available' => true,
        ]);

        $this->assertInstanceOf(Zone::class, $zone);
        $this->assertEquals('Test Zone', $zone->name);
        $this->assertEquals('test_root_001', $zone->root_node_id);
        $this->assertEquals('TEST_MESH_001', $zone->mesh_network_id);
        $this->assertTrue($zone->is_active);
        $this->assertTrue($zone->is_available);
    }

    #[Test]
    public function it_belongs_to_root_node()
    {
        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        $rootNode = $zone->rootNode;

        $this->assertInstanceOf(Node::class, $rootNode);
        $this->assertEquals('test_root_001', $rootNode->node_id);
        $this->assertEquals('root', $rootNode->node_type);
    }

    #[Test]
    public function it_has_many_nodes()
    {
        // Очищаем все узлы кроме root node
        Node::where('node_id', '!=', 'test_root_001')->delete();
        
        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        // Создаем узлы для зоны
        Node::create([
            'node_id' => 'ph_test_001',
            'node_type' => 'ph_ec',
            'root_node_id' => 'test_root_001',
            'online' => true,
        ]);

        Node::create([
            'node_id' => 'climate_test_001',
            'node_type' => 'climate',
            'root_node_id' => 'test_root_001',
            'online' => true,
        ]);

        $nodes = $zone->nodes;

        $this->assertCount(2, $nodes);
        $this->assertTrue($nodes->contains('node_id', 'ph_test_001'));
        $this->assertTrue($nodes->contains('node_id', 'climate_test_001'));
    }

    #[Test]
    public function it_can_get_node_by_role()
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

        $this->assertEquals('ph_test_001', $zone->getNodeByRole('ph_node'));
        $this->assertEquals('climate_test_001', $zone->getNodeByRole('climate_node'));
        $this->assertEquals('relay_test_001', $zone->getNodeByRole('relay_node'));
        $this->assertNull($zone->getNodeByRole('water_node'));
    }

    #[Test]
    public function it_can_get_all_nodes()
    {
        // Используем существующий Root Node из setUp
        $rootNode = Node::where('node_id', 'test_root_001')->first();
        
        // Создаем дочерние узлы
        $phNode = Node::create(['node_id' => 'ph_test_001', 'node_type' => 'ph_ec', 'root_node_id' => 'test_root_001']);
        $climateNode = Node::create(['node_id' => 'climate_test_001', 'node_type' => 'climate', 'root_node_id' => 'test_root_001']);
        $relayNode = Node::create(['node_id' => 'relay_test_001', 'node_type' => 'relay', 'root_node_id' => 'test_root_001']);

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

        $nodes = $zone->getAllNodes();

        $this->assertCount(3, $nodes);
        $this->assertTrue($nodes->contains('node_id', 'ph_test_001'));
        $this->assertTrue($nodes->contains('node_id', 'climate_test_001'));
        $this->assertTrue($nodes->contains('node_id', 'relay_test_001'));
    }

    #[Test]
    public function it_checks_availability_for_new_cycle()
    {
        // Зона доступна
        $zone1 = Zone::create([
            'name' => 'Available Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => null,
        ]);

        $this->assertTrue($zone1->isAvailableForCycle());

        // Создаем второй Root Node для второй зоны
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
            'online' => true,
            'last_seen_at' => now(),
        ]);

        // Зона недоступна (неактивна)
        $zone2 = Zone::create([
            'name' => 'Inactive Zone',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'TEST_MESH_002',
            'zone_type' => 'dwc',
            'is_active' => false,
            'is_available' => true,
            'current_cycle_id' => null,
        ]);

        $this->assertFalse($zone2->isAvailableForCycle());

        // Создаем третий Root Node для занятой зоны
        Node::create([
            'node_id' => 'test_root_003',
            'node_type' => 'root',
            'root_node_id' => 'test_root_003',
            'online' => true,
            'last_seen_at' => now(),
        ]);

        // Зона занята (есть активный цикл)
        $busyZone = Zone::create([
            'name' => 'Busy Zone',
            'root_node_id' => 'test_root_003',
            'mesh_network_id' => 'TEST_MESH_003',
            'zone_type' => 'nft',
            'is_active' => true,
            'is_available' => false,
            'current_cycle_id' => 123,
        ]);

        $this->assertFalse($busyZone->isAvailableForCycle());
    }

    #[Test]
    public function it_generates_mqtt_topic_prefix()
    {
        // Создаем второй root узел (test_root_001 уже создан в setUp)
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
            'online' => true,
            'last_seen_at' => now(),
        ]);

        // С явно заданным префиксом
        $zone1 = Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
            'mqtt_topic_prefix' => 'hydro/zone1/',
        ]);

        $this->assertEquals('hydro/zone1/', $zone1->getMqttTopicPrefix());

        // Без префикса - генерируется автоматически
        $zone2 = Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'TEST_MESH_002',
            'zone_type' => 'dwc',
        ]);

        $expectedPrefix = "hydro/zone{$zone2->id}/";
        $this->assertEquals($expectedPrefix, $zone2->getMqttTopicPrefix());
    }

    #[Test]
    public function it_has_icon_attribute()
    {
        $zoneNFT = Zone::factory()->create(['zone_type' => 'nft']);
        $zoneDWC = Zone::factory()->create(['zone_type' => 'dwc']);
        $zoneDrip = Zone::factory()->create(['zone_type' => 'drip']);
        $zoneOther = Zone::factory()->create(['zone_type' => 'other']);

        $this->assertEquals('mdi-water-pump', $zoneNFT->icon);
        $this->assertEquals('mdi-water', $zoneDWC->icon);
        $this->assertEquals('mdi-water-opacity', $zoneDrip->icon);
        $this->assertEquals('mdi-sprout', $zoneOther->icon);
    }

    #[Test]
    public function it_has_status_attribute()
    {
        // Используем существующий Root Node из setUp
        $rootNode = Node::where('node_id', 'test_root_001')->first();
        
        // Создаем дочерние узлы
        $phNode = Node::create(['node_id' => 'ph_test_001', 'node_type' => 'ph_ec', 'root_node_id' => 'test_root_001']);
        $climateNode = Node::create(['node_id' => 'climate_test_001', 'node_type' => 'climate', 'root_node_id' => 'test_root_001']);

        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
            'is_active' => true,
            'is_available' => true,
            'assigned_nodes' => [
                'ph_node' => 'ph_test_001',
                'climate_node' => 'climate_test_001',
            ],
        ]);

        $status = $zone->status;

        $this->assertIsArray($status);
        $this->assertArrayHasKey('is_active', $status);
        $this->assertArrayHasKey('is_available', $status);
        $this->assertArrayHasKey('has_active_cycle', $status);
        $this->assertArrayHasKey('nodes_count', $status);
        $this->assertArrayHasKey('root_node_online', $status);

        $this->assertTrue($status['is_active']);
        $this->assertTrue($status['is_available']);
        $this->assertFalse($status['has_active_cycle']);
        $this->assertEquals(2, $status['nodes_count']);
        $this->assertTrue($status['root_node_online']);
    }

    #[Test]
    public function it_filters_active_zones()
    {
        Zone::factory()->create(['is_active' => true]);
        Zone::factory()->create(['is_active' => true]);
        Zone::factory()->create(['is_active' => false]);

        $activeZones = Zone::active()->get();

        $this->assertCount(2, $activeZones);
        $this->assertTrue($activeZones->every(fn($zone) => $zone->is_active === true));
    }

    #[Test]
    public function it_filters_available_zones()
    {
        Zone::factory()->create([
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => null,
        ]);

        Zone::factory()->create([
            'is_active' => true,
            'is_available' => true,
            'current_cycle_id' => 1,
        ]);

        Zone::factory()->create([
            'is_active' => true,
            'is_available' => false,
            'current_cycle_id' => null,
        ]);

        $availableZones = Zone::available()->get();

        $this->assertCount(1, $availableZones);
    }

    #[Test]
    public function it_filters_zones_by_type()
    {
        Zone::factory()->nft()->create();
        Zone::factory()->nft()->create();
        Zone::factory()->dwc()->create();

        $nftZones = Zone::ofType('nft')->get();

        $this->assertCount(2, $nftZones);
        $this->assertTrue($nftZones->every(fn($zone) => $zone->zone_type === 'nft'));
    }

    #[Test]
    public function it_casts_assigned_nodes_to_array()
    {
        $zone = Zone::create([
            'name' => 'Test Zone',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
            'assigned_nodes' => [
                'ph_node' => 'ph_test_001',
                'climate_node' => 'climate_test_001',
            ],
        ]);

        $this->assertIsArray($zone->assigned_nodes);
        $this->assertEquals('ph_test_001', $zone->assigned_nodes['ph_node']);
        $this->assertEquals('climate_test_001', $zone->assigned_nodes['climate_node']);
    }

    #[Test]
    public function it_requires_unique_root_node_id()
    {
        Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        $this->expectException(\Illuminate\Database\QueryException::class);

        // Попытка создать вторую зону с тем же root_node_id
        Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_002',
            'zone_type' => 'dwc',
        ]);
    }

    #[Test]
    public function it_requires_unique_mesh_network_id()
    {
        Zone::create([
            'name' => 'Zone 1',
            'root_node_id' => 'test_root_001',
            'mesh_network_id' => 'TEST_MESH_001',
            'zone_type' => 'nft',
        ]);

        $this->expectException(\Illuminate\Database\QueryException::class);

        // Создаем второй Root Node
        Node::create([
            'node_id' => 'test_root_002',
            'node_type' => 'root',
            'root_node_id' => 'test_root_002',
        ]);

        // Попытка создать зону с тем же mesh_network_id
        Zone::create([
            'name' => 'Zone 2',
            'root_node_id' => 'test_root_002',
            'mesh_network_id' => 'TEST_MESH_001', // Дубликат!
            'zone_type' => 'dwc',
        ]);
    }
}

