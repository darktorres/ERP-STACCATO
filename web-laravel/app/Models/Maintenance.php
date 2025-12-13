<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

class Maintenance extends Model
{
    protected $table = 'maintenance';
    protected $primaryKey = 'id';
    protected $keyType = 'int';
    public $timestamps = false;

    protected $fillable = [
        'lastInvalidated',
        'ultimaConsultaNSU',
        'emManutencao',
        'created',
        'lastUpdated',
    ];

    protected $casts = [
        'emManutencao' => 'boolean',
        'created' => 'datetime',
        'lastUpdated' => 'datetime',
        'lastInvalidated' => 'datetime',
        'ultimaConsultaNSU' => 'datetime',
    ];
}
