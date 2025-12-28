# Módulo: NFe (Nota Fiscal Eletrônica)

> Status: **Rascunho**
> Prioridade: 6 (após Vendas)
> Complexidade: **Alta**

---

## Visão Geral

NFe (Nota Fiscal Eletrônica) é o sistema brasileiro de fatura eletrônica. Este é um dos módulos mais complexos devido a:
- Integração com API governamental (SEFAZ)
- Assinatura de XML com certificados digitais
- Regras de validação rigorosas
- Múltiplos tipos de documentos (NFe, NFCe, NFSe)
- Cálculos tributários (ICMS, IPI, PIS, COFINS, IBS, CBS)

---

## Implementação Atual (C++)

### Classes
| Classe | Arquivo | Finalidade |
|--------|---------|------------|
| `TabNFe` | `tabnfe.cpp` | Container principal da aba |
| `WidgetNFe*` | Diversos | Widgets de gestão de NFe |
| `CadastrarNFe` | `cadastrarnfe.cpp` | Criação/emissão de NFe |
| `ACBr` | `acbr.cpp` | Wrapper do ACBrLib |
| `XML` | `xml.cpp` | Geração/parsing de XML |

### Integração Atual
- Utiliza **ACBrLib** (biblioteca baseada em Delphi)
- Chamada via interface DLL
- Funções: Geração de XML, assinatura, comunicação com SEFAZ

---

## Opções de Integração

### Opção 1: Manter ACBr (via CLI/API)

**Abordagem**: Executar ACBr como serviço Windows, chamar via API REST ou CLI.

```
┌─────────────┐      HTTP/REST      ┌─────────────┐
│   Laravel   │ ◄─────────────────► │  ACBr API   │
│   (Linux)   │                     │  (Windows)  │
└─────────────┘                     └─────────────┘
```

**Prós**:
- Já funciona
- Sem reimplementação
- Gratuito (código aberto)

**Contras**:
- Requer servidor Windows
- Infraestrutura adicional
- Dependência de rede
- Deploy complexo

### Opção 2: Provedor SaaS

**Provedores**: Focus NFe, Enotas, Tecnospeed, Olist

**Abordagem**: Chamadas de API para serviço de terceiros.

```php
// Exemplo com Focus NFe
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

**Prós**:
- Integração simples
- Sem infraestrutura para gerenciar
- Atualizações tratadas pelo provedor
- Suporte incluso

**Contras**:
- Custo mensal (R$ 50-500+/mês baseado no volume)
- Dependência de fornecedor
- Dependência de internet

### Opção 3: PHP Nativo (sped-nfe)

**Biblioteca**: [nfephp-org/sped-nfe](https://github.com/nfephp-org/sped-nfe)

**Abordagem**: Implementação em PHP puro.

```php
use NFePHP\NFe\Make;
use NFePHP\NFe\Tools;

$nfe = new Make();
$nfe->taginfNFe($std);
$nfe->tagide($std);
$nfe->tagemit($std);
// ... construir XML

$tools = new Tools($config, Certificate::readPfx($certPath, $password));
$response = $tools->sefazEnviaLote([$xml], $idLote);
```

**Prós**:
- Controle total
- Sem dependências externas
- Gratuito (código aberto)
- Roda em qualquer servidor

**Contras**:
- Mais trabalho de desenvolvimento
- Deve manter atualizações (mudanças da SEFAZ)
- Gerenciar certificados
- Tratamento de erros complexo

---

## Recomendação

**Abordagem Híbrida**:

1. **Começar com SaaS** (Focus NFe ou Enotas)
   - Mais rápido para produção
   - Menor complexidade inicial
   - Pode trocar depois

2. **Abstrair atrás de interface de serviço**
   - Fácil trocar implementações
   - Testar com mocks

3. **Considerar PHP nativo depois** se:
   - Volume justificar economia de custos
   - Precisar de mais controle
   - Equipe tiver capacidade

---

## Implementação Laravel

### Interface de Serviço

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

### Implementação SaaS (Focus NFe)

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

        // ... tratar resposta
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
            // ... payload completo
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
                // ... campos de impostos
            ])->toArray(),
        ];
    }
}
```

### Registro do Service Provider

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
                'mock' => new MockNfeService(), // para testes
                default => throw new \Exception('Driver NFe inválido'),
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

### Job (Processamento em Background)

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

## Cálculos Tributários

### Tipos de Impostos Atuais
- ICMS (imposto estadual sobre vendas)
- IPI (imposto federal sobre produtos industrializados)
- PIS (contribuição social federal)
- COFINS (contribuição social federal)

### Novos (Reforma Tributária 2025+)
- IBS (estadual/municipal - substitui ICMS + ISS)
- CBS (federal - substitui PIS + COFINS)
- IS (Imposto Seletivo - imposto seletivo)

### Serviço de Cálculo de Impostos

```php
// app/Services/NFe/TaxCalculationService.php
class TaxCalculationService
{
    public function calcular(NfeItem $item): array
    {
        $produto = $item->produto;
        $ncm = $produto->ncm;

        // Obter regras tributárias aplicáveis
        $regras = $this->getRegras($ncm, $item->cfop);

        return [
            'icms' => $this->calcularICMS($item, $regras),
            'ipi' => $this->calcularIPI($item, $regras),
            'pis' => $this->calcularPIS($item, $regras),
            'cofins' => $this->calcularCOFINS($item, $regras),
            // Futuro:
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

    // ... outros cálculos de impostos
}
```

---

## Componentes de UI

### Lista de NFe
- Filtro por status (Pendente, Autorizada, Rejeitada, Cancelada)
- Filtro por período
- Busca por número/chave
- Ações rápidas (Visualizar XML, Baixar DANFE, Cancelar)

### Formulário de NFe (Manual)
- Seleção de destinatário
- Itens de produto com preview de cálculo tributário
- Informações de pagamento
- Informações de transporte
- Informações adicionais

### Importação de NFe (do fornecedor)
- Upload de XML
- Preview de validação
- Vincular ao pedido de compra
- Auto-preencher estoque

---

## Tarefas Agendadas

```php
// Verificar status de NFe pendentes
$schedule->job(new ConsultarNFePendentesJob)->everyFiveMinutes();

// Baixar NFe da SEFAZ (DFe)
$schedule->job(new BaixarNFeRecebidasJob)->hourly();

// Retentar emissões falhas
$schedule->job(new RetentarNFeRejeitadasJob)->everyThirtyMinutes();
```

---

## Estratégia de Testes

```php
// Usar serviço mock nos testes
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
