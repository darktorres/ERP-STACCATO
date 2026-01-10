<?php

namespace Database\Factories;

use Illuminate\Database\Eloquent\Factories\Factory;

/**
 * @extends \Illuminate\Database\Eloquent\Factories\Factory<\App\Models\Loja>
 */
class LojaFactory extends Factory
{
    /**
     * Define the model's default state.
     *
     * @return array<string, mixed>
     */
    public function definition(): array
    {
        return [
            'codigo' => $this->faker->unique()->numerify('LOJA###'),
            'nome' => $this->faker->company(),
            'cnpj' => $this->faker->numerify('##############'),
            'inscricao_estadual' => $this->faker->numerify('###############'),
            'config' => [],
            'is_ativo' => true,
        ];
    }
}
