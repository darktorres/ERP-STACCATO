<?php

namespace Database\Factories;

use App\Models\Loja;
use Illuminate\Database\Eloquent\Factories\Factory;

/**
 * @extends \Illuminate\Database\Eloquent\Factories\Factory<\App\Models\Cliente>
 */
class ClienteFactory extends Factory
{
    /**
     * Define the model's default state.
     *
     * @return array<string, mixed>
     */
    public function definition(): array
    {
        $tipo = $this->faker->randomElement(['PF', 'PJ']);
        $limite = $this->faker->numberBetween(1000, 100000);

        return [
            'tipo' => $tipo,
            'nome_razao' => $tipo === 'PF' ? $this->faker->name() : $this->faker->company(),
            'nome_fantasia' => $this->faker->optional()->sentence(2),
            'cpf_cnpj' => $this->faker->numerify($tipo === 'PF' ? '###########' : '##############'),
            'inscricao_estadual' => $this->faker->optional()->numerify('###############'),
            'email' => $this->faker->unique()->safeEmail(),
            'telefone' => $this->faker->numerify('##########'),
            'limite_credito' => $limite,
            'saldo_credito' => $this->faker->numberBetween(0, $limite),
            'loja_id' => Loja::factory(),
            'vendedor_id' => null,
            'is_incompleto' => false,
            'is_ativo' => true,
        ];
    }
}
