<?php

namespace App\Http\Resources;

use Illuminate\Http\Request;
use Illuminate\Http\Resources\Json\JsonResource;

class ClienteResource extends JsonResource
{
    /**
     * Transform the resource into an array.
     *
     * @return array<string, mixed>
     */
    public function toArray(Request $request): array
    {
        return [
            'id' => $this->id,
            'tipo' => $this->tipo,
            'nome_razao' => $this->nome_razao,
            'nome_fantasia' => $this->nome_fantasia,
            'cpf_cnpj' => $this->cpf_cnpj,
            'inscricao_estadual' => $this->inscricao_estadual,
            'email' => $this->email,
            'telefone' => $this->telefone,
            'limite_credito' => (float) $this->limite_credito,
            'saldo_credito' => (float) $this->saldo_credito,
            'is_incompleto' => (bool) $this->is_incompleto,
            'is_ativo' => (bool) $this->is_ativo,
            'loja' => new LojaResource($this->whenLoaded('loja')),
            'vendedor' => $this->whenLoaded('vendedor') ? [
                'id' => $this->vendedor->id,
                'name' => $this->vendedor->name,
                'email' => $this->vendedor->email,
            ] : null,
            'created_at' => $this->created_at,
            'updated_at' => $this->updated_at,
        ];
    }
}
