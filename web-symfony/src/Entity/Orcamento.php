<?php

namespace App\Entity;

use Doctrine\ORM\Mapping as ORM;
use DateTime;

#[ORM\Entity]
#[ORM\Table(name: 'orcamento')]
class Orcamento
{
    #[ORM\Id]
    #[ORM\Column(name: 'idOrcamento', type: 'string', length: 30)]
    private string $idOrcamento;

    #[ORM\Column(name: 'idOrcamentoBase', type: 'string', length: 11, nullable: true)]
    private ?string $idOrcamentoBase = null;

    #[ORM\Column(name: 'idLoja', type: 'integer')]
    private int $idLoja;

    #[ORM\Column(name: 'idUsuario', type: 'integer')]
    private int $idUsuario;

    #[ORM\Column(name: 'idUsuarioConsultor', type: 'integer', nullable: true)]
    private ?int $idUsuarioConsultor = null;

    #[ORM\Column(name: 'idCliente', type: 'integer', nullable: true)]
    private ?int $idCliente = null;

    #[ORM\Column(name: 'idProfissional', type: 'integer', nullable: true)]
    private ?int $idProfissional = null;

    #[ORM\Column(type: 'datetime')]
    private DateTime $data;

    #[ORM\Column(type: 'string', length: 45, options: ['default' => 'ATIVO'])]
    private string $status = 'ATIVO';

    #[ORM\Column(type: 'decimal', precision: 12, scale: 2, nullable: true)]
    private ?string $total = null;

    #[ORM\Column(type: 'integer', options: ['default' => 7])]
    private int $validade = 7;

    #[ORM\Column(type: 'decimal', precision: 10, scale: 2, nullable: true)]
    private ?string $frete = null;

    #[ORM\Column(name: 'descontoPorc', type: 'decimal', precision: 10, scale: 2, nullable: true)]
    private ?string $descontoPorc = null;

    #[ORM\Column(name: 'descontoReais', type: 'decimal', precision: 10, scale: 2, nullable: true)]
    private ?string $descontoReais = null;

    #[ORM\Column(type: 'boolean', options: ['default' => false])]
    private bool $representacao = false;

    #[ORM\Column(name: 'replicadoDe', type: 'string', length: 30, nullable: true)]
    private ?string $replicadoDe = null;

    #[ORM\Column(name: 'replicadoEm', type: 'string', length: 30, nullable: true)]
    private ?string $replicadoEm = null;

    #[ORM\Column(type: 'string', length: 200, nullable: true)]
    private ?string $fornecedores = null;

    #[ORM\Column(type: 'string', length: 3000, nullable: true)]
    private ?string $observacao = null;

    #[ORM\ManyToOne(targetEntity: Loja::class)]
    #[ORM\JoinColumn(name: 'idLoja', referencedColumnName: 'idLoja', nullable: false)]
    private Loja $loja;

    #[ORM\ManyToOne(targetEntity: Usuario::class)]
    #[ORM\JoinColumn(name: 'idUsuario', referencedColumnName: 'idUsuario', nullable: false)]
    private Usuario $usuario;

    #[ORM\ManyToOne(targetEntity: Usuario::class)]
    #[ORM\JoinColumn(name: 'idUsuarioConsultor', referencedColumnName: 'idUsuario', nullable: true)]
    private ?Usuario $usuarioConsultor = null;

    public function getIdOrcamento(): string
    {
        return $this->idOrcamento;
    }

    public function setIdOrcamento(string $idOrcamento): self
    {
        $this->idOrcamento = $idOrcamento;
        return $this;
    }

    public function getIdOrcamentoBase(): ?string
    {
        return $this->idOrcamentoBase;
    }

    public function setIdOrcamentoBase(?string $idOrcamentoBase): self
    {
        $this->idOrcamentoBase = $idOrcamentoBase;
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

    public function getIdUsuario(): int
    {
        return $this->idUsuario;
    }

    public function setIdUsuario(int $idUsuario): self
    {
        $this->idUsuario = $idUsuario;
        return $this;
    }

    public function getIdUsuarioConsultor(): ?int
    {
        return $this->idUsuarioConsultor;
    }

    public function setIdUsuarioConsultor(?int $idUsuarioConsultor): self
    {
        $this->idUsuarioConsultor = $idUsuarioConsultor;
        return $this;
    }

    public function getIdCliente(): ?int
    {
        return $this->idCliente;
    }

    public function setIdCliente(?int $idCliente): self
    {
        $this->idCliente = $idCliente;
        return $this;
    }

    public function getIdProfissional(): ?int
    {
        return $this->idProfissional;
    }

    public function setIdProfissional(?int $idProfissional): self
    {
        $this->idProfissional = $idProfissional;
        return $this;
    }

    public function getData(): DateTime
    {
        return $this->data;
    }

    public function setData(DateTime $data): self
    {
        $this->data = $data;
        return $this;
    }

    public function getStatus(): string
    {
        return $this->status;
    }

    public function setStatus(string $status): self
    {
        $this->status = $status;
        return $this;
    }

    public function getTotal(): ?string
    {
        return $this->total;
    }

    public function setTotal(?string $total): self
    {
        $this->total = $total;
        return $this;
    }

    public function getValidade(): int
    {
        return $this->validade;
    }

    public function setValidade(int $validade): self
    {
        $this->validade = $validade;
        return $this;
    }

    public function getFrete(): ?string
    {
        return $this->frete;
    }

    public function setFrete(?string $frete): self
    {
        $this->frete = $frete;
        return $this;
    }

    public function getDescontoPorc(): ?string
    {
        return $this->descontoPorc;
    }

    public function setDescontoPorc(?string $descontoPorc): self
    {
        $this->descontoPorc = $descontoPorc;
        return $this;
    }

    public function getDescontoReais(): ?string
    {
        return $this->descontoReais;
    }

    public function setDescontoReais(?string $descontoReais): self
    {
        $this->descontoReais = $descontoReais;
        return $this;
    }

    public function isRepresentacao(): bool
    {
        return $this->representacao;
    }

    public function setRepresentacao(bool $representacao): self
    {
        $this->representacao = $representacao;
        return $this;
    }

    public function getReplicadoDe(): ?string
    {
        return $this->replicadoDe;
    }

    public function setReplicadoDe(?string $replicadoDe): self
    {
        $this->replicadoDe = $replicadoDe;
        return $this;
    }

    public function getReplicadoEm(): ?string
    {
        return $this->replicadoEm;
    }

    public function setReplicadoEm(?string $replicadoEm): self
    {
        $this->replicadoEm = $replicadoEm;
        return $this;
    }

    public function getFornecedores(): ?string
    {
        return $this->fornecedores;
    }

    public function setFornecedores(?string $fornecedores): self
    {
        $this->fornecedores = $fornecedores;
        return $this;
    }

    public function getObservacao(): ?string
    {
        return $this->observacao;
    }

    public function setObservacao(?string $observacao): self
    {
        $this->observacao = $observacao;
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

    public function getUsuario(): Usuario
    {
        return $this->usuario;
    }

    public function setUsuario(Usuario $usuario): self
    {
        $this->usuario = $usuario;
        return $this;
    }

    public function getUsuarioConsultor(): ?Usuario
    {
        return $this->usuarioConsultor;
    }

    public function setUsuarioConsultor(?Usuario $usuarioConsultor): self
    {
        $this->usuarioConsultor = $usuarioConsultor;
        return $this;
    }

    /**
     * Calculate remaining days of validity
     * Returns: creation_date + validity_days - today
     * Can be negative if expired
     */
    public function getDiasRestantes(): int
    {
        $today = new DateTime();
        $expireDate = clone $this->data;
        $expireDate->modify("+{$this->validade} days");

        return (int)$today->diff($expireDate)->format('%r%d');
    }

    /**
     * Check if quote is expired based on validity days
     */
    public function isExpired(): bool
    {
        return $this->getDiasRestantes() < 0;
    }
}
