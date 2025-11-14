<?php

namespace App\Services;

use App\Models\NewNode;
use App\Services\Mqtt\CommandPublisher;
use Illuminate\Http\Client\Factory as HttpFactory;
use Illuminate\Support\Facades\Log;

class NewNodeConfigurator
{
    public function __construct(
        private readonly CommandPublisher $commandPublisher,
        private readonly HttpFactory $httpClient,
    ) {
    }

    public function configure(NewNode $node, array $config): bool
    {
        if ($node->status === 'confirmed') {
            return false;
        }

        if (!$node->isOnline()) {
            Log::warning('Attempt to configure offline new node', [
                'mac_address' => $node->mac_address,
            ]);
            return false;
        }

        $config['pin'] = $node->pin;

        $isConfigured = $node->is_root
            ? $this->sendConfigViaHttp($node, $config)
            : $this->sendConfigViaMqtt($node, $config);

        if ($isConfigured) {
            $node->status = 'configuring';
            $node->save();
        }

        return $isConfigured;
    }

    private function sendConfigViaHttp(NewNode $node, array $config): bool
    {
        $ip = $node->metadata['ip_address'] ?? null;
        if (!$ip) {
            Log::error('Cannot send config via HTTP without IP address', [
                'mac_address' => $node->mac_address,
            ]);
            return false;
        }

        try {
            $response = $this->httpClient->timeout(5)->post("http://{$ip}/api/config", $config);

            if ($response->successful()) {
                return true;
            }

            Log::error('HTTP config request failed', [
                'mac_address' => $node->mac_address,
                'status' => $response->status(),
                'body' => $response->body(),
            ]);

            return false;
        } catch (\Throwable $e) {
            Log::error('HTTP config request error', [
                'mac_address' => $node->mac_address,
                'error' => $e->getMessage(),
            ]);

            return false;
        }
    }

    private function sendConfigViaMqtt(NewNode $node, array $config): bool
    {
        try {
            $nodeId = $config['node_id'] ?? $node->mac_address;

            $this->commandPublisher->sendCommand(
                $nodeId,
                'write_config',
                $config
            );

            return true;
        } catch (\Throwable $e) {
            Log::error('MQTT config send failed', [
                'mac_address' => $node->mac_address,
                'error' => $e->getMessage(),
            ]);

            return false;
        }
    }
}

