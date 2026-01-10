<?php

namespace Tests\Unit\Models;

use App\Models\Fornecedor;
use App\Models\Produto;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class FornecedorTest extends TestCase
{
    use DatabaseTransactions;

    public function test_fornecedor_can_be_created_with_valid_data(): void
    {
        $fornecedor = Fornecedor::create([
            'razao_social' => 'Empresa de Distribuidora Ltda',
            'nome_fantasia' => 'Distribuidora Top',
            'cnpj' => '12.345.678/0001-90',
            'email' => 'contato@distribuidor.com',
            'telefone' => '1133334444',
        ]);

        $this->assertDatabaseHas('fornecedores', [
            'id' => $fornecedor->id,
            'cnpj' => '12.345.678/0001-90',
        ]);
    }

    public function test_fornecedor_fillable_attributes(): void
    {
        $fillable = (new Fornecedor())->getFillable();

        $this->assertContains('razao_social', $fillable);
        $this->assertContains('nome_fantasia', $fillable);
        $this->assertContains('cnpj', $fillable);
        $this->assertContains('email', $fillable);
        $this->assertContains('telefone', $fillable);
    }

    public function test_fornecedor_casts_is_ativo_to_boolean(): void
    {
        $fornecedor = Fornecedor::create([
            'razao_social' => 'Test Supplier',
            'cnpj' => '11.111.111/0001-11',
            'email' => 'test@supplier.com',
            'telefone' => '1111111111',
            'is_ativo' => true,
        ]);

        $retrieved = Fornecedor::find($fornecedor->id);

        $this->assertTrue($retrieved->is_ativo);
        $this->assertIsBool($retrieved->is_ativo);
    }

    public function test_fornecedor_has_many_produtos(): void
    {
        $fornecedor = Fornecedor::factory()->create();
        Produto::factory()->count(5)->create(['fornecedor_id' => $fornecedor->id]);

        $this->assertEquals(5, $fornecedor->produtos()->count());
        $this->assertInstanceOf(Produto::class, $fornecedor->produtos->first());
    }

    public function test_fornecedor_inscricao_estadual_is_nullable(): void
    {
        $fornecedor = Fornecedor::create([
            'razao_social' => 'No IE Supplier',
            'cnpj' => '22.222.222/0001-22',
            'email' => 'noie@supplier.com',
            'telefone' => '2222222222',
            'inscricao_estadual' => null,
        ]);

        $this->assertNull($fornecedor->inscricao_estadual);
    }

    public function test_fornecedor_nome_fantasia_is_nullable(): void
    {
        $fornecedor = Fornecedor::create([
            'razao_social' => 'No Fantasy Supplier',
            'cnpj' => '33.333.333/0001-33',
            'email' => 'nofantasy@supplier.com',
            'telefone' => '3333333333',
        ]);

        $this->assertNull($fornecedor->nome_fantasia);
    }
}
