<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Usuario extends Model
{
    protected $table = 'usuario';
    protected $primaryKey = 'idUsuario';
    protected $keyType = 'int';
    public $timestamps = false;

    protected $fillable = [
        'user',
        'password',
        'nome',
        'tipo',
        'idLoja',
        'email',
        'senhaUsoUnico',
        'valorMinimoFrete',
        'desativado',
    ];

    protected $casts = [
        'desativado' => 'boolean',
        'valorMinimoFrete' => 'decimal:2',
    ];

    public function loja()
    {
        return $this->belongsTo(Loja::class, 'idLoja', 'idLoja');
    }
}
