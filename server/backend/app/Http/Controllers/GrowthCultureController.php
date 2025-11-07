<?php

namespace App\Http\Controllers;

use App\Models\GrowthCulture;
use Illuminate\Http\Request;

/**
 * ⭐ GROWTH PLANNER: Контроллер культур
 */
class GrowthCultureController extends Controller
{
    public function index()
    {
        $cultures = GrowthCulture::active()
            ->with(['presets' => function ($query) {
                $query->active();
            }])
            ->get();

        return response()->json($cultures);
    }

    public function show(GrowthCulture $culture)
    {
        $culture->load(['presets.stages']);
        return response()->json($culture);
    }

    public function store(Request $request)
    {
        $validated = $request->validate([
            'name' => 'required|string|max:255',
            'slug' => 'required|string|unique:growth_cultures,slug',
            'category' => 'required|in:leafy_greens,berries,vegetables,herbs',
            'description' => 'nullable|string',
            'typical_cycle_days' => 'nullable|integer|min:1',
            'optimal_temp_min' => 'nullable|numeric',
            'optimal_temp_max' => 'nullable|numeric',
            'optimal_humidity_min' => 'nullable|numeric',
            'optimal_humidity_max' => 'nullable|numeric',
            'image_url' => 'nullable|url',
        ]);

        $culture = GrowthCulture::create($validated);

        return response()->json($culture, 201);
    }

    public function update(Request $request, GrowthCulture $culture)
    {
        $validated = $request->validate([
            'name' => 'sometimes|string|max:255',
            'category' => 'sometimes|in:leafy_greens,berries,vegetables,herbs',
            'description' => 'nullable|string',
            'typical_cycle_days' => 'nullable|integer|min:1',
            'optimal_temp_min' => 'nullable|numeric',
            'optimal_temp_max' => 'nullable|numeric',
            'optimal_humidity_min' => 'nullable|numeric',
            'optimal_humidity_max' => 'nullable|numeric',
            'image_url' => 'nullable|url',
            'is_active' => 'boolean',
        ]);

        $culture->update($validated);

        return response()->json($culture);
    }

    public function destroy(GrowthCulture $culture)
    {
        if ($culture->is_system) {
            return response()->json(['message' => 'Cannot delete system culture'], 403);
        }

        $culture->delete();
        return response()->json(null, 204);
    }
}

