<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Factories\HasFactory;
use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasMany;
use Illuminate\Database\Eloquent\SoftDeletes;

class Fornecedor extends Model
{
    use HasFactory, SoftDeletes;

    protected $table = 'fornecedores';

    protected $fillable = [
        'razao_social',
        'nome_fantasia',
        'cnpj',
        'inscricao_estadual',
        'email',
        'telefone',
        'is_ativo',
    ];

    protected $casts = [
        'is_ativo' => 'boolean',
    ];

    /**
     * Get all products from this supplier
     */
    public function produtos(): HasMany
    {
        return $this->hasMany(Produto::class);
    }
}
