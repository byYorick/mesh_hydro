<?php

namespace App\Http\Controllers;

use App\Models\PidPreset;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Validator;
use Illuminate\Support\Facades\Log;

class PidPresetController extends Controller
{
    /**
     * Получить все пресеты
     */
    public function index()
    {
        try {
            $presets = PidPreset::orderBy('is_default', 'desc')
                ->orderBy('created_at', 'desc')
                ->get();

            return response()->json($presets);
        } catch (\Exception $e) {
            Log::error('Error fetching PID presets', ['error' => $e->getMessage()]);
            return response()->json(['error' => 'Failed to fetch presets'], 500);
        }
    }

    /**
     * Получить один пресет
     */
    public function show($id)
    {
        try {
            $preset = PidPreset::findOrFail($id);
            return response()->json($preset);
        } catch (\Exception $e) {
            Log::error('Error fetching PID preset', ['id' => $id, 'error' => $e->getMessage()]);
            return response()->json(['error' => 'Preset not found'], 404);
        }
    }

    /**
     * Создать новый пресет
     */
    public function store(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'name' => 'required|string|max:255',
            'description' => 'nullable|string',
            'ph_config' => 'required|array',
            'ec_config' => 'required|array',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'messages' => $validator->errors()
            ], 422);
        }

        try {
            $preset = PidPreset::create([
                'name' => $request->name,
                'description' => $request->description,
                'ph_config' => $request->ph_config,
                'ec_config' => $request->ec_config,
                'is_default' => false,
            ]);

            Log::info('PID preset created', ['preset_id' => $preset->id, 'name' => $preset->name]);

            return response()->json($preset, 201);
        } catch (\Exception $e) {
            Log::error('Error creating PID preset', ['error' => $e->getMessage()]);
            return response()->json(['error' => 'Failed to create preset'], 500);
        }
    }

    /**
     * Обновить пресет
     */
    public function update(Request $request, $id)
    {
        $validator = Validator::make($request->all(), [
            'name' => 'sometimes|required|string|max:255',
            'description' => 'nullable|string',
            'ph_config' => 'sometimes|required|array',
            'ec_config' => 'sometimes|required|array',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'messages' => $validator->errors()
            ], 422);
        }

        try {
            $preset = PidPreset::findOrFail($id);

            // Запретить редактирование дефолтных пресетов
            if ($preset->is_default) {
                return response()->json(['error' => 'Cannot modify default presets'], 403);
            }

            $preset->update($request->only(['name', 'description', 'ph_config', 'ec_config']));

            Log::info('PID preset updated', ['preset_id' => $preset->id, 'name' => $preset->name]);

            return response()->json($preset);
        } catch (\Exception $e) {
            Log::error('Error updating PID preset', ['id' => $id, 'error' => $e->getMessage()]);
            return response()->json(['error' => 'Failed to update preset'], 500);
        }
    }

    /**
     * Удалить пресет
     */
    public function destroy($id)
    {
        try {
            $preset = PidPreset::findOrFail($id);

            // Запретить удаление дефолтных пресетов
            if ($preset->is_default) {
                return response()->json(['error' => 'Cannot delete default presets'], 403);
            }

            $presetName = $preset->name;
            $preset->delete();

            Log::info('PID preset deleted', ['preset_id' => $id, 'name' => $presetName]);

            return response()->json(['message' => 'Preset deleted successfully']);
        } catch (\Exception $e) {
            Log::error('Error deleting PID preset', ['id' => $id, 'error' => $e->getMessage()]);
            return response()->json(['error' => 'Failed to delete preset'], 500);
        }
    }
}

