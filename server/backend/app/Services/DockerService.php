<?php

namespace App\Services;

use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Process;

class DockerService
{
    private string $dockerComposePath;
    private bool $enabled;

    public function __construct()
    {
        $this->dockerComposePath = base_path('../');
        $this->enabled = config('docker.enabled', true);
    }

    /**
     * Получить статус всех контейнеров
     */
    public function getStatus(): array
    {
        if (!$this->enabled) {
            return ['enabled' => false];
        }

        try {
            $result = Process::path($this->dockerComposePath)
                ->run('docker-compose ps --format json');

            if ($result->successful()) {
                $output = $result->output();
                $containers = [];
                
                // Парсинг JSON вывода
                foreach (explode("\n", trim($output)) as $line) {
                    if (!empty($line)) {
                        $container = json_decode($line, true);
                        if ($container) {
                            $containers[] = [
                                'name' => $container['Name'] ?? $container['Service'] ?? 'unknown',
                                'service' => $container['Service'] ?? 'unknown',
                                'state' => $container['State'] ?? 'unknown',
                                'status' => $container['Status'] ?? 'unknown',
                                'health' => $container['Health'] ?? 'N/A',
                            ];
                        }
                    }
                }

                return [
                    'enabled' => true,
                    'running' => true,
                    'containers' => $containers,
                ];
            }

            return [
                'enabled' => true,
                'running' => false,
                'error' => $result->errorOutput(),
            ];
        } catch (\Exception $e) {
            Log::error("Failed to get Docker status", ['error' => $e->getMessage()]);
            
            return [
                'enabled' => true,
                'running' => false,
                'error' => $e->getMessage(),
            ];
        }
    }

    /**
     * Перезапустить все контейнеры
     */
    public function restartAll(): array
    {
        if (!$this->enabled) {
            return ['success' => false, 'message' => 'Docker management disabled'];
        }

        Log::info("Restarting all Docker containers");

        try {
            $result = Process::path($this->dockerComposePath)
                ->timeout(120)
                ->run('docker-compose restart');

            if ($result->successful()) {
                Log::info("Docker containers restarted successfully");
                
                return [
                    'success' => true,
                    'message' => 'All containers restarted successfully',
                    'output' => $result->output(),
                ];
            }

            Log::error("Failed to restart containers", [
                'error' => $result->errorOutput()
            ]);

            return [
                'success' => false,
                'message' => 'Failed to restart containers',
                'error' => $result->errorOutput(),
            ];
        } catch (\Exception $e) {
            Log::error("Exception during container restart", [
                'error' => $e->getMessage()
            ]);

            return [
                'success' => false,
                'message' => 'Exception: ' . $e->getMessage(),
            ];
        }
    }

    /**
     * Перезапустить конкретный контейнер
     */
    public function restartContainer(string $containerName): array
    {
        if (!$this->enabled) {
            return ['success' => false, 'message' => 'Docker management disabled'];
        }

        Log::info("Restarting Docker container: {$containerName}");

        try {
            $result = Process::path($this->dockerComposePath)
                ->timeout(60)
                ->run("docker-compose restart {$containerName}");

            if ($result->successful()) {
                Log::info("Container restarted successfully", ['container' => $containerName]);
                
                return [
                    'success' => true,
                    'message' => "Container {$containerName} restarted successfully",
                    'output' => $result->output(),
                ];
            }

            return [
                'success' => false,
                'message' => "Failed to restart {$containerName}",
                'error' => $result->errorOutput(),
            ];
        } catch (\Exception $e) {
            Log::error("Exception during container restart", [
                'container' => $containerName,
                'error' => $e->getMessage()
            ]);

            return [
                'success' => false,
                'message' => 'Exception: ' . $e->getMessage(),
            ];
        }
    }

    /**
     * Остановить все контейнеры
     */
    public function stopAll(): array
    {
        if (!$this->enabled) {
            return ['success' => false, 'message' => 'Docker management disabled'];
        }

        Log::warning("Stopping all Docker containers");

        try {
            $result = Process::path($this->dockerComposePath)
                ->timeout(60)
                ->run('docker-compose stop');

            if ($result->successful()) {
                return [
                    'success' => true,
                    'message' => 'All containers stopped',
                ];
            }

            return [
                'success' => false,
                'message' => 'Failed to stop containers',
                'error' => $result->errorOutput(),
            ];
        } catch (\Exception $e) {
            return [
                'success' => false,
                'message' => 'Exception: ' . $e->getMessage(),
            ];
        }
    }

    /**
     * Запустить все контейнеры
     */
    public function startAll(): array
    {
        if (!$this->enabled) {
            return ['success' => false, 'message' => 'Docker management disabled'];
        }

        Log::info("Starting all Docker containers");

        try {
            $result = Process::path($this->dockerComposePath)
                ->timeout(120)
                ->run('docker-compose up -d');

            if ($result->successful()) {
                return [
                    'success' => true,
                    'message' => 'All containers started',
                ];
            }

            return [
                'success' => false,
                'message' => 'Failed to start containers',
                'error' => $result->errorOutput(),
            ];
        } catch (\Exception $e) {
            return [
                'success' => false,
                'message' => 'Exception: ' . $e->getMessage(),
            ];
        }
    }

    /**
     * Получить логи контейнера
     */
    public function getLogs(string $containerName, int $lines = 100): array
    {
        if (!$this->enabled) {
            return ['success' => false, 'message' => 'Docker management disabled'];
        }

        try {
            $result = Process::path($this->dockerComposePath)
                ->timeout(30)
                ->run("docker-compose logs --tail={$lines} {$containerName}");

            if ($result->successful()) {
                return [
                    'success' => true,
                    'logs' => $result->output(),
                ];
            }

            return [
                'success' => false,
                'error' => $result->errorOutput(),
            ];
        } catch (\Exception $e) {
            return [
                'success' => false,
                'error' => $e->getMessage(),
            ];
        }
    }
}

