<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Support\Facades\Cache;

class Setting extends Model
{
    protected $fillable = [
        'key',
        'value',
        'type',
        'description',
        'group',
        'is_sensitive',
    ];

    protected $casts = [
        'is_sensitive' => 'boolean',
    ];

    /**
     * Получить значение настройки
     */
    public static function get(string $key, $default = null)
    {
        $cacheKey = "setting:{$key}";

        return Cache::remember($cacheKey, 3600, function () use ($key, $default) {
            $setting = self::where('key', $key)->first();

            if (!$setting) {
                return $default;
            }

            return self::castValue($setting->value, $setting->type);
        });
    }

    /**
     * Установить значение настройки
     */
    public static function set(string $key, $value): bool
    {
        $setting = self::where('key', $key)->first();

        if (!$setting) {
            $setting = new self(['key' => $key]);
        }

        $setting->value = is_array($value) ? json_encode($value) : (string)$value;
        $setting->save();

        // Очистить кэш
        Cache::forget("setting:{$key}");

        return true;
    }

    /**
     * Получить все настройки группы
     */
    public static function getGroup(string $group): array
    {
        $settings = self::where('group', $group)->get();
        $result = [];

        foreach ($settings as $setting) {
            $key = str_replace($group . '.', '', $setting->key);
            $result[$key] = [
                'value' => self::castValue($setting->value, $setting->type),
                'type' => $setting->type,
                'description' => $setting->description,
                'is_sensitive' => $setting->is_sensitive,
            ];
        }

        return $result;
    }

    /**
     * Приведение типа значения
     */
    private static function castValue($value, string $type)
    {
        return match($type) {
            'boolean' => filter_var($value, FILTER_VALIDATE_BOOLEAN),
            'integer' => (int)$value,
            'float' => (float)$value,
            'json' => json_decode($value, true),
            default => $value,
        };
    }

    /**
     * Очистить весь кэш настроек
     */
    public static function clearCache(): void
    {
        $settings = self::all();
        foreach ($settings as $setting) {
            Cache::forget("setting:{$setting->key}");
        }
    }
}

