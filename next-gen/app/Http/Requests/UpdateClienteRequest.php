<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class UpdateClienteRequest extends FormRequest
{
    public function authorize(): bool
    {
        return true;
    }

    public function rules(): array
    {
        $clienteId = $this->route('cliente');

        return [
            'tipo' => ['sometimes', Rule::in(['PF', 'PJ'])],
            'nome_razao' => ['sometimes', 'string', 'max:255'],
            'nome_fantasia' => ['nullable', 'string', 'max:255'],
            'cpf_cnpj' => ['sometimes', 'string', 'max:20', Rule::unique('clientes')->ignore($clienteId)],
            'inscricao_estadual' => ['nullable', 'string', 'max:20'],
            'email' => ['sometimes', 'email', 'max:255'],
            'telefone' => ['sometimes', 'string', 'max:20'],
            'limite_credito' => ['sometimes', 'numeric', 'min:0'],
            'saldo_credito' => ['sometimes', 'numeric', 'min:0'],
            'loja_id' => ['sometimes', 'exists:lojas,id'],
            'vendedor_id' => ['nullable', 'exists:users,id'],
            'is_ativo' => ['boolean'],
            'is_incompleto' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'tipo.in' => 'O tipo deve ser PF ou PJ.',
            'cpf_cnpj.unique' => 'Já existe um cliente com este CPF/CNPJ.',
            'email.email' => 'O email deve ser um endereço válido.',
            'loja_id.exists' => 'A loja selecionada não existe.',
            'vendedor_id.exists' => 'O vendedor selecionado não existe.',
        ];
    }
}
