# Documentation Inconsistencies - Detailed Source Mapping

> Generated: 2026-01-10
> Purpose: Track exactly where each conflicting value comes from

---

## #1: CRITICAL - Table Naming: `estoques` vs `estoque_lotes`

### Source 1: `02-banco-dados.md` - Uses `estoques`

| Field | File | Line | Definition |
|-------|------|------|-----------|
| Table Name | `04-arquitetura/02-banco-dados.md` | 443 | `CREATE TABLE estoques (...)` |
| Reference | `04-arquitetura/02-banco-dados.md` | 145 | `estoque_id INTEGER REFERENCES estoques(id)` |
| Consumption | `04-arquitetura/02-banco-dados.md` | 473 | `estoque_id INTEGER REFERENCES estoques(id) NOT NULL` |

**Full Definition:**
```sql
-- File: 04-arquitetura/02-banco-dados.md, Line 443
CREATE TABLE estoques (
    id SERIAL PRIMARY KEY,
    loja_id INTEGER REFERENCES lojas(id) NOT NULL,
    compra_id INTEGER REFERENCES compras(id),
    produto_id INTEGER REFERENCES produtos(id) NOT NULL,
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    quantidade DECIMAL(15,4) NOT NULL,
    quantidade_disponivel DECIMAL(15,4) NOT NULL,
    -- ...
)
```

### Source 2: `modulos/estoque.md` - Uses `estoque_lotes`

| Field | File | Line | Definition |
|-------|------|------|-----------|
| Table Name (Comment) | `04-arquitetura/modulos/estoque.md` | 93 | `estoque_lotes` |
| Table Structure | `04-arquitetura/modulos/estoque.md` | 94-114 | Full estoque_lotes definition |
| Laravel Model | `04-arquitetura/modulos/estoque.md` | 252 | `protected $table = 'estoque_lotes';` |
| Alocacoes FK Reference | `04-arquitetura/modulos/estoque.md` | 121 | `├── estoque_lote_id (FK)` |

**Full Definition:**
```sql
-- File: 04-arquitetura/modulos/estoque.md, Line 94
estoque_lotes
├── id (PK)
├── loja_id (FK)
├── produto_id (FK)
├── nfe_id (FK, nullable)
├── compra_id (FK, nullable)
├── fornecedor_id (FK, nullable)
├── status
├── quantidade
├── quantidade_disponivel
├── quantidade_reservada
├── custo_unitario
├── lote
├── data_validade (nullable)
├── data_entrada
├── ... (tax fields)
└── bloco_id (nullable)
```

**Laravel Model:**
```php
// File: 04-arquitetura/modulos/estoque.md, Line 252
class EstoqueLote extends Model
{
    protected $table = 'estoque_lotes';  // ← Explicit table name
}
```

### Source 3: `modulos/vendas.md` - References both

| Context | File | Line | Usage |
|---------|------|------|-------|
| Allocation table | `04-arquitetura/modulos/vendas.md` | ? | References `alocacoes` which links to `estoque_lotes` |

### IMPACT
- **02-banco-dados.md** defines `estoques` table
- **modulos/estoque.md** PHP models and code use `estoque_lotes`
- FK references in `alocacoes` point to `estoque_lote_id`, implying table is `estoque_lotes`
- **CODE WILL FAIL**: Foreign key constraint violations when trying to insert into alocacoes with estoque_lote_id pointing to non-existent table

---

## #2: HIGH - EstoqueStatus Enum Values Mismatch

### Source 1: `modulos/estoque.md` - PHP Enum (3 Values)

| Enum Value | File | Line | Label | Color |
|-----------|------|------|-------|-------|
| `TEMP` | `04-arquitetura/modulos/estoque.md` | 660 | 'Temporário' | yellow |
| `ESTOQUE` | `04-arquitetura/modulos/estoque.md` | 661 | 'Em Estoque' | green |
| `CANCELADO` | `04-arquitetura/modulos/estoque.md` | 662 | 'Cancelado' | red |

**Full Code:**
```php
// File: 04-arquitetura/modulos/estoque.md, Line 658
enum EstoqueStatus: string
{
    case TEMP = 'TEMP';
    case ESTOQUE = 'ESTOQUE';
    case CANCELADO = 'CANCELADO';

    public function label(): string
    {
        return match($this) {
            self::TEMP => 'Temporário',
            self::ESTOQUE => 'Em Estoque',
            self::CANCELADO => 'Cancelado',
        };
    }
}
```

### Source 2: `02-banco-dados.md` - SQL Type Definition (4 Values)

| SQL Type Value | File | Line | Meaning |
|----------------|------|------|---------|
| `DISPONIVEL` | `04-arquitetura/02-banco-dados.md` | 206 | Available for sale |
| `RESERVADO` | `04-arquitetura/02-banco-dados.md` | 207 | Reserved for sale |
| `ESGOTADO` | `04-arquitetura/02-banco-dados.md` | 208 | Fully consumed |
| `BLOQUEADO` | `04-arquitetura/02-banco-dados.md` | 209 | Blocked (damage, etc) |

**Full Definition:**
```sql
-- File: 04-arquitetura/02-banco-dados.md, Line 205
CREATE TYPE estoque_lote_status AS ENUM (
    'DISPONIVEL',
    'RESERVADO',
    'ESGOTADO',
    'BLOQUEADO'
);
```

### Source 3: `modulos/estoque.md` - State Machine (Different States Again!)

| State | File | Line | From → To |
|-------|------|------|----------|
| `TEMP` | `04-arquitetura/modulos/estoque.md` | 137 | [*] → TEMP |
| `ESTOQUE` | `04-arquitetura/modulos/estoque.md` | 138 | TEMP → ESTOQUE |
| `CONSUMIDO` | `04-arquitetura/modulos/estoque.md` | 141 | ESTOQUE → CONSUMIDO |
| `CANCELADO` | `04-arquitetura/modulos/estoque.md` | 142 | ESTOQUE → CANCELADO |

**Mermaid State Diagram:**
```mermaid
-- File: 04-arquitetura/modulos/estoque.md, Line 135
[*] --> TEMP : Importação NFe iniciada
TEMP --> ESTOQUE : Importação confirmada
TEMP --> CANCELADO : Importação cancelada
ESTOQUE --> ESTOQUE : Consumo parcial
ESTOQUE --> CONSUMIDO : Totalmente consumido (restante=0)
ESTOQUE --> CANCELADO : Cancelamento
```

### CONFLICT SUMMARY

| Enum Location | Values | Notes |
|---------------|--------|-------|
| **PHP Enum (estoque.md:660)** | TEMP, ESTOQUE, CANCELADO | 3 values - for import flow |
| **SQL Type (02-banco-dados.md:206)** | DISPONIVEL, RESERVADO, ESGOTADO, BLOQUEADO | 4 values - for allocation states |
| **State Machine (estoque.md:135)** | TEMP, ESTOQUE, CONSUMIDO, CANCELADO | 4 values - but different from SQL |

**MISMATCH**: No clear mapping between three different enum definitions with overlapping but inconsistent values.

---

## #3: HIGH - Status Semantics Confusion in estoque.md

### Source 1: SQL Comment in estoque.md (Line 101)

```sql
-- File: 04-arquitetura/modulos/estoque.md, Line 101
├── status                          -- RECEBIDO, RESERVADO, CONSUMIDO, QUEBRA, DEVOLUCAO
```

**States mentioned:** RECEBIDO, RESERVADO, CONSUMIDO, QUEBRA, DEVOLUCAO (5 states)

### Source 2: Same File - PHP Enum (Line 660)

```php
-- File: 04-arquitetura/modulos/estoque.md, Line 660
enum EstoqueStatus: string
{
    case TEMP = 'TEMP';           // ← Not RECEBIDO
    case ESTOQUE = 'ESTOQUE';    // ← Not RESERVADO
    case CANCELADO = 'CANCELADO'; // ← QUEBRA/DEVOLUCAO not in enum
}
```

**States in enum:** TEMP, ESTOQUE, CANCELADO (3 states - completely different!)

### CONFLICT
- **Line 101 SQL comment** says: RECEBIDO, RESERVADO, CONSUMIDO, QUEBRA, DEVOLUCAO
- **Line 660 PHP Enum** says: TEMP, ESTOQUE, CANCELADO
- **No mapping** between the two

---

## #4: HIGH - Field Naming: `estoque_lote_id` vs `estoque_id`

### Source 1: `modulos/estoque.md` - Table Definition (Line 121)

```sql
-- File: 04-arquitetura/modulos/estoque.md, Line 121
alocacoes
├── id (PK)
├── venda_item_id (FK)         -- Item da venda
├── estoque_lote_id (FK)       -- ← FIELD NAME: estoque_lote_id
```

**Field Name:** `estoque_lote_id`

### Source 2: `02-banco-dados.md` - FK Reference (Line 145)

```sql
-- File: 04-arquitetura/02-banco-dados.md, Line 145
estoque_id INTEGER REFERENCES estoques(id), -- qual estoque foi consumido
```

**Field Name:** `estoque_id`

### Source 3: `modulos/estoque.md` - Code Example (Line 886)

```php
-- File: 04-arquitetura/modulos/estoque.md, Line 886
'estoque_id' => $estoqueId,  // ← FIELD NAME: estoque_id (not estoque_lote_id!)
```

**Field Name:** `estoque_id`

### CONFLICT SUMMARY

| Field Name | Source | Line | Context |
|-----------|--------|------|---------|
| `estoque_lote_id` | modulos/estoque.md | 121 | Alocacoes table definition |
| `estoque_id` | 02-banco-dados.md | 145 | venda_itens.estoque_id |
| `estoque_id` | modulos/estoque.md | 886 | Code example |

**Which is correct?** Inconsistent naming across documentation.

---

## #5: MEDIUM - `tipo_financeiro` Field Definition (Actually TWO Different Fields)

### Source 1: `modulos/financeiro.md` - RECEBER/PAGAR Enum (Line 1424)

| Enum Type | File | Line | Values |
|-----------|------|------|--------|
| `tipo_financeiro` | `04-arquitetura/modulos/financeiro.md` | 1424 | RECEBER, PAGAR |

**Definition:**
```sql
-- File: 04-arquitetura/modulos/financeiro.md, Line 1424
CREATE TYPE tipo_financeiro AS ENUM ('RECEBER', 'PAGAR');

-- File: 04-arquitetura/modulos/financeiro.md, Line 1463
CREATE TABLE financeiro_parcelas (
    id BIGSERIAL PRIMARY KEY,
    tipo tipo_financeiro NOT NULL,  -- ← This is the discriminator
    -- ...
)
```

**Usage:** Line 1463 - `tipo` column in `financeiro_parcelas` table
- **Values:** RECEBER (receivable), PAGAR (payable)
- **Purpose:** Discriminator to distinguish between receivables and payables

### Source 2: `modulos/financeiro.md` - OPERACIONAL/ADMINISTRATIVO/COMERCIAL (Line 1690)

| Table | File | Line | Field Name | Values |
|-------|------|------|-----------|--------|
| `centros_custo` | `04-arquitetura/modulos/financeiro.md` | 1690 | `tipo` | OPERACIONAL, ADMINISTRATIVO, COMERCIAL |

**Definition:**
```sql
-- File: 04-arquitetura/modulos/financeiro.md, Line 1690
CREATE TABLE centros_custo (
    id BIGSERIAL PRIMARY KEY,
    codigo VARCHAR(20) NOT NULL UNIQUE,
    nome VARCHAR(100) NOT NULL,
    tipo VARCHAR(20) NOT NULL DEFAULT 'OPERACIONAL', -- ← Different tipo field!
    -- ...
)
```

**Usage:** Line 1690 - `tipo` column in `centros_custo` table
- **Values:** OPERACIONAL (operational), ADMINISTRATIVO (administrative), COMERCIAL (commercial)
- **Purpose:** Classification of cost centers

### CONFLICT SUMMARY

| Table | Field | Enum Type | Values | Purpose |
|-------|-------|-----------|--------|---------|
| `financeiro_parcelas` | `tipo` | `tipo_financeiro` | RECEBER, PAGAR | Discriminator (receivable vs payable) |
| `centros_custo` | `tipo` | N/A (VARCHAR) | OPERACIONAL, ADMINISTRATIVO, COMERCIAL | Cost center classification |

**ISSUE**: Same field name `tipo` in different tables with completely different meanings and value sets. Could confuse developers.

---

## #6: MEDIUM - Event Sourcing Table Naming Inconsistency

### Source 1: `modulos/estoque.md` - Portuguese

| Table | File | Line | Purpose |
|-------|------|------|---------|
| `estoque_movimentacoes` | `04-arquitetura/modulos/estoque.md` | 411 | Movement/event log |

**Definition:**
```sql
-- File: 04-arquitetura/modulos/estoque.md, Line 411
CREATE TABLE estoque_movimentacoes (...)
```

### Source 2: `modulos/estoque.md` - Mixed Portuguese/English

| Table | File | Line | Purpose |
|-------|------|------|---------|
| `alocacoes_eventos` | `04-arquitetura/modulos/estoque.md` | ? | Allocation events |

**Definition:**
```sql
-- File: 04-arquitetura/modulos/estoque.md (referenced)
CREATE TABLE alocacoes_eventos (...)
```

### Source 3: `modulos/financeiro.md` - English

| Table | File | Line | Purpose |
|-------|------|------|---------|
| `financeiro_parcelas_events` | `04-arquitetura/modulos/financeiro.md` | 1643 | Financial events |

**Definition:**
```sql
-- File: 04-arquitetura/modulos/financeiro.md, Line 1643
CREATE TABLE financeiro_parcelas_events (...)
```

### CONFLICT SUMMARY

| Naming Pattern | Example | Module | Issue |
|---|---|---|---|
| `*_movimentacoes` | `estoque_movimentacoes` | Estoque | Portuguese |
| `*_eventos` | `alocacoes_eventos` | Estoque | Portuguese |
| `*_events` | `financeiro_parcelas_events` | Financeiro | English |

**INCONSISTENCY**: No consistent naming convention. Should be either:
- `estoque_movimentacoes`, `alocacoes_movimentacoes`, `financeiro_parcelas_movimentacoes`, OR
- `estoque_eventos`, `alocacoes_eventos`, `financeiro_parcelas_eventos`, OR
- `estoque_events`, `alocacoes_events`, `financeiro_parcelas_events`

---

## #7: HIGH - Alocacao Status vs Allocation Event Type Duplication

### Source 1: `modulos/estoque.md` - Status Enum (Line 684)

| Value | File | Line | Label | Color |
|-------|------|------|-------|-------|
| ATIVO | `04-arquitetura/modulos/estoque.md` | 686 | 'Ativa' | green |
| REVERTIDA | `04-arquitetura/modulos/estoque.md` | 687 | 'Revertida' | orange |
| CANCELADA | `04-arquitetura/modulos/estoque.md` | 688 | 'Cancelada' | red |

**Code:**
```php
-- File: 04-arquitetura/modulos/estoque.md, Line 684
enum AlocacaoStatus: string
{
    case ATIVO = 'ATIVO';
    case REVERTIDA = 'REVERTIDA';
    case CANCELADA = 'CANCELADA';

    public function label(): string
    {
        return match($this) {
            self::ATIVO => 'Ativa',
            self::REVERTIDA => 'Revertida',
            self::CANCELADA => 'Cancelada',
        };
    }
}
```

### Source 2: Same File - Event Type Enum

| Value | File | Line | Purpose |
|-------|------|------|---------|
| CRIADA | `04-arquitetura/modulos/estoque.md` | ? | Allocation created |
| REVERTIDA | `04-arquitetura/modulos/estoque.md` | ? | Allocation reversed |
| CANCELADA | `04-arquitetura/modulos/estoque.md` | ? | Allocation cancelled |

**Code:**
```php
-- File: 04-arquitetura/modulos/estoque.md
enum AllocationEventType: string
{
    case CRIADA = 'CRIADA';
    case REVERTIDA = 'REVERTIDA';
    case CANCELADA = 'CANCELADA';
}
```

### CONFLICT SUMMARY

| Enum | Values | Purpose | Issue |
|------|--------|---------|-------|
| `AlocacaoStatus` | ATIVO, REVERTIDA, CANCELADA | Current state of allocation | Current state enum |
| `AllocationEventType` | CRIADA, REVERTIDA, CANCELADA | Event log type | Event type enum |

**OVERLAP**: Both enums contain REVERTIDA and CANCELADA, but:
- `AlocacaoStatus.REVERTIDA` = current state is reverted
- `AllocationEventType.REVERTIDA` = event that occurred

**AMBIGUITY**: Which enum should be used where? Can create bugs if wrong enum is used.

---

## #8: HIGH - Phase 1/2 Scope Inconsistency

### Source 1: `modulos/cadastros.md` - Marks Banking Fields as Phase 2

| Field | File | Line | Status | Phase |
|-------|------|------|--------|-------|
| `banco, agencia, conta` | `04-arquitetura/modulos/cadastros.md` | 119 | Not implemented | Phase 2 |
| `comissao_1, comissao_2` | `04-arquitetura/modulos/cadastros.md` | 120 | Not implemented | Phase 2 |
| `is_representacao` | `04-arquitetura/modulos/cadastros.md` | 121 | Not implemented | Phase 2 |
| `is_frete_pago_loja` | `04-arquitetura/modulos/cadastros.md` | 122 | Not implemented | Phase 2 |

**From Document:**
```sql
-- File: 04-arquitetura/modulos/cadastros.md, Line 119-122
├── banco, agencia, conta     -- Dados bancários (Phase 2)
├── comissao_1, comissao_2    -- Taxas de comissão (Phase 2)
├── is_representacao          -- É representação? (Phase 2)
├── is_frete_pago_loja        -- Quem paga frete (Phase 2)
```

### Source 2: `modulos/financeiro.md` - Phase 1 Foundation Depends on Banking

| Feature | File | Line | Phase | Status |
|---------|------|------|-------|--------|
| Unified parcelas | `04-arquitetura/modulos/financeiro.md` | 1712 | P0 | Phase 1 |
| Proper FKs | `04-arquitetura/modulos/financeiro.md` | 1713 | P0 | Phase 1 |
| Bank Integration | `04-arquitetura/modulos/financeiro.md` | 1722 | P1 | Phase 2 |

**From Document:**
```markdown
-- File: 04-arquitetura/modulos/financeiro.md, Line 1412-1722
FASE 1: FUNDAÇÃO (P0 - Crítico)
- Tabela unificada: financeiro_parcelas com tipo discriminador
- FKs proper: cliente_id / fornecedor_id com constraints
- Pagamentos parciais: Múltiplos pagamentos por parcela

FASE 2: INTEGRAÇÃO BANCÁRIA (P1 - Compliance)
- Contas bancárias
- CNAB multi-bank integration
```

### DEPENDENCY ISSUE

| Module | Phase | Depends On | Status |
|--------|-------|-----------|--------|
| Cadastros - Banking Fields | Phase 2 | Not ready in Phase 1 | ⚠️ Phase 2 |
| Financeiro - Bank Integration | Phase 2 | Needs banking fields | ⚠️ Can't start Phase 2! |
| Financeiro - Foundation | Phase 1 | Should work without banking | ⚠️ OK |

**CONFLICT**: Cadastros marks banking fields as Phase 2, but Financeiro Phase 2 (Bank Integration) needs those fields. Circular dependency!

---

## #9: MEDIUM - Venda_item `origem` Field Definition Clarity

### Source 1: `02-schema-visual-overview.md` - Visual Reference (Line 54-56)

```mermaid
-- File: 03-decisoes/02-schema-visual-overview.md, Line 54-56
VendaItem["venda_itens<br/>(origem: COMPRA ou ESTOQUE)"]
```

**Mentioned Values:** COMPRA, ESTOQUE

### Source 2: `modulos/estoque.md` - Definition

```sql
-- File: 04-arquitetura/modulos/estoque.md, Line 361
├── origem: COMPRA ou ESTOQUE
```

**Mentioned Values:** COMPRA, ESTOQUE

### Source 3: `modulos/vendas.md` - References but No Clear Values Defined

**Issue:** No explicit enum definition showing all `origem` values and their meanings

### MISSING CLARITY
- Visual docs show: COMPRA, ESTOQUE
- No enum definition found
- No clear documentation of what each value means
- No consistency check across modules

---

## #10: MEDIUM - FK Reference Mismatch: Table Names vs Model Names

### Source 1: `02-banco-dados.md` - References `estoques` Table

```sql
-- File: 04-arquitetura/02-banco-dados.md, Line 145
estoque_id INTEGER REFERENCES estoques(id),
```

**Table Referenced:** `estoques`

### Source 2: `modulos/estoque.md` - Model Uses `estoque_lotes` Table Name

```php
-- File: 04-arquitetura/modulos/estoque.md, Line 252
class EstoqueLote extends Model
{
    protected $table = 'estoque_lotes';
}
```

**Table Name:** `estoque_lotes`

### CONFLICT
- SQL says reference table is `estoques`
- PHP models use `estoque_lotes`
- **When creating alocacoes table with FK to estoque_lote_id, it references non-existent table**

---

## Summary Table: All 10 Inconsistencies at a Glance

| # | Severity | Category | Files | Issue | Values/Fields |
|---|----------|----------|-------|-------|---------------|
| 1 | CRITICAL | Table Name | 02-banco-dados.md vs modulos/estoque.md | `estoques` vs `estoque_lotes` | estoques (L145, L473) vs estoque_lotes (L94, L252) |
| 2 | HIGH | Enum Values | 02-banco-dados.md vs modulos/estoque.md | Status enum mismatch | DISPONIVEL/RESERVADO/ESGOTADO/BLOQUEADO vs TEMP/ESTOQUE/CANCELADO |
| 3 | HIGH | Status Semantics | modulos/estoque.md | Comment conflicts with enum | RECEBIDO/RESERVADO/CONSUMIDO/QUEBRA/DEVOLUCAO vs TEMP/ESTOQUE/CANCELADO |
| 4 | HIGH | Field Naming | modulos/estoque.md vs 02-banco-dados.md | estoque_lote_id vs estoque_id | estoque_lote_id (L121) vs estoque_id (L145, L886) |
| 5 | MEDIUM | Field Usage | modulos/financeiro.md | Different `tipo` fields | tipo='RECEBER/PAGAR' vs tipo='OPERACIONAL/ADMINISTRATIVO/COMERCIAL' |
| 6 | MEDIUM | Naming Convention | modulos/estoque.md, modulos/financeiro.md | Event sourcing naming | *_movimentacoes vs *_eventos vs *_events |
| 7 | HIGH | Enum Duplication | modulos/estoque.md | Status + Event type overlap | AlocacaoStatus vs AllocationEventType |
| 8 | HIGH | Phase Planning | modulos/cadastros.md vs modulos/financeiro.md | Circular dependencies | Phase 2 banking in cadastros vs Phase 2 bank integration in financeiro |
| 9 | MEDIUM | Missing Definition | 02-schema-visual-overview.md, modulos/estoque.md | origen field values unclear | COMPRA, ESTOQUE (no enum) |
| 10 | MEDIUM | Reference Mismatch | 02-banco-dados.md vs modulos/estoque.md | Schema vs code mismatch | References estoques but models use estoque_lotes |

---

## File-by-File Breakdown

### `04-arquitetura/02-banco-dados.md`
- **Defines:** `estoques` table (not `estoque_lotes`)
- **Defines:** SQL enum `estoque_lote_status` with DISPONIVEL/RESERVADO/ESGOTADO/BLOQUEADO
- **Issues:** Conflicts with modulos/estoque.md on table name and enum values

### `04-arquitetura/modulos/estoque.md`
- **Defines:** `estoque_lotes` table (not `estoques`)
- **Defines:** `EstoqueStatus` enum with TEMP/ESTOQUE/CANCELADO
- **Defines:** `AlocacaoStatus` enum (overlaps with AllocationEventType)
- **Issues:** Multiple enum definitions, table name conflicts, field naming conflicts (estoque_lote_id vs estoque_id)

### `04-arquitetura/modulos/financeiro.md`
- **Defines:** `tipo_financeiro` enum (RECEBER/PAGAR) in `financeiro_parcelas`
- **Defines:** `tipo` field in `centros_custo` (OPERACIONAL/ADMINISTRATIVO/COMERCIAL)
- **Issues:** Phase 2 bank integration depends on Phase 2 banking fields

### `04-arquitetura/modulos/cadastros.md`
- **Marks as Phase 2:** Banking fields (banco, agencia, conta)
- **Issues:** Phase 2 financeiro module needs these fields

### `03-decisoes/02-schema-visual-overview.md`
- **References:** `origem` field with COMPRA/ESTOQUE values
- **Issues:** No formal enum definition

---

## Recommendations for Fixes

### CRITICAL (Fix First)
1. **Standardize table name:** Decide on `estoques` OR `estoque_lotes` and update ALL files
2. **Define single status enum:** Choose one source of truth for estoque/lote status values
3. **Fix FK references:** Ensure all references point to correct table name

### HIGH PRIORITY (Fix Next)
4. **Clarify `tipo` fields:** Use different column names if they serve different purposes
5. **Resolve AlocacaoStatus duplication:** Merge with AllocationEventType or document difference
6. **Define `origem` enum:** Add formal ENUM definition for venda_itens.origem
7. **Resolve Phase dependencies:** Can't start Financeiro Phase 2 if Cadastros Phase 2 isn't ready

### MEDIUM PRIORITY (Fix Soon)
8. **Standardize event sourcing naming:** Pick Portuguese or English consistently
9. **Add master data dictionary:** Create single source of truth for all enums
10. **Sync schema diagrams:** Update visual overviews to match SQL definitions

