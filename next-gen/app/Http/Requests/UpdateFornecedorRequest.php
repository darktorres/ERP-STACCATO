<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class UpdateFornecedorRequest extends FormRequest
{
    /**
     * Determine if the user is authorized to make this request.
     * NOTE: Authorization policies will be implemented in Phase 2.
     */
    public function authorize(): bool
    {
        return $this->user() !== null;
    }

    public function rules(): array
    {
        $fornecedorId = $this->route('fornecedor');

        return [
            'razao_social' => ['sometimes', 'string', 'max:255'],
            'nome_fantasia' => ['nullable', 'string', 'max:255'],
            'cnpj' => ['sometimes', 'string', 'max:20', Rule::unique('fornecedores')->ignore($fornecedorId)],
            'inscricao_estadual' => ['nullable', 'string', 'max:20'],
            'email' => ['sometimes', 'email', 'max:255'],
            'telefone' => ['sometimes', 'string', 'max:20'],
            'is_ativo' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'cnpj.unique' => 'Já existe um fornecedor com este CNPJ.',
            'email.email' => 'O email deve ser um endereço válido.',
        ];
    }
}
