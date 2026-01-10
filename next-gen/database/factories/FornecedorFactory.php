<?php

namespace Database\Factories;

use Illuminate\Database\Eloquent\Factories\Factory;

/**
 * @extends \Illuminate\Database\Eloquent\Factories\Factory<\App\Models\Fornecedor>
 */
class FornecedorFactory extends Factory
{
    /**
     * Define the model's default state.
     *
     * @return array<string, mixed>
     */
    public function definition(): array
    {
        return [
            'razao_social' => $this->faker->company(),
            'nome_fantasia' => $this->faker->optional()->sentence(2),
            'cnpj' => $this->faker->unique()->numerify('##############'),
            'inscricao_estadual' => $this->faker->optional()->numerify('###############'),
            'email' => $this->faker->unique()->companyEmail(),
            'telefone' => $this->faker->numerify('##########'),
            'is_ativo' => true,
        ];
    }
}
