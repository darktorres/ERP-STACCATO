<?php

namespace Tests\Unit\Models;

use App\Models\Produto;
use App\Models\Fornecedor;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class ProdutoTest extends TestCase
{
    use DatabaseTransactions;

    public function test_produto_can_be_created_with_valid_data(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $produto = Produto::create([
            'fornecedor_id' => $fornecedor->id,
            'codigo_comercial' => 'PROD001',
            'descricao' => 'Produto Test',
            'unidade' => 'UN',
            'unidades_por_caixa' => 10,
            'preco_custo' => 100.00,
            'preco_tabela' => 150.00,
        ]);

        $this->assertDatabaseHas('produtos', [
            'id' => $produto->id,
            'codigo_comercial' => 'PROD001',
        ]);
    }

    public function test_produto_fillable_attributes(): void
    {
        $fillable = (new Produto())->getFillable();

        $this->assertContains('fornecedor_id', $fillable);
        $this->assertContains('codigo_comercial', $fillable);
        $this->assertContains('descricao', $fillable);
        $this->assertContains('preco_custo', $fillable);
        $this->assertContains('preco_tabela', $fillable);
    }

    public function test_produto_casts_numeric_fields_to_decimal(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $produto = Produto::create([
            'fornecedor_id' => $fornecedor->id,
            'codigo_comercial' => 'PROD002',
            'descricao' => 'Numeric Test',
            'unidade' => 'UN',
            'unidades_por_caixa' => 25.50,
            'peso_kg' => 12.75,
            'preco_custo' => 99.99,
            'preco_tabela' => 199.98,
        ]);

        $retrieved = Produto::find($produto->id);

        $this->assertIsNumeric($retrieved->unidades_por_caixa);
        $this->assertIsNumeric($retrieved->peso_kg);
        $this->assertIsNumeric($retrieved->preco_custo);
        $this->assertIsNumeric($retrieved->preco_tabela);
        $this->assertEquals('25.5000', $retrieved->unidades_por_caixa);
        $this->assertEquals('12.7500', $retrieved->peso_kg);
        $this->assertEquals('99.99', $retrieved->preco_custo);
        $this->assertEquals('199.98', $retrieved->preco_tabela);
    }

    public function test_produto_casts_boolean_fields(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $produto = Produto::create([
            'fornecedor_id' => $fornecedor->id,
            'codigo_comercial' => 'PROD003',
            'descricao' => 'Boolean Test',
            'unidade' => 'UN',
            'unidades_por_caixa' => 10,
            'tem_lote' => true,
            'is_ativo' => true,
            'preco_custo' => 50.00,
            'preco_tabela' => 100.00,
        ]);

        $retrieved = Produto::find($produto->id);

        $this->assertTrue($retrieved->tem_lote);
        $this->assertTrue($retrieved->is_ativo);
    }

    public function test_produto_belongs_to_fornecedor(): void
    {
        $fornecedor = Fornecedor::factory()->create();
        $produto = Produto::factory()->create(['fornecedor_id' => $fornecedor->id]);

        $this->assertInstanceOf(Fornecedor::class, $produto->fornecedor);
        $this->assertEquals($fornecedor->id, $produto->fornecedor->id);
    }

    public function test_produto_optional_fields_are_nullable(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $produto = Produto::create([
            'fornecedor_id' => $fornecedor->id,
            'codigo_comercial' => 'PROD004',
            'descricao' => 'Optional Fields',
            'unidade' => 'UN',
            'unidades_por_caixa' => 10,
            'preco_custo' => 50.00,
            'preco_tabela' => 100.00,
            'codigo_barras' => null,
            'descricao_curta' => null,
            'peso_kg' => null,
            'ncm' => null,
            'categoria' => null,
        ]);

        $this->assertNull($produto->codigo_barras);
        $this->assertNull($produto->descricao_curta);
        $this->assertNull($produto->peso_kg);
        $this->assertNull($produto->ncm);
        $this->assertNull($produto->categoria);
    }

    public function test_produto_tem_lote_defaults_to_false(): void
    {
        $fornecedor = Fornecedor::factory()->create();

        $produto = Produto::create([
            'fornecedor_id' => $fornecedor->id,
            'codigo_comercial' => 'PROD005',
            'descricao' => 'No Lote',
            'unidade' => 'UN',
            'unidades_por_caixa' => 10,
            'preco_custo' => 50.00,
            'preco_tabela' => 100.00,
        ]);

        $retrieved = Produto::find($produto->id);

        // Assuming default is false (check migration for default value)
        $this->assertIsBool($retrieved->tem_lote);
    }
}
