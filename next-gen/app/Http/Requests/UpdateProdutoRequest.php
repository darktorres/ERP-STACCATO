<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class UpdateProdutoRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        $produtoId = $this->route('produto');

        return [
            'fornecedor_id' => ['sometimes', 'exists:fornecedores,id'],
            'codigo_comercial' => ['sometimes', 'string', 'max:100', Rule::unique('produtos')->ignore($produtoId)],
            'codigo_barras' => ['nullable', 'string', 'max:100', Rule::unique('produtos')->ignore($produtoId)],
            'descricao' => ['sometimes', 'string', 'max:500'],
            'descricao_curta' => ['nullable', 'string', 'max:255'],
            'unidade' => ['sometimes', 'string', 'max:10'],
            'unidades_por_caixa' => ['sometimes', 'numeric', 'min:1'],
            'peso_kg' => ['nullable', 'numeric', 'min:0'],
            'ncm' => ['nullable', 'string', 'max:8'],
            'categoria' => ['nullable', 'string', 'max:100'],
            'tem_lote' => ['boolean'],
            'preco_custo' => ['sometimes', 'numeric', 'min:0'],
            'preco_tabela' => ['sometimes', 'numeric', 'min:0'],
            'is_ativo' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'fornecedor_id.exists' => 'O fornecedor selecionado não existe.',
            'codigo_comercial.unique' => 'Já existe outro produto com este código comercial.',
            'codigo_barras.unique' => 'Já existe outro produto com este código de barras.',
            'unidades_por_caixa.min' => 'A quantidade de unidades por caixa deve ser maior que 0.',
            'preco_custo.min' => 'O preço de custo não pode ser negativo.',
            'preco_tabela.min' => 'O preço de tabela não pode ser negativo.',
        ];
    }
}
