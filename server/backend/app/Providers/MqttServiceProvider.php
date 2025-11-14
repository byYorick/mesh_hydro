<?php

namespace App\Providers;

use App\Contracts\Messaging\MqttClientInterface;
use App\Infrastructure\Messaging\PhpMqttClient;
use App\Services\Mqtt\Enum\MqttMessageType;
use App\Services\Mqtt\MqttMessageRouter;
use Illuminate\Contracts\Foundation\Application;
use Illuminate\Support\ServiceProvider;
use PhpMqtt\Client\ConnectionSettings;
use PhpMqtt\Client\MqttClient;

class MqttServiceProvider extends ServiceProvider
{
    public function register(): void
    {
        $this->app->bind(MqttClientInterface::class, function (Application $app) {
            $config = $app['config']->get('mqtt');

            $settings = new ConnectionSettings();

            if (!empty($config['username'])) {
                $settings->setUsername($config['username']);
            }

            if (!empty($config['password'])) {
                $settings->setPassword($config['password']);
            }

            $settings
                ->setKeepAliveInterval($config['keep_alive'] ?? 60)
                ->setLastWillTopic($config['status_topic'] ?? 'hydro/server/status')
                ->setLastWillMessage('offline')
                ->setLastWillQualityOfService(1)
                ->setRetainLastWill(true);

            $client = new MqttClient(
                $config['host'] ?? 'localhost',
                $config['port'] ?? 1883,
                $config['client_id'] ?? ('hydro_backend_' . uniqid())
            );

            $logger = $config['log_channel'] ?? null
                ? $app['log']->channel($config['log_channel'])
                : $app['log']->driver();

            return new PhpMqttClient(
                $client,
                $logger,
                $settings
            );
        });

        $this->app->singleton(MqttMessageRouter::class, function (Application $app) {
            $handlerMap = collect($app['config']->get('mqtt.handlers', []))
                ->mapWithKeys(function (string $handler, string $type): array {
                    $enum = MqttMessageType::from($type);
                    return [$enum->value => $handler];
                })
                ->toArray();

            return new MqttMessageRouter($app, $handlerMap);
        });
    }
}

