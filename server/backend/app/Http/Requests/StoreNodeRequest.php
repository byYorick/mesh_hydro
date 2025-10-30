<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

class StoreNodeRequest extends FormRequest
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
            'node_id' => ['required', 'string', 'max:50', 'unique:nodes,node_id', 'regex:/^[a-z0-9_]+$/'],
            'node_type' => ['required', 'string', 'in:ph_ec,ph,ec,climate,relay,water,display,root'],
            'zone' => ['nullable', 'string', 'max:100'],
            'mac_address' => ['nullable', 'string', 'regex:/^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/'],
            'config' => ['nullable', 'array'],
            'metadata' => ['nullable', 'array'],
        ];
    }

    /**
     * Get custom messages for validator errors.
     */
    public function messages(): array
    {
        return [
            'node_id.required' => 'ID узла обязателен',
            'node_id.unique' => 'Узел с таким ID уже существует',
            'node_id.regex' => 'ID узла может содержать только строчные буквы, цифры и подчеркивания',
            'node_type.required' => 'Тип узла обязателен',
            'node_type.in' => 'Недопустимый тип узла',
            'mac_address.regex' => 'Неверный формат MAC адреса',
        ];
    }
}

