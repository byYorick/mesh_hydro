<?php

namespace App\Http\Controllers;

use App\Models\GrowthPreset;
use App\Models\PresetRating;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

/**
 * ⭐ GROWTH PLANNER: Контроллер рейтингов пресетов
 */
class PresetRatingController extends Controller
{
    /**
     * Добавить или обновить рейтинг пресета
     */
    public function rate(Request $request, GrowthPreset $preset): JsonResponse
    {
        $validated = $request->validate([
            'rating' => 'required|integer|min:1|max:5',
            'comment' => 'nullable|string|max:1000',
        ]);

        // TODO: Получить user_id из аутентификации
        $userId = $request->user()?->id ?? 1; // Заглушка

        // Проверяем существующий рейтинг
        $existingRating = PresetRating::where('preset_id', $preset->id)
            ->where('user_id', $userId)
            ->first();

        if ($existingRating) {
            // Обновляем существующий
            $existingRating->update([
                'rating' => $validated['rating'],
                'comment' => $validated['comment'] ?? null,
            ]);
            $rating = $existingRating;
        } else {
            // Создаем новый
            $rating = PresetRating::create([
                'preset_id' => $preset->id,
                'user_id' => $userId,
                'rating' => $validated['rating'],
                'comment' => $validated['comment'] ?? null,
            ]);
        }

        return response()->json($rating);
    }

    /**
     * Получить рейтинги пресета
     */
    public function index(GrowthPreset $preset): JsonResponse
    {
        $ratings = PresetRating::where('preset_id', $preset->id)
            ->orderBy('created_at', 'desc')
            ->get();

        $avgRating = $preset->average_rating;
        $totalRatings = $ratings->count();

        return response()->json([
            'average' => $avgRating,
            'total' => $totalRatings,
            'ratings' => $ratings,
        ]);
    }

    /**
     * Удалить рейтинг
     */
    public function destroy(GrowthPreset $preset, PresetRating $rating): JsonResponse
    {
        // TODO: Проверить что рейтинг принадлежит текущему пользователю
        
        $rating->delete();

        return response()->json([
            'message' => 'Rating deleted successfully',
        ]);
    }
}

