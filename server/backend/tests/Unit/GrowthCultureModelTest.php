<?php

namespace Tests\Unit;

use Tests\TestCase;
use App\Models\GrowthCulture;
use App\Models\GrowthPreset;
use Illuminate\Foundation\Testing\RefreshDatabase;

/**
 * ⭐ GROWTH PLANNER: Unit тесты для GrowthCulture
 */
class GrowthCultureModelTest extends TestCase
{
    use RefreshDatabase;

    /** @test */
    public function it_can_create_a_culture()
    {
        $culture = GrowthCulture::create([
            'name' => 'Test Culture',
            'slug' => 'test-culture',
            'category' => 'leafy_greens',
            'typical_cycle_days' => 30,
            'is_system' => false,
            'is_active' => true,
        ]);

        $this->assertDatabaseHas('growth_cultures', [
            'name' => 'Test Culture',
            'slug' => 'test-culture',
        ]);
    }

    /** @test */
    public function it_has_many_presets()
    {
        $culture = GrowthCulture::create([
            'name' => 'Test Culture',
            'slug' => 'test-culture',
            'category' => 'leafy_greens',
        ]);

        GrowthPreset::create([
            'culture_id' => $culture->id,
            'name' => 'Preset 1',
            'slug' => 'preset-1',
            'preset_type' => 'custom',
            'total_days' => 30,
            'difficulty' => 'easy',
            'recommended_system' => 'nft',
        ]);

        $this->assertCount(1, $culture->presets);
    }

    /** @test */
    public function it_filters_active_cultures()
    {
        GrowthCulture::create([
            'name' => 'Active',
            'slug' => 'active',
            'category' => 'leafy_greens',
            'is_active' => true,
        ]);

        GrowthCulture::create([
            'name' => 'Inactive',
            'slug' => 'inactive',
            'category' => 'leafy_greens',
            'is_active' => false,
        ]);

        $activeCultures = GrowthCulture::active()->get();
        $this->assertCount(1, $activeCultures);
        $this->assertEquals('Active', $activeCultures->first()->name);
    }

    /** @test */
    public function it_has_icon_attribute()
    {
        $culture = GrowthCulture::create([
            'name' => 'Lettuce',
            'slug' => 'lettuce',
            'category' => 'leafy_greens',
        ]);

        $this->assertEquals('mdi-leaf', $culture->icon);
    }

    /** @test */
    public function it_has_category_color_attribute()
    {
        $culture = GrowthCulture::create([
            'name' => 'Strawberry',
            'slug' => 'strawberry',
            'category' => 'berries',
        ]);

        $this->assertEquals('error', $culture->category_color);
    }
}

