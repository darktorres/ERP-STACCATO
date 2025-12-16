<?php

namespace App\Entity;

use Doctrine\ORM\Mapping as ORM;

#[ORM\Entity]
#[ORM\Table(name: 'maintenance')]
class Maintenance
{
    #[ORM\Id]
    #[ORM\Column(name: 'id', type: 'integer')]
    private int $id;

    #[ORM\Column(name: 'emManutencao', type: 'boolean', options: ['default' => false])]
    private bool $emManutencao = false;

    public function getId(): int
    {
        return $this->id;
    }

    public function isEmManutencao(): bool
    {
        return $this->emManutencao;
    }

    public function setEmManutencao(bool $emManutencao): self
    {
        $this->emManutencao = $emManutencao;
        return $this;
    }
}
