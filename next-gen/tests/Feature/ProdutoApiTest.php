<?php

namespace Tests\Feature;

use App\Models\Produto;
use App\Models\Fornecedor;
use App\Models\User;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class ProdutoApiTest extends TestCase
{
    use DatabaseTransactions;

    private User $user;
    private Fornecedor $fornecedor;

    protected function setUp(): void
    {
        parent::setUp();
        $this->user = User::factory()->create();
        $this->fornecedor = Fornecedor::factory()->create();
    }

    public function test_can_get_all_produtos(): void
    {
        Produto::factory()->count(3)->create(['fornecedor_id' => $this->fornecedor->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson('/api/cadastros/produtos');

        $response->assertOk()
            ->assertJsonStructure([
                'data' => [
                    '*' => ['id', 'codigo_comercial', 'descricao', 'preco_custo', 'created_at'],
                ],
                'meta',
            ]);
    }

    public function test_can_create_produto(): void
    {
        $data = [
            'fornecedor_id' => $this->fornecedor->id,
            'codigo_comercial' => 'PROD001',
            'codigo_barras' => '1234567890123',
            'descricao' => 'Produto de Teste',
            'descricao_curta' => 'Teste',
            'unidade' => 'UN',
            'unidades_por_caixa' => 10,
            'peso_kg' => 2.5,
            'ncm' => '12345678',
            'categoria' => 'Eletrônicos',
            'tem_lote' => true,
            'preco_custo' => 100.00,
            'preco_tabela' => 150.00,
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/produtos', $data);

        $response->assertCreated()
            ->assertJsonPath('data.codigo_comercial', 'PROD001')
            ->assertJsonPath('data.descricao', 'Produto de Teste');

        $this->assertDatabaseHas('produtos', [
            'codigo_comercial' => 'PROD001',
        ]);
    }

    public function test_can_get_produto_by_id(): void
    {
        $produto = Produto::factory()->create(['fornecedor_id' => $this->fornecedor->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/produtos/{$produto->id}");

        $response->assertOk()
            ->assertJsonPath('data.id', $produto->id)
            ->assertJsonPath('data.descricao', $produto->descricao);
    }

    public function test_can_get_produto_with_margin(): void
    {
        $produto = Produto::factory()->create([
            'fornecedor_id' => $this->fornecedor->id,
            'preco_custo' => 100.00,
            'preco_tabela' => 150.00,
        ]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/produtos/{$produto->id}");

        $response->assertOk();
        $this->assertEquals(50, $response->json('data.margem_percentual'));
    }

    public function test_can_update_produto(): void
    {
        $produto = Produto::factory()->create(['fornecedor_id' => $this->fornecedor->id]);

        $data = [
            'descricao' => 'Produto Updated',
            'preco_tabela' => 200.00,
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->putJson("/api/cadastros/produtos/{$produto->id}", $data);

        $response->assertOk()
            ->assertJsonPath('data.descricao', 'Produto Updated');

        $this->assertDatabaseHas('produtos', [
            'id' => $produto->id,
            'descricao' => 'Produto Updated',
        ]);
    }

    public function test_can_delete_produto(): void
    {
        $produto = Produto::factory()->create(['fornecedor_id' => $this->fornecedor->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->deleteJson("/api/cadastros/produtos/{$produto->id}");

        $response->assertNoContent();
        $this->assertSoftDeleted('produtos', ['id' => $produto->id]);
    }

    public function test_validation_fails_without_required_fields(): void
    {
        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/produtos', []);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors([
                'fornecedor_id',
                'codigo_comercial',
                'descricao',
                'unidade',
                'unidades_por_caixa',
                'preco_custo',
                'preco_tabela',
            ]);
    }

    public function test_validation_fails_with_duplicate_codigo_comercial(): void
    {
        Produto::factory()->create([
            'codigo_comercial' => 'PROD_DUP',
            'fornecedor_id' => $this->fornecedor->id,
        ]);

        $data = [
            'fornecedor_id' => $this->fornecedor->id,
            'codigo_comercial' => 'PROD_DUP',
            'descricao' => 'Duplicate Code',
            'unidade' => 'UN',
            'unidades_por_caixa' => 10,
            'preco_custo' => 50.00,
            'preco_tabela' => 100.00,
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/produtos', $data);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['codigo_comercial']);
    }

    public function test_can_filter_produtos_by_fornecedor(): void
    {
        $fornecedor1 = Fornecedor::factory()->create();
        $fornecedor2 = Fornecedor::factory()->create();

        Produto::factory()->count(2)->create(['fornecedor_id' => $fornecedor1->id]);
        Produto::factory()->count(3)->create(['fornecedor_id' => $fornecedor2->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/fornecedores/{$fornecedor1->id}/produtos");

        $response->assertOk()
            ->assertJsonCount(2, 'data');
    }

    public function test_can_filter_produtos_by_categoria(): void
    {
        Produto::factory()->create([
            'fornecedor_id' => $this->fornecedor->id,
            'categoria' => 'Eletrônicos',
        ]);
        Produto::factory()->create([
            'fornecedor_id' => $this->fornecedor->id,
            'categoria' => 'Informática',
        ]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson('/api/cadastros/produtos?categoria=Eletrônicos');

        $response->assertOk()
            ->assertJsonCount(1, 'data');
    }

    public function test_requires_authentication(): void
    {
        $response = $this->getJson('/api/cadastros/produtos');

        $response->assertUnauthorized();
    }
}
