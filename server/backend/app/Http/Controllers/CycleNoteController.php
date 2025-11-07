<?php

namespace App\Http\Controllers;

use App\Models\GrowthCycle;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;

/**
 * ⭐ GROWTH PLANNER: Контроллер заметок для циклов
 */
class CycleNoteController extends Controller
{
    /**
     * Добавить заметку к циклу
     */
    public function addNote(Request $request, GrowthCycle $cycle): JsonResponse
    {
        $validated = $request->validate([
            'note' => 'required|string|max:5000',
            'stage_id' => 'nullable|exists:growth_stages,id',
            'is_important' => 'nullable|boolean',
        ]);

        // Добавляем заметку к существующим notes
        $timestamp = now()->format('Y-m-d H:i:s');
        $stageName = $validated['stage_id'] 
            ? \App\Models\GrowthStage::find($validated['stage_id'])->name 
            : 'Общая заметка';
        
        $important = ($validated['is_important'] ?? false) ? '⭐ ' : '';
        $newNote = "{$important}[{$timestamp}] [{$stageName}] {$validated['note']}";
        
        $currentNotes = $cycle->notes ?? '';
        $updatedNotes = $currentNotes . "\n\n" . $newNote;
        
        $cycle->update(['notes' => trim($updatedNotes)]);

        return response()->json([
            'message' => 'Note added successfully',
            'cycle' => $cycle->fresh(),
        ]);
    }

    /**
     * Получить заметки цикла
     */
    public function getNotes(GrowthCycle $cycle): JsonResponse
    {
        $notes = $cycle->notes ?? '';
        
        // Парсим заметки в структурированный формат
        $parsedNotes = [];
        $lines = explode("\n\n", $notes);
        
        foreach ($lines as $line) {
            if (empty(trim($line))) continue;
            
            // Парсим формат: [timestamp] [stage] note
            if (preg_match('/^(⭐ )?\[([^\]]+)\] \[([^\]]+)\] (.+)$/s', $line, $matches)) {
                $parsedNotes[] = [
                    'is_important' => !empty($matches[1]),
                    'timestamp' => $matches[2],
                    'stage' => $matches[3],
                    'text' => $matches[4],
                ];
            } else {
                // Старый формат или простой текст
                $parsedNotes[] = [
                    'is_important' => false,
                    'timestamp' => null,
                    'stage' => null,
                    'text' => $line,
                ];
            }
        }

        return response()->json([
            'raw_notes' => $notes,
            'parsed_notes' => $parsedNotes,
            'total' => count($parsedNotes),
        ]);
    }
}

