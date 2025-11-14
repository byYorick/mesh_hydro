<?php

namespace App\Providers;

use Illuminate\Support\ServiceProvider;

class AppServiceProvider extends ServiceProvider
{
    /**
     * Register any application services.
     */
    public function register(): void
    {
        $this->app->singleton(
            \App\Services\Nodes\Contracts\NodeZoneResolverInterface::class,
            \App\Services\Nodes\ZoneResolver::class,
        );

        $this->app->singleton(
            \App\Services\Nodes\Domain\Contracts\NodeStatusServiceInterface::class,
            \App\Services\Nodes\Domain\NodeStatusService::class,
        );
    }

    /**
     * Bootstrap any application services.
     */
    public function boot(): void
    {
        //
    }
}

