<?php

namespace App\Http\Controllers;

use App\Models\NewNode;
use Illuminate\Http\Request;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\Log;

class NewNodeController extends Controller
{
    public function index(): JsonResponse
    {
        $newNodes = NewNode::orderByDesc('is_root')
            ->orderByDesc('discovered_at')
            ->get()
            ->map(function (NewNode $node) {
                $node->is_online = $node->isOnline();
                return $node;
            });

        return response()->json($newNodes);
    }

    public function show(string $mac): JsonResponse
    {
        $node = NewNode::where('mac_address', $mac)->firstOrFail();
        $node->is_online = $node->isOnline();

        return response()->json($node);
    }

    public function configure(Request $request, string $mac): JsonResponse
    {
        $node = NewNode::where('mac_address', $mac)->firstOrFail();

        $rules = [
            'node_id' => ['required', 'string', 'max:32'],
            'mesh_id' => ['required', 'string', 'max:32'],
            'zone' => ['required', 'string', 'max:64'],
        ];

        if ($node->is_root) {
            $rules['mqtt_broker_host'] = ['required', 'string'];
            $rules['mqtt_broker_port'] = ['required', 'integer'];
            $rules['wifi_ssid'] = ['required', 'string'];
            $rules['wifi_password'] = ['required', 'string'];
        }

        $config = $request->validate($rules);

        $config['is_root'] = $node->is_root;
        $config['mac_address'] = $node->mac_address;

        $metadata = $node->metadata ?? [];
        $metadata['requested_zone'] = $config['zone'];
        $metadata['requested_mesh_id'] = $config['mesh_id'];
        if ($node->is_root) {
            $metadata['wifi_ssid'] = $config['wifi_ssid'];
            $metadata['mqtt_host'] = $config['mqtt_broker_host'];
            $metadata['mqtt_port'] = $config['mqtt_broker_port'];
        }
        $node->metadata = $metadata;
        $node->save();

        $success = $node->configure($config);

        if (!$success) {
            return response()->json([
                'success' => false,
                'message' => 'Не удалось отправить конфигурацию. Проверьте, что устройство онлайн.',
            ], 409);
        }

        Log::info('New node configuration sent', [
            'mac_address' => $mac,
            'node_id' => $config['node_id'],
        ]);

        return response()->json([
            'success' => true,
            'message' => 'Конфигурация отправлена на устройство',
        ]);
    }

    public function destroy(string $mac): JsonResponse
    {
        $node = NewNode::where('mac_address', $mac)->firstOrFail();
        $node->delete();

        return response()->json([
            'success' => true,
            'message' => 'Новый узел удалён',
        ]);
    }
}
