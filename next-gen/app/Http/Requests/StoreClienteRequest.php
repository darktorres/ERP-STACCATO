<?php

namespace App\Http\Requests;

use Illuminate\Foundation\Http\FormRequest;
use Illuminate\Validation\Rule;

class StoreClienteRequest extends FormRequest
{
    /**
     * Determine if the user is authorized to make this request.
     *
     * NOTE: Authorization policies will be implemented in Phase 2.
     * For now, all authenticated users can create clients.
     * In Phase 2, this will be replaced with a policy check:
     *   return $this->user()->can('create', Cliente::class);
     */
    public function authorize(): bool
    {
        // Phase 1: All authenticated users can create. Phase 2: Add policy check.
        return $this->user() !== null;
    }

    public function rules(): array
    {
        return [
            'tipo' => ['required', Rule::in(['PF', 'PJ'])],
            'nome_razao' => ['required', 'string', 'max:255'],
            'nome_fantasia' => ['nullable', 'string', 'max:255'],
            'cpf_cnpj' => ['required', 'string', 'max:20', 'unique:clientes'],
            'inscricao_estadual' => ['nullable', 'string', 'max:20'],
            'email' => ['required', 'email', 'max:255'],
            'telefone' => ['required', 'string', 'max:20'],
            'limite_credito' => ['required', 'numeric', 'min:0'],
            'saldo_credito' => ['required', 'numeric', 'min:0'],
            'loja_id' => ['required', 'exists:lojas,id'],
            'vendedor_id' => ['nullable', 'exists:users,id'],
            'is_ativo' => ['boolean'],
        ];
    }

    public function messages(): array
    {
        return [
            'tipo.required' => 'O tipo de cliente é obrigatório.',
            'tipo.in' => 'O tipo deve ser PF ou PJ.',
            'nome_razao.required' => 'O nome/razão social é obrigatório.',
            'cpf_cnpj.required' => 'O CPF/CNPJ é obrigatório.',
            'cpf_cnpj.unique' => 'Já existe um cliente com este CPF/CNPJ.',
            'email.required' => 'O email é obrigatório.',
            'email.email' => 'O email deve ser um endereço válido.',
            'telefone.required' => 'O telefone é obrigatório.',
            'limite_credito.required' => 'O limite de crédito é obrigatório.',
            'loja_id.required' => 'A loja é obrigatória.',
            'loja_id.exists' => 'A loja selecionada não existe.',
            'vendedor_id.exists' => 'O vendedor selecionado não existe.',
        ];
    }
}
