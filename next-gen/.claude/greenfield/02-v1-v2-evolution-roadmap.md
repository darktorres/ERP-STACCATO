# V1 → V2+ Evolution Roadmap

> **Status**: Strategic planning document for incremental architecture evolution
> **Date**: 2026-01-10
> **Purpose**: Bridge pragmatic v1 design (proposed schema) with enterprise v2+ patterns (greenfield design)
> **Audience**: Technical architects, senior developers planning future enhancements

---

## Executive Summary

The ERP system is designed as a **two-horizon architecture**:

- **v1.0 (Current)**: Pragmatic, simplified schema focusing on rapid market entry and core functionality
- **v2.0+ (Future)**: Enterprise patterns including full Event Sourcing, rich Fulfillment model, advanced state machines

This document outlines the **incremental migration path** from v1→v2, showing:
- What v1 gets right (foundation for v2)
- What v1 defers (planned for v2+)
- How to evolve without rewriting
- Technical debt and design decisions

---

## Part 1: V1 Status (Proposed Schema)

### V1 Architecture Characteristics

| Aspect | v1 Approach | Rationale | v2 Plan |
|--------|------------|-----------|---------|
| **State Storage** | Normal CRUD tables + audit log | Familiar, simple to implement | Full Event Sourcing |
| **Allocation Model** | M:N Alocacao (simple) | Supports FIFO/FEFO, good enough | Rich Fulfillment lifecycle |
| **Order Items** | Mutable VendaItem | Allows splits, corrections | Immutable OrderItem (pristine) |
| **Returns** | is_estornado flag | Simple reversal tracking | Explicit StockReturn entity |
| **Event Trail** | Append-only *_events tables | Audit trail + foundation | CQRS + replay architecture |
| **Delivery** | Indirect (EntregaItem junction) | Loose coupling | Direct (Fulfillment.delivery_id) |
| **Stock States** | 5 states (RECEBIDO/DISPONIVEL/RESERVADO/CONSUMIDO/CANCELADO) | Simple workflows | 8 states with rich transitions |

### V1 Readiness Level

**✅ Ready for Production:**
- M:N allocation model with quantity tracking
- Mutable items for business flexibility
- Hybrid event sourcing foundation (append-only events + immutability triggers)
- Soft-delete pattern for reversals (is_estornado)
- Unified financial tables (recebiveis + pagaveis → financeiro_parcelas)
- SCD-2 product versioning
- FIFO/FEFO stock selection

**⚠️ Simplified (Good Enough for v1):**
- Fulfillment lifecycle (single Alocacao table vs rich state machine)
- Event auditing (record events in tables, but no replay/reconstruction)
- Stock state transitions (linear flow, not full state machine)
- Delivery integration (junction table vs direct link)

**❌ Deferred to v2+:**
- Full Event Sourcing (CQRS pattern, state reconstruction)
- Point-in-time query capability
- Temporal analytics on entity lifecycle
- Advanced audit trail querying
- Polymorphic event handlers

---

## Part 2: V2 Vision (Greenfield Features)

### V2 Enhancements from Greenfield

**Phase 2.1 - Fulfillment Model Upgrade** (6-8 weeks after v1 launch)

```sql
-- Add Fulfillment entity (separate lifecycle tracking)
CREATE TABLE fulfillments (
    id BIGSERIAL PRIMARY KEY,
    alocacao_id BIGINT UNIQUE REFERENCES alocacoes(id),

    -- Fulfillment states: RESERVED → PICKED → PACKED → IN_TRANSIT → DELIVERED
    status fulfillment_status_enum NOT NULL DEFAULT 'RESERVED',

    -- Tracking
    data_reserva TIMESTAMPTZ,
    data_coleta TIMESTAMPTZ,
    data_embalagem TIMESTAMPTZ,
    data_envio TIMESTAMPTZ,
    data_entrega TIMESTAMPTZ,

    -- Integration
    entrega_id INTEGER REFERENCES entregas(id),
    nfe_id INTEGER REFERENCES nfes(id),

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_fulfillments_alocacao ON fulfillments(alocacao_id);
CREATE INDEX idx_fulfillments_status ON fulfillments(status);
```

**Benefits:**
- Explicit lifecycle tracking (currently implicit in Alocacao)
- Separate concern from allocation (what stock item) and fulfillment (how/when delivered)
- Enable fulfillment-specific workflows and metrics
- Track pick/pack efficiency

**Migration Path:**
1. Create new `fulfillments` table (alongside existing schema)
2. Add foreign key from alocacoes to fulfillments (optional)
3. Gradually migrate business logic to use fulfillments
4. Keep alocacoes unchanged for backward compatibility
5. After v1.5: Retire separate fulfillment tracking

---

**Phase 2.2 - Order Item Immutability** (3-4 weeks in v1.5)

```sql
-- Current v1: venda_itens is mutable, track changes via audit
-- v2: Create immutable OrderItem record + VendaAdjustment audit trail

CREATE TABLE venda_itens_original (
    id BIGSERIAL PRIMARY KEY,
    venda_id INTEGER NOT NULL REFERENCES vendas(id),
    produto_id INTEGER NOT NULL REFERENCES produtos(id),

    -- Original pristine record (IMMUTABLE after insert)
    quantidade_original DECIMAL(15,4) NOT NULL,
    preco_unitario_original DECIMAL(15,2) NOT NULL,

    -- Immutable audit
    criado_em TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    criado_por INTEGER NOT NULL REFERENCES usuarios(id),

    -- Prevent modifications
    CONSTRAINT venda_itens_original_immutable CHECK (FALSE)
);

CREATE TABLE venda_ajustes (
    id BIGSERIAL PRIMARY KEY,
    venda_item_id BIGINT NOT NULL REFERENCES venda_itens(id),

    tipo_ajuste adjustment_type_enum,  -- SPLIT, QUANTITY_CHANGE, CANCEL, etc
    quantidade_anterior DECIMAL(15,4),
    quantidade_nova DECIMAL(15,4),
    motivo VARCHAR(500),

    ajustado_em TIMESTAMPTZ DEFAULT NOW(),
    ajustado_por INTEGER REFERENCES usuarios(id),

    -- Link to resulting items (if SPLIT)
    resultado_item_ids BIGINT[] -- array of new item IDs
);
```

**Benefits:**
- Preserve original order as customer placed it (compliance, auditing)
- Clear history of what changed and why
- Distinguish between order vs fulfillment changes
- Enable accurate historical reporting

**Migration Path:**
1. Create immutable_venda_itens view (SELECT from venda_itens at creation)
2. Create venda_ajustes table to track changes
3. Modify VendaService to log adjustments to venda_ajustes
4. Add immutability check to prevent direct updates (soft enforcement)
5. Post-v2.0: Strong enforcement (trigger prevents UPDATE)

---

**Phase 2.3 - Full Event Sourcing Foundation** (8-10 weeks post v1.2)

Currently v1 has hybrid event sourcing:
- ✅ Append-only *_events tables
- ✅ Immutability triggers (fn_prevent_mutation)
- ✅ Event recording in services
- ❌ No event replay/reconstruction
- ❌ No CQRS command/query separation

**Path to Full ES:**

```php
// v1: Current approach (separate state + events)
class EstoqueService {
    public function registrarEntrada(EstoqueLote $lote, $quantidade) {
        // 1. Update state table
        $lote->quantidade_disponivel += $quantidade;
        $lote->save();

        // 2. Record event (separate table)
        EstoqueMovimentacao::create([
            'estoque_lote_id' => $lote->id,
            'tipo' => 'ENTRADA',
            'quantidade' => $quantidade,
            'usuario_id' => auth()->id(),
        ]);
    }
}

// v2: Event-first approach (state derived from events)
class EstoqueAggregate extends AggregateRoot {
    private $quantidadeDisponivel = 0;

    public function registrarEntrada($quantidade, $nfeItemId) {
        // 1. Record event (primary)
        $this->recordEvent(new EntradaRegistrada(
            $this->id,
            $quantidade,
            $nfeItemId,
            now(),
            auth()->id()
        ));
    }

    // 2. Apply event to update state (derived)
    protected function applyEntradaRegistrada(EntradaRegistrada $event) {
        $this->quantidadeDisponivel += $event->quantidade;
    }

    // 3. Persistence reconstructs from events
    public function persist() {
        EventStore::store($this->pendingEvents);
        EstoqueProjector::project($this);  // Rebuild materialized view
    }
}
```

**Benefits:**
- Point-in-time reconstruction (audit regulatory requirement)
- Event replay for bug fixes (test fix, replay, compare)
- Temporal analytics (what was stock level on date X?)
- Compliance-friendly audit trail

**Phased Implementation:**
1. **Phase 2.3a** (v1.3): Infrastructure
   - Deploy EventStore table
   - Create event projector framework
   - Start recording to both systems

2. **Phase 2.3b** (v1.4): Gradual Migration
   - Migrate one entity at a time (lowest risk)
   - Estoque first (most event-like)
   - Then Venda, then Financeiro

3. **Phase 2.3c** (v2.0): Full Separation
   - CQRS pattern: Commands → Events → Projections
   - State tables become materialized views
   - Old state tables deprecated

4. **Phase 2.3d** (v2.1): Advanced Features
   - Temporal snapshots
   - Event versioning for schema evolution
   - Multi-model projections

---

**Phase 2.4 - Rich Stock State Machine** (4-6 weeks in v1.5)

Current v1 states (simplified):
- RECEBIDO → DISPONIVEL → RESERVADO → CONSUMIDO → CANCELADO

Greenfield v2 vision (rich):
- DISPONIVEL → RESERVED → PICKED → PACKED → IN_TRANSIT → DELIVERED
- Plus: RETURN_INITIATED → RETURN_IN_TRANSIT → RETURN_RECEIVED → RESTOCKED

**Implementation Strategy:**

```sql
-- v1: Simple state enum
ALTER TYPE estoque_status_enum ADD VALUE 'PICKING';
ALTER TYPE estoque_status_enum ADD VALUE 'PACKED';
ALTER TYPE estoque_status_enum ADD VALUE 'IN_TRANSIT';
ALTER TYPE estoque_status_enum ADD VALUE 'DELIVERED';

-- v2: State machine with explicit transitions
CREATE TABLE estoque_transicoes (
    id BIGSERIAL PRIMARY KEY,
    estoque_lote_id BIGINT NOT NULL REFERENCES estoque_lotes(id),

    status_anterior estoque_status_enum NOT NULL,
    status_novo estoque_status_enum NOT NULL,

    -- Why and who
    motivo VARCHAR(255),
    usuario_id INTEGER NOT NULL REFERENCES usuarios(id),

    -- Timestamp and metadata
    transicionado_em TIMESTAMPTZ DEFAULT NOW(),
    dados_adicionais JSONB  -- for capture fulfillment_id, entrega_id, etc
);

-- Enforce valid transitions (constraint or trigger)
CREATE FUNCTION validate_estoque_transition()
RETURNS TRIGGER AS $$
BEGIN
    -- DISPONIVEL can only go to RESERVADO
    -- RESERVADO can only go to PICKING
    -- etc...
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_validate_estoque_transition
BEFORE UPDATE ON estoque_lotes
FOR EACH ROW EXECUTE validate_estoque_transition();
```

**Benefits:**
- Explicit state machine prevents invalid transitions
- Track reason for each state change
- Audit trail of lifecycle
- Foundation for workflow rules

**Migration Path:**
1. Add new states to enum (additive only)
2. Create transicoes table to track changes
3. Modify EstoqueService to log transitions
4. In v2.0: Add validation trigger to prevent invalid transitions
5. After adoption: Retire simple status field (keep for read-only compatibility)

---

**Phase 2.5 - Explicit Returns Model** (3-4 weeks in v2.0)

Current v1 approach: `is_estornado` flag + `estorno_motivo` text

Greenfield v2 approach: Explicit `StockReturn` entity with full lifecycle

```sql
-- v2 addition
CREATE TABLE estoque_devolucoes (
    id BIGSERIAL PRIMARY KEY,
    alocacao_id BIGINT NOT NULL REFERENCES alocacoes(id),

    -- What's being returned
    quantidade_devolvida DECIMAL(15,4) NOT NULL,
    motivo_devolucao return_reason_enum NOT NULL,  -- DEFECTIVE, OVER_SHIPPED, WRONG_ITEM, etc
    descricao_problema TEXT,

    -- Process tracking
    data_solicitacao TIMESTAMPTZ DEFAULT NOW(),
    data_aprovacao TIMESTAMPTZ,
    data_recebimento TIMESTAMPTZ,

    -- Restock decision
    pode_reestoquear BOOLEAN DEFAULT TRUE,
    motivo_nao_reestoque VARCHAR(255),

    -- Inspection
    inspecionado_por INTEGER REFERENCES usuarios(id),
    condicao_fisica return_condition_enum,  -- GOOD, DAMAGED, CONTAMINATED, etc

    -- Financial impact
    credito_valor DECIMAL(15,2),
    desconto_restituicao DECIMAL(15,2),  -- If not full credit

    created_at TIMESTAMPTZ DEFAULT NOW(),
    updated_at TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX idx_estoque_devolucoes_alocacao ON estoque_devolucoes(alocacao_id);
CREATE INDEX idx_estoque_devolucoes_status ON estoque_devolucoes(data_aprovacao, data_recebimento);
```

**Benefits:**
- Explicit return workflow (request → approve → receive → inspect → restock/scrap)
- Traceability of returned items
- Separate financial impact calculation
- Integration with warranty/support system

**Current v1 Migration Path:**
1. Create estoque_devolucoes table (parallel to is_estornado flag)
2. Add trigger: when alocacao.is_estornado = true, create estoque_devolucoes record
3. Gradually migrate code to create explicit returns instead of setting flag
4. Post-v2: Retire is_estornado flag, use estoque_devolucoes only

---

## Part 3: Technical Debt and Design Trade-offs

### V1 Simplified Decisions (Can Be Enhanced)

| Decision | v1 Trade-off | v2 Enhancement | Priority |
|----------|-------------|-----------------|----------|
| M:N Allocation (single table) | Less structure, queries simpler | Fulfillment adds rich lifecycle | Medium |
| Mutable VendaItem | Allows corrections, changes | Immutable OrderItem + adjustment trail | Low |
| is_estornado flag | Simple reversal | Explicit StockReturn entity | Medium |
| Indirect delivery (EntregaItem) | Loose coupling | Direct Fulfillment.delivery_id | Low |
| 5 stock states | Fewer transitions to validate | 8 states + state machine | Low |
| Single Alocacao record | No substate tracking | Fulfillment with pick/pack tracking | Medium |

### Strategic Implementation Path

**What Blocks Nothing (Can Do Independently):**
- Fulfillment model (new entity, no breaking changes)
- Order item immutability (new view + adjustment tracking)
- Returns entity (new table, parallel to flag)

**What Requires Coordination (Sequential):**
- Event Sourcing (must gradually migrate entities)
- Stock state machine (affects all stock operations)
- State transitions (impacts business logic layer)

**What Depends on Others (Ordered):**
- Event Sourcing → Point-in-time queries → Temporal analytics
- Fulfillment → Advanced metrics → SLA tracking
- Immutable items + Fulfillment → Complete order lifecycle audit

---

## Part 4: Implementation Timeline Estimates

### Conservative Approach (Feature-Safe)

```
v1.0 (Launch)
├─ Current proposed schema
├─ Hybrid event sourcing (append-only events)
└─ 2-3 months to stable production

v1.1 (Stabilization)
├─ Bug fixes and user feedback
├─ Performance optimization
└─ 4-6 weeks post-launch

v1.2 (Small Enhancements)
├─ Fulfillment model (parallel table)
├─ UI improvements
└─ 6-8 weeks after v1.1

v1.3 (Event Store Setup)
├─ Deploy EventStore infrastructure
├─ Create projector framework
├─ Begin dual-writing (state + events)
└─ 8-10 weeks after v1.2

v1.4 (Gradual ES Migration)
├─ Migrate Estoque to full ES
├─ Test replay and reconstruction
├─ Add temporal queries
└─ 6-8 weeks per entity type

v2.0 (Enterprise Patterns)
├─ Full CQRS implementation
├─ Order item immutability enforcement
├─ Rich state machines
├─ Explicit returns model
└─ 12-16 weeks after complete v1.4

v2.1+ (Advanced Features)
├─ Temporal analytics
├─ Multi-model projections
├─ Advanced compliance reporting
└─ Ongoing based on business needs
```

---

## Part 5: Migration Checklist

### Before Starting v2 Implementation

**Infrastructure Readiness:**
- [ ] v1.0 has been running stable for 2+ months
- [ ] Performance baseline established (response times, throughput)
- [ ] Audit trail from *_events tables validated in production
- [ ] Backup and recovery procedures proven
- [ ] Team is familiar with event-based thinking

**Code Quality Requirements:**
- [ ] Unit test coverage >80% for entities/services
- [ ] Integration tests pass against production data snapshot
- [ ] Code review process established
- [ ] Database migration testing proven (dev→staging→production)

**Business Alignment:**
- [ ] Stakeholders understand v2 is internal architecture (no UI changes)
- [ ] Performance SLAs defined
- [ ] Compliance requirements documented (especially temporal queries)
- [ ] Support/training updated for new features

**Team Readiness:**
- [ ] At least one team member completed ES training course
- [ ] Knowledge transfer plan for CQRS patterns
- [ ] Pair programming setup for high-risk code
- [ ] Rollback procedures documented and tested

---

## Part 6: Risk Mitigation

### Highest Risk Areas

**Risk: Event Sourcing Complexity**
- **Exposure**: Full ES requires new thinking about state
- **Mitigation**:
  - Do Estoque first (most natural, least complex)
  - Use spatie/laravel-event-sourcing (proven library)
  - Maintain dual-write during transition (safety net)
  - Run parallel systems for 1-2 weeks before cutover

**Risk: Migration Data Loss**
- **Exposure**: Rebuilding projections from events could lose data
- **Mitigation**:
  - Keep old state tables during transition
  - Compare projection rebuild results row-by-row
  - Maintain md5 checksums of totals
  - Use feature flags to toggle between old/new
  - Have rollback plan (revert to old tables)

**Risk: Performance Degradation**
- **Exposure**: Event replay could be slow with large event streams
- **Mitigation**:
  - Implement snapshots (save state every 1000 events)
  - Archive old events after snapshot
  - Use async projectors (don't block on projection writes)
  - Benchmark replay performance before v2.0

**Risk: Event Versioning**
- **Exposure**: Old events have different schema after code changes
- **Mitigation**:
  - Design events to be backward-compatible
  - Use upcaster pattern (transform old events on read)
  - Document event schema versions
  - Plan event migration strategy early

---

## Part 7: Greenfield vs Proposed - Feature Mapping

### How v1 Supports v2 Migration

| Greenfield v2 Feature | v1 Foundation | Migration Path | Effort |
|----------------------|---------------|-----------------|--------|
| Fulfillment lifecycle | Alocacao table | Add Fulfillment entity | Medium |
| Full Event Sourcing | Append-only *_events | Build event store + projectors | High |
| CQRS pattern | Separate read/write logic | Restructure services | High |
| Rich state machine | Current states | Add transitions table + validator | Medium |
| Order item immutability | Mutable VendaItem | Create OrderItem + adjustments | Low |
| Explicit returns | is_estornado flag | Create StockReturn entity | Low |
| Point-in-time queries | Event tables + events | Add temporal query framework | Medium |
| Multi-model projections | Single state table | Build projection engine | High |
| Advanced state tracking | Basic 5 states | Expand to 8 states + tracking | Low |
| Integration delivery | EntregaItem junction | Fulfillment.delivery_id direct | Low |

### "Building Blocks" Approach

The v1 schema is designed to **minimize rework** when upgrading to v2:

```text
v1 to v2 "Evolutionary" Path (No Major Rewrites)

Alocacao (v1) ──────────────────────► Alocacao + Fulfillment (v2)
                                       └─ New companion table
                                       └─ Backward compatible

VendaItem (v1) ─────────────────────► VendaItem + VendaAdjustments (v2)
                                       └─ New audit trail
                                       └─ Keep old table for compat

estoque_lotes (v1) ────────────────► estoque_lotes + EstoqueTransicoes (v2)
                                      └─ New transition tracking
                                      └─ Enhanced state machine

*_events (v1) ────────────────────► EventStore + Projectors (v2)
                                     └─ Tables already exist
                                     └─ Just need replay logic

is_estornado flag (v1) ─────────────► EstoqueDevolucoes (v2)
                                       └─ New entity
                                       └─ Parallel table until cutover
```

**Key Principle**: Add new capabilities alongside old ones, migrate gradually, no big-bang rewrites.

---

## Part 8: Recommendations

### Immediate (v1 Stabilization)
1. ✅ Finalize proposed schema (complete)
2. ✅ Create Laravel models for v1 (next phase)
3. ✅ Implement data migration from MySQL → PostgreSQL (next phase)
4. Document known v1 limitations (for v2 planning)
5. Set up event recording infrastructure (partially done)

### Short-term (v1.2-v1.3)
1. **Fulfillment Model**: Lowest risk, high value. Implement parallel to Alocacao.
2. **Event Store Setup**: Build infrastructure for point-in-time queries.
3. **State Transition Tracking**: Add transaction audit for compliance.

### Medium-term (v1.4-v2.0)
1. **Gradual ES Migration**: One entity at a time (Estoque → Venda → Financeiro).
2. **CQRS Architecture**: Separate command/query layers.
3. **Order Item Immutability**: Add OrderItem view + adjustment tracking.

### Long-term (v2.1+)
1. **Advanced Projections**: Multi-model queries from single event stream.
2. **Temporal Analytics**: Point-in-time state reconstruction for reporting.
3. **Compliance Suite**: Automated audit trail generation for regulatory needs.

---

## Conclusion

The proposed v1 schema is **enterprise-ready** for launch while providing a **clear upgrade path** to advanced v2+ features.

- **v1 Strengths**: Simple, pragmatic, proven patterns
- **v1 Trade-offs**: Necessary simplifications for time-to-market
- **v2 Vision**: Enterprise patterns layered incrementally on v1
- **Migration Strategy**: Additive, non-destructive, reversible

The key insight: **Evolution, not revolution**. By designing v1 to layer new capabilities on top (Fulfillment alongside Alocacao, OrderItem alongside VendaItem, EventStore alongside current tables), the system grows in sophistication without destabilizing what already works.

---

## Related Documents

- [01-design-greenfield.md](./01-design-greenfield.md) - Complete v2.0 vision with all features
- [03-greenfield-vs-schema-comparison.md](./03-greenfield-vs-schema-comparison.md) - Detailed feature comparison
- [02-schema-redesenhado.md](../03-decisoes/02-schema-redesenhado.md) - v1.0 proposed schema (primary reference)
- [event-sourcing-analise.md](../rascunhos/event-sourcing-analise.md) - Full ES patterns and implementation details
- [alocacao-m2n-workflow.md](../rascunhos/alocacao-m2n-workflow.md) - v1 allocation implementation guide
- [04-arquitetura/modulos/](../04-arquitetura/modulos/) - v1 module-specific implementations
