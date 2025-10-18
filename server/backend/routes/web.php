<?php

use Illuminate\Support\Facades\Route;

/*
|--------------------------------------------------------------------------
| Web Routes
|--------------------------------------------------------------------------
|
| Веб маршруты (если нужны)
| Frontend на Vue.js работает отдельно на порту 3000
|
*/

Route::get('/', function () {
    return response()->json([
        'name' => 'Mesh Hydro System API',
        'version' => '1.0.0',
        'documentation' => '/api/documentation',
    ]);
});

