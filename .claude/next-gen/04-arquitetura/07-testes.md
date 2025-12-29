# Estratégia de Testes

> Status: **Aprovado**
> Última atualização: 2025-12-28

---

## Visão Geral

Este documento define a estratégia de testes para o ERP Staccato em Laravel, estabelecendo padrões, ferramentas e métricas de qualidade.

### Objetivos

| Objetivo | Meta |
|----------|------|
| Cobertura de código | ≥ 80% em Services/Actions |
| Cobertura crítica | 100% em regras de negócio |
| Tempo de execução (unit) | < 30 segundos |
| Tempo de execução (full) | < 10 minutos |
| Flaky tests | 0% tolerância |

### Pirâmide de Testes

```text
         ▲
        /E2E\         (5%)  Cypress/Playwright
       /─────\
      / Integração    (20%) Feature tests
     /───────────\
    /    Unit          (75%) PHPUnit
   /───────────────\
  ─────────────────────
```

---

## Estrutura de Diretórios

```text
tests/
├── Unit/
│   ├── Actions/           # Actions isolados
│   ├── Services/          # Services isolados
│   ├── Models/            # Model tests (casts, scopes)
│   ├── Rules/             # Custom validation rules
│   ├── ValueObjects/      # Value Objects
│   └── Helpers/           # Helper functions
├── Feature/
│   ├── Api/               # API endpoints
│   ├── Http/              # Controllers web
│   ├── Jobs/              # Queue jobs
│   ├── Console/           # Artisan commands
│   └── Workflows/         # Multi-step business flows
├── Integration/
│   ├── Acbr/              # ACBr integration
│   ├── Cnab/              # CNAB file generation
│   └── External/          # APIs externas
├── E2E/
│   └── cypress/           # Testes Cypress
├── Fixtures/
│   └── test_data.sql      # Dados de teste
├── Support/
│   ├── Traits/            # Test traits reutilizáveis
│   ├── Factories/         # Custom factory states
│   └── Mocks/             # Mock classes
└── TestCase.php           # Base test case
```

---

## Ferramentas

### Stack Principal

| Ferramenta | Uso | Versão |
|------------|-----|--------|
| PHPUnit | Unit/Feature tests | 11.x |
| Pest | Sintaxe alternativa | 3.x |
| Cypress | E2E browser tests | 13.x |
| Laravel Dusk | Browser tests Laravel | 8.x |
| Mockery | Mocking framework | 1.6.x |
| Faker | Geração de dados | 1.23.x |

### Pacotes Auxiliares

```json
{
    "require-dev": {
        "phpunit/phpunit": "^11.0",
        "pestphp/pest": "^3.0",
        "pestphp/pest-plugin-laravel": "^3.0",
        "mockery/mockery": "^1.6",
        "fakerphp/faker": "^1.23",
        "laravel/dusk": "^8.0",
        "spatie/laravel-ray": "^1.35",
        "nunomaduro/collision": "^8.0",
        "larastan/larastan": "^2.9"
    }
}
```

---

## Testes Unitários

### Padrão de Teste

```php
// tests/Unit/Services/DescontoServiceTest.php
namespace Tests\Unit\Services;

use App\Services\DescontoService;
use App\ValueObjects\Money;
use PHPUnit\Framework\TestCase;

class DescontoServiceTest extends TestCase
{
    private DescontoService $service;

    protected function setUp(): void
    {
        parent::setUp();
        $this->service = new DescontoService();
    }

    public function test_calcula_desconto_nivel1_corretamente(): void
    {
        $valorOriginal = Money::fromCents(100000); // R$ 1.000,00
        $percentual = 5.0;

        $resultado = $this->service->calcular($valorOriginal, $percentual);

        $this->assertEquals(95000, $resultado->cents());
        $this->assertEquals('R$ 950,00', $resultado->formatted());
    }

    public function test_desconto_maximo_respeitado(): void
    {
        $this->expectException(DescontoExcedidoException::class);

        $this->service->calcular(
            Money::fromCents(100000),
            percentual: 50.0,
            limiteMaximo: 15.0
        );
    }

    /**
     * @dataProvider descontosProvider
     */
    public function test_calcula_descontos_diversos(
        int $valorCents,
        float $percentual,
        int $esperadoCents
    ): void {
        $resultado = $this->service->calcular(
            Money::fromCents($valorCents),
            $percentual
        );

        $this->assertEquals($esperadoCents, $resultado->cents());
    }

    public static function descontosProvider(): array
    {
        return [
            'sem desconto' => [10000, 0.0, 10000],
            '5% desconto' => [10000, 5.0, 9500],
            '10% desconto' => [10000, 10.0, 9000],
            '15% desconto' => [10000, 15.0, 8500],
        ];
    }
}
```

### Testes de Value Objects

```php
// tests/Unit/ValueObjects/CpfTest.php
namespace Tests\Unit\ValueObjects;

use App\ValueObjects\Cpf;
use App\Exceptions\InvalidCpfException;
use PHPUnit\Framework\TestCase;

class CpfTest extends TestCase
{
    public function test_cria_cpf_valido(): void
    {
        $cpf = new Cpf('529.982.247-25');

        $this->assertEquals('52998224725', $cpf->value());
        $this->assertEquals('529.982.247-25', $cpf->formatted());
    }

    public function test_rejeita_cpf_invalido(): void
    {
        $this->expectException(InvalidCpfException::class);

        new Cpf('111.111.111-11');
    }

    public function test_aceita_cpf_sem_formatacao(): void
    {
        $cpf = new Cpf('52998224725');

        $this->assertEquals('52998224725', $cpf->value());
    }

    /**
     * @dataProvider cpfsInvalidosProvider
     */
    public function test_rejeita_cpfs_invalidos(string $cpf): void
    {
        $this->expectException(InvalidCpfException::class);

        new Cpf($cpf);
    }

    public static function cpfsInvalidosProvider(): array
    {
        return [
            'todos zeros' => ['000.000.000-00'],
            'digitos iguais' => ['111.111.111-11'],
            'digito errado' => ['529.982.247-26'],
            'muito curto' => ['123.456.789'],
            'muito longo' => ['123.456.789-001'],
        ];
    }
}
```

### Testes de Models

```php
// tests/Unit/Models/ClienteTest.php
namespace Tests\Unit\Models;

use App\Models\Cliente;
use App\Enums\TipoPessoa;
use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;

class ClienteTest extends TestCase
{
    use RefreshDatabase;

    public function test_tipo_pessoa_e_detectado_automaticamente(): void
    {
        $pf = Cliente::factory()->create(['cpf_cnpj' => '52998224725']);
        $pj = Cliente::factory()->create(['cpf_cnpj' => '11222333000181']);

        $this->assertEquals(TipoPessoa::FISICA, $pf->tipo_pessoa);
        $this->assertEquals(TipoPessoa::JURIDICA, $pj->tipo_pessoa);
    }

    public function test_scope_com_credito_filtra_corretamente(): void
    {
        Cliente::factory()->create(['credito' => 100]);
        Cliente::factory()->create(['credito' => 0]);
        Cliente::factory()->create(['credito' => 50]);

        $resultado = Cliente::comCredito()->get();

        $this->assertCount(2, $resultado);
    }

    public function test_soft_delete_funciona(): void
    {
        $cliente = Cliente::factory()->create();

        $cliente->delete();

        $this->assertSoftDeleted($cliente);
        $this->assertNull(Cliente::find($cliente->id));
        $this->assertNotNull(Cliente::withTrashed()->find($cliente->id));
    }

    public function test_relacionamento_enderecos(): void
    {
        $cliente = Cliente::factory()
            ->hasEnderecos(3)
            ->create();

        $this->assertCount(3, $cliente->enderecos);
        $this->assertInstanceOf(\App\Models\Endereco::class, $cliente->enderecos->first());
    }
}
```

---

## Testes de Feature

### Testes de API

```php
// tests/Feature/Api/ClienteApiTest.php
namespace Tests\Feature\Api;

use App\Models\Cliente;
use App\Models\User;
use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;

class ClienteApiTest extends TestCase
{
    use RefreshDatabase;

    private User $user;

    protected function setUp(): void
    {
        parent::setUp();
        $this->user = User::factory()->create();
    }

    public function test_lista_clientes_requer_autenticacao(): void
    {
        $response = $this->getJson('/api/v1/clientes');

        $response->assertUnauthorized();
    }

    public function test_lista_clientes_paginados(): void
    {
        Cliente::factory()
            ->count(30)
            ->create(['loja_id' => $this->user->loja_id]);

        $response = $this->actingAs($this->user)
            ->getJson('/api/v1/clientes?per_page=10');

        $response
            ->assertOk()
            ->assertJsonCount(10, 'data')
            ->assertJsonStructure([
                'data' => [
                    '*' => [
                        'id',
                        'type',
                        'attributes' => ['nome', 'cpf_cnpj', 'email'],
                    ],
                ],
                'meta' => ['current_page', 'per_page', 'total'],
                'links',
            ]);
    }

    public function test_cria_cliente_com_dados_validos(): void
    {
        $payload = [
            'nome_razao' => 'João Silva',
            'cpf_cnpj' => '529.982.247-25',
            'email' => 'joao@email.com',
            'tel' => '(11) 99999-9999',
        ];

        $response = $this->actingAs($this->user)
            ->postJson('/api/v1/clientes', $payload);

        $response
            ->assertCreated()
            ->assertJsonPath('data.attributes.nome', 'João Silva');

        $this->assertDatabaseHas('cliente', [
            'nome_razao' => 'João Silva',
            'loja_id' => $this->user->loja_id,
        ]);
    }

    public function test_valida_cpf_invalido(): void
    {
        $payload = [
            'nome_razao' => 'João Silva',
            'cpf_cnpj' => '111.111.111-11',
        ];

        $response = $this->actingAs($this->user)
            ->postJson('/api/v1/clientes', $payload);

        $response
            ->assertUnprocessable()
            ->assertJsonValidationErrors(['cpf_cnpj']);
    }

    public function test_cliente_isolado_por_loja(): void
    {
        $clienteOutraLoja = Cliente::factory()->create(['loja_id' => 999]);

        $response = $this->actingAs($this->user)
            ->getJson("/api/v1/clientes/{$clienteOutraLoja->id}");

        $response->assertNotFound();
    }

    public function test_atualiza_cliente(): void
    {
        $cliente = Cliente::factory()->create(['loja_id' => $this->user->loja_id]);

        $response = $this->actingAs($this->user)
            ->putJson("/api/v1/clientes/{$cliente->id}", [
                'nome_razao' => 'Nome Atualizado',
            ]);

        $response->assertOk();
        $this->assertDatabaseHas('cliente', [
            'id' => $cliente->id,
            'nome_razao' => 'Nome Atualizado',
        ]);
    }

    public function test_deleta_cliente(): void
    {
        $cliente = Cliente::factory()->create(['loja_id' => $this->user->loja_id]);

        $response = $this->actingAs($this->user)
            ->deleteJson("/api/v1/clientes/{$cliente->id}");

        $response->assertNoContent();
        $this->assertSoftDeleted('cliente', ['id' => $cliente->id]);
    }
}
```

### Testes de Workflow

```php
// tests/Feature/Workflows/VendaWorkflowTest.php
namespace Tests\Feature\Workflows;

use App\Models\{User, Cliente, Produto, Orcamento, Venda, Estoque};
use App\Enums\{OrcamentoStatus, VendaStatus};
use App\Services\VendaService;
use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;

class VendaWorkflowTest extends TestCase
{
    use RefreshDatabase;

    private User $vendedor;
    private Cliente $cliente;
    private Produto $produto;

    protected function setUp(): void
    {
        parent::setUp();

        $this->vendedor = User::factory()->vendedor()->create();
        $this->cliente = Cliente::factory()->create(['loja_id' => $this->vendedor->loja_id]);
        $this->produto = Produto::factory()->create();

        // Seed estoque
        Estoque::factory()->create([
            'produto_id' => $this->produto->id,
            'quantidade' => 100,
            'loja_id' => $this->vendedor->loja_id,
        ]);
    }

    public function test_fluxo_completo_orcamento_para_venda(): void
    {
        // 1. Criar orçamento
        $orcamento = Orcamento::factory()
            ->hasItens(1, ['produto_id' => $this->produto->id, 'quantidade' => 2])
            ->create([
                'cliente_id' => $this->cliente->id,
                'loja_id' => $this->vendedor->loja_id,
                'status' => OrcamentoStatus::ORCAMENTO,
            ]);

        // 2. Converter em venda
        $response = $this->actingAs($this->vendedor)
            ->postJson("/api/v1/orcamentos/{$orcamento->id}/converter");

        $response->assertOk();

        // 3. Verificar venda criada
        $venda = Venda::where('orcamento_id', $orcamento->id)->first();
        $this->assertNotNull($venda);
        $this->assertEquals(VendaStatus::PENDENTE, $venda->status);

        // 4. Verificar orçamento atualizado
        $orcamento->refresh();
        $this->assertEquals(OrcamentoStatus::FECHADO, $orcamento->status);

        // 5. Verificar estoque reservado
        $estoque = Estoque::where('produto_id', $this->produto->id)->first();
        $this->assertEquals(98, $estoque->disponivel);
        $this->assertEquals(2, $estoque->reservado);
    }

    public function test_nao_converte_orcamento_sem_estoque(): void
    {
        // Criar orçamento com quantidade maior que estoque
        $orcamento = Orcamento::factory()
            ->hasItens(1, ['produto_id' => $this->produto->id, 'quantidade' => 999])
            ->create([
                'cliente_id' => $this->cliente->id,
                'loja_id' => $this->vendedor->loja_id,
            ]);

        $response = $this->actingAs($this->vendedor)
            ->postJson("/api/v1/orcamentos/{$orcamento->id}/converter");

        $response
            ->assertUnprocessable()
            ->assertJsonPath('error.code', 'INSUFFICIENT_STOCK');
    }

    public function test_cancelamento_de_venda_libera_estoque(): void
    {
        // Setup: criar venda com estoque reservado
        $venda = Venda::factory()
            ->hasItens(1, ['produto_id' => $this->produto->id, 'quantidade' => 5])
            ->create([
                'cliente_id' => $this->cliente->id,
                'loja_id' => $this->vendedor->loja_id,
                'status' => VendaStatus::PENDENTE,
            ]);

        $estoque = Estoque::where('produto_id', $this->produto->id)->first();
        $estoque->update(['disponivel' => 95, 'reservado' => 5]);

        // Cancelar venda
        $response = $this->actingAs($this->vendedor)
            ->postJson("/api/v1/vendas/{$venda->id}/cancelar", [
                'motivo' => 'Cliente desistiu',
            ]);

        $response->assertOk();

        // Verificar estoque liberado
        $estoque->refresh();
        $this->assertEquals(100, $estoque->disponivel);
        $this->assertEquals(0, $estoque->reservado);

        // Verificar venda cancelada
        $venda->refresh();
        $this->assertEquals(VendaStatus::CANCELADA, $venda->status);
    }
}
```

---

## Testes de Integração

### ACBr Integration

```php
// tests/Integration/Acbr/NfeTransmissaoTest.php
namespace Tests\Integration\Acbr;

use App\Services\Acbr\AcbrClient;
use App\Services\Nfe\NfeService;
use App\Models\{Nfe, Venda};
use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;

class NfeTransmissaoTest extends TestCase
{
    use RefreshDatabase;

    protected function setUp(): void
    {
        parent::setUp();

        // Skip se não estiver em ambiente de integração
        if (!config('services.acbr.integration_tests')) {
            $this->markTestSkipped('ACBr integration tests disabled');
        }
    }

    public function test_transmite_nfe_para_sefaz_homologacao(): void
    {
        $venda = Venda::factory()
            ->hasItens(2)
            ->create();

        $nfeService = app(NfeService::class);

        $nfe = $nfeService->gerarParaVenda($venda);
        $resultado = $nfeService->transmitir($nfe);

        $this->assertTrue($resultado->autorizada());
        $this->assertNotEmpty($resultado->protocolo);
        $this->assertEquals(100, $resultado->cstat);
    }

    public function test_consulta_status_servico_sefaz(): void
    {
        $client = app(AcbrClient::class);

        $resultado = $client->consultarStatusServico();

        $this->assertTrue($resultado->emOperacao());
        $this->assertNotEmpty($resultado->tempoMedio);
    }
}
```

### CNAB Integration

```php
// tests/Integration/Cnab/Cnab240Test.php
namespace Tests\Integration\Cnab;

use App\Services\Cnab\Cnab240Service;
use App\Models\{ContaReceber, Boleto};
use Tests\TestCase;
use Illuminate\Foundation\Testing\RefreshDatabase;
use Illuminate\Support\Facades\Storage;

class Cnab240Test extends TestCase
{
    use RefreshDatabase;

    public function test_gera_arquivo_remessa_itau(): void
    {
        $boletos = Boleto::factory()->count(3)->create();

        $service = app(Cnab240Service::class);
        $arquivo = $service->gerarRemessa($boletos, banco: 'itau');

        // Verificar estrutura do arquivo
        $linhas = explode("\n", $arquivo);

        // Header de arquivo (240 caracteres)
        $this->assertEquals(240, strlen(trim($linhas[0])));
        $this->assertEquals('341', substr($linhas[0], 0, 3)); // Código Itaú

        // Header de lote
        $this->assertEquals('1', substr($linhas[1], 7, 1)); // Tipo registro

        // Detalhes (Segmento P + Segmento Q para cada boleto)
        $this->assertCount(3 * 2 + 4, array_filter($linhas)); // 3 boletos * 2 segmentos + 4 headers/trailers
    }

    public function test_processa_retorno_cnab(): void
    {
        // Setup: criar boletos pendentes
        $boletos = Boleto::factory()->count(2)->create([
            'status' => 'pendente',
        ]);

        // Simular arquivo de retorno
        $retorno = $this->gerarArquivoRetornoMock($boletos);

        $service = app(Cnab240Service::class);
        $resultado = $service->processarRetorno($retorno);

        // Verificar processamento
        $this->assertEquals(2, $resultado->processados);
        $this->assertEquals(0, $resultado->erros);

        // Verificar baixa automática
        foreach ($boletos as $boleto) {
            $boleto->refresh();
            $this->assertEquals('pago', $boleto->status);
        }
    }

    private function gerarArquivoRetornoMock(iterable $boletos): string
    {
        // Gera arquivo de retorno mock para testes
        // ...
    }
}
```

---

## Testes E2E

### Configuração Cypress

```javascript
// cypress.config.js
const { defineConfig } = require('cypress');

module.exports = defineConfig({
  e2e: {
    baseUrl: 'http://localhost:8000',
    viewportWidth: 1280,
    viewportHeight: 720,
    video: false,
    screenshotOnRunFailure: true,
    defaultCommandTimeout: 10000,
    env: {
      apiUrl: 'http://localhost:8000/api/v1',
    },
    setupNodeEvents(on, config) {
      // Seed database before tests
      on('task', {
        seedDatabase() {
          // Run artisan migrate:fresh --seed
        },
        clearDatabase() {
          // Run artisan migrate:fresh
        },
      });
    },
  },
});
```

### Teste E2E de Login

```javascript
// cypress/e2e/auth/login.cy.js
describe('Login', () => {
  beforeEach(() => {
    cy.task('seedDatabase');
    cy.visit('/login');
  });

  it('faz login com credenciais válidas', () => {
    cy.get('[data-testid="email"]').type('admin@staccato.com.br');
    cy.get('[data-testid="password"]').type('senha123');
    cy.get('[data-testid="submit"]').click();

    cy.url().should('include', '/dashboard');
    cy.contains('Bem-vindo').should('be.visible');
  });

  it('mostra erro com credenciais inválidas', () => {
    cy.get('[data-testid="email"]').type('admin@staccato.com.br');
    cy.get('[data-testid="password"]').type('senhaerrada');
    cy.get('[data-testid="submit"]').click();

    cy.contains('Credenciais inválidas').should('be.visible');
    cy.url().should('include', '/login');
  });

  it('bloqueia após 5 tentativas', () => {
    for (let i = 0; i < 5; i++) {
      cy.get('[data-testid="email"]').clear().type('admin@staccato.com.br');
      cy.get('[data-testid="password"]').clear().type('senhaerrada');
      cy.get('[data-testid="submit"]').click();
    }

    cy.contains('Muitas tentativas').should('be.visible');
    cy.get('[data-testid="submit"]').should('be.disabled');
  });
});
```

### Teste E2E de Venda

```javascript
// cypress/e2e/vendas/criar-orcamento.cy.js
describe('Criar Orçamento', () => {
  beforeEach(() => {
    cy.task('seedDatabase');
    cy.login('vendedor@staccato.com.br', 'senha123');
    cy.visit('/orcamentos/novo');
  });

  it('cria orçamento completo', () => {
    // Selecionar cliente
    cy.get('[data-testid="cliente-select"]').click();
    cy.get('[data-testid="cliente-search"]').type('João');
    cy.contains('João Silva').click();

    // Adicionar produto
    cy.get('[data-testid="adicionar-produto"]').click();
    cy.get('[data-testid="produto-search"]').type('Mesa');
    cy.contains('Mesa Escritório').click();
    cy.get('[data-testid="quantidade"]').clear().type('2');
    cy.get('[data-testid="confirmar-item"]').click();

    // Verificar totais
    cy.get('[data-testid="subtotal"]').should('contain', 'R$ 1.000,00');

    // Salvar orçamento
    cy.get('[data-testid="salvar"]').click();

    // Verificar sucesso
    cy.contains('Orçamento criado').should('be.visible');
    cy.url().should('match', /\/orcamentos\/\d+/);
  });

  it('aplica desconto com autorização', () => {
    // Setup orçamento
    cy.criarOrcamentoBase();

    // Tentar desconto acima do limite
    cy.get('[data-testid="desconto-input"]').type('20');
    cy.get('[data-testid="aplicar-desconto"]').click();

    // Modal de autorização
    cy.get('[data-testid="modal-autorizacao"]').should('be.visible');
    cy.get('[data-testid="autorizador-user"]').type('gerente');
    cy.get('[data-testid="autorizador-pass"]').type('senha123');
    cy.get('[data-testid="confirmar-autorizacao"]').click();

    // Desconto aplicado
    cy.get('[data-testid="desconto-aplicado"]').should('contain', '20%');
  });
});

// cypress/support/commands.js
Cypress.Commands.add('login', (email, password) => {
  cy.session([email, password], () => {
    cy.visit('/login');
    cy.get('[data-testid="email"]').type(email);
    cy.get('[data-testid="password"]').type(password);
    cy.get('[data-testid="submit"]').click();
    cy.url().should('include', '/dashboard');
  });
});

Cypress.Commands.add('criarOrcamentoBase', () => {
  cy.get('[data-testid="cliente-select"]').click();
  cy.contains('João Silva').click();
  cy.get('[data-testid="adicionar-produto"]').click();
  cy.contains('Mesa Escritório').click();
  cy.get('[data-testid="confirmar-item"]').click();
});
```

---

## Factories

### Padrão de Factory

```php
// database/factories/ClienteFactory.php
namespace Database\Factories;

use App\Models\Cliente;
use App\Models\Loja;
use Illuminate\Database\Eloquent\Factories\Factory;

class ClienteFactory extends Factory
{
    protected $model = Cliente::class;

    public function definition(): array
    {
        $isPf = $this->faker->boolean(70);

        return [
            'loja_id' => Loja::factory(),
            'nome_razao' => $isPf
                ? $this->faker->name()
                : $this->faker->company(),
            'cpf_cnpj' => $isPf
                ? $this->faker->cpf(false)
                : $this->faker->cnpj(false),
            'email' => $this->faker->unique()->safeEmail(),
            'tel' => $this->faker->phoneNumber(),
            'credito' => 0,
            'created_at' => now(),
            'updated_at' => now(),
        ];
    }

    public function pessoaFisica(): static
    {
        return $this->state(fn() => [
            'nome_razao' => $this->faker->name(),
            'cpf_cnpj' => $this->faker->cpf(false),
        ]);
    }

    public function pessoaJuridica(): static
    {
        return $this->state(fn() => [
            'nome_razao' => $this->faker->company(),
            'cpf_cnpj' => $this->faker->cnpj(false),
        ]);
    }

    public function comCredito(float $valor = 1000.00): static
    {
        return $this->state(fn() => [
            'credito' => $valor,
        ]);
    }

    public function inadimplente(): static
    {
        return $this->state(fn() => [
            'inadimplente' => true,
            'data_inadimplencia' => now()->subDays(30),
        ]);
    }
}
```

### Factory de Venda

```php
// database/factories/VendaFactory.php
namespace Database\Factories;

use App\Models\{Venda, Cliente, User, Loja};
use App\Enums\VendaStatus;
use Illuminate\Database\Eloquent\Factories\Factory;

class VendaFactory extends Factory
{
    protected $model = Venda::class;

    public function definition(): array
    {
        return [
            'loja_id' => Loja::factory(),
            'cliente_id' => Cliente::factory(),
            'vendedor_id' => User::factory(),
            'status' => VendaStatus::PENDENTE,
            'valor_produtos' => 0,
            'valor_frete' => 0,
            'valor_desconto' => 0,
            'valor_total' => 0,
            'data_venda' => now(),
        ];
    }

    public function pendente(): static
    {
        return $this->state(fn() => ['status' => VendaStatus::PENDENTE]);
    }

    public function confirmada(): static
    {
        return $this->state(fn() => ['status' => VendaStatus::CONFIRMADA]);
    }

    public function entregue(): static
    {
        return $this->state(fn() => [
            'status' => VendaStatus::ENTREGUE,
            'data_entrega' => now(),
        ]);
    }

    public function cancelada(): static
    {
        return $this->state(fn() => [
            'status' => VendaStatus::CANCELADA,
            'motivo_cancelamento' => $this->faker->sentence(),
        ]);
    }

    public function configure(): static
    {
        return $this->afterCreating(function (Venda $venda) {
            $venda->recalcularTotais();
        });
    }
}
```

---

## Mocking

### Mock de Serviços Externos

```php
// tests/Support/Mocks/AcbrClientMock.php
namespace Tests\Support\Mocks;

use App\Contracts\AcbrClientInterface;
use App\DTOs\Nfe\{NfeResult, StatusServicoResult};

class AcbrClientMock implements AcbrClientInterface
{
    public bool $shouldFail = false;
    public array $responses = [];

    public function transmitirNfe(string $xml): NfeResult
    {
        if ($this->shouldFail) {
            return new NfeResult(
                autorizada: false,
                cstat: 539,
                motivo: 'Rejeição: Duplicidade de NF-e',
            );
        }

        return new NfeResult(
            autorizada: true,
            cstat: 100,
            protocolo: '135250000123456',
            chave: '35250112345678000199550010000123451234567890',
        );
    }

    public function consultarStatusServico(): StatusServicoResult
    {
        return new StatusServicoResult(
            emOperacao: true,
            tempoMedio: 1.5,
            cstat: 107,
        );
    }
}

// tests/Feature/Nfe/NfeTransmissaoTest.php
public function test_transmite_nfe_com_sucesso(): void
{
    $this->app->bind(AcbrClientInterface::class, AcbrClientMock::class);

    $nfe = Nfe::factory()->create();

    $response = $this->actingAs($this->user)
        ->postJson("/api/v1/nfe/{$nfe->id}/transmitir");

    $response->assertOk()
        ->assertJsonPath('data.attributes.status', 'autorizada');
}

public function test_trata_rejeicao_sefaz(): void
{
    $mock = new AcbrClientMock();
    $mock->shouldFail = true;
    $this->app->instance(AcbrClientInterface::class, $mock);

    $nfe = Nfe::factory()->create();

    $response = $this->actingAs($this->user)
        ->postJson("/api/v1/nfe/{$nfe->id}/transmitir");

    $response->assertUnprocessable()
        ->assertJsonPath('error.code', 'NFE_REJECTED');
}
```

### Mock com Mockery

```php
// tests/Feature/Services/CepServiceTest.php
namespace Tests\Feature\Services;

use App\Services\CepService;
use App\Contracts\CepProviderInterface;
use Mockery;
use Tests\TestCase;

class CepServiceTest extends TestCase
{
    public function test_busca_cep_no_banco_local_primeiro(): void
    {
        // Seed CEP local
        \DB::table('cep')->insert([
            'cep' => '01310100',
            'logradouro' => 'Avenida Paulista',
            'bairro' => 'Bela Vista',
            'cidade' => 'São Paulo',
            'uf' => 'SP',
        ]);

        // Mock do provider externo (não deve ser chamado)
        $mock = Mockery::mock(CepProviderInterface::class);
        $mock->shouldNotReceive('buscar');
        $this->app->instance(CepProviderInterface::class, $mock);

        $service = app(CepService::class);
        $resultado = $service->buscar('01310-100');

        $this->assertEquals('Avenida Paulista', $resultado->logradouro);
    }

    public function test_fallback_para_api_externa(): void
    {
        $mock = Mockery::mock(CepProviderInterface::class);
        $mock->shouldReceive('buscar')
            ->once()
            ->with('01310100')
            ->andReturn([
                'logradouro' => 'Avenida Paulista',
                'bairro' => 'Bela Vista',
                'cidade' => 'São Paulo',
                'uf' => 'SP',
            ]);
        $this->app->instance(CepProviderInterface::class, $mock);

        $service = app(CepService::class);
        $resultado = $service->buscar('01310-100');

        $this->assertEquals('São Paulo', $resultado->cidade);
    }

    protected function tearDown(): void
    {
        Mockery::close();
        parent::tearDown();
    }
}
```

---

## CI/CD

### GitHub Actions

```yaml
# .github/workflows/tests.yml
name: Tests

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  unit-tests:
    runs-on: ubuntu-latest

    services:
      mysql:
        image: mysql:8.0
        env:
          MYSQL_ROOT_PASSWORD: secret
          MYSQL_DATABASE: erp_test
        ports:
          - 3306:3306
        options: >-
          --health-cmd="mysqladmin ping"
          --health-interval=10s
          --health-timeout=5s
          --health-retries=3

      redis:
        image: redis:7
        ports:
          - 6379:6379

    steps:
      - uses: actions/checkout@v4

      - name: Setup PHP
        uses: shivammathur/setup-php@v2
        with:
          php-version: '8.3'
          extensions: mbstring, mysql, redis
          coverage: xdebug

      - name: Install Composer dependencies
        run: composer install --no-progress --prefer-dist --optimize-autoloader

      - name: Copy .env
        run: cp .env.testing .env

      - name: Generate key
        run: php artisan key:generate

      - name: Run migrations
        run: php artisan migrate --force

      - name: Run PHPUnit tests
        run: php artisan test --parallel --coverage-clover coverage.xml

      - name: Upload coverage
        uses: codecov/codecov-action@v3
        with:
          files: coverage.xml

  e2e-tests:
    runs-on: ubuntu-latest
    needs: unit-tests

    steps:
      - uses: actions/checkout@v4

      - name: Setup PHP
        uses: shivammathur/setup-php@v2
        with:
          php-version: '8.3'

      - name: Install dependencies
        run: |
          composer install
          npm ci

      - name: Build assets
        run: npm run build

      - name: Start server
        run: php artisan serve &

      - name: Run Cypress
        uses: cypress-io/github-action@v6
        with:
          wait-on: 'http://localhost:8000'
          config-file: cypress.config.js

      - name: Upload screenshots
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: cypress-screenshots
          path: cypress/screenshots

  static-analysis:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Setup PHP
        uses: shivammathur/setup-php@v2
        with:
          php-version: '8.3'

      - name: Install dependencies
        run: composer install

      - name: Run PHPStan
        run: ./vendor/bin/phpstan analyse --error-format=github

      - name: Run Pint
        run: ./vendor/bin/pint --test
```

---

## Métricas e Cobertura

### Configuração PHPUnit

```xml
<!-- phpunit.xml -->
<?xml version="1.0" encoding="UTF-8"?>
<phpunit xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
         xsi:noNamespaceSchemaLocation="vendor/phpunit/phpunit/phpunit.xsd"
         bootstrap="vendor/autoload.php"
         colors="true"
         cacheResultFile=".phpunit.cache/test-results"
         executionOrder="depends,defects"
         failOnRisky="true"
         failOnWarning="true"
         beStrictAboutOutputDuringTests="true">
    <testsuites>
        <testsuite name="Unit">
            <directory>tests/Unit</directory>
        </testsuite>
        <testsuite name="Feature">
            <directory>tests/Feature</directory>
        </testsuite>
        <testsuite name="Integration">
            <directory>tests/Integration</directory>
        </testsuite>
    </testsuites>
    <source>
        <include>
            <directory>app</directory>
        </include>
        <exclude>
            <directory>app/Console</directory>
            <directory>app/Exceptions</directory>
            <directory>app/Providers</directory>
        </exclude>
    </source>
    <coverage>
        <report>
            <html outputDirectory="coverage-report"/>
            <clover outputFile="coverage.xml"/>
        </report>
    </coverage>
    <php>
        <env name="APP_ENV" value="testing"/>
        <env name="DB_CONNECTION" value="mysql"/>
        <env name="DB_DATABASE" value="erp_test"/>
        <env name="CACHE_DRIVER" value="array"/>
        <env name="QUEUE_CONNECTION" value="sync"/>
        <env name="SESSION_DRIVER" value="array"/>
    </php>
</phpunit>
```

### Cobertura Mínima por Módulo

| Módulo | Cobertura Mínima | Crítico |
|--------|------------------|---------|
| Services | 90% | Sim |
| Actions | 85% | Sim |
| Models | 75% | Não |
| Controllers | 70% | Não |
| Jobs | 85% | Sim |
| Rules | 100% | Sim |
| ValueObjects | 100% | Sim |

---

## Checklist de Testes

### Por Feature

- [ ] Testes unitários para Services
- [ ] Testes unitários para Value Objects
- [ ] Testes de API (endpoints)
- [ ] Testes de autorização
- [ ] Testes de validação
- [ ] Testes de edge cases
- [ ] Testes de integração (quando aplicável)

### Por Release

- [ ] Todos os testes passando
- [ ] Cobertura acima do mínimo
- [ ] Sem flaky tests
- [ ] Performance aceitável
- [ ] E2E críticos passando

---

## Documentos Relacionados

- [01-arquitetura.md](./01-arquitetura.md) - Arquitetura geral
- [06-api.md](./06-api.md) - Design de API
- [17-validacao.md](./17-validacao.md) - Estratégia de validação
