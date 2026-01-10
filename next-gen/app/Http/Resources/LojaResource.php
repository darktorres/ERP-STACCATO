<?php

namespace App\Http\Resources;

use Illuminate\Http\Request;
use Illuminate\Http\Resources\Json\JsonResource;

class LojaResource extends JsonResource
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
            'codigo' => $this->codigo,
            'nome' => $this->nome,
            'cnpj' => $this->cnpj,
            'inscricao_estadual' => $this->inscricao_estadual,
            'config' => $this->config ?? new \stdClass(),
            'is_ativo' => (bool) $this->is_ativo,
            'clientes' => ClienteResource::collection($this->whenLoaded('clientes')),
            'created_at' => $this->created_at,
            'updated_at' => $this->updated_at,
        ];
    }
}
