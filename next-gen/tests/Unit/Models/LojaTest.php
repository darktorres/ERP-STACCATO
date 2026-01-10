<?php

namespace Tests\Unit\Models;

use App\Models\Loja;
use App\Models\Cliente;
use Illuminate\Foundation\Testing\DatabaseTransactions;
use Tests\TestCase;

class LojaTest extends TestCase
{
    use DatabaseTransactions;

    public function test_loja_can_be_created_with_valid_data(): void
    {
        $loja = Loja::create([
            'codigo' => 'LOJA001',
            'nome' => 'Loja Principal',
            'cnpj' => '12.345.678/0001-90',
            'inscricao_estadual' => '123.456.789.012',
        ]);

        $this->assertDatabaseHas('lojas', [
            'id' => $loja->id,
            'codigo' => 'LOJA001',
        ]);
    }

    public function test_loja_fillable_attributes(): void
    {
        $fillable = (new Loja())->getFillable();

        $this->assertContains('codigo', $fillable);
        $this->assertContains('nome', $fillable);
        $this->assertContains('cnpj', $fillable);
        $this->assertContains('config', $fillable);
    }

    public function test_loja_casts_config_to_array(): void
    {
        $config = ['theme' => 'dark', 'language' => 'pt_BR'];
        $loja = Loja::create([
            'codigo' => 'LOJA002',
            'nome' => 'Loja Config',
            'cnpj' => '12.345.678/0001-91',
            'config' => $config,
        ]);

        $retrieved = Loja::find($loja->id);

        $this->assertIsArray($retrieved->config);
        $this->assertEquals($config, $retrieved->config);
    }

    public function test_loja_casts_is_ativo_to_boolean(): void
    {
        $loja = Loja::create([
            'codigo' => 'LOJA003',
            'nome' => 'Loja Teste',
            'cnpj' => '12.345.678/0001-92',
            'is_ativo' => true,
        ]);

        $retrieved = Loja::find($loja->id);

        $this->assertTrue($retrieved->is_ativo);
        $this->assertIsBool($retrieved->is_ativo);
    }

    public function test_loja_has_many_clientes(): void
    {
        $loja = Loja::factory()->create();
        Cliente::factory()->count(3)->create(['loja_id' => $loja->id]);

        $this->assertEquals(3, $loja->clientes()->count());
        $this->assertInstanceOf(Cliente::class, $loja->clientes->first());
    }

    public function test_loja_deleting_cascade(): void
    {
        $loja = Loja::factory()->create();
        $cliente = Cliente::factory()->create(['loja_id' => $loja->id]);

        $loja->delete();

        $this->assertSoftDeleted('lojas', ['id' => $loja->id]);
    }
}
