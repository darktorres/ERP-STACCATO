<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class OrcamentoView extends Model
{
    protected $table = 'view_orcamento';
    protected $primaryKey = 'idOrcamento';
    protected $keyType = 'string';
    public $timestamps = false;
    public $incrementing = false;

    protected $fillable = [
        'idOrcamento',
        'idLoja',
        'idUsuario',
        'idUsuarioConsultor',
        'status',
        'diasRestantes',
        'vendedor',
        'consultor',
        'cliente',
        'profissional',
        'tel',
        'telCel',
        'telProf',
        'data',
        'data2',
        'total',
        'idFollowup',
        'dataFollowup',
        'dataProxFollowup',
        'observacao',
        'semaforo',
        'fornecedores',
    ];

    protected $casts = [
        'data' => 'datetime',
        'dataFollowup' => 'datetime',
        'dataProxFollowup' => 'datetime',
        'total' => 'decimal:2',
        'diasRestantes' => 'integer',
        'semaforo' => 'integer',
    ];
}
