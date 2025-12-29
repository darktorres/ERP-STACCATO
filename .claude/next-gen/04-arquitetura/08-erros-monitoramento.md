# Tratamento de Erros e Monitoramento

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define a estratégia de tratamento de erros, logging e monitoramento para o ERP Staccato em Laravel.

### Objetivos

| Objetivo | Meta |
|----------|------|
| Uptime | 99.9% |
| MTTR (Mean Time to Recovery) | < 15 minutos |
| Alertas críticos | < 5 minutos |
| Retenção de logs | 90 dias |

---

## Hierarquia de Exceções

### Estrutura de Classes

```text
Exception
├── BusinessException (422)
│   ├── ValidationException
│   ├── InsufficientStockException
│   ├── InsufficientCreditException
│   ├── InvalidStatusTransitionException
│   ├── AuthorizationRequiredException
│   └── BusinessRuleViolationException
├── IntegrationException (503)
│   ├── AcbrException
│   ├── SefazException
│   ├── CnabException
│   └── ExternalApiException
├── InfrastructureException (500)
│   ├── DatabaseException
│   ├── CacheException
│   └── QueueException
└── SecurityException (403)
    ├── UnauthorizedException
    ├── ForbiddenException
    └── RateLimitException
```

### Implementação

```php
// app/Exceptions/BusinessException.php
namespace App\Exceptions;

use Exception;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;

abstract class BusinessException extends Exception
{
    protected string $errorCode;
    protected array $context = [];
    protected int $httpStatus = 422;

    public function __construct(
        string $message,
        string $errorCode = 'BUSINESS_ERROR',
        array $context = [],
        ?Exception $previous = null
    ) {
        parent::__construct($message, 0, $previous);
        $this->errorCode = $errorCode;
        $this->context = $context;
    }

    public function getErrorCode(): string
    {
        return $this->errorCode;
    }

    public function getContext(): array
    {
        return $this->context;
    }

    public function render(Request $request): JsonResponse
    {
        return response()->json([
            'error' => [
                'code' => $this->errorCode,
                'message' => $this->getMessage(),
                'context' => $this->context,
            ],
            'meta' => [
                'request_id' => $request->header('X-Request-ID', (string) \Str::uuid()),
                'timestamp' => now()->toIso8601String(),
            ],
        ], $this->httpStatus);
    }

    public function report(): bool
    {
        // Log apenas se for relevante (não logar validações esperadas)
        return false;
    }
}
```

### Exceções Específicas

```php
// app/Exceptions/InsufficientStockException.php
namespace App\Exceptions;

class InsufficientStockException extends BusinessException
{
    public function __construct(
        int $produtoId,
        int $solicitado,
        int $disponivel
    ) {
        parent::__construct(
            message: "Estoque insuficiente para o produto #{$produtoId}",
            errorCode: 'INSUFFICIENT_STOCK',
            context: [
                'produto_id' => $produtoId,
                'solicitado' => $solicitado,
                'disponivel' => $disponivel,
            ]
        );
    }
}

// app/Exceptions/InvalidStatusTransitionException.php
namespace App\Exceptions;

class InvalidStatusTransitionException extends BusinessException
{
    public function __construct(
        string $entity,
        string $currentStatus,
        string $targetStatus
    ) {
        parent::__construct(
            message: "Transição inválida de '{$currentStatus}' para '{$targetStatus}'",
            errorCode: 'INVALID_STATUS_TRANSITION',
            context: [
                'entity' => $entity,
                'current_status' => $currentStatus,
                'target_status' => $targetStatus,
            ]
        );
    }
}

// app/Exceptions/SefazException.php
namespace App\Exceptions;

class SefazException extends IntegrationException
{
    protected int $httpStatus = 503;

    public function __construct(
        int $cstat,
        string $motivo,
        ?string $chave = null
    ) {
        parent::__construct(
            message: "SEFAZ: {$motivo}",
            errorCode: 'NFE_SEFAZ_ERROR',
            context: [
                'cstat' => $cstat,
                'motivo' => $motivo,
                'chave' => $chave,
            ]
        );
    }

    public function report(): bool
    {
        // Sempre logar erros de SEFAZ
        return true;
    }
}
```

---

## Exception Handler

### Configuração

```php
// app/Exceptions/Handler.php
namespace App\Exceptions;

use Illuminate\Foundation\Exceptions\Handler as ExceptionHandler;
use Illuminate\Http\JsonResponse;
use Illuminate\Http\Request;
use Illuminate\Validation\ValidationException;
use Illuminate\Auth\AuthenticationException;
use Illuminate\Auth\Access\AuthorizationException;
use Illuminate\Database\Eloquent\ModelNotFoundException;
use Symfony\Component\HttpKernel\Exception\HttpException;
use Throwable;

class Handler extends ExceptionHandler
{
    protected $dontFlash = [
        'current_password',
        'password',
        'password_confirmation',
    ];

    protected $dontReport = [
        ValidationException::class,
        AuthenticationException::class,
        AuthorizationException::class,
        ModelNotFoundException::class,
        BusinessException::class,
    ];

    public function register(): void
    {
        $this->reportable(function (Throwable $e) {
            if (app()->bound('sentry')) {
                app('sentry')->captureException($e);
            }
        });

        $this->renderable(function (Throwable $e, Request $request) {
            if ($request->expectsJson() || $request->is('api/*')) {
                return $this->renderJsonException($request, $e);
            }
        });
    }

    protected function renderJsonException(Request $request, Throwable $e): JsonResponse
    {
        $requestId = $request->header('X-Request-ID', (string) \Str::uuid());

        $response = [
            'error' => [
                'code' => $this->getErrorCode($e),
                'message' => $this->getErrorMessage($e),
            ],
            'meta' => [
                'request_id' => $requestId,
                'timestamp' => now()->toIso8601String(),
            ],
        ];

        // Adicionar detalhes de validação
        if ($e instanceof ValidationException) {
            $response['error']['details'] = collect($e->errors())
                ->map(fn($messages, $field) => [
                    'field' => $field,
                    'messages' => $messages,
                ])
                ->values()
                ->all();
        }

        // Adicionar contexto de exceções de negócio
        if ($e instanceof BusinessException) {
            $response['error']['context'] = $e->getContext();
        }

        // Debug info em ambiente de desenvolvimento
        if (config('app.debug')) {
            $response['debug'] = [
                'exception' => get_class($e),
                'file' => $e->getFile(),
                'line' => $e->getLine(),
                'trace' => collect($e->getTrace())
                    ->take(10)
                    ->map(fn($t) => [
                        'file' => $t['file'] ?? null,
                        'line' => $t['line'] ?? null,
                        'function' => $t['function'] ?? null,
                    ])
                    ->all(),
            ];
        }

        return response()->json($response, $this->getHttpStatus($e))
            ->header('X-Request-ID', $requestId);
    }

    protected function getErrorCode(Throwable $e): string
    {
        return match (true) {
            $e instanceof BusinessException => $e->getErrorCode(),
            $e instanceof ValidationException => 'VALIDATION_ERROR',
            $e instanceof AuthenticationException => 'UNAUTHENTICATED',
            $e instanceof AuthorizationException => 'FORBIDDEN',
            $e instanceof ModelNotFoundException => 'RESOURCE_NOT_FOUND',
            $e instanceof HttpException => 'HTTP_ERROR',
            default => 'INTERNAL_ERROR',
        };
    }

    protected function getErrorMessage(Throwable $e): string
    {
        return match (true) {
            $e instanceof ValidationException => 'Os dados fornecidos são inválidos',
            $e instanceof AuthenticationException => 'Autenticação necessária',
            $e instanceof AuthorizationException => 'Acesso negado',
            $e instanceof ModelNotFoundException => 'Recurso não encontrado',
            config('app.debug') => $e->getMessage(),
            default => 'Ocorreu um erro interno',
        };
    }

    protected function getHttpStatus(Throwable $e): int
    {
        return match (true) {
            $e instanceof BusinessException => $e->httpStatus ?? 422,
            $e instanceof ValidationException => 422,
            $e instanceof AuthenticationException => 401,
            $e instanceof AuthorizationException => 403,
            $e instanceof ModelNotFoundException => 404,
            $e instanceof HttpException => $e->getStatusCode(),
            default => 500,
        };
    }
}
```

---

## Logging

### Configuração de Logging

```php
// config/logging.php
return [
    'default' => env('LOG_CHANNEL', 'stack'),

    'channels' => [
        'stack' => [
            'driver' => 'stack',
            'channels' => ['daily', 'sentry'],
            'ignore_exceptions' => false,
        ],

        'daily' => [
            'driver' => 'daily',
            'path' => storage_path('logs/laravel.log'),
            'level' => env('LOG_LEVEL', 'debug'),
            'days' => 14,
        ],

        'sentry' => [
            'driver' => 'sentry',
            'level' => 'error',
        ],

        'business' => [
            'driver' => 'daily',
            'path' => storage_path('logs/business.log'),
            'level' => 'info',
            'days' => 90,
        ],

        'integrations' => [
            'driver' => 'daily',
            'path' => storage_path('logs/integrations.log'),
            'level' => 'debug',
            'days' => 30,
        ],

        'security' => [
            'driver' => 'daily',
            'path' => storage_path('logs/security.log'),
            'level' => 'info',
            'days' => 365,
        ],

        'performance' => [
            'driver' => 'daily',
            'path' => storage_path('logs/performance.log'),
            'level' => 'info',
            'days' => 30,
        ],
    ],
];
```

### Logging Estruturado

```php
// app/Support/Logging/StructuredLogger.php
namespace App\Support\Logging;

use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Auth;

class StructuredLogger
{
    public static function business(
        string $action,
        string $entity,
        ?int $entityId = null,
        array $context = []
    ): void {
        Log::channel('business')->info($action, array_merge([
            'entity' => $entity,
            'entity_id' => $entityId,
            'user_id' => Auth::id(),
            'loja_id' => Auth::user()?->loja_id,
            'ip' => request()->ip(),
            'user_agent' => request()->userAgent(),
            'timestamp' => now()->toIso8601String(),
        ], $context));
    }

    public static function integration(
        string $service,
        string $operation,
        array $request = [],
        array $response = [],
        ?float $duration = null,
        bool $success = true
    ): void {
        $level = $success ? 'info' : 'error';

        Log::channel('integrations')->{$level}("{$service}:{$operation}", [
            'service' => $service,
            'operation' => $operation,
            'request' => $request,
            'response' => $response,
            'duration_ms' => $duration ? round($duration * 1000, 2) : null,
            'success' => $success,
            'timestamp' => now()->toIso8601String(),
        ]);
    }

    public static function security(
        string $event,
        array $context = [],
        string $level = 'warning'
    ): void {
        Log::channel('security')->{$level}($event, array_merge([
            'user_id' => Auth::id(),
            'ip' => request()->ip(),
            'user_agent' => request()->userAgent(),
            'url' => request()->fullUrl(),
            'method' => request()->method(),
            'timestamp' => now()->toIso8601String(),
        ], $context));
    }

    public static function performance(
        string $operation,
        float $duration,
        array $context = []
    ): void {
        $level = $duration > 1.0 ? 'warning' : 'info';

        Log::channel('performance')->{$level}($operation, array_merge([
            'duration_ms' => round($duration * 1000, 2),
            'memory_peak_mb' => round(memory_get_peak_usage(true) / 1024 / 1024, 2),
            'timestamp' => now()->toIso8601String(),
        ], $context));
    }
}
```

### Uso do Logger

```php
// Em um Service
use App\Support\Logging\StructuredLogger;

class VendaService
{
    public function criar(CreateVendaDTO $dto): Venda
    {
        $start = microtime(true);

        try {
            $venda = DB::transaction(function () use ($dto) {
                // ... lógica de criação
            });

            StructuredLogger::business(
                action: 'venda.criada',
                entity: 'Venda',
                entityId: $venda->id,
                context: [
                    'cliente_id' => $dto->clienteId,
                    'valor_total' => $venda->valor_total,
                    'itens_count' => $venda->itens->count(),
                ]
            );

            return $venda;
        } catch (\Exception $e) {
            StructuredLogger::business(
                action: 'venda.falha',
                entity: 'Venda',
                context: [
                    'error' => $e->getMessage(),
                    'dto' => $dto->toArray(),
                ]
            );

            throw $e;
        } finally {
            StructuredLogger::performance(
                operation: 'venda.criar',
                duration: microtime(true) - $start
            );
        }
    }
}
```

---

## Monitoramento

### Sentry

```php
// config/sentry.php
return [
    'dsn' => env('SENTRY_LARAVEL_DSN'),

    'release' => env('APP_VERSION'),

    'environment' => env('APP_ENV', 'production'),

    'breadcrumbs' => [
        'logs' => true,
        'sql_queries' => true,
        'sql_bindings' => true,
        'queue_info' => true,
        'command_info' => true,
    ],

    'send_default_pii' => false,

    'traces_sample_rate' => (float) env('SENTRY_TRACES_SAMPLE_RATE', 0.1),

    'profiles_sample_rate' => (float) env('SENTRY_PROFILES_SAMPLE_RATE', 0.1),

    'controllers_base_namespace' => 'App\\Http\\Controllers',

    'before_send' => function (\Sentry\Event $event): ?\Sentry\Event {
        // Filtrar dados sensíveis
        $exceptions = $event->getExceptions();

        foreach ($exceptions as $exception) {
            if ($exception->getValue()) {
                // Mascarar CPF/CNPJ em mensagens de erro
                $value = preg_replace(
                    '/\d{3}\.\d{3}\.\d{3}-\d{2}/',
                    '***.***.***-**',
                    $exception->getValue()
                );
                $exception->setValue($value);
            }
        }

        return $event;
    },
];
```

### Laravel Telescope (Desenvolvimento)

```php
// config/telescope.php
return [
    'enabled' => env('TELESCOPE_ENABLED', false),

    'domain' => env('TELESCOPE_DOMAIN'),

    'path' => 'telescope',

    'driver' => env('TELESCOPE_DRIVER', 'database'),

    'storage' => [
        'database' => [
            'connection' => 'mysql',
            'chunk' => 1000,
        ],
    ],

    'watchers' => [
        \Laravel\Telescope\Watchers\CacheWatcher::class => true,
        \Laravel\Telescope\Watchers\CommandWatcher::class => true,
        \Laravel\Telescope\Watchers\DumpWatcher::class => true,
        \Laravel\Telescope\Watchers\EventWatcher::class => true,
        \Laravel\Telescope\Watchers\ExceptionWatcher::class => true,
        \Laravel\Telescope\Watchers\GateWatcher::class => true,
        \Laravel\Telescope\Watchers\JobWatcher::class => true,
        \Laravel\Telescope\Watchers\LogWatcher::class => true,
        \Laravel\Telescope\Watchers\MailWatcher::class => true,
        \Laravel\Telescope\Watchers\ModelWatcher::class => true,
        \Laravel\Telescope\Watchers\NotificationWatcher::class => true,
        \Laravel\Telescope\Watchers\QueryWatcher::class => [
            'enabled' => true,
            'slow' => 100, // ms
        ],
        \Laravel\Telescope\Watchers\RedisWatcher::class => true,
        \Laravel\Telescope\Watchers\RequestWatcher::class => [
            'enabled' => true,
            'size_limit' => 64,
        ],
        \Laravel\Telescope\Watchers\ScheduleWatcher::class => true,
        \Laravel\Telescope\Watchers\ViewWatcher::class => true,
    ],
];
```

### Laravel Pulse (Produção)

```php
// config/pulse.php
return [
    'enabled' => env('PULSE_ENABLED', true),

    'domain' => env('PULSE_DOMAIN'),

    'path' => 'pulse',

    'storage' => [
        'driver' => env('PULSE_STORAGE_DRIVER', 'database'),
    ],

    'ingest' => [
        'driver' => env('PULSE_INGEST_DRIVER', 'database'),
    ],

    'recorders' => [
        \Laravel\Pulse\Recorders\CacheInteractions::class => [],
        \Laravel\Pulse\Recorders\Exceptions::class => [],
        \Laravel\Pulse\Recorders\Queues::class => [],
        \Laravel\Pulse\Recorders\Requests::class => [],
        \Laravel\Pulse\Recorders\SlowJobs::class => [
            'threshold' => 1000, // ms
        ],
        \Laravel\Pulse\Recorders\SlowQueries::class => [
            'threshold' => 100, // ms
        ],
        \Laravel\Pulse\Recorders\SlowRequests::class => [
            'threshold' => 1000, // ms
        ],
        \Laravel\Pulse\Recorders\UserJobs::class => [],
        \Laravel\Pulse\Recorders\UserRequests::class => [],
    ],
];
```

---

## Health Checks

### Endpoint de Health

```php
// routes/api.php
Route::get('/health', [HealthController::class, 'check']);
Route::get('/health/detailed', [HealthController::class, 'detailed'])
    ->middleware('auth:sanctum');

// app/Http/Controllers/Api/HealthController.php
namespace App\Http\Controllers\Api;

use App\Services\Health\HealthService;
use Illuminate\Http\JsonResponse;

class HealthController extends Controller
{
    public function __construct(
        private readonly HealthService $healthService
    ) {}

    public function check(): JsonResponse
    {
        $health = $this->healthService->check();

        return response()->json([
            'status' => $health->isHealthy() ? 'healthy' : 'unhealthy',
            'timestamp' => now()->toIso8601String(),
        ], $health->isHealthy() ? 200 : 503);
    }

    public function detailed(): JsonResponse
    {
        $health = $this->healthService->checkDetailed();

        return response()->json([
            'status' => $health->isHealthy() ? 'healthy' : 'unhealthy',
            'checks' => $health->getChecks(),
            'timestamp' => now()->toIso8601String(),
        ], $health->isHealthy() ? 200 : 503);
    }
}
```

### Health Service

```php
// app/Services/Health/HealthService.php
namespace App\Services\Health;

use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Redis;
use Illuminate\Support\Facades\Cache;
use App\Services\Acbr\AcbrClient;

class HealthService
{
    public function check(): HealthResult
    {
        $checks = [
            'database' => $this->checkDatabase(),
        ];

        return new HealthResult($checks);
    }

    public function checkDetailed(): HealthResult
    {
        $checks = [
            'database' => $this->checkDatabase(),
            'redis' => $this->checkRedis(),
            'cache' => $this->checkCache(),
            'queue' => $this->checkQueue(),
            'storage' => $this->checkStorage(),
            'acbr' => $this->checkAcbr(),
        ];

        return new HealthResult($checks);
    }

    private function checkDatabase(): array
    {
        try {
            $start = microtime(true);
            DB::select('SELECT 1');
            $latency = (microtime(true) - $start) * 1000;

            return [
                'status' => 'healthy',
                'latency_ms' => round($latency, 2),
            ];
        } catch (\Exception $e) {
            return [
                'status' => 'unhealthy',
                'error' => $e->getMessage(),
            ];
        }
    }

    private function checkRedis(): array
    {
        try {
            $start = microtime(true);
            Redis::ping();
            $latency = (microtime(true) - $start) * 1000;

            return [
                'status' => 'healthy',
                'latency_ms' => round($latency, 2),
            ];
        } catch (\Exception $e) {
            return [
                'status' => 'unhealthy',
                'error' => $e->getMessage(),
            ];
        }
    }

    private function checkCache(): array
    {
        try {
            $key = 'health_check_' . uniqid();
            Cache::put($key, true, 10);
            $result = Cache::get($key);
            Cache::forget($key);

            return [
                'status' => $result === true ? 'healthy' : 'unhealthy',
            ];
        } catch (\Exception $e) {
            return [
                'status' => 'unhealthy',
                'error' => $e->getMessage(),
            ];
        }
    }

    private function checkQueue(): array
    {
        try {
            $pendingJobs = DB::table('jobs')->count();
            $failedJobs = DB::table('failed_jobs')
                ->where('failed_at', '>=', now()->subHour())
                ->count();

            return [
                'status' => 'healthy',
                'pending_jobs' => $pendingJobs,
                'failed_jobs_last_hour' => $failedJobs,
            ];
        } catch (\Exception $e) {
            return [
                'status' => 'unhealthy',
                'error' => $e->getMessage(),
            ];
        }
    }

    private function checkStorage(): array
    {
        try {
            $disk = storage_path();
            $free = disk_free_space($disk);
            $total = disk_total_space($disk);
            $usedPercent = (($total - $free) / $total) * 100;

            return [
                'status' => $usedPercent < 90 ? 'healthy' : 'warning',
                'free_gb' => round($free / 1024 / 1024 / 1024, 2),
                'total_gb' => round($total / 1024 / 1024 / 1024, 2),
                'used_percent' => round($usedPercent, 1),
            ];
        } catch (\Exception $e) {
            return [
                'status' => 'unhealthy',
                'error' => $e->getMessage(),
            ];
        }
    }

    private function checkAcbr(): array
    {
        try {
            $client = app(AcbrClient::class);
            $start = microtime(true);
            $status = $client->consultarStatusServico();
            $latency = (microtime(true) - $start) * 1000;

            return [
                'status' => $status->emOperacao() ? 'healthy' : 'warning',
                'sefaz_status' => $status->cstat,
                'latency_ms' => round($latency, 2),
            ];
        } catch (\Exception $e) {
            return [
                'status' => 'unhealthy',
                'error' => $e->getMessage(),
            ];
        }
    }
}

// app/Services/Health/HealthResult.php
namespace App\Services\Health;

class HealthResult
{
    public function __construct(
        private readonly array $checks
    ) {}

    public function isHealthy(): bool
    {
        return collect($this->checks)
            ->every(fn($check) => $check['status'] === 'healthy');
    }

    public function getChecks(): array
    {
        return $this->checks;
    }
}
```

---

## Alertas

### Configuração de Alertas

```php
// config/alerts.php
return [
    'channels' => [
        'slack' => [
            'webhook_url' => env('SLACK_WEBHOOK_URL'),
            'channel' => '#erp-alerts',
        ],
        'email' => [
            'recipients' => explode(',', env('ALERT_EMAILS', '')),
        ],
    ],

    'thresholds' => [
        'error_rate' => [
            'warning' => 5,  // 5 erros/minuto
            'critical' => 20, // 20 erros/minuto
        ],
        'response_time' => [
            'warning' => 2000, // 2 segundos
            'critical' => 5000, // 5 segundos
        ],
        'queue_size' => [
            'warning' => 100,
            'critical' => 500,
        ],
        'disk_usage' => [
            'warning' => 80, // 80%
            'critical' => 90, // 90%
        ],
    ],
];
```

### Notificação de Alerta

```php
// app/Notifications/CriticalErrorNotification.php
namespace App\Notifications;

use Illuminate\Bus\Queueable;
use Illuminate\Notifications\Notification;
use Illuminate\Notifications\Messages\SlackMessage;
use Illuminate\Notifications\Messages\MailMessage;

class CriticalErrorNotification extends Notification
{
    use Queueable;

    public function __construct(
        private readonly string $errorCode,
        private readonly string $message,
        private readonly array $context,
        private readonly string $environment
    ) {}

    public function via(object $notifiable): array
    {
        return ['slack', 'mail'];
    }

    public function toSlack(object $notifiable): SlackMessage
    {
        return (new SlackMessage)
            ->error()
            ->content("Erro Crítico no ERP ({$this->environment})")
            ->attachment(function ($attachment) {
                $attachment
                    ->title($this->errorCode)
                    ->content($this->message)
                    ->fields([
                        'Ambiente' => $this->environment,
                        'Horário' => now()->format('d/m/Y H:i:s'),
                        'Request ID' => $this->context['request_id'] ?? 'N/A',
                    ]);
            });
    }

    public function toMail(object $notifiable): MailMessage
    {
        return (new MailMessage)
            ->error()
            ->subject("Erro Crítico: {$this->errorCode}")
            ->line("Ocorreu um erro crítico no ERP ({$this->environment})")
            ->line("Código: {$this->errorCode}")
            ->line("Mensagem: {$this->message}")
            ->line("Horário: " . now()->format('d/m/Y H:i:s'));
    }
}
```

### Listener de Erros

```php
// app/Listeners/CriticalErrorListener.php
namespace App\Listeners;

use App\Events\CriticalErrorOccurred;
use App\Notifications\CriticalErrorNotification;
use Illuminate\Support\Facades\Notification;

class CriticalErrorListener
{
    public function handle(CriticalErrorOccurred $event): void
    {
        // Rate limit: máximo 1 notificação por erro por 5 minutos
        $cacheKey = "alert_sent:{$event->errorCode}";

        if (cache()->has($cacheKey)) {
            return;
        }

        cache()->put($cacheKey, true, now()->addMinutes(5));

        Notification::route('slack', config('alerts.channels.slack.webhook_url'))
            ->route('mail', config('alerts.channels.email.recipients'))
            ->notify(new CriticalErrorNotification(
                errorCode: $event->errorCode,
                message: $event->message,
                context: $event->context,
                environment: app()->environment()
            ));
    }
}
```

---

## Métricas

### Custom Metrics

```php
// app/Services/Metrics/MetricsService.php
namespace App\Services\Metrics;

use Illuminate\Support\Facades\Redis;

class MetricsService
{
    public function increment(string $metric, int $value = 1, array $tags = []): void
    {
        $key = $this->buildKey($metric, $tags);
        Redis::incrby($key, $value);
        Redis::expire($key, 3600); // 1 hora
    }

    public function timing(string $metric, float $duration, array $tags = []): void
    {
        $key = $this->buildKey($metric . ':timing', $tags);
        Redis::rpush($key, $duration);
        Redis::ltrim($key, -1000, -1); // Manter últimos 1000
        Redis::expire($key, 3600);
    }

    public function gauge(string $metric, float $value, array $tags = []): void
    {
        $key = $this->buildKey($metric . ':gauge', $tags);
        Redis::set($key, $value);
        Redis::expire($key, 3600);
    }

    public function getStats(string $metric, array $tags = []): array
    {
        $key = $this->buildKey($metric . ':timing', $tags);
        $values = array_map('floatval', Redis::lrange($key, 0, -1));

        if (empty($values)) {
            return ['count' => 0];
        }

        sort($values);
        $count = count($values);

        return [
            'count' => $count,
            'min' => min($values),
            'max' => max($values),
            'avg' => array_sum($values) / $count,
            'p50' => $values[(int) ($count * 0.5)],
            'p95' => $values[(int) ($count * 0.95)],
            'p99' => $values[(int) ($count * 0.99)],
        ];
    }

    private function buildKey(string $metric, array $tags): string
    {
        $tagString = collect($tags)
            ->map(fn($v, $k) => "{$k}:{$v}")
            ->implode(',');

        return "metrics:{$metric}" . ($tagString ? ":{$tagString}" : '');
    }
}
```

### Middleware de Métricas

```php
// app/Http/Middleware/MetricsMiddleware.php
namespace App\Http\Middleware;

use App\Services\Metrics\MetricsService;
use Closure;
use Illuminate\Http\Request;

class MetricsMiddleware
{
    public function __construct(
        private readonly MetricsService $metrics
    ) {}

    public function handle(Request $request, Closure $next)
    {
        $start = microtime(true);

        $response = $next($request);

        $duration = (microtime(true) - $start) * 1000;

        $this->metrics->increment('http.requests', 1, [
            'method' => $request->method(),
            'status' => $response->status(),
            'route' => $request->route()?->getName() ?? 'unknown',
        ]);

        $this->metrics->timing('http.request_duration', $duration, [
            'route' => $request->route()?->getName() ?? 'unknown',
        ]);

        return $response;
    }
}
```

---

## Dashboard de Erros

### Endpoint de Estatísticas

```php
// app/Http/Controllers/Admin/ErrorDashboardController.php
namespace App\Http\Controllers\Admin;

use App\Services\Metrics\MetricsService;
use Illuminate\Http\JsonResponse;
use Illuminate\Support\Facades\DB;

class ErrorDashboardController extends Controller
{
    public function __construct(
        private readonly MetricsService $metrics
    ) {}

    public function stats(): JsonResponse
    {
        return response()->json([
            'errors' => [
                'last_hour' => $this->getErrorCount(60),
                'last_24h' => $this->getErrorCount(1440),
                'by_type' => $this->getErrorsByType(),
            ],
            'performance' => [
                'request_stats' => $this->metrics->getStats('http.request_duration'),
                'slow_queries' => $this->getSlowQueries(),
            ],
            'queue' => [
                'pending' => DB::table('jobs')->count(),
                'failed' => DB::table('failed_jobs')->count(),
                'failed_today' => DB::table('failed_jobs')
                    ->whereDate('failed_at', today())
                    ->count(),
            ],
            'uptime' => $this->getUptimeStats(),
        ]);
    }

    private function getErrorCount(int $minutes): int
    {
        // Buscar do Sentry API ou banco local
        return 0;
    }

    private function getErrorsByType(): array
    {
        // Agrupar erros por tipo
        return [];
    }

    private function getSlowQueries(): array
    {
        // Buscar queries lentas recentes
        return [];
    }

    private function getUptimeStats(): array
    {
        return [
            'current_uptime' => $this->calculateUptime(),
            'last_incident' => null,
        ];
    }

    private function calculateUptime(): string
    {
        // Calcular uptime baseado em health checks
        return '99.9%';
    }
}
```

---

## Checklist de Implementação

- [ ] Criar hierarquia de exceções
- [ ] Configurar Exception Handler
- [ ] Configurar Sentry
- [ ] Configurar canais de log
- [ ] Implementar StructuredLogger
- [ ] Criar health checks
- [ ] Configurar alertas Slack/Email
- [ ] Implementar métricas customizadas
- [ ] Criar dashboard de erros
- [ ] Configurar Laravel Telescope (dev)
- [ ] Configurar Laravel Pulse (prod)

---

## Documentos Relacionados

- [01-arquitetura.md](./01-arquitetura.md) - Arquitetura geral
- [07-testes.md](./07-testes.md) - Estratégia de testes
- [17-validacao.md](./17-validacao.md) - Validação
