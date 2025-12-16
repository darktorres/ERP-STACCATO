<?php

namespace App\Entity;

use Doctrine\ORM\Mapping as ORM;
use DateTime;

/**
 * Read-only entity mapping to view_orcamento database view
 * Used for displaying quotation lists with computed fields
 */
#[ORM\Entity(readOnly: true)]
#[ORM\Table(name: 'view_orcamento')]
class OrcamentoView
{
    #[ORM\Id]
    #[ORM\Column(name: 'idOrcamento', type: 'string', length: 30)]
    private string $idOrcamento;

    #[ORM\Column(name: 'idLoja', type: 'integer')]
    private int $idLoja;

    #[ORM\Column(name: 'idUsuario', type: 'integer')]
    private int $idUsuario;

    #[ORM\Column(name: 'idUsuarioConsultor', type: 'integer', nullable: true)]
    private ?int $idUsuarioConsultor = null;

    #[ORM\Column(name: 'status', type: 'string', length: 45)]
    private string $status;

    #[ORM\Column(name: 'diasRestantes', type: 'integer', nullable: true)]
    private ?int $diasRestantes = null;

    #[ORM\Column(name: 'vendedor', type: 'string', length: 150, nullable: true)]
    private ?string $vendedor = null;

    #[ORM\Column(name: 'consultor', type: 'string', length: 150, nullable: true)]
    private ?string $consultor = null;

    #[ORM\Column(name: 'cliente', type: 'string', length: 150, nullable: true)]
    private ?string $cliente = null;

    #[ORM\Column(name: 'profissional', type: 'string', length: 150, nullable: true)]
    private ?string $profissional = null;

    #[ORM\Column(name: 'tel', type: 'string', length: 20, nullable: true)]
    private ?string $tel = null;

    #[ORM\Column(name: 'telCel', type: 'string', length: 20, nullable: true)]
    private ?string $telCel = null;

    #[ORM\Column(name: 'telProf', type: 'string', length: 20, nullable: true)]
    private ?string $telProf = null;

    #[ORM\Column(name: 'data', type: 'datetime', nullable: true)]
    private ?DateTime $data = null;

    #[ORM\Column(name: 'data2', type: 'string', length: 7, nullable: true)]
    private ?string $data2 = null; // YYYY-MM format

    #[ORM\Column(name: 'total', type: 'decimal', precision: 12, scale: 2, nullable: true)]
    private ?string $total = null;

    #[ORM\Column(name: 'idFollowup', type: 'integer', nullable: true)]
    private ?int $idFollowup = null;

    #[ORM\Column(name: 'dataFollowup', type: 'datetime', nullable: true)]
    private ?DateTime $dataFollowup = null;

    #[ORM\Column(name: 'dataProxFollowup', type: 'datetime', nullable: true)]
    private ?DateTime $dataProxFollowup = null;

    #[ORM\Column(name: 'observacao', type: 'string', length: 3000, nullable: true)]
    private ?string $observacao = null;

    #[ORM\Column(name: 'semaforo', type: 'integer', nullable: true)]
    private ?int $semaforo = null; // 1=QUENTE, 2=MORNO, 3=FRIO

    #[ORM\Column(name: 'fornecedores', type: 'string', length: 200, nullable: true)]
    private ?string $fornecedores = null;

    public function getIdOrcamento(): string
    {
        return $this->idOrcamento;
    }

    public function getIdLoja(): int
    {
        return $this->idLoja;
    }

    public function getIdUsuario(): int
    {
        return $this->idUsuario;
    }

    public function getIdUsuarioConsultor(): ?int
    {
        return $this->idUsuarioConsultor;
    }

    public function getStatus(): string
    {
        return $this->status;
    }

    public function getDiasRestantes(): ?int
    {
        return $this->diasRestantes;
    }

    public function getVendedor(): ?string
    {
        return $this->vendedor;
    }

    public function getConsultor(): ?string
    {
        return $this->consultor;
    }

    public function getCliente(): ?string
    {
        return $this->cliente;
    }

    public function getProfissional(): ?string
    {
        return $this->profissional;
    }

    public function getTel(): ?string
    {
        return $this->tel;
    }

    public function getTelCel(): ?string
    {
        return $this->telCel;
    }

    public function getTelProf(): ?string
    {
        return $this->telProf;
    }

    public function getData(): ?DateTime
    {
        return $this->data;
    }

    public function getData2(): ?string
    {
        return $this->data2;
    }

    public function getTotal(): ?string
    {
        return $this->total;
    }

    public function getIdFollowup(): ?int
    {
        return $this->idFollowup;
    }

    public function getDataFollowup(): ?DateTime
    {
        return $this->dataFollowup;
    }

    public function getDataProxFollowup(): ?DateTime
    {
        return $this->dataProxFollowup;
    }

    public function getObservacao(): ?string
    {
        return $this->observacao;
    }

    public function getSemaforo(): ?int
    {
        return $this->semaforo;
    }

    public function getFornecedores(): ?string
    {
        return $this->fornecedores;
    }

    /**
     * Convert to API response format
     */
    public function toArray(): array
    {
        return [
            'idOrcamento' => $this->idOrcamento,
            'idLoja' => $this->idLoja,
            'idUsuario' => $this->idUsuario,
            'idUsuarioConsultor' => $this->idUsuarioConsultor,
            'status' => $this->status,
            'diasRestantes' => $this->diasRestantes !== null ? (string)$this->diasRestantes : '-',
            'vendedor' => $this->vendedor,
            'consultor' => $this->consultor,
            'cliente' => $this->cliente,
            'profissional' => $this->profissional,
            'tel' => $this->tel,
            'telCel' => $this->telCel,
            'telProf' => $this->telProf,
            'data' => $this->data?->format('c'),
            'data2' => $this->data2,
            'total' => $this->total,
            'dataFollowup' => $this->dataFollowup?->format('c'),
            'dataProxFollowup' => $this->dataProxFollowup?->format('c'),
            'observacao' => $this->observacao,
            'semaforo' => $this->semaforo,
            'fornecedores' => $this->fornecedores,
        ];
    }
}
