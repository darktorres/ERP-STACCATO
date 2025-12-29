# Análise: Event Sourcing para o ERP

> **Status**: Avaliado e **não adotado** para v1
> **Data**: 2025-12-28
> **Motivo**: Complexidade vs benefício para equipe pequena

---

## O que é Event Sourcing?

Em vez de armazenar o estado atual, armazena-se todos os eventos que levaram ao estado atual:

```text
CRUD Tradicional:
┌─────────────────┐
│ produtos        │
│ valor_venda: 48 │  ← só o valor atual
└─────────────────┘

Event Sourcing:
┌─────────────────────────────────────────────┐
│ events                                       │
│ 1. ProdutoCriado { valor_venda: 40 }        │
│ 2. PrecoAlterado { anterior: 40, novo: 45 } │
│ 3. PrecoAlterado { anterior: 45, novo: 48 } │
└─────────────────────────────────────────────┘
         │
         ▼ replay
┌─────────────────┐
│ produtos        │
│ valor_venda: 48 │  ← projeção derivada
└─────────────────┘
```

---

## Arquitetura Proposta (Se Fosse Adotado)

```text
┌─────────────────────────────────────────────────────────────┐
│                     EVENT STORE                              │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ events (append-only, imutável)                       │    │
│  │ - id, aggregate_type, aggregate_id, event_type,     │    │
│  │   payload (JSONB), created_at, version              │    │
│  └─────────────────────────────────────────────────────┘    │
│                           │                                  │
│           ┌───────────────┼───────────────┐                 │
│           ▼               ▼               ▼                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  produtos   │  │   vendas    │  │  estoques   │         │
│  │ (projeção)  │  │ (projeção)  │  │ (projeção)  │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│    Estado atual    Estado atual     Estado atual            │
└─────────────────────────────────────────────────────────────┘
```

### Schema do Event Store

```sql
-- Event Store (única fonte da verdade)
CREATE TABLE events (
    id BIGSERIAL PRIMARY KEY,
    aggregate_type VARCHAR(50) NOT NULL,  -- 'Produto', 'Venda', 'Estoque'
    aggregate_id INTEGER NOT NULL,
    event_type VARCHAR(100) NOT NULL,     -- 'PrecoCriado', 'PrecoAlterado'
    payload JSONB NOT NULL,
    metadata JSONB,                        -- user_id, ip, etc
    version INTEGER NOT NULL,              -- para optimistic locking
    created_at TIMESTAMPTZ DEFAULT NOW(),

    UNIQUE(aggregate_type, aggregate_id, version)
);

CREATE INDEX idx_events_aggregate ON events(aggregate_type, aggregate_id, version);
CREATE INDEX idx_events_type ON events(event_type);
CREATE INDEX idx_events_created ON events(created_at);

-- Projeções (materialized views, atualizadas por eventos)
CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,
    descricao VARCHAR(500),
    valor_venda DECIMAL(15,4),
    -- ... estado atual
    last_event_id BIGINT,  -- para saber até qual evento está sincronizado
    updated_at TIMESTAMPTZ
);
```

### Exemplo de Eventos

```json
// ProdutoCriado
{
  "aggregate_type": "Produto",
  "aggregate_id": 123,
  "event_type": "ProdutoCriado",
  "payload": {
    "descricao": "Porcelanato 60x60",
    "ncm": "69072100",
    "unidade": "M2"
  },
  "version": 1
}

// PrecoDefinido
{
  "aggregate_type": "Produto",
  "aggregate_id": 123,
  "event_type": "PrecoDefinido",
  "payload": {
    "custo": 30.00,
    "valor_venda": 45.00,
    "margem": 0.50
  },
  "version": 2
}

// PrecoAlterado
{
  "aggregate_type": "Produto",
  "aggregate_id": 123,
  "event_type": "PrecoAlterado",
  "payload": {
    "custo_anterior": 30.00,
    "custo_novo": 32.00,
    "valor_venda_anterior": 45.00,
    "valor_venda_novo": 48.00,
    "motivo": "Aumento do fornecedor"
  },
  "version": 3
}
```

---

## Prós e Contras

### Benefícios

| Benefício | Descrição |
|-----------|-----------|
| **Audit trail grátis** | Todo histórico já está nos eventos |
| **Point-in-time** | Pode reconstruir estado em qualquer momento |
| **Debugging** | "Por que o preço está errado?" → replay eventos |
| **Múltiplas projeções** | Diferentes views dos mesmos dados |
| **Compliance** | LGPD, auditoria fiscal facilitados |
| **Correção de bugs** | Corrigir projeção e re-replay |
| **Temporal queries** | Grátis - só filtrar eventos por data |

### Problemas

| Problema | Impacto | Severidade |
|----------|---------|------------|
| **Complexidade** | Curva de aprendizado significativa | Alta |
| **Eventual consistency** | Projeções podem estar atrasadas | Média |
| **Queries cross-aggregate** | JOINs ficam mais difíceis | Alta |
| **Schema evolution** | Eventos antigos com formato diferente | Média |
| **Storage** | Eventos nunca deletados | Baixa |
| **Rebuild time** | Reconstruir projeção de milhões de eventos | Média |
| **Tooling** | Menos ferramentas prontas que CRUD | Média |
| **Debugging paradoxo** | ES mal implementado é pior de debugar | Alta |

---

## Comparação de Complexidade

### CRUD + Audit Log (Abordagem Atual)

```php
// Simples e direto
$produto->update(['valor_venda' => 48.00]);

// Audit log captura automaticamente via trigger PostgreSQL
// Histórico disponível na tabela audit_log
```

**Linhas de código**: ~5
**Arquivos envolvidos**: 1 (Controller ou Service)

### Event Sourcing

```php
// 1. Command
class AlterarPrecoCommand {
    public function __construct(
        public int $produtoId,
        public float $novoPreco,
        public string $motivo
    ) {}
}

// 2. Handler
class AlterarPrecoHandler {
    public function __construct(
        private ProdutoRepository $repository,
        private EventStore $eventStore
    ) {}

    public function handle(AlterarPrecoCommand $cmd): void {
        $produto = $this->repository->load($cmd->produtoId);
        $produto->alterarPreco($cmd->novoPreco, $cmd->motivo);
        $this->repository->save($produto);
    }
}

// 3. Aggregate
class Produto extends AggregateRoot {
    private float $valorVenda;

    public function alterarPreco(float $novo, string $motivo): void {
        $this->recordEvent(new PrecoAlterado(
            $this->id,
            $this->valorVenda,
            $novo,
            $motivo
        ));
    }

    protected function applyPrecoAlterado(PrecoAlterado $event): void {
        $this->valorVenda = $event->novoPreco;
    }
}

// 4. Event
class PrecoAlterado implements DomainEvent {
    public function __construct(
        public int $produtoId,
        public float $precoAnterior,
        public float $novoPreco,
        public string $motivo
    ) {}
}

// 5. Projector
class ProdutoProjector {
    public function onPrecoAlterado(PrecoAlterado $event): void {
        DB::table('produtos')
            ->where('id', $event->produtoId)
            ->update([
                'valor_venda' => $event->novoPreco,
                'updated_at' => now()
            ]);
    }
}

// 6. Event Store persistence
// 7. Projection rebuilder
// 8. Snapshot mechanism (para aggregates com muitos eventos)
```

**Linhas de código**: ~100+
**Arquivos envolvidos**: 6+
**Infraestrutura adicional**: Event bus, projection workers

---

## Análise para o ERP Staccato

### Onde Faria Sentido

| Entidade | Motivo | Eventos Exemplo |
|----------|--------|-----------------|
| **Estoque** | Movimentações são naturalmente eventos | EntradaRegistrada, ConsumoRealizado, EstornoFeito |
| **Vendas** | Ciclo de vida com muitas transições | VendaCriada, ItemAdicionado, StatusAlterado |
| **Financeiro** | Auditoria crítica, reconciliação | ParcelaCriada, PagamentoRecebido, BaixaRealizada |
| **NFe** | Histórico de tentativas, status | NFeGerada, EnviadaSefaz, Autorizada, Rejeitada |

### Onde é Overkill

| Entidade | Motivo |
|----------|--------|
| **Produtos (cadastro)** | CRUD simples, audit log suficiente |
| **Clientes** | Muda raramente, audit log suficiente |
| **Fornecedores** | Muda raramente |
| **Configurações** | Lookup tables |
| **Usuários** | CRUD simples |
| **Transportadoras** | Dados mestres simples |

---

## Abordagem Híbrida (Se Fosse Implementar)

```text
┌────────────────────────────────────────────────────────────┐
│                                                            │
│   Event Sourcing              CRUD + Audit Log            │
│   ┌────────────────┐          ┌────────────────┐          │
│   │ estoque_eventos│          │ produtos       │          │
│   │ venda_eventos  │          │ clientes       │          │
│   │ financ_eventos │          │ fornecedores   │          │
│   │ nfe_eventos    │          │ usuarios       │          │
│   └────────────────┘          │ config         │          │
│          │                    └────────────────┘          │
│          │                           │                    │
│          ▼                           ▼                    │
│   ┌────────────────┐          ┌────────────────┐          │
│   │ Projeções      │          │ audit_log      │          │
│   │ (pg_ivm)       │          │ (trigger)      │          │
│   └────────────────┘          └────────────────┘          │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

| Camada | Abordagem | Entidades |
|--------|-----------|-----------|
| **Core Domain** | Event Sourcing | Estoque, Vendas, Financeiro |
| **Supporting** | CRUD + Audit | Produtos, Clientes, Fornecedores |
| **Generic** | CRUD simples | Config, Lookup tables |

---

## Ferramentas/Bibliotecas para Laravel

Se fosse implementar no futuro:

| Biblioteca | Descrição | GitHub Stars |
|------------|-----------|--------------|
| **spatie/laravel-event-sourcing** | Mais popular, bem documentada | 1.5k+ |
| **EventSaucePHP** | Mais opinativo, boas práticas | 800+ |
| **prooph/event-store** | Mais complexo, mais flexível | 500+ |

### Exemplo com Spatie

```php
// Aggregate
use Spatie\EventSourcing\AggregateRoots\AggregateRoot;

class EstoqueAggregate extends AggregateRoot
{
    private float $quantidadeDisponivel = 0;

    public function registrarEntrada(float $quantidade, int $nfeItemId): self
    {
        $this->recordThat(new EntradaRegistrada(
            $this->uuid(),
            $quantidade,
            $nfeItemId
        ));

        return $this;
    }

    protected function applyEntradaRegistrada(EntradaRegistrada $event): void
    {
        $this->quantidadeDisponivel += $event->quantidade;
    }

    public function consumir(float $quantidade, int $vendaItemId): self
    {
        if ($quantidade > $this->quantidadeDisponivel) {
            throw new EstoqueInsuficienteException();
        }

        $this->recordThat(new ConsumoRealizado(
            $this->uuid(),
            $quantidade,
            $vendaItemId
        ));

        return $this;
    }

    protected function applyConsumoRealizado(ConsumoRealizado $event): void
    {
        $this->quantidadeDisponivel -= $event->quantidade;
    }
}

// Uso
EstoqueAggregate::retrieve($estoqueId)
    ->consumir(10, $vendaItemId)
    ->persist();
```

---

## Decisão Final

### Por que NÃO adotar para v1?

1. **Equipe pequena** - Event Sourcing requer expertise específica que precisa ser desenvolvida
2. **Complexidade desnecessária** - 80% das entidades são CRUD simples
3. **Time to market** - Adiciona 2-4 meses ao desenvolvimento
4. **Risco** - ES mal implementado é pior que CRUD bem feito
5. **Audit log suficiente** - Para a maioria dos casos de uso, audit_log resolve

### Quando reconsiderar?

- [ ] Auditoria fiscal exigir reconstrução de estado histórico
- [ ] Necessidade de "replay" para corrigir dados
- [ ] Múltiplas projeções do mesmo dado (dashboards diferentes)
- [ ] Equipe crescer e ter expertise em ES
- [ ] Requisitos de compliance mais rigorosos

### Abordagem Adotada

```text
Fase 1 (v1): CRUD + Audit Log para tudo
Fase 2:      Identificar pain points específicos
Fase 3:      Migrar entidades críticas para ES se necessário
```

---

## Referências

- [Martin Fowler - Event Sourcing](https://martinfowler.com/eaaDev/EventSourcing.html)
- [Spatie Laravel Event Sourcing](https://spatie.be/docs/laravel-event-sourcing)
- [Event Sourcing in Practice](https://eventstore.com/blog/event-sourcing-in-practice/)
- [Why Event Sourcing is Hard](https://chriskiehl.com/article/event-sourcing-is-hard)

---

## Documentos Relacionados

- [04-infraestrutura.md](../tecnico/04-infraestrutura.md) - Seção 2: Dados Temporais
- [02-decisoes.md](./02-decisoes.md) - ADRs do projeto
- [07-esquema-redesenhado.md](./07-esquema-redesenhado.md) - Schema atual (CRUD)
