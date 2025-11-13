<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Support\Arr;
use Illuminate\Support\Str;

class StoreGreenhouseRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        return [
            'name' => ['required', 'string', 'max:150'],
            'code' => ['required', 'string', 'max:100', 'unique:greenhouses,code'],
            'description' => ['required', 'string', 'max:1024'],
            'status' => ['nullable', 'string', 'in:active,draft,maintenance,offline'],
            'root_node_id' => ['nullable', 'string', 'max:50', 'exists:nodes,node_id'],
            'root_node_mac' => ['nullable', 'string', 'max:17'],
            'image_url' => ['nullable', 'string', 'max:255'],
            'tags' => ['nullable', 'array'],
            'tags.*' => ['string', 'max:64'],
            'settings' => ['nullable', 'array'],
            'climate_profiles' => ['nullable', 'array', 'max:20'],
            'climate_profiles.*.name' => ['required', 'string', 'max:100'],
            'climate_profiles.*.notes' => ['nullable', 'string', 'max:255'],
            'climate_profiles.*.day' => ['nullable', 'array'],
            'climate_profiles.*.night' => ['nullable', 'array'],
            'climate_profiles.*.day.temperature' => ['nullable', 'numeric'],
            'climate_profiles.*.day.humidity' => ['nullable', 'numeric'],
            'climate_profiles.*.day.co2' => ['nullable', 'numeric'],
            'climate_profiles.*.day.light' => ['nullable', 'numeric'],
            'climate_profiles.*.day.vpd' => ['nullable', 'numeric'],
            'climate_profiles.*.night.temperature' => ['nullable', 'numeric'],
            'climate_profiles.*.night.humidity' => ['nullable', 'numeric'],
            'climate_profiles.*.night.co2' => ['nullable', 'numeric'],
            'climate_profiles.*.night.light' => ['nullable', 'numeric'],
            'climate_profiles.*.night.vpd' => ['nullable', 'numeric'],
            'climate_profiles.*.settings' => ['nullable', 'array'],
        ];
    }

    /**
     * Нормализуем входные данные перед валидацией.
     */
    protected function prepareForValidation(): void
    {
        $input = $this->all();

        foreach (['name', 'code', 'description', 'root_node_id', 'root_node_mac'] as $field) {
            if (array_key_exists($field, $input) && is_string($input[$field])) {
                $value = trim($input[$field]);
                $input[$field] = $value === '' ? null : $value;
            }
        }

        if (array_key_exists('tags', $input)) {
            $tags = $input['tags'];

            if (is_string($tags)) {
                $tags = preg_split('/[,;]+/', $tags) ?: [];
            }

            if (is_array($tags)) {
                $tags = array_values(array_unique(array_filter(array_map(
                    static fn ($tag) => Str::limit(trim((string) $tag), 64, ''),
                    $tags
                ))));
            } else {
                $tags = [];
            }

            $input['tags'] = $tags ?: null;
        }

        $input['settings'] = $this->normalizeSettings(Arr::get($input, 'settings'));
        $input['climate_profiles'] = $this->normalizeClimateProfiles(Arr::get($input, 'climate_profiles'));

        $this->replace($input);
    }

    private function normalizeSettings($settings): array
    {
        if (is_string($settings)) {
            $decoded = json_decode($settings, true);
            if (json_last_error() === JSON_ERROR_NONE) {
                $settings = $decoded;
            }
        }

        return is_array($settings) ? $settings : [];
    }

    private function normalizeClimateProfiles($profiles): array
    {
        if (is_string($profiles)) {
            $decoded = json_decode($profiles, true);
            if (json_last_error() === JSON_ERROR_NONE) {
                $profiles = $decoded;
            }
        }

        if (!is_array($profiles)) {
            return [];
        }

        $normalized = [];

        foreach ($profiles as $profile) {
            if (!is_array($profile)) {
                continue;
            }

            $name = isset($profile['name']) ? trim((string) $profile['name']) : '';
            $notes = isset($profile['notes']) ? trim((string) $profile['notes']) : null;

            $day = $this->normalizeSetpoints($profile['day'] ?? []);
            $night = $this->normalizeSetpoints($profile['night'] ?? []);
            $settings = $profile['settings'] ?? [];
            $settings = is_array($settings) ? $settings : [];

            $normalized[] = array_filter([
                'name' => $name,
                'notes' => $notes ?: null,
                'day' => $day ?: null,
                'night' => $night ?: null,
                'settings' => !empty($settings) ? $settings : null,
            ], static fn ($value) => $value !== null);
        }

        return $normalized;
    }

    private function normalizeSetpoints($data): array
    {
        if (!is_array($data)) {
            return [];
        }

        $allowedKeys = ['temperature', 'humidity', 'co2', 'light', 'vpd'];
        $normalized = [];

        foreach ($allowedKeys as $key) {
            if (!array_key_exists($key, $data)) {
                continue;
            }

            $value = $data[$key];
            if ($value === '' || $value === null) {
                continue;
            }

            $normalized[$key] = is_numeric($value) ? (float) $value : $value;
        }

        return $normalized;
    }
}


