<?php

namespace Tests\Feature;

use App\Models\Fornecedor;
use App\Models\Produto;
use App\Models\User;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class FornecedorApiTest extends TestCase
{
    use DatabaseTransactions;

    private User $user;

    protected function setUp(): void
    {
        parent::setUp();
        $this->user = User::factory()->create();
    }

    public function test_can_get_all_fornecedores(): void
    {
        Fornecedor::factory()->count(3)->create();

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson('/api/cadastros/fornecedores');

        $response->assertOk()
            ->assertJsonStructure([
                'data' => [
                    '*' => ['id', 'razao_social', 'cnpj', 'email', 'created_at'],
                ],
                'meta',
            ]);
    }

    public function test_can_create_fornecedor(): void
    {
        $data = [
            'razao_social' => 'Distribuidora ABC Ltda',
            'nome_fantasia' => 'ABC',
            'cnpj' => '12.345.678/0001-90',
            'inscricao_estadual' => '123.456.789.012',
            'email' => 'contato@abc.com',
            'telefone' => '1133334444',
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/fornecedores', $data);

        $response->assertCreated()
            ->assertJsonPath('data.razao_social', 'Distribuidora ABC Ltda')
            ->assertJsonPath('data.cnpj', '12.345.678/0001-90');

        $this->assertDatabaseHas('fornecedores', [
            'cnpj' => '12.345.678/0001-90',
        ]);
    }

    public function test_can_get_fornecedor_by_id(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/fornecedores/{$fornecedor->id}");

        $response->assertOk()
            ->assertJsonPath('data.id', $fornecedor->id)
            ->assertJsonPath('data.razao_social', $fornecedor->razao_social);
    }

    public function test_can_get_fornecedor_with_produtos(): void
    {
        $fornecedor = Fornecedor::factory()->create();
        Produto::factory()->count(5)->create(['fornecedor_id' => $fornecedor->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/fornecedores/{$fornecedor->id}");

        $response->assertOk()
            ->assertJsonIsArray('data.produtos')
            ->assertJsonCount(5, 'data.produtos');
    }

    public function test_can_update_fornecedor(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $data = [
            'razao_social' => 'Distribuidora XYZ Ltda Updated',
            'email' => 'xyz.updated@example.com',
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->putJson("/api/cadastros/fornecedores/{$fornecedor->id}", $data);

        $response->assertOk()
            ->assertJsonPath('data.razao_social', 'Distribuidora XYZ Ltda Updated');

        $this->assertDatabaseHas('fornecedores', [
            'id' => $fornecedor->id,
            'razao_social' => 'Distribuidora XYZ Ltda Updated',
        ]);
    }

    public function test_can_delete_fornecedor(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $response = $this->actingAs($this->user, 'sanctum')
            ->deleteJson("/api/cadastros/fornecedores/{$fornecedor->id}");

        $response->assertNoContent();
        $this->assertSoftDeleted('fornecedores', ['id' => $fornecedor->id]);
    }

    public function test_validation_fails_without_required_fields(): void
    {
        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/fornecedores', []);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['razao_social', 'cnpj', 'email', 'telefone']);
    }

    public function test_validation_fails_with_duplicate_cnpj(): void
    {
        Fornecedor::factory()->create(['cnpj' => '12.345.678/0001-90']);

        $data = [
            'razao_social' => 'Another Company',
            'cnpj' => '12.345.678/0001-90',
            'email' => 'another@example.com',
            'telefone' => '1133334444',
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/fornecedores', $data);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['cnpj']);
    }

    public function test_requires_authentication(): void
    {
        $response = $this->getJson('/api/cadastros/fornecedores');

        $response->assertUnauthorized();
    }
}
