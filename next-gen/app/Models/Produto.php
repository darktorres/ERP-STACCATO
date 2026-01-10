<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class Produto extends Model
{
    protected $fillable = [
        'fornecedor_id',
        'codigo_comercial',
        'codigo_barras',
        'descricao',
        'descricao_curta',
        'unidade',
        'unidades_por_caixa',
        'peso_kg',
        'ncm',
        'categoria',
        'tem_lote',
        'preco_custo',
        'preco_tabela',
        'is_ativo',
    ];

    protected $casts = [
        'unidades_por_caixa' => 'decimal:4',
        'peso_kg' => 'decimal:4',
        'preco_custo' => 'decimal:2',
        'preco_tabela' => 'decimal:2',
        'tem_lote' => 'boolean',
        'is_ativo' => 'boolean',
    ];

    /**
     * Get the supplier for this product
     */
    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }
}
