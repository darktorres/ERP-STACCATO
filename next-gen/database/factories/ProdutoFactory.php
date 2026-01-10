<?php

namespace Database\Factories;

use App\Models\Fornecedor;
use Illuminate\Database\Eloquent\Factories\Factory;

/**
 * @extends \Illuminate\Database\Eloquent\Factories\Factory<\App\Models\Produto>
 */
class ProdutoFactory extends Factory
{
    /**
     * Define the model's default state.
     *
     * @return array<string, mixed>
     */
    public function definition(): array
    {
        $preco_custo = $this->faker->randomFloat(2, 10, 500);
        $preco_tabela = $preco_custo * $this->faker->randomFloat(2, 1.1, 3.0);

        return [
            'fornecedor_id' => Fornecedor::factory(),
            'codigo_comercial' => $this->faker->unique()->ean8(),
            'codigo_barras' => $this->faker->optional()->ean13(),
            'descricao' => $this->faker->sentence(4),
            'descricao_curta' => $this->faker->optional()->sentence(2),
            'unidade' => $this->faker->randomElement(['UN', 'CX', 'PC', 'KG', 'L', 'M']),
            'unidades_por_caixa' => $this->faker->numberBetween(1, 100),
            'peso_kg' => $this->faker->optional()->randomFloat(4, 0.1, 100),
            'ncm' => $this->faker->numerify('########'),
            'categoria' => $this->faker->word(),
            'tem_lote' => $this->faker->boolean(),
            'preco_custo' => $preco_custo,
            'preco_tabela' => round($preco_tabela, 2),
            'is_ativo' => true,
        ];
    }
}
