<?php

namespace Tests\Feature;

use App\Models\Loja;
use App\Models\Cliente;
use App\Models\User;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class LojaApiTest extends TestCase
{
    use DatabaseTransactions;

    private User $user;

    protected function setUp(): void
    {
        parent::setUp();
        $this->user = User::factory()->create();
    }

    public function test_can_get_all_lojas(): void
    {
        Loja::factory()->count(3)->create();

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson('/api/cadastros/lojas');

        $response->assertOk()
            ->assertJsonStructure([
                'data' => [
                    '*' => ['id', 'codigo', 'nome', 'cnpj', 'is_ativo', 'created_at'],
                ],
                'meta',
            ]);
    }

    public function test_can_create_loja(): void
    {
        $data = [
            'codigo' => 'LOJA001',
            'nome' => 'Loja Principal',
            'cnpj' => '12.345.678/0001-90',
            'inscricao_estadual' => '123.456.789.012',
            'config' => ['theme' => 'light', 'language' => 'pt_BR'],
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/lojas', $data);

        $response->assertCreated()
            ->assertJsonPath('data.codigo', 'LOJA001')
            ->assertJsonPath('data.nome', 'Loja Principal');

        $this->assertDatabaseHas('lojas', [
            'codigo' => 'LOJA001',
        ]);
    }

    public function test_can_get_loja_by_id(): void
    {
        $loja = Loja::factory()->create();

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/lojas/{$loja->id}");

        $response->assertOk()
            ->assertJsonPath('data.id', $loja->id)
            ->assertJsonPath('data.nome', $loja->nome);
    }

    public function test_can_get_loja_with_clientes(): void
    {
        $loja = Loja::factory()->create();
        Cliente::factory()->count(5)->create(['loja_id' => $loja->id]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson("/api/cadastros/lojas/{$loja->id}");

        $response->assertOk()
            ->assertJsonIsArray('data.clientes')
            ->assertJsonCount(5, 'data.clientes');
    }

    public function test_can_update_loja(): void
    {
        $loja = Loja::factory()->create();

        $data = [
            'nome' => 'Loja Principal Updated',
            'config' => ['theme' => 'dark', 'language' => 'en_US'],
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->putJson("/api/cadastros/lojas/{$loja->id}", $data);

        $response->assertOk()
            ->assertJsonPath('data.nome', 'Loja Principal Updated');

        $this->assertDatabaseHas('lojas', [
            'id' => $loja->id,
            'nome' => 'Loja Principal Updated',
        ]);
    }

    public function test_can_delete_loja(): void
    {
        $loja = Loja::factory()->create();

        $response = $this->actingAs($this->user, 'sanctum')
            ->deleteJson("/api/cadastros/lojas/{$loja->id}");

        $response->assertNoContent();
        $this->assertSoftDeleted('lojas', ['id' => $loja->id]);
    }

    public function test_validation_fails_without_required_fields(): void
    {
        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/lojas', []);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['codigo', 'nome', 'cnpj']);
    }

    public function test_validation_fails_with_duplicate_codigo(): void
    {
        Loja::factory()->create(['codigo' => 'LOJA_DUP']);

        $data = [
            'codigo' => 'LOJA_DUP',
            'nome' => 'Another Store',
            'cnpj' => '12.345.678/0001-90',
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/lojas', $data);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['codigo']);
    }

    public function test_validation_fails_with_duplicate_cnpj(): void
    {
        Loja::factory()->create(['cnpj' => '12.345.678/0001-90']);

        $data = [
            'codigo' => 'LOJA002',
            'nome' => 'Another Store',
            'cnpj' => '12.345.678/0001-90',
        ];

        $response = $this->actingAs($this->user, 'sanctum')
            ->postJson('/api/cadastros/lojas', $data);

        $response->assertUnprocessable()
            ->assertJsonValidationErrors(['cnpj']);
    }

    public function test_can_search_lojas(): void
    {
        Loja::factory()->create([
            'codigo' => 'LOJA_SP',
            'nome' => 'Loja São Paulo',
        ]);
        Loja::factory()->create([
            'codigo' => 'LOJA_RJ',
            'nome' => 'Loja Rio de Janeiro',
        ]);

        $response = $this->actingAs($this->user, 'sanctum')
            ->getJson('/api/cadastros/lojas?search=São Paulo');

        $response->assertOk()
            ->assertJsonCount(1, 'data');
    }

    public function test_requires_authentication(): void
    {
        $response = $this->getJson('/api/cadastros/lojas');

        $response->assertUnauthorized();
    }
}
