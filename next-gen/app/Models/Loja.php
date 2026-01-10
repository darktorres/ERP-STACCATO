<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasMany;

class Loja extends Model
{
    protected $fillable = [
        'codigo',
        'nome',
        'cnpj',
        'inscricao_estadual',
        'config',
        'is_ativo',
    ];

    protected $casts = [
        'config' => 'array',
        'is_ativo' => 'boolean',
    ];

    /**
     * Get all customers for this store
     */
    public function clientes(): HasMany
    {
        return $this->hasMany(Cliente::class);
    }
}
