<?php

namespace App\Services;

/**
 * ИНСТРУКЦИИ ПО ОБНОВЛЕНИЮ MqttService
 * 
 * Этот файл содержит обновленные методы для интеграции в существующий MqttService.php
 * 
 * ВАЖНО: Этот файл не исполняемый! Используйте код ниже для обновления MqttService.php
 */

/*
 * ============================================================================
 * МЕТОД 1: extractZoneFromTopic() - ЗАМЕНИТЬ ПОЛНОСТЬЮ
 * ============================================================================
 */
class MqttServiceUpdates_extractZoneFromTopic
{
    /**
     * Извлечь zone (mesh_id) из MQTT топика
     * 
     * Формат: hydro/{mesh_id}/{message_type}/{node_id}
     * Пример: hydro/zone_greenhouse_1/heartbeat/climate_001
     * 
     * @param string $topic MQTT топик
     * @return string|null mesh_id или null если топик невалидный
     */
    private function extractZoneFromTopic(string $topic): ?string
    {
        // Ожидаем формат: hydro/{mesh_id}/{message_type}/{node_id}
        $parts = explode('/', $topic);
        
        if (count($parts) < 3 || $parts[0] !== 'hydro') {
            Log::error("Topic does not match multizone format", [
                'topic' => $topic,
                'expected' => 'hydro/{mesh_id}/{message_type}/{node_id}'
            ]);
            return null;
        }
        
        $meshId = $parts[1];
        
        // Валидация mesh_id
        if (!$this->validateZone($meshId)) {
            Log::error("Invalid mesh_id in topic", [
                'topic' => $topic,
                'mesh_id' => $meshId,
                'validation' => 'failed: must be 3-31 chars, [a-zA-Z0-9_-]'
            ]);
            return null;
        }
        
        return $meshId;
    }
}

/*
 * ============================================================================
 * МЕТОД 2: validateZone() - НОВЫЙ МЕТОД, ДОБАВИТЬ
 * ============================================================================
 */
class MqttServiceUpdates_validateZone
{
    /**
     * Валидация zone строки
     * 
     * Правила:
     * - Длина: 3-31 символов
     * - Символы: [a-zA-Z0-9_-]
     * - Не может быть: "setup" (зарезервировано)
     * 
     * @param string $zone Zone строка для валидации
     * @return bool true если валидна
     */
    private function validateZone(string $zone): bool
    {
        // Проверка длины
        $length = strlen($zone);
        if ($length < 3 || $length > 31) {
            return false;
        }
        
        // Проверка зарезервированных значений
        if ($zone === 'setup') {
            return false;
        }
        
        // Проверка символов: только буквы, цифры, _, -
        if (!preg_match('/^[a-zA-Z0-9_-]+$/', $zone)) {
            return false;
        }
        
        return true;
    }
}

/*
 * ============================================================================
 * МЕТОД 3: handleHeartbeat() - ОБНОВИТЬ ОБРАБОТКУ ZONE
 * ============================================================================
 * 
 * ИЗМЕНЕНИЯ:
 * 1. Удалить fallback на "Auto-discovered"
 * 2. Отклонять сообщения без валидного zone
 * 3. Логировать ERROR при отсутствии zone
 * 4. Создавать критическое событие при отклонении
 */
class MqttServiceUpdates_handleHeartbeat
{
    public function handleHeartbeat(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            if (!$data) {
                Log::warning("Invalid JSON in heartbeat", ['topic' => $topic]);
                return;
            }

            // SETUP MODE: временная обработка для новых неконфигурированных узлов
            if (($data['type'] ?? null) === 'heartbeat' && isset($data['mac_address'])) {
                $handledBySetup = $this->processSetupHeartbeat($data, null);
                if ($handledBySetup) {
                    return;
                }
            }
            
            if (!isset($data['node_id'])) {
                Log::warning("Heartbeat без node_id", ['topic' => $topic]);
                return;
            }

            // ⭐ ОБЯЗАТЕЛЬНАЯ проверка zone
            $meshId = $this->extractZoneFromTopic($topic);
            if (!$meshId) {
                // Попытка взять из payload
                $meshId = $data['mesh_network_id'] ?? null;
                if (!$meshId || !$this->validateZone($meshId)) {
                    Log::error("Heartbeat without valid zone - REJECTED", [
                        'topic' => $topic,
                        'node_id' => $data['node_id'],
                        'payload_mesh_id' => $data['mesh_network_id'] ?? 'missing',
                        'action' => 'REJECTED'
                    ]);
                    
                    // ⭐ Создать критическое событие
                    \App\Models\Event::create([
                        'node_id' => $data['node_id'],
                        'level' => \App\Models\Event::LEVEL_CRITICAL,
                        'message' => "Node sent heartbeat without valid zone - REJECTED",
                        'data' => ['topic' => $topic, 'reason' => 'missing_zone']
                    ]);
                    
                    return; // ⭐ ОТКЛОНИТЬ сообщение
                }
            }

            // Проверка root_node_id в payload (опционально, но желательно)
            if (!isset($data['root_node_id']) || empty($data['root_node_id'])) {
                Log::warning("Heartbeat without root_node_id", [
                    'node_id' => $data['node_id'],
                    'zone' => $meshId
                ]);
            }

            // ⭐ Обновление узла с zone
            $updateData = [
                'online' => true,
                'last_seen_at' => now(),
                'zone' => $meshId, // ⭐ Обязательно
            ];

            if (isset($data['root_node_id'])) {
                $updateData['root_node_id'] = $data['root_node_id'];
            }

            $node = \App\Models\Node::updateOrCreate(
                ['node_id' => $data['node_id']],
                $updateData
            );

            // WebSocket broadcast если статус изменился
            if ($node->wasRecentlyCreated || $node->wasChanged('online')) {
                broadcast(new \App\Events\NodeStatusChanged([
                    'node_id' => $node->node_id,
                    'online' => true,
                    'zone' => $meshId,
                ]));
            }
            
            Log::debug("Heartbeat processed", [
                'node_id' => $data['node_id'],
                'zone' => $meshId,
                'online' => true
            ]);
            
        } catch (\Exception $e) {
            Log::error("Heartbeat handling error", [
                'topic' => $topic,
                'error' => $e->getMessage(),
                'trace' => $e->getTraceAsString()
            ]);
        }
    }
}

/*
 * ============================================================================
 * МЕТОД 4: handleTelemetry() - АНАЛОГИЧНЫЕ ИЗМЕНЕНИЯ
 * ============================================================================
 */
class MqttServiceUpdates_handleTelemetry
{
    public function handleTelemetry(string $topic, string $payload): void
    {
        try {
            $data = json_decode($payload, true);
            if (!$data || !isset($data['node_id'])) {
                return;
            }

            // ⭐ ОБЯЗАТЕЛЬНАЯ проверка zone
            $meshId = $this->extractZoneFromTopic($topic);
            if (!$meshId) {
                $meshId = $data['mesh_network_id'] ?? null;
                if (!$meshId || !$this->validateZone($meshId)) {
                    Log::error("Telemetry without valid zone - REJECTED", [
                        'topic' => $topic,
                        'node_id' => $data['node_id']
                    ]);
                    return;
                }
            }

            // Обновить узел с zone
            \App\Models\Node::updateOrCreate(
                ['node_id' => $data['node_id']],
                [
                    'zone' => $meshId,
                    'last_seen_at' => now(),
                    'online' => true,
                    'root_node_id' => $data['root_node_id'] ?? null,
                ]
            );

            // Сохранить телеметрию
            \App\Models\Telemetry::create([
                'node_id' => $data['node_id'],
                'zone' => $meshId,
                'data' => $data['data'] ?? [],
                'received_at' => now(),
            ]);

            // WebSocket broadcast
            broadcast(new \App\Events\TelemetryReceived([
                'node_id' => $data['node_id'],
                'zone' => $meshId,
                'data' => $data['data'] ?? []
            ]));

        } catch (\Exception $e) {
            Log::error("Telemetry handling error", [
                'topic' => $topic,
                'error' => $e->getMessage()
            ]);
        }
    }
}

/*
 * ============================================================================
 * МЕТОД 5: sendCommand() - ОБНОВИТЬ ДЛЯ НОВЫХ ТОПИКОВ
 * ============================================================================
 */
class MqttServiceUpdates_sendCommand
{
    /**
     * Отправить команду узлу
     * 
     * @param string $nodeId ID узла
     * @param string $command Команда
     * @param array $params Параметры команды
     * @param int|null $commandId ID команды для отслеживания
     * @return void
     * @throws \Exception если узел не найден или zone невалиден
     */
    public function sendCommand(string $nodeId, string $command, array $params = [], ?int $commandId = null): void
    {
        // Получить узел из БД
        $node = \App\Models\Node::where('node_id', $nodeId)->first();
        if (!$node) {
            throw new \Exception("Node not found: {$nodeId}");
        }
        
        $meshId = $node->zone;
        if (!$meshId || !$this->validateZone($meshId)) {
            throw new \Exception("Node {$nodeId} has invalid zone: {$meshId}");
        }
        
        // Формируем payload
        $payload = json_encode([
            'type' => 'command',
            'command_id' => $commandId,
            'node_id' => $nodeId,
            'command' => $command,
            'params' => $params,
            'timestamp' => time(),
        ]);

        // ⭐ Новый формат топика: hydro/{mesh_id}/command/{node_id}
        $topic = "hydro/{$meshId}/command/{$nodeId}";
        $this->publish($topic, $payload, 1);
        
        Log::info("Command sent", [
            'node_id' => $nodeId,
            'zone' => $meshId,
            'command' => $command,
            'command_id' => $commandId,
            'topic' => $topic
        ]);
    }
}

/*
 * ============================================================================
 * МЕТОД 6: sendConfig() - АНАЛОГИЧНО sendCommand()
 * ============================================================================
 */
class MqttServiceUpdates_sendConfig
{
    public function sendConfig(string $nodeId, array $config): void
    {
        $node = \App\Models\Node::where('node_id', $nodeId)->first();
        if (!$node) {
            throw new \Exception("Node not found: {$nodeId}");
        }
        
        $meshId = $node->zone;
        if (!$meshId || !$this->validateZone($meshId)) {
            throw new \Exception("Node {$nodeId} has invalid zone: {$meshId}");
        }
        
        $payload = json_encode([
            'type' => 'config',
            'node_id' => $nodeId,
            'config' => $config,
            'timestamp' => time(),
        ]);

        // ⭐ Новый формат: hydro/{mesh_id}/config/{node_id}
        $topic = "hydro/{$meshId}/config/{$nodeId}";
        $this->publish($topic, $payload, 1);
        
        Log::info("Config sent", [
            'node_id' => $nodeId,
            'zone' => $meshId,
            'topic' => $topic
        ]);
    }
}

/*
 * ============================================================================
 * ИТОГОВЫЙ ЧЕКЛИСТ ОБНОВЛЕНИЙ MqttService
 * ============================================================================
 * 
 * [ ] 1. Обновить extractZoneFromTopic() - строгий парсинг, без fallback
 * [ ] 2. Добавить validateZone() - валидация по regex
 * [ ] 3. Обновить handleHeartbeat() - отклонение без zone, логирование ERROR
 * [ ] 4. Обновить handleTelemetry() - аналогично heartbeat
 * [ ] 5. Обновить handleEvent() - аналогично
 * [ ] 6. Обновить handleDiscovery() - аналогично
 * [ ] 7. Обновить handleConfigResponse() - аналогично
 * [ ] 8. Обновить sendCommand() - новый формат топика
 * [ ] 9. Обновить sendConfig() - новый формат топика
 * [ ] 10. Удалить все fallback на "Auto-discovered"
 * [ ] 11. Обновить subscribe() в MQTT Listener - подписка на hydro/+/heartbeat/#
 * [ ] 12. Добавить логирование rejected messages для мониторинга
 * [ ] 13. Создать unit тесты для новой логики
 * 
 * ============================================================================
 * ВАЖНО: SETUP MODE
 * ============================================================================
 * 
 * Для новых неконфигурированных узлов временно сохранить поддержку:
 * - Топик: hydro/setup/discovery
 * - Топик: hydro/setup/heartbeat/{mac_address}
 * 
 * После конфигурации узел переходит на стандартные топики с zone.
 * 
 * ============================================================================
 */


