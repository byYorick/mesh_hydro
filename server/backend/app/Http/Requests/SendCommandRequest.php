<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

class SendCommandRequest extends FormRequest
{
    /**
     * Determine if the user is authorized to make this request.
     */
    public function authorize(): bool
    {
        return true; // TODO: Add proper authorization
    }

    /**
     * Get the validation rules that apply to the request.
     */
    public function rules(): array
    {
        return [
            'command' => ['required', 'string', 'max:100'],
            'params' => ['nullable', 'array'],
            'params.*' => ['nullable'], // Параметры могут быть любого типа
        ];
    }

    /**
     * Get custom messages for validator errors.
     */
    public function messages(): array
    {
        return [
            'command.required' => 'Команда обязательна',
            'command.string' => 'Команда должна быть строкой',
            'params.array' => 'Параметры должны быть массивом',
        ];
    }
}

