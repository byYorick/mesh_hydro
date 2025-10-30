<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class UpdateNodeRequest extends FormRequest
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
        $nodeId = $this->route('nodeId');

        return [
            'node_id' => ['sometimes', 'string', 'max:50', Rule::unique('nodes', 'node_id')->ignore($nodeId, 'node_id'), 'regex:/^[a-z0-9_]+$/'],
            'node_type' => ['sometimes', 'string', 'in:ph_ec,ph,ec,climate,relay,water,display,root'],
            'zone' => ['nullable', 'string', 'max:100'],
            'mac_address' => ['nullable', 'string', 'regex:/^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/'],
            'config' => ['nullable', 'array'],
            'metadata' => ['nullable', 'array'],
            'online' => ['sometimes', 'boolean'],
        ];
    }

    /**
     * Get custom messages for validator errors.
     */
    public function messages(): array
    {
        return [
            'node_id.unique' => 'Узел с таким ID уже существует',
            'node_id.regex' => 'ID узла может содержать только строчные буквы, цифры и подчеркивания',
            'node_type.in' => 'Недопустимый тип узла',
            'mac_address.regex' => 'Неверный формат MAC адреса',
        ];
    }
}

