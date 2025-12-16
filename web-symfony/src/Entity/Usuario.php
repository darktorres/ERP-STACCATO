<?php

namespace App\Entity;

use Doctrine\ORM\Mapping as ORM;

#[ORM\Entity]
#[ORM\Table(name: 'usuario')]
class Usuario
{
    #[ORM\Id]
    #[ORM\GeneratedValue]
    #[ORM\Column(name: 'idUsuario', type: 'integer')]
    private int $idUsuario;

    #[ORM\Column(name: 'user', type: 'string', length: 45)]
    private string $user;

    #[ORM\Column(name: 'password', type: 'string', length: 255)]
    private string $password;

    #[ORM\Column(name: 'nome', type: 'string', length: 150)]
    private string $nome;

    #[ORM\Column(name: 'tipo', type: 'string', length: 45)]
    private string $tipo;

    #[ORM\Column(name: 'idLoja', type: 'integer')]
    private int $idLoja;

    #[ORM\Column(name: 'email', type: 'string', length: 255, nullable: true)]
    private ?string $email = null;

    #[ORM\Column(name: 'senhaUsoUnico', type: 'string', length: 255, nullable: true)]
    private ?string $senhaUsoUnico = null;

    #[ORM\Column(name: 'valorMinimoFrete', type: 'decimal', precision: 10, scale: 2, nullable: true)]
    private ?string $valorMinimoFrete = null;

    #[ORM\Column(name: 'desativado', type: 'boolean', options: ['default' => false])]
    private bool $desativado = false;

    #[ORM\ManyToOne(targetEntity: Loja::class)]
    #[ORM\JoinColumn(name: 'idLoja', referencedColumnName: 'idLoja', nullable: false)]
    private Loja $loja;

    public function getIdUsuario(): int
    {
        return $this->idUsuario;
    }

    public function getUser(): string
    {
        return $this->user;
    }

    public function setUser(string $user): self
    {
        $this->user = $user;
        return $this;
    }

    public function getPassword(): string
    {
        return $this->password;
    }

    public function setPassword(string $password): self
    {
        $this->password = $password;
        return $this;
    }

    public function getNome(): string
    {
        return $this->nome;
    }

    public function setNome(string $nome): self
    {
        $this->nome = $nome;
        return $this;
    }

    public function getTipo(): string
    {
        return $this->tipo;
    }

    public function setTipo(string $tipo): self
    {
        $this->tipo = $tipo;
        return $this;
    }

    public function getIdLoja(): int
    {
        return $this->idLoja;
    }

    public function setIdLoja(int $idLoja): self
    {
        $this->idLoja = $idLoja;
        return $this;
    }

    public function getEmail(): ?string
    {
        return $this->email;
    }

    public function setEmail(?string $email): self
    {
        $this->email = $email;
        return $this;
    }

    public function getSenhaUsoUnico(): ?string
    {
        return $this->senhaUsoUnico;
    }

    public function setSenhaUsoUnico(?string $senhaUsoUnico): self
    {
        $this->senhaUsoUnico = $senhaUsoUnico;
        return $this;
    }

    public function getValorMinimoFrete(): ?string
    {
        return $this->valorMinimoFrete;
    }

    public function setValorMinimoFrete(?string $valorMinimoFrete): self
    {
        $this->valorMinimoFrete = $valorMinimoFrete;
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

    public function getLoja(): Loja
    {
        return $this->loja;
    }

    public function setLoja(Loja $loja): self
    {
        $this->loja = $loja;
        return $this;
    }

    // User type checks matching C++ User class
    public function isAdmin(): bool
    {
        return in_array($this->tipo, ['ADMINISTRADOR', 'DIRETOR']);
    }

    public function isAdministrativo(): bool
    {
        return in_array($this->tipo, ['ADMINISTRADOR', 'ADMINISTRATIVO', 'DIRETOR']);
    }

    public function isGerente(): bool
    {
        return in_array($this->tipo, ['GERENTE DEPARTAMENTO', 'GERENTE FINANCEIRO', 'GERENTE LOJA']);
    }

    public function isVendedor(): bool
    {
        return $this->tipo === 'VENDEDOR';
    }

    public function isEspecial(): bool
    {
        return $this->tipo === 'VENDEDOR ESPECIAL';
    }

    public function isVendedorOrEspecial(): bool
    {
        return in_array($this->tipo, ['VENDEDOR', 'VENDEDOR ESPECIAL']);
    }

    public function isOperacional(): bool
    {
        return $this->tipo === 'OPERACIONAL';
    }

    public function isAssistenteAdministrativo(): bool
    {
        return $this->tipo === 'ASSISTENTE ADMINISTRATIVO';
    }
}
