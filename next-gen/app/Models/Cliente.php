<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\SoftDeletes;

class Cliente extends Model
{
    use HasFactory, SoftDeletes;

    protected $fillable = [
        'tipo',
        'nome_razao',
        'nome_fantasia',
        'cpf_cnpj',
        'inscricao_estadual',
        'email',
        'telefone',
        'limite_credito',
        'saldo_credito',
        'loja_id',
        'vendedor_id',
        'is_incompleto',
        'is_ativo',
    ];

    protected $casts = [
        'limite_credito' => 'decimal:2',
        'saldo_credito' => 'decimal:2',
        'is_incompleto' => 'boolean',
        'is_ativo' => 'boolean',
    ];

    /**
     * Get the store this customer belongs to
     */
    public function loja(): BelongsTo
    {
        return $this->belongsTo(Loja::class);
    }

    /**
     * Get the salesman for this customer
     */
    public function vendedor(): BelongsTo
    {
        return $this->belongsTo(User::class, 'vendedor_id');
    }
}
