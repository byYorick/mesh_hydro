<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class UpdateGreenhouseRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        $greenhouseId = $this->route('greenhouse')?->id ?? $this->route('id');

        return [
            'name' => ['sometimes', 'string', 'max:150'],
            'code' => [
                'sometimes',
                'string',
                'max:100',
                Rule::unique('greenhouses', 'code')->ignore($greenhouseId),
            ],
            'location' => ['nullable', 'string', 'max:150'],
            'description' => ['nullable', 'string', 'max:1024'],
            'timezone' => ['nullable', 'string', 'max:64'],
            'status' => ['nullable', 'string', 'in:active,draft,maintenance,offline'],
            'mesh_group' => ['nullable', 'string', 'max:100'],
            'root_node_id' => ['nullable', 'string', 'max:50', 'exists:nodes,node_id'],
            'root_node_mac' => ['nullable', 'string', 'max:17'],
            'image_url' => ['nullable', 'string', 'max:255'],
            'tags' => ['nullable', 'array'],
            'tags.*' => ['string', 'max:64'],
            'settings' => ['nullable', 'array'],
        ];
    }
}


