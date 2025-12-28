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

| Classe         | Arquivo            | Finalidade                 |
| -------------- | ------------------ | -------------------------- |
| `TabNFe`       | `tabnfe.cpp`       | Container principal da aba |
| `WidgetNFe*`   | Diversos           | Widgets de gestão de NFe   |
| `CadastrarNFe` | `cadastrarnfe.cpp` | Criação/emissão de NFe     |
| `ACBr`         | `acbr.cpp`         | Wrapper do ACBrLib         |
| `XML`          | `xml.cpp`          | Geração/parsing de XML     |

### Integração Atual

- Utiliza **ACBrLib** (biblioteca baseada em Delphi)
- Chamada via interface DLL
- Funções: Geração de XML, assinatura, comunicação com SEFAZ

---

## Opções de Integração

### Opção 1: ACBrMonitorConsole (Linux Headless) ✅ Recomendada

**Abordagem**: Executar ACBrMonitorConsole no servidor Linux, comunicação via TCP/IP.

```text
┌─────────────┐      TCP Socket      ┌─────────────────────┐
│   Laravel   │ ◄───────────────────►│  ACBrMonitorConsole │
│   (Linux)   │      porta 3434      │  (mesmo servidor)   │
└─────────────┘                      └─────────────────────┘
```

**Características**:

- Versão **console** do ACBrMonitor (não precisa de GUI)
- Funciona em **Linux headless** (sem Servidor X)
- Comunicação via **TCP/IP** ou **arquivos TXT**
- Pode rodar em **segundo plano** como serviço

**Execução**:

```bash
# Rodar em segundo plano com log
./ACBrMonitorConsole > /var/log/acbr.log 2>&1 &

# Ou com systemd service
sudo systemctl start acbrmonitor
```

**Comunicação Laravel**:

```php
// app/Services/NFe/AcbrSocketService.php
class AcbrSocketService
{
    private string $host = 'localhost';
    private int $port = 3434;

    public function enviarComando(string $comando): string
    {
        $socket = fsockopen($this->host, $this->port, $errno, $errstr, 10);
        if (!$socket) {
            throw new \Exception("ACBr connection failed: $errstr");
        }

        fwrite($socket, $comando . "\r\n");
        $response = '';
        while (!feof($socket)) {
            $response .= fread($socket, 4096);
        }
        fclose($socket);

        return $response;
    }

    public function emitirNFe(string $xmlPath): string
    {
        return $this->enviarComando("NFE.EnviarNFe(\"{$xmlPath}\", 1, 1)");
    }

    public function cancelarNFe(string $chave, string $justificativa): string
    {
        return $this->enviarComando("NFE.Cancelar(\"{$chave}\", \"{$justificativa}\")");
    }
}
```

**Prós**:

- Roda no **mesmo servidor Linux** (sem Windows)
- Gratuito (código aberto)
- Já conhecemos o ACBr
- Sem dependência de rede externa

**Contras**:

- Configuração inicial mais complexa
- Dependências do ACBr no Linux

**Referências**:

- [Como usar ACBrMonitorConsole no Linux](https://acbr.sourceforge.io/ACBrMonitor/ComousaroACBrMonitorConsolenoLin.html)
- [Curso ACBrLib Linux Server](https://projetoacbr.com.br/cursos/linux-server-acbrlib/)

---

### Opção 2: PHP Nativo (sped-nfe)

**Biblioteca**: [nfephp-org/sped-nfe](https://github.com/nfephp-org/sped-nfe)

**Abordagem**: Implementação 100% PHP, roda em qualquer servidor Linux.

```bash
composer require nfephp-org/sped-nfe
```

```php
use NFePHP\NFe\Make;
use NFePHP\NFe\Tools;
use NFePHP\Common\Certificate;

$nfe = new Make();
$nfe->taginfNFe($std);
$nfe->tagide($std);
$nfe->tagemit($std);
// ... construir XML

$tools = new Tools($config, Certificate::readPfx($certPath, $password));
$response = $tools->sefazEnviaLote([$xml], $idLote);
```

**Características**:

- Atualizado para **Reforma Tributária 2025** (NT 2025.002)
- Aderente aos PSR-1, PSR-2 e PSR-4
- Requer **certificado A1** (formato .pfx)
- Documentação e grupo de discussão ativos

**Prós**:

- Controle total
- Sem dependências externas (100% PHP)
- Gratuito (código aberto)
- Roda em qualquer servidor Linux

**Contras**:

- Mais trabalho de desenvolvimento inicial
- Deve acompanhar atualizações da SEFAZ
- Gerenciar certificados digitais
- Tratamento de erros complexo

**Referências**:

- [GitHub sped-nfe](https://github.com/nfephp-org/sped-nfe)
- [Tutorial Laravel + sped-nfe](https://medium.com/@geovanent/emitindo-uma-nf-e-como-sped-php-9e325570e6c4)
- [Pacote laravel-nfe](https://github.com/docode-web/laravel-nfe)
- [Grupo NFePHP](https://groups.google.com/g/nfephp)

---

### ❌ ACBrLib em Linux Headless: Não Recomendado

**Problema**: ACBrLib (.so) tem dependência do **FortesReport** que requer interface gráfica para geração de PDF (DANFE).

| Tentativa            | Resultado                          |
| -------------------- | ---------------------------------- |
| Carregar .so sem GUI | Falha ao executar funções          |
| Xvfb (GUI virtual)   | Não funciona na prática            |
| Testes Python/PHP    | Biblioteca carrega, funções falham |

> "ACBrLib tem suas DLL's para Linux, mas são para Desktop com interface gráfica (GUI). Ninguém conseguiu fazer ACBrLib rodar 100% em servidores Linux sem GUI porque suas dependências dependem da GUI, como o FortesReport."
> — [Fórum Projeto ACBr](https://www.projetoacbr.com.br/forum/topic/76976-acbrlib-linux/)

**Alternativa**: Use **ACBrMonitorConsole** (Opção 1) que foi feito para modo texto.

---

## Recomendação

| Cenário                               | Opção Recomendada                                    |
| ------------------------------------- | ---------------------------------------------------- |
| **Servidor Linux único**              | ACBrMonitorConsole (Opção 1)                         |
| **Controle total / sem dependências** | sped-nfe PHP (Opção 2)                               |
| **Migração gradual**                  | ACBrMonitorConsole primeiro, avaliar sped-nfe depois |

**Abordagem**:

1. **Abstrair atrás de interface de serviço**
   - Fácil trocar implementações
   - Testar com mocks

2. **Começar com ACBrMonitorConsole**
   - Já conhecemos o ACBr
   - Configuração similar ao atual
   - Roda no mesmo servidor Linux

3. **Avaliar sped-nfe** se:
   - Quiser eliminar dependência do ACBr
   - Precisar de mais controle sobre o código
   - Preferir solução 100% PHP

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

### Implementação ACBrMonitorConsole

```php
// app/Services/NFe/AcbrNfeService.php
namespace App\Services\NFe;

use App\Contracts\NfeServiceInterface;
use App\DTOs\NfeResult;
use App\Models\Nfe;

class AcbrNfeService implements NfeServiceInterface
{
    private string $host;
    private int $port;
    private int $timeout;

    public function __construct()
    {
        $this->host = config('nfe.acbr.host', 'localhost');
        $this->port = config('nfe.acbr.port', 3434);
        $this->timeout = config('nfe.acbr.timeout', 30);
    }

    public function emitir(Nfe $nfe): NfeResult
    {
        // Gerar XML e salvar em arquivo
        $xmlPath = $this->gerarXml($nfe);

        // Enviar comando para ACBrMonitorConsole
        $response = $this->enviarComando("NFE.EnviarNFe(\"{$xmlPath}\", 1, 1)");

        return $this->parseResponse($response);
    }

    public function cancelar(Nfe $nfe, string $justificativa): NfeResult
    {
        $response = $this->enviarComando(
            "NFE.Cancelar(\"{$nfe->chave}\", \"{$justificativa}\")"
        );

        return $this->parseResponse($response);
    }

    public function consultar(string $chave): NfeResult
    {
        $response = $this->enviarComando("NFE.ConsultarNFe(\"{$chave}\")");

        return $this->parseResponse($response);
    }

    public function inutilizar(int $inicio, int $fim, string $justificativa): NfeResult
    {
        $response = $this->enviarComando(
            "NFE.InutilizarNFe(\"{$inicio}\", \"{$fim}\", \"{$justificativa}\")"
        );

        return $this->parseResponse($response);
    }

    public function cartaCorrecao(Nfe $nfe, string $correcao): NfeResult
    {
        $response = $this->enviarComando(
            "NFE.EnviarEvento(\"{$nfe->chave}\", \"CCE\", \"{$correcao}\")"
        );

        return $this->parseResponse($response);
    }

    private function enviarComando(string $comando): string
    {
        $socket = @fsockopen($this->host, $this->port, $errno, $errstr, $this->timeout);

        if (!$socket) {
            throw new \Exception("Falha ao conectar ao ACBr: {$errstr} ({$errno})");
        }

        fwrite($socket, $comando . "\r\n");

        $response = '';
        stream_set_timeout($socket, $this->timeout);

        while (!feof($socket)) {
            $response .= fread($socket, 4096);
            $info = stream_get_meta_data($socket);
            if ($info['timed_out']) {
                throw new \Exception("Timeout aguardando resposta do ACBr");
            }
        }

        fclose($socket);

        return trim($response);
    }

    private function parseResponse(string $response): NfeResult
    {
        // ACBr retorna OK: ou ERRO: no início
        if (str_starts_with($response, 'OK:')) {
            $data = $this->parseOkResponse($response);
            return new NfeResult(
                success: true,
                protocolo: $data['protocolo'] ?? null,
                chave: $data['chave'] ?? null,
                xml: $data['xml'] ?? null,
            );
        }

        return new NfeResult(
            success: false,
            erro: str_replace('ERRO:', '', $response),
        );
    }

    private function gerarXml(Nfe $nfe): string
    {
        // Implementar geração do XML conforme layout NFe
        $xmlPath = storage_path("app/nfe/{$nfe->id}.xml");
        // ... gerar XML
        return $xmlPath;
    }

    private function parseOkResponse(string $response): array
    {
        // Parse da resposta OK do ACBr
        // Formato varia por comando
        return [];
    }
}
```

### Registro do Service Provider

```php
// app/Providers/NfeServiceProvider.php
namespace App\Providers;

use App\Contracts\NfeServiceInterface;
use App\Services\NFe\AcbrNfeService;
use App\Services\NFe\SpedNfeService;
use App\Services\NFe\MockNfeService;

class NfeServiceProvider extends ServiceProvider
{
    public function register()
    {
        $this->app->bind(NfeServiceInterface::class, function ($app) {
            return match (config('nfe.driver')) {
                'acbr' => new AcbrNfeService(),
                'sped' => new SpedNfeService(),
                'mock' => new MockNfeService(), // para testes
                default => throw new \Exception('Driver NFe inválido'),
            };
        });
    }
}
```

### Configuração

```php
// config/nfe.php
return [
    'driver' => env('NFE_DRIVER', 'acbr'), // 'acbr' ou 'sped'

    'acbr' => [
        'host' => env('ACBR_HOST', 'localhost'),
        'port' => env('ACBR_PORT', 3434),
        'timeout' => env('ACBR_TIMEOUT', 30),
    ],

    'certificado' => [
        'path' => env('NFE_CERT_PATH'),
        'password' => env('NFE_CERT_PASSWORD'),
    ],

    'ambiente' => env('NFE_AMBIENTE', 'homologacao'), // 'homologacao' ou 'producao'
];
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

## Módulo DFe (Download de NFe) - Servidor

### Problema Atual

No sistema legado, o download automático de NFes da SEFAZ roda **dentro do app desktop C++**:

```cpp
// widgetnfedistribuicao.cpp - HACK atual
QTimer timer;
connect(&timer, &QTimer::timeout, this, &WidgetNFeDistribuicao::downloadAutomatico);
timer.start(0min);
```

**Problemas**:

- Depende do PC do usuário estar ligado
- Depende do app estar aberto
- Timer reinicia se app reiniciar
- ACBrMonitorPlus precisa estar rodando no PC do usuário
- Não confiável para operação 24/7

### Solução: Serviço no Servidor

```text
┌─────────────────────────────────────────────────────────────┐
│                      SERVIDOR LINUX                         │
│                                                             │
│  ┌─────────────┐     ┌───────────────────┐     ┌─────────┐ │
│  │  Laravel    │────►│ ACBrMonitorConsole│────►│  SEFAZ  │ │
│  │  Scheduler  │◄────│   (localhost)     │◄────│   API   │ │
│  └─────────────┘     └───────────────────┘     └─────────┘ │
│        │                                                    │
│        ▼                                                    │
│  ┌─────────────┐                                           │
│  │  PostgreSQL │                                           │
│  │  (NFes)     │                                           │
│  └─────────────┘                                           │
└─────────────────────────────────────────────────────────────┘
```

### Fluxo DFe (Distribuição de Documentos Fiscais)

```text
┌──────────────────────────────────────────────────────────────────┐
│                        FLUXO DFe                                  │
├──────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Scheduler dispara job a cada X minutos                       │
│           │                                                      │
│           ▼                                                      │
│  2. Para cada CNPJ configurado:                                  │
│           │                                                      │
│           ▼                                                      │
│  3. Verifica se pode consultar (proximaConsultaPermitida)        │
│           │                                                      │
│           ▼                                                      │
│  4. Chama ACBr: NFe.DistribuicaoDFePorUltNSU(UF, CNPJ, ultNSU)  │
│           │                                                      │
│           ▼                                                      │
│  5. Processa resposta:                                           │
│      - Salva resumos/XMLs no banco                               │
│      - Atualiza ultimoNSU/maximoNSU                              │
│           │                                                      │
│           ▼                                                      │
│  6. Enquanto ultimoNSU < maximoNSU, repete passo 4               │
│           │                                                      │
│           ▼                                                      │
│  7. Envia eventos de manifestação (Ciência, Confirmação, etc.)   │
│           │                                                      │
│           ▼                                                      │
│  8. Define próxima consulta permitida:                           │
│      - Sem documentos: +65 min                                   │
│      - Com documentos: +20 min                                   │
│      - Consumo indevido: +65 min                                 │
│                                                                  │
└──────────────────────────────────────────────────────────────────┘
```

### Implementação Laravel - DFe

#### Serviço DFe

```php
// app/Services/NFe/DfeService.php
namespace App\Services\NFe;

use App\Models\Loja;
use App\Models\Nfe;
use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Log;

class DfeService
{
    public function __construct(
        private AcbrNfeService $acbr
    ) {}

    /**
     * Consulta NFes para todas as lojas configuradas
     */
    public function consultarTodasLojas(): void
    {
        $lojas = Loja::query()
            ->where('desativado', false)
            ->whereNotNull('cnpj')
            ->where(function ($q) {
                $q->whereNull('proxima_consulta_permitida')
                  ->orWhere('proxima_consulta_permitida', '<=', now());
            })
            ->get();

        foreach ($lojas as $loja) {
            try {
                $this->consultarLoja($loja);
            } catch (\Exception $e) {
                Log::error("DFe error for CNPJ {$loja->cnpj}: {$e->getMessage()}");
                // Continua para próxima loja
            }
        }
    }

    /**
     * Consulta NFes para uma loja específica
     */
    public function consultarLoja(Loja $loja): void
    {
        $cnpj = preg_replace('/\D/', '', $loja->cnpj);

        do {
            $response = $this->acbr->distribuicaoDFe(
                uf: '35', // TODO: parametrizar
                cnpj: $cnpj,
                ultNSU: $loja->ultimo_nsu
            );

            if (!$response->success) {
                $this->tratarErro($loja, $response->erro);
                return;
            }

            DB::transaction(function () use ($loja, $response) {
                $this->processarResposta($loja, $response);
            });

        } while ($loja->ultimo_nsu < $loja->maximo_nsu);

        // Enviar eventos pendentes
        $this->enviarEventosPendentes($loja);

        // Atualizar próxima consulta
        $this->atualizarProximaConsulta($loja, $response);
    }

    private function processarResposta(Loja $loja, DfeResponse $response): void
    {
        // Atualizar NSUs
        $loja->update([
            'ultimo_nsu' => $response->ultNSU,
            'maximo_nsu' => $response->maxNSU,
        ]);

        // Processar cada documento
        foreach ($response->documentos as $doc) {
            $this->processarDocumento($loja, $doc);
        }
    }

    private function processarDocumento(Loja $loja, object $doc): void
    {
        $exists = Nfe::where('chave_acesso', $doc->chaveAcesso)->exists();

        if (!$exists) {
            Nfe::create([
                'loja_id' => $loja->id,
                'tipo' => 'ENTRADA',
                'numero' => substr($doc->chaveAcesso, 25, 9),
                'chave_acesso' => $doc->chaveAcesso,
                'cnpj_emitente' => $doc->cnpjEmitente,
                'emitente' => $doc->nomeEmitente,
                'valor_total' => $doc->valor,
                'xml' => $doc->xml,
                'status' => $doc->schema === 'procNFe' ? 'AUTORIZADA' : 'RESUMO',
                'status_distribuicao' => 'DESCONHECIDO',
                'nsu' => $doc->nsu,
                'data_emissao' => $doc->dataEmissao,
                'pendente_ciencia' => $doc->schema !== 'procNFe',
            ]);
        } elseif ($doc->schema === 'procNFe') {
            // Atualizar resumo com XML completo
            Nfe::where('chave_acesso', $doc->chaveAcesso)
                ->where('status', 'RESUMO')
                ->update([
                    'xml' => $doc->xml,
                    'status' => 'AUTORIZADA',
                ]);
        }
    }

    private function enviarEventosPendentes(Loja $loja): void
    {
        // Ciência
        $pendentes = Nfe::where('loja_id', $loja->id)
            ->where('pendente_ciencia', true)
            ->limit(20)
            ->get();

        if ($pendentes->isNotEmpty()) {
            $this->acbr->enviarEventoManifestacao(
                $pendentes->pluck('chave_acesso')->toArray(),
                'CIENCIA'
            );

            $pendentes->each->update(['pendente_ciencia' => false]);
        }

        // Confirmação, Desconhecimento, etc. (similar)
    }

    private function tratarErro(Loja $loja, string $erro): void
    {
        if (str_contains($erro, 'Consumo Indevido')) {
            $loja->update([
                'proxima_consulta_permitida' => now()->addMinutes(65),
            ]);
            Log::warning("Consumo Indevido para {$loja->cnpj}, próxima consulta em 65min");
        }
    }

    private function atualizarProximaConsulta(Loja $loja, DfeResponse $response): void
    {
        $minutos = $response->documentosEncontrados > 0 ? 20 : 65;

        $loja->update([
            'proxima_consulta_permitida' => now()->addMinutes($minutos),
            'ultima_consulta_nsu' => now(),
        ]);
    }
}
```

#### Job Agendado

```php
// app/Jobs/ConsultarDFeJob.php
namespace App\Jobs;

use App\Services\NFe\DfeService;
use Illuminate\Bus\Queueable;
use Illuminate\Contracts\Queue\ShouldQueue;
use Illuminate\Foundation\Bus\Dispatchable;
use Illuminate\Queue\InteractsWithQueue;
use Illuminate\Queue\SerializesModels;

class ConsultarDFeJob implements ShouldQueue
{
    use Dispatchable, InteractsWithQueue, Queueable, SerializesModels;

    public int $tries = 1; // Não retentar - próxima execução do scheduler cuidará
    public int $timeout = 300; // 5 minutos max

    public function handle(DfeService $dfeService): void
    {
        $dfeService->consultarTodasLojas();
    }
}
```

#### Scheduler

```php
// app/Console/Kernel.php
protected function schedule(Schedule $schedule): void
{
    // DFe - Consultar a cada 5 minutos, o job verifica internamente quais lojas podem consultar
    $schedule->job(new ConsultarDFeJob)
        ->everyFiveMinutes()
        ->withoutOverlapping()
        ->runInBackground();

    // Auto-confirmar NFes antigas (evitar multa por não manifestar)
    $schedule->job(new AutoConfirmarNFeAntigasJob)
        ->dailyAt('06:00');

    // Retentar emissões falhas
    $schedule->job(new RetentarNFeRejeitadasJob)
        ->everyThirtyMinutes();
}
```

#### Command para Debug

```php
// app/Console/Commands/DfeConsultarCommand.php
namespace App\Console\Commands;

use App\Models\Loja;
use App\Services\NFe\DfeService;
use Illuminate\Console\Command;

class DfeConsultarCommand extends Command
{
    protected $signature = 'dfe:consultar {--loja= : ID da loja específica}';
    protected $description = 'Consulta NFes da SEFAZ (DFe)';

    public function handle(DfeService $dfeService): int
    {
        if ($lojaId = $this->option('loja')) {
            $loja = Loja::findOrFail($lojaId);
            $this->info("Consultando DFe para loja: {$loja->razao_social}");
            $dfeService->consultarLoja($loja);
        } else {
            $this->info('Consultando DFe para todas as lojas...');
            $dfeService->consultarTodasLojas();
        }

        $this->info('Consulta concluída!');
        return Command::SUCCESS;
    }
}
```

### Migração do Sistema Legado

| Fase | Ação                                                   |
| ---- | ------------------------------------------------------ |
| 1    | Instalar ACBrMonitorConsole no servidor Linux          |
| 2    | Implementar DfeService + Job no Laravel                |
| 3    | Desabilitar timer no C++ (`User/monitorarNFe = false`) |
| 4    | Ativar scheduler Laravel (`php artisan schedule:work`) |
| 5    | Monitorar logs por 1 semana                            |
| 6    | Remover código DFe do C++                              |

### Modelo de Dados

```sql
-- Adicionar colunas na tabela loja
ALTER TABLE lojas ADD COLUMN ultimo_nsu INTEGER DEFAULT 0;
ALTER TABLE lojas ADD COLUMN maximo_nsu INTEGER DEFAULT 0;
ALTER TABLE lojas ADD COLUMN proxima_consulta_permitida TIMESTAMP;
ALTER TABLE lojas ADD COLUMN ultima_consulta_nsu TIMESTAMP;

-- Adicionar colunas na tabela nfe
ALTER TABLE nfes ADD COLUMN nsu INTEGER;
ALTER TABLE nfes ADD COLUMN status_distribuicao VARCHAR(20);
ALTER TABLE nfes ADD COLUMN pendente_ciencia BOOLEAN DEFAULT FALSE;
ALTER TABLE nfes ADD COLUMN pendente_confirmacao BOOLEAN DEFAULT FALSE;
ALTER TABLE nfes ADD COLUMN pendente_desconhecimento BOOLEAN DEFAULT FALSE;
ALTER TABLE nfes ADD COLUMN pendente_nao_realizada BOOLEAN DEFAULT FALSE;
```

### Tarefas Agendadas (Resumo)

```php
// Consultar DFe (download de NFes)
$schedule->job(new ConsultarDFeJob)->everyFiveMinutes()->withoutOverlapping();

// Auto-confirmar NFes antigas
$schedule->job(new AutoConfirmarNFeAntigasJob)->dailyAt('06:00');

// Retentar emissões falhas
$schedule->job(new RetentarNFeRejeitadasJob)->everyThirtyMinutes();

// Consultar status de NFes pendentes
$schedule->job(new ConsultarNFePendentesJob)->everyFiveMinutes();
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
