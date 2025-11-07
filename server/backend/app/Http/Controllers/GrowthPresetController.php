<?php

namespace App\Http\Controllers;

use App\Models\GrowthPreset;
use App\Models\GrowthStage;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Str;

/**
 * ⭐ GROWTH PLANNER: Контроллер пресетов
 */
class GrowthPresetController extends Controller
{
    public function index(Request $request)
    {
        $query = GrowthPreset::with(['culture', 'stages'])
            ->active();

        if ($request->has('culture_id')) {
            $query->where('culture_id', $request->culture_id);
        }

        if ($request->has('type')) {
            $query->where('preset_type', $request->type);
        }

        if ($request->has('system')) {
            $query->where('recommended_system', $request->system);
        }

        $presets = $query->orderBy('usage_count', 'desc')->get();

        return response()->json($presets);
    }

    public function show(GrowthPreset $preset)
    {
        $preset->load(['culture', 'stages', 'cycles']);
        return response()->json($preset);
    }

    public function store(Request $request)
    {
        $validated = $request->validate([
            'culture_id' => 'required|exists:growth_cultures,id',
            'name' => 'required|string|max:255',
            'description' => 'nullable|string',
            'preset_type' => 'required|in:system,custom',
            'total_days' => 'required|integer|min:1',
            'difficulty' => 'required|in:easy,medium,hard',
            'recommended_system' => 'required|in:nft,dwc,drip,ebb_flow,any',
            'is_public' => 'boolean',
            'stages' => 'required|array|min:1',
            'stages.*.name' => 'required|string',
            'stages.*.duration_days' => 'required|integer|min:1',
            'stages.*.target_params' => 'required|array',
            'stages.*.description' => 'nullable|string',
        ]);

        DB::beginTransaction();
        try {
            $validated['slug'] = Str::slug($validated['name'] . '-' . uniqid());

            $preset = GrowthPreset::create($validated);

            foreach ($validated['stages'] as $index => $stageData) {
                GrowthStage::create([
                    'preset_id' => $preset->id,
                    'name' => $stageData['name'],
                    'order' => $index + 1,
                    'duration_days' => $stageData['duration_days'],
                    'target_params' => $stageData['target_params'],
                    'description' => $stageData['description'] ?? null,
                ]);
            }

            DB::commit();

            $preset->load(['culture', 'stages']);
            return response()->json($preset, 201);
        } catch (\Exception $e) {
            DB::rollBack();
            return response()->json(['message' => 'Error creating preset', 'error' => $e->getMessage()], 500);
        }
    }

    public function update(Request $request, GrowthPreset $preset)
    {
        $validated = $request->validate([
            'name' => 'sometimes|string|max:255',
            'description' => 'nullable|string',
            'total_days' => 'sometimes|integer|min:1',
            'difficulty' => 'sometimes|in:easy,medium,hard',
            'recommended_system' => 'sometimes|in:nft,dwc,drip,ebb_flow,any',
            'is_public' => 'boolean',
            'is_active' => 'boolean',
        ]);

        $preset->update($validated);

        return response()->json($preset);
    }

    public function destroy(GrowthPreset $preset)
    {
        if ($preset->preset_type === 'system') {
            return response()->json(['message' => 'Cannot delete system preset'], 403);
        }

        if ($preset->cycles()->where('status', 'active')->exists()) {
            return response()->json(['message' => 'Cannot delete preset with active cycles'], 409);
        }

        $preset->delete();
        return response()->json(null, 204);
    }

    /**
     * Клонировать пресет
     */
    public function clone(GrowthPreset $preset, Request $request)
    {
        $validated = $request->validate([
            'name' => 'required|string|max:255',
        ]);

        DB::beginTransaction();
        try {
            $newPreset = $preset->replicate();
            $newPreset->name = $validated['name'];
            $newPreset->slug = Str::slug($validated['name'] . '-' . uniqid());
            $newPreset->preset_type = 'custom';
            $newPreset->is_public = false;
            $newPreset->usage_count = 0;
            $newPreset->save();

            foreach ($preset->stages as $stage) {
                $newStage = $stage->replicate();
                $newStage->preset_id = $newPreset->id;
                $newStage->save();
            }

            DB::commit();

            $newPreset->load(['culture', 'stages']);
            return response()->json($newPreset, 201);
        } catch (\Exception $e) {
            DB::rollBack();
            return response()->json(['message' => 'Error cloning preset', 'error' => $e->getMessage()], 500);
        }
    }
}

