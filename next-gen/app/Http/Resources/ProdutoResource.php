<?php

namespace App\Http\Resources;

use Illuminate\Http\Request;
use Illuminate\Http\Resources\Json\JsonResource;

class ProdutoResource extends JsonResource
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
            'codigo_comercial' => $this->codigo_comercial,
            'codigo_barras' => $this->codigo_barras,
            'descricao' => $this->descricao,
            'descricao_curta' => $this->descricao_curta,
            'unidade' => $this->unidade,
            'unidades_por_caixa' => (float) $this->unidades_por_caixa,
            'peso_kg' => $this->peso_kg ? (float) $this->peso_kg : null,
            'ncm' => $this->ncm,
            'categoria' => $this->categoria,
            'tem_lote' => (bool) $this->tem_lote,
            'preco_custo' => (float) $this->preco_custo,
            'preco_tabela' => (float) $this->preco_tabela,
            'margem_percentual' => $this->calcularMargemPercentual(),
            'is_ativo' => (bool) $this->is_ativo,
            'fornecedor' => new FornecedorResource($this->whenLoaded('fornecedor')),
            'created_at' => $this->created_at,
            'updated_at' => $this->updated_at,
        ];
    }

    /**
     * Calculate the margin percentage between cost and table price.
     */
    private function calcularMargemPercentual(): ?float
    {
        if ($this->preco_custo > 0) {
            return (($this->preco_tabela - $this->preco_custo) / $this->preco_custo) * 100;
        }

        return null;
    }
}
