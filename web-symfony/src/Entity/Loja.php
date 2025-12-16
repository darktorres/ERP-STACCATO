<?php

namespace App\Entity;

use Doctrine\ORM\Mapping as ORM;

#[ORM\Entity]
#[ORM\Table(name: 'loja')]
class Loja
{
    #[ORM\Id]
    #[ORM\GeneratedValue]
    #[ORM\Column(name: 'idLoja', type: 'integer')]
    private int $idLoja;

    #[ORM\Column(name: 'descricao', type: 'string', length: 150)]
    private string $descricao;

    #[ORM\Column(name: 'nomeFantasia', type: 'string', length: 150, nullable: true)]
    private ?string $nomeFantasia = null;

    #[ORM\Column(name: 'desativado', type: 'boolean', options: ['default' => false])]
    private bool $desativado = false;

    public function getIdLoja(): int
    {
        return $this->idLoja;
    }

    public function getDescricao(): string
    {
        return $this->descricao;
    }

    public function setDescricao(string $descricao): self
    {
        $this->descricao = $descricao;
        return $this;
    }

    public function getNomeFantasia(): ?string
    {
        return $this->nomeFantasia;
    }

    public function setNomeFantasia(?string $nomeFantasia): self
    {
        $this->nomeFantasia = $nomeFantasia;
        return $this;
    }

    public function isDesativado(): bool
    {
        return $this->desativado;
    }

    public function setDesativado(bool $desativado): self
    {
        $this->desativado = $desativado;
        return $this;
    }
}
