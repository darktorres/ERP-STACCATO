<?php

namespace Tests\Unit\Models;

use App\Models\Cliente;
use App\Models\Loja;
use App\Models\User;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class ClienteTest extends TestCase
{
    use DatabaseTransactions;

    public function test_cliente_can_be_created_with_valid_data(): void
    {
        $loja = Loja::factory()->create();

        $cliente = Cliente::create([
            'tipo' => 'PF',
            'nome_razao' => 'João da Silva',
            'cpf_cnpj' => '123.456.789-00',
            'email' => 'joao@example.com',
            'telefone' => '11999999999',
            'limite_credito' => 1000.00,
            'saldo_credito' => 1000.00,
            'loja_id' => $loja->id,
        ]);

        $this->assertDatabaseHas('clientes', [
            'id' => $cliente->id,
            'cpf_cnpj' => '123.456.789-00',
        ]);
    }

    public function test_cliente_fillable_attributes(): void
    {
        $fillable = (new Cliente())->getFillable();

        $this->assertContains('tipo', $fillable);
        $this->assertContains('nome_razao', $fillable);
        $this->assertContains('cpf_cnpj', $fillable);
        $this->assertContains('email', $fillable);
        $this->assertContains('limite_credito', $fillable);
    }

    public function test_cliente_casts_numeric_fields_to_decimal(): void
    {
        $loja = Loja::factory()->create();

        $cliente = Cliente::create([
            'tipo' => 'PJ',
            'nome_razao' => 'Empresa Ltda',
            'cpf_cnpj' => '12.345.678/0001-90',
            'email' => 'empresa@example.com',
            'telefone' => '1133333333',
            'limite_credito' => 5000.50,
            'saldo_credito' => 2500.25,
            'loja_id' => $loja->id,
        ]);

        $retrieved = Cliente::find($cliente->id);

        $this->assertIsNumeric($retrieved->limite_credito);
        $this->assertIsNumeric($retrieved->saldo_credito);
        $this->assertEquals('5000.50', $retrieved->limite_credito);
        $this->assertEquals('2500.25', $retrieved->saldo_credito);
    }

    public function test_cliente_casts_boolean_fields(): void
    {
        $loja = Loja::factory()->create();

        $cliente = Cliente::create([
            'tipo' => 'PF',
            'nome_razao' => 'Test User',
            'cpf_cnpj' => '111.111.111-11',
            'email' => 'test@example.com',
            'telefone' => '1111111111',
            'limite_credito' => 0,
            'saldo_credito' => 0,
            'loja_id' => $loja->id,
            'is_ativo' => true,
            'is_incompleto' => false,
        ]);

        $retrieved = Cliente::find($cliente->id);

        $this->assertTrue($retrieved->is_ativo);
        $this->assertFalse($retrieved->is_incompleto);
    }

    public function test_cliente_belongs_to_loja(): void
    {
        $loja = Loja::factory()->create();
        $cliente = Cliente::factory()->create(['loja_id' => $loja->id]);

        $this->assertInstanceOf(Loja::class, $cliente->loja);
        $this->assertEquals($loja->id, $cliente->loja->id);
    }

    public function test_cliente_belongs_to_vendedor(): void
    {
        $loja = Loja::factory()->create();
        $vendedor = User::factory()->create();
        $cliente = Cliente::factory()->create([
            'loja_id' => $loja->id,
            'vendedor_id' => $vendedor->id,
        ]);

        $this->assertInstanceOf(User::class, $cliente->vendedor);
        $this->assertEquals($vendedor->id, $cliente->vendedor->id);
    }

    public function test_cliente_vendedor_is_nullable(): void
    {
        $loja = Loja::factory()->create();
        $cliente = Cliente::factory()->create([
            'loja_id' => $loja->id,
            'vendedor_id' => null,
        ]);

        $this->assertNull($cliente->vendedor_id);
    }
}
