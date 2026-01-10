<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class UpdateLojaRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        $lojaId = $this->route('loja');

        return [
            'codigo' => ['sometimes', 'string', 'max:50', Rule::unique('lojas')->ignore($lojaId)],
            'nome' => ['sometimes', 'string', 'max:255'],
            'cnpj' => ['sometimes', 'string', 'max:20', Rule::unique('lojas')->ignore($lojaId)],
            'inscricao_estadual' => ['nullable', 'string', 'max:20'],
            'config' => ['nullable', 'array'],
            'is_ativo' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'codigo.unique' => 'Já existe outra loja com este código.',
            'cnpj.unique' => 'Já existe outra loja com este CNPJ.',
        ];
    }
}
