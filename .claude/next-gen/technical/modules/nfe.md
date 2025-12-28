# Module: NFe (Electronic Invoice)

> Status: **Draft**
> Priority: 6 (after Vendas)
> Complexity: **High**

---

## Overview

NFe (Nota Fiscal Eletrônica) is the Brazilian electronic invoice system. This is one of the most complex modules due to:
- Government API integration (SEFAZ)
- XML signing with digital certificates
- Strict validation rules
- Multiple document types (NFe, NFCe, NFSe)
- Tax calculations (ICMS, IPI, PIS, COFINS, IBS, CBS)

---

## Current Implementation (C++)

### Classes
| Class | File | Purpose |
|-------|------|---------|
| `TabNFe` | `tabnfe.cpp` | Main tab container |
| `WidgetNFe*` | Various | NFe management widgets |
| `CadastrarNFe` | `cadastrarnfe.cpp` | NFe creation/emission |
| `ACBr` | `acbr.cpp` | ACBrLib wrapper |
| `XML` | `xml.cpp` | XML generation/parsing |

### Current Integration
- Uses **ACBrLib** (Delphi-based library)
- Called via DLL interface
- Handles: XML generation, signing, SEFAZ communication

---

## Integration Options

### Option 1: Keep ACBr (via CLI/API)

**Approach**: Run ACBr as a Windows service, call via REST API or CLI.

```
┌─────────────┐      HTTP/REST      ┌─────────────┐
│   Laravel   │ ◄─────────────────► │  ACBr API   │
│   (Linux)   │                     │  (Windows)  │
└─────────────┘                     └─────────────┘
```

**Pros**:
- Already works
- No reimplementation
- Free (open source)

**Cons**:
- Requires Windows server
- Extra infrastructure
- Network dependency
- Complex deployment

### Option 2: SaaS Provider

**Providers**: Focus NFe, Enotas, Tecnospeed, Olist

**Approach**: API calls to third-party service.

```php
// Example with Focus NFe
$response = Http::withHeaders([
    'Authorization' => 'Bearer ' . config('services.focus.token'),
])->post('https://api.focusnfe.com.br/v2/nfe', [
    'natureza_operacao' => 'Venda',
    'forma_pagamento' => 0,
    'cnpj_emitente' => $loja->cnpj,
    'items' => $itens,
    // ...
]);
```

**Pros**:
- Simple integration
- No infrastructure to manage
- Updates handled by provider
- Support included

**Cons**:
- Monthly cost (R$ 50-500+/month based on volume)
- Vendor lock-in
- Internet dependency

### Option 3: Native PHP (sped-nfe)

**Library**: [nfephp-org/sped-nfe](https://github.com/nfephp-org/sped-nfe)

**Approach**: Pure PHP implementation.

```php
use NFePHP\NFe\Make;
use NFePHP\NFe\Tools;

$nfe = new Make();
$nfe->taginfNFe($std);
$nfe->tagide($std);
$nfe->tagemit($std);
// ... build XML

$tools = new Tools($config, Certificate::readPfx($certPath, $password));
$response = $tools->sefazEnviaLote([$xml], $idLote);
```

**Pros**:
- Full control
- No external dependencies
- Free (open source)
- Runs on any server

**Cons**:
- More development work
- Must maintain updates (SEFAZ changes)
- Handle certificate management
- Complex error handling

---

## Recommendation

**Hybrid Approach**:

1. **Start with SaaS** (Focus NFe or Enotas)
   - Faster to market
   - Lower initial complexity
   - Can switch later

2. **Abstract behind service interface**
   - Easy to swap implementations
   - Test with mocks

3. **Consider native PHP later** if:
   - Volume justifies cost savings
   - Need more control
   - Team has capacity

---

## Laravel Implementation

### Service Interface

```php
// app/Contracts/NfeServiceInterface.php
namespace App\Contracts;

interface NfeServiceInterface
{
    public function emitir(Nfe $nfe): NfeResult;
    public function cancelar(Nfe $nfe, string $justificativa): NfeResult;
    public function consultar(string $chave): NfeResult;
    public function inutilizar(int $inicio, int $fim, string $justificativa): NfeResult;
    public function cartaCorrecao(Nfe $nfe, string $correcao): NfeResult;
}

// app/DTOs/NfeResult.php
class NfeResult
{
    public function __construct(
        public bool $success,
        public ?string $protocolo = null,
        public ?string $chave = null,
        public ?string $xml = null,
        public ?string $erro = null,
        public ?int $codigoErro = null,
    ) {}
}
```

### SaaS Implementation (Focus NFe)

```php
// app/Services/NFe/FocusNfeService.php
namespace App\Services\NFe;

use App\Contracts\NfeServiceInterface;
use App\Models\Nfe;
use Illuminate\Support\Facades\Http;

class FocusNfeService implements NfeServiceInterface
{
    private string $baseUrl;
    private string $token;

    public function __construct()
    {
        $this->baseUrl = config('services.focus.url');
        $this->token = config('services.focus.token');
    }

    public function emitir(Nfe $nfe): NfeResult
    {
        $payload = $this->buildPayload($nfe);

        $response = Http::withToken($this->token)
            ->post("{$this->baseUrl}/v2/nfe", $payload);

        if ($response->successful()) {
            $data = $response->json();

            return new NfeResult(
                success: true,
                protocolo: $data['protocolo'],
                chave: $data['chave'],
                xml: $data['xml'],
            );
        }

        return new NfeResult(
            success: false,
            erro: $response->json('mensagem'),
            codigoErro: $response->json('codigo'),
        );
    }

    public function cancelar(Nfe $nfe, string $justificativa): NfeResult
    {
        $response = Http::withToken($this->token)
            ->delete("{$this->baseUrl}/v2/nfe/{$nfe->chave}", [
                'justificativa' => $justificativa,
            ]);

        // ... handle response
    }

    private function buildPayload(Nfe $nfe): array
    {
        return [
            'natureza_operacao' => $nfe->natureza_operacao,
            'forma_pagamento' => $nfe->forma_pagamento,
            'tipo_documento' => 1, // NFe
            'finalidade_emissao' => 1, // Normal
            'cnpj_emitente' => $nfe->loja->cnpj,
            'nome_emitente' => $nfe->loja->razao_social,
            // ... complete payload
            'items' => $nfe->itens->map(fn($item) => [
                'numero_item' => $item->numero,
                'codigo_produto' => $item->produto->codigo,
                'descricao' => $item->descricao,
                'cfop' => $item->cfop,
                'ncm' => $item->ncm,
                'quantidade' => $item->quantidade,
                'valor_unitario' => $item->valor_unitario,
                'valor_total' => $item->valor_total,
                'icms_situacao_tributaria' => $item->cst_icms,
                // ... tax fields
            ])->toArray(),
        ];
    }
}
```

### Service Provider Registration

```php
// app/Providers/NfeServiceProvider.php
namespace App\Providers;

use App\Contracts\NfeServiceInterface;
use App\Services\NFe\FocusNfeService;
use App\Services\NFe\SpedNfeService;

class NfeServiceProvider extends ServiceProvider
{
    public function register()
    {
        $this->app->bind(NfeServiceInterface::class, function ($app) {
            return match (config('nfe.driver')) {
                'focus' => new FocusNfeService(),
                'sped' => new SpedNfeService(),
                'mock' => new MockNfeService(), // for testing
                default => throw new \Exception('Invalid NFe driver'),
            };
        });
    }
}
```

### Model

```php
// app/Models/Nfe.php
class Nfe extends Model
{
    protected $fillable = [
        'loja_id', 'venda_id', 'compra_id',
        'tipo', 'numero', 'serie', 'chave',
        'status', 'natureza_operacao',
        'valor_total', 'valor_produtos', 'valor_frete',
        'xml_envio', 'xml_retorno', 'xml_cancelamento',
        'protocolo', 'data_emissao', 'data_autorizacao',
        'motivo_cancelamento',
    ];

    protected $casts = [
        'status' => NfeStatus::class,
        'tipo' => NfeTipo::class,
        'data_emissao' => 'datetime',
        'data_autorizacao' => 'datetime',
    ];

    public function loja(): BelongsTo
    {
        return $this->belongsTo(Loja::class);
    }

    public function venda(): BelongsTo
    {
        return $this->belongsTo(Venda::class);
    }

    public function itens(): HasMany
    {
        return $this->hasMany(NfeItem::class);
    }
}
```

### Enums

```php
// app/Enums/NfeStatus.php
enum NfeStatus: string
{
    case PENDENTE = 'PENDENTE';
    case PROCESSANDO = 'PROCESSANDO';
    case AUTORIZADA = 'AUTORIZADA';
    case REJEITADA = 'REJEITADA';
    case CANCELADA = 'CANCELADA';
    case DENEGADA = 'DENEGADA';
    case INUTILIZADA = 'INUTILIZADA';

    public function label(): string { /* ... */ }
    public function color(): string { /* ... */ }
}

// app/Enums/NfeTipo.php
enum NfeTipo: string
{
    case NFE = 'NFE';       // Nota Fiscal Eletrônica (modelo 55)
    case NFCE = 'NFCE';     // Nota Fiscal Consumidor (modelo 65)
    case NFSE = 'NFSE';     // Nota Fiscal de Serviço
}
```

### Job (Background Processing)

```php
// app/Jobs/ProcessarNFeJob.php
namespace App\Jobs;

use App\Contracts\NfeServiceInterface;
use App\Models\Nfe;

class ProcessarNFeJob implements ShouldQueue
{
    use Dispatchable, InteractsWithQueue, Queueable, SerializesModels;

    public int $tries = 3;
    public int $backoff = 60;

    public function __construct(
        public Nfe $nfe
    ) {}

    public function handle(NfeServiceInterface $nfeService): void
    {
        $this->nfe->update(['status' => NfeStatus::PROCESSANDO]);

        $result = $nfeService->emitir($this->nfe);

        if ($result->success) {
            $this->nfe->update([
                'status' => NfeStatus::AUTORIZADA,
                'chave' => $result->chave,
                'protocolo' => $result->protocolo,
                'xml_retorno' => $result->xml,
                'data_autorizacao' => now(),
            ]);

            event(new NfeAutorizada($this->nfe));
        } else {
            $this->nfe->update([
                'status' => NfeStatus::REJEITADA,
                'motivo_rejeicao' => $result->erro,
                'codigo_rejeicao' => $result->codigoErro,
            ]);

            event(new NfeRejeitada($this->nfe, $result->erro));
        }
    }

    public function failed(\Throwable $exception): void
    {
        $this->nfe->update([
            'status' => NfeStatus::REJEITADA,
            'motivo_rejeicao' => $exception->getMessage(),
        ]);
    }
}
```

---

## Tax Calculations

### Current Tax Types
- ICMS (state sales tax)
- IPI (federal manufacturing tax)
- PIS (federal social contribution)
- COFINS (federal social contribution)

### New (Reforma Tributária 2025+)
- IBS (state/municipal - replaces ICMS + ISS)
- CBS (federal - replaces PIS + COFINS)
- IS (Imposto Seletivo - selective tax)

### Tax Service

```php
// app/Services/NFe/TaxCalculationService.php
class TaxCalculationService
{
    public function calcular(NfeItem $item): array
    {
        $produto = $item->produto;
        $ncm = $produto->ncm;

        // Get applicable tax rules
        $regras = $this->getRegras($ncm, $item->cfop);

        return [
            'icms' => $this->calcularICMS($item, $regras),
            'ipi' => $this->calcularIPI($item, $regras),
            'pis' => $this->calcularPIS($item, $regras),
            'cofins' => $this->calcularCOFINS($item, $regras),
            // Future:
            // 'ibs' => $this->calcularIBS($item, $regras),
            // 'cbs' => $this->calcularCBS($item, $regras),
        ];
    }

    private function calcularICMS(NfeItem $item, array $regras): array
    {
        $cst = $regras['cst_icms'];
        $aliquota = $regras['aliq_icms'];
        $baseCalculo = $item->valor_total;

        return [
            'orig' => $item->produto->origem ?? '0',
            'cst' => $cst,
            'modBC' => 3, // valor da operação
            'vBC' => $baseCalculo,
            'pICMS' => $aliquota,
            'vICMS' => round($baseCalculo * $aliquota / 100, 2),
        ];
    }

    // ... other tax calculations
}
```

---

## UI Components

### NFe List
- Status filter (Pendente, Autorizada, Rejeitada, Cancelada)
- Date range filter
- Search by number/key
- Quick actions (View XML, Download DANFE, Cancel)

### NFe Form (Manual)
- Recipient selection
- Product line items with tax calculation preview
- Payment info
- Transport info
- Additional info

### NFe Import (from supplier)
- XML upload
- Validation preview
- Link to purchase order
- Auto-populate estoque

---

## Scheduled Tasks

```php
// Check pending NFe status
$schedule->job(new ConsultarNFePendentesJob)->everyFiveMinutes();

// Download NFe from SEFAZ (DFe)
$schedule->job(new BaixarNFeRecebidasJob)->hourly();

// Retry failed emissions
$schedule->job(new RetentarNFeRejeitadasJob)->everyThirtyMinutes();
```

---

## Testing Strategy

```php
// Use mock service in tests
public function test_emitir_nfe_sucesso()
{
    $this->mock(NfeServiceInterface::class, function ($mock) {
        $mock->shouldReceive('emitir')
            ->once()
            ->andReturn(new NfeResult(
                success: true,
                protocolo: '123456789',
                chave: '35240112345678901234550010000000011234567890',
            ));
    });

    $nfe = Nfe::factory()->create(['status' => NfeStatus::PENDENTE]);

    ProcessarNFeJob::dispatch($nfe);

    $this->assertEquals(NfeStatus::AUTORIZADA, $nfe->fresh()->status);
}
```
