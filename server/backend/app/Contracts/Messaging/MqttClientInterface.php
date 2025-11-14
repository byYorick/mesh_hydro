<?php

namespace App\Contracts\Messaging;

/**
 * High-level abstraction over an MQTT client implementation.
 */
interface MqttClientInterface
{
    /**
     * Establish connection to broker (idempotent).
     */
    public function connect(): void;

    /**
     * Gracefully disconnect from broker.
     */
    public function disconnect(): void;

    /**
     * Publish payload into topic.
     *
     * @param string $topic
     * @param string $payload
     * @param int $qos
     * @param bool $retain
     */
    public function publish(string $topic, string $payload, int $qos = 0, bool $retain = false): void;

    /**
     * Subscribe to topic with callback.
     *
     * @param string $topic
     * @param callable $callback function (string $topic, string $payload): void
     * @param int $qos
     */
    public function subscribe(string $topic, callable $callback, int $qos = 0): void;

    /**
     * Unsubscribe from topic.
     *
     * @param string $topic
     */
    public function unsubscribe(string $topic): void;

    /**
     * Process MQTT traffic loop.
     *
     * @param bool $blocking
     */
    public function loop(bool $blocking = true): void;
}

