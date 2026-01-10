<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;

class StoreLojaRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        return [
            'codigo' => ['required', 'string', 'max:50', 'unique:lojas'],
            'nome' => ['required', 'string', 'max:255'],
            'cnpj' => ['required', 'string', 'max:20', 'unique:lojas'],
            'inscricao_estadual' => ['nullable', 'string', 'max:20'],
            'config' => ['nullable', 'array'],
            'is_ativo' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'codigo.required' => 'O código da loja é obrigatório.',
            'codigo.unique' => 'Já existe uma loja com este código.',
            'nome.required' => 'O nome da loja é obrigatório.',
            'cnpj.required' => 'O CNPJ da loja é obrigatório.',
            'cnpj.unique' => 'Já existe uma loja com este CNPJ.',
        ];
    }
}
