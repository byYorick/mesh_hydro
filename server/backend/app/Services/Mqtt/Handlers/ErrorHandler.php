<?php

namespace App\Services\Mqtt\Handlers;

use App\Services\Mqtt\Contracts\MqttMessageHandlerInterface;
use App\Services\Mqtt\DTO\MqttMessageContext;
use App\Services\Mqtt\NodeWorkflowService;

class ErrorHandler implements MqttMessageHandlerInterface
{
    public function __construct(
        private readonly NodeWorkflowService $workflow
    ) {
    }

    public function handle(MqttMessageContext $context): void
    {
        $this->workflow->handleError($context);
    }
}

