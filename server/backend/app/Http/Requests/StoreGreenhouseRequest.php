<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

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


