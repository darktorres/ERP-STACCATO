<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Loja extends Model
{
    protected $table = 'loja';
    protected $primaryKey = 'idLoja';
    protected $keyType = 'int';
    public $timestamps = false;

    protected $fillable = [
        'descricao',
        'nomeFantasia',
        'desativado',
    ];

    protected $casts = [
        'desativado' => 'boolean',
    ];

    public function usuarios()
    {
        return $this->hasMany(Usuario::class, 'idLoja', 'idLoja');
    }
}
