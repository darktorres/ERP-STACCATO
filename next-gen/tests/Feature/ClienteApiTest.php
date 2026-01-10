<?php

namespace Tests\Feature;

use App\Models\Cliente;
use App\Models\Loja;
use App\Models\User;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class ClienteApiTest extends TestCase
{
    use DatabaseTransactions;

    private User $user;
    private Loja $loja;

    protected function setUp(): void
    {
        parent::setUp();
        $this->user = User::factory()->create();
        $this->loja = Loja::factory()->create();
    }

    public function test_can_get_all_clientes(): void
    {
        Cliente::factory()->count(3)->create(['loja_id' => $this->loja->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson('/api/cadastros/clientes');

        $response->assertOk()
            ->assertJsonStructure([
                'data' => [
                    '*' => ['id', 'tipo', 'nome_razao', 'email', 'created_at'],
                ],
                'meta',
            ]);
    }

    public function test_can_create_cliente(): void
    {
        $data = [
            'tipo' => 'PF',
            'nome_razao' => 'João Silva',
            'cpf_cnpj' => '123.456.789-00',
            'email' => 'joao@example.com',
            'telefone' => '11999999999',
            'limite_credito' => 5000.00,
            'saldo_credito' => 5000.00,
            'loja_id' => $this->loja->id,
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/clientes', $data);

        $response->assertCreated()
            ->assertJsonPath('data.nome_razao', 'João Silva')
            ->assertJsonPath('data.tipo', 'PF');

        $this->assertDatabaseHas('clientes', [
            'cpf_cnpj' => '123.456.789-00',
        ]);
    }

    public function test_can_get_cliente_by_id(): void
    {
        $cliente = Cliente::factory()->create(['loja_id' => $this->loja->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/clientes/{$cliente->id}");

        $response->assertOk()
            ->assertJsonPath('data.id', $cliente->id)
            ->assertJsonPath('data.nome_razao', $cliente->nome_razao);
    }

    public function test_can_update_cliente(): void
    {
        $cliente = Cliente::factory()->create(['loja_id' => $this->loja->id]);

        $data = [
            'nome_razao' => 'João da Silva Updated',
            'email' => 'joao.updated@example.com',
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->putJson("/api/cadastros/clientes/{$cliente->id}", $data);

        $response->assertOk()
            ->assertJsonPath('data.nome_razao', 'João da Silva Updated');

        $this->assertDatabaseHas('clientes', [
            'id' => $cliente->id,
            'nome_razao' => 'João da Silva Updated',
        ]);
    }

    public function test_can_delete_cliente(): void
    {
        $cliente = Cliente::factory()->create(['loja_id' => $this->loja->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->deleteJson("/api/cadastros/clientes/{$cliente->id}");

        $response->assertNoContent();
        $this->assertSoftDeleted('clientes', ['id' => $cliente->id]);
    }

    public function test_validation_fails_without_required_fields(): void
    {
        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/clientes', []);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['tipo', 'nome_razao', 'cpf_cnpj', 'email', 'telefone', 'limite_credito', 'loja_id']);
    }

    public function test_validation_fails_with_duplicate_cpf_cnpj(): void
    {
        Cliente::factory()->create([
            'cpf_cnpj' => '123.456.789-00',
            'loja_id' => $this->loja->id,
        ]);

        $data = [
            'tipo' => 'PF',
            'nome_razao' => 'Duplicate Client',
            'cpf_cnpj' => '123.456.789-00',
            'email' => 'duplicate@example.com',
            'telefone' => '11999999999',
            'limite_credito' => 5000.00,
            'saldo_credito' => 5000.00,
            'loja_id' => $this->loja->id,
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/clientes', $data);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['cpf_cnpj']);
    }

    public function test_can_filter_clientes_by_loja(): void
    {
        $loja1 = Loja::factory()->create();
        $loja2 = Loja::factory()->create();

        Cliente::factory()->count(2)->create(['loja_id' => $loja1->id]);
        Cliente::factory()->count(3)->create(['loja_id' => $loja2->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/lojas/{$loja1->id}/clientes");

        $response->assertOk()
            ->assertJsonCount(2, 'data');
    }

    public function test_requires_authentication(): void
    {
        $response = $this->getJson('/api/cadastros/clientes');

        $response->assertUnauthorized();
    }
}
