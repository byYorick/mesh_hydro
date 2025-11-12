<?php

namespace App\Http\Controllers;

use App\Models\NewNode;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;
use Illuminate\Support\Str;

class SetupDiscoveryController extends Controller
{
    public function root(Request $request): JsonResponse
    {
        $data = $request->validate([
            'mac_address' => ['required', 'string', 'max:17'],
            'node_type' => ['nullable', 'string', 'max:32'],
            'chip_model' => ['nullable', 'string', 'max:64'],
            'firmware_version' => ['nullable', 'string', 'max:32'],
            'ip_address' => ['nullable', 'ip'],
            'temp_mesh_id' => ['nullable', 'string', 'max:32'],
        ]);

        $mac = strtoupper($data['mac_address']);
        $pin = $this->generatePinFromMac($mac);
        $tempMeshId = $data['temp_mesh_id'] ?? $this->generateMeshIdFromPin($pin);

        $metadata = [
            'chip_model' => $data['chip_model'] ?? null,
            'firmware_version' => $data['firmware_version'] ?? null,
            'ip_address' => $data['ip_address'] ?? $request->ip(),
        ];

        $newNode = NewNode::updateOrCreate(
            ['mac_address' => $mac],
            [
                'node_type' => $data['node_type'] ?? 'root',
                'is_root' => true,
                'pin' => $pin,
                'temp_mesh_id' => $tempMeshId,
                'status' => 'discovered',
                'metadata' => array_filter($metadata),
                'discovered_at' => now(),
                'last_heartbeat_at' => now(),
            ]
        );

        return response()->json([
            'success' => true,
            'mac_address' => $newNode->mac_address,
            'pin' => $newNode->pin,
            'temp_mesh_id' => $newNode->temp_mesh_id,
        ]);
    }

    protected function generatePinFromMac(string $mac): string
    {
        $hex = strtoupper(Str::of($mac)->replace(':', '')->substr(-6));
        return str_pad($hex, 6, '0', STR_PAD_LEFT);
    }

    protected function generateMeshIdFromPin(string $pin): string
    {
        return 'HYDRO_' . $pin;
    }
}
