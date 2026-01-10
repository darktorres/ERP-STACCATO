<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

class StoreProdutoRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        return [
            'fornecedor_id' => ['required', 'exists:fornecedores,id'],
            'codigo_comercial' => ['required', 'string', 'max:100', 'unique:produtos'],
            'codigo_barras' => ['nullable', 'string', 'max:100', 'unique:produtos'],
            'descricao' => ['required', 'string', 'max:500'],
            'descricao_curta' => ['nullable', 'string', 'max:255'],
            'unidade' => ['required', 'string', 'max:10'],
            'unidades_por_caixa' => ['required', 'numeric', 'min:1'],
            'peso_kg' => ['nullable', 'numeric', 'min:0'],
            'ncm' => ['nullable', 'string', 'max:8'],
            'categoria' => ['nullable', 'string', 'max:100'],
            'tem_lote' => ['boolean'],
            'preco_custo' => ['required', 'numeric', 'min:0'],
            'preco_tabela' => ['required', 'numeric', 'min:0'],
            'is_ativo' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'fornecedor_id.required' => 'O fornecedor é obrigatório.',
            'fornecedor_id.exists' => 'O fornecedor selecionado não existe.',
            'codigo_comercial.required' => 'O código comercial é obrigatório.',
            'codigo_comercial.unique' => 'Já existe um produto com este código comercial.',
            'codigo_barras.unique' => 'Já existe um produto com este código de barras.',
            'descricao.required' => 'A descrição é obrigatória.',
            'unidade.required' => 'A unidade é obrigatória.',
            'unidades_por_caixa.required' => 'A quantidade de unidades por caixa é obrigatória.',
            'unidades_por_caixa.min' => 'A quantidade de unidades por caixa deve ser maior que 0.',
            'preco_custo.required' => 'O preço de custo é obrigatório.',
            'preco_custo.min' => 'O preço de custo não pode ser negativo.',
            'preco_tabela.required' => 'O preço de tabela é obrigatório.',
            'preco_tabela.min' => 'O preço de tabela não pode ser negativo.',
        ];
    }
}
