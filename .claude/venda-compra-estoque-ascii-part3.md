# Venda → Pedido Fornecedor → Estoque: Old vs New (ASCII Version)

## Part 3: Partial NFe Scenario, Stock Consumption, & Summary

---

## Scenario 3: Partial NFe (Split Across Shipments)

**Context**: Customer orders 100 units. First NFe only has 60. Second NFe has remaining 40 (2 weeks later).

### OLD SYSTEM: Partial NFe (Broken FIFO & Ambiguous State)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ STARTING STATE (from Scenario 1)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  venda_has_produto2 [999]:                                                  │
│    ├─ quantidade = 100                                                      │
│    ├─ fornecedor = 'ACME Corp'                                              │
│    └─ status = 'PEDIDO'                                                    │
│                                                                             │
│  pedido_fornecedor_has_produto2 [200]:                                      │
│    ├─ quantidade = 100                                                      │
│    └─ status = 'PENDENTE'                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ EVENT: NFe #1 arrives with only 60 units (not 100!)                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Received: 60 units                                                         │
│  Expected: 100 units                                                       │
│  Partial shipment!                                                          │
│                                                                             │
│  ❌ OLD SYSTEM: What now?                                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

APPROACH: App developer decides how to handle (inconsistently)

  Option 1: Mark original as 'PARCIAL'
    ├─ UPDATE venda_has_produto2 SET status = 'PARCIAL'
    ├─ But WHERE is the 60 units? Still in 'PEDIDO' status
    └─ ❌ Status meaning is ambiguous

  Option 2: Update quantity and status
    ├─ UPDATE venda_has_produto2 SET quantidade = 60, status = 'ESTOQUE'
    ├─ But what if more arrives later?
    ├─ We lost the original 100 quantity
    └─ ❌ Can't track what was ordered vs received

  Option 3: Create new row for first shipment
    ├─ Keep original at 100 with status 'PEDIDO'
    ├─ Create new row for 60 received
    ├─ idRelacionado = original (or ?)
    └─ ❌ Same ambiguity as Scenario 2

┌─────────────────────────────────────────────────────────────────────────────┐
│ WHAT ACTUALLY HAPPENS (likely Option 2)                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  UPDATE venda_has_produto2                                                 │
│  SET quantidade = 60             ← LOST original 100!                       │
│  WHERE idVendaProduto2 = 999;                                              │
│                                                                             │
│  INSERT INTO estoque (                                                      │
│    quantidade = 60,              ← Only the partial shipment                │
│    fornecedor = 'ACME Corp',     ← STRING (must match)                     │
│    -- NO data_entrada! ⚠️                                                    │
│  );                                                                         │
│  → estoque.idEstoque = 888                                                  │
│                                                                             │
│  INSERT INTO estoque_has_consumo (                                          │
│    idEstoque = 888,                                                        │
│    idVendaProduto2 = 999,                                                  │
│    quantidade = 60                                                         │
│  );                                                                         │
│                                                                             │
│  UPDATE pedido_fornecedor_has_produto2                                     │
│  SET status = 'FATURADO'                                                   │
│  WHERE idPedidoProduto2 = 200;                                             │
│                                                                             │
│  ⚠️ STATE AFTER NFe #1:                                                     │
│     venda_has_produto2 [999]:                                              │
│       ├─ quantidade = 60 (was 100! ❌)                                      │
│       ├─ status = 'ESTOQUE' (or 'PARCIAL'? unclear)                        │
│       └─ NO trace of original 100                                          │
│                                                                             │
│     estoque_has_consumo:                                                   │
│       ├─ Shows 60 units consumed                                           │
│       └─ Looks like original order was for 60, not 100!                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ WAITING FOR THE REMAINING 40 UNITS                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  System state is BROKEN:                                                    │
│                                                                             │
│    ❌ venda_has_produto2.quantidade = 60 (lost the 40 pending!)             │
│    ❌ How to track: 60 received, 40 still pending?                          │
│    ❌ No separate record for "40 remaining"                                 │
│    ❌ If user queries sales, shows 60 not 100                              │
│    ❌ Financial reports might be wrong                                      │
│                                                                             │
│  If someone asks: "When will this order be complete?"                       │
│    → Have to manually track outside the system                             │
│    → Check email, phone, notes                                              │
│    → Not in database!                                                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ EVENT: NFe #2 arrives 2 weeks later with 40 units                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  But venda_has_produto2 already shows 60 consumed!                          │
│                                                                             │
│  INSERT INTO estoque (                                                      │
│    quantidade = 40,                                                        │
│    fornecedor = 'ACME Corp'     ← STRING (must match 999's record)          │
│  );                                                                         │
│  → estoque.idEstoque = 889                                                  │
│                                                                             │
│  INSERT INTO estoque_has_consumo (                                          │
│    idEstoque = 889,                                                        │
│    idVendaProduto2 = 999,       ← ❌ Same row as first consumption!         │
│    quantidade = 40                                                         │
│  );                                                                         │
│                                                                             │
│  ⚠️ CONFLICT!                                                               │
│     Now venda_has_produto2 [999] is linked to 2 estoque records:           │
│       - estoque 888 (60 units)                                              │
│       - estoque 889 (40 units)                                              │
│                                                                             │
│     But venda_has_produto2.quantidade = 60, not 100!                        │
│     Data inconsistency = CHAOS                                              │
│                                                                             │
│  ⚠️ FIFO BROKEN:                                                            │
│     Both estoques have NO data_entrada timestamp                            │
│     Which one is FIFO? (888 from Jan 14, 889 from Jan 28?)                 │
│     Can't tell!                                                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

SUMMARY: OLD SYSTEM NIGHTMARE WITH PARTIAL NFe

  ❌ Lost original order quantity (100 → 60)
  ❌ No tracking of "40 remaining"
  ❌ Ambiguous status (ESTOQUE? PARCIAL? PEDIDO?)
  ❌ When second NFe arrives, data inconsistency
  ❌ No FIFO support (no timestamps)
  ❌ Multiple estoques → same venda_has_produto2 (confusing)
  ❌ Can't reconstruct what happened
  ❌ Reports are inaccurate
  ❌ Manual workarounds needed (spreadsheets, notes)
```

---

### NEW SYSTEM: Partial NFe (Proper State Management) ✅

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ STARTING STATE (from Scenario 1)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  venda_itens [1]:                                                           │
│    ├─ quantidade = 100                                                      │
│    ├─ fornecedor_id = 2 (ACME)   ← FK                                       │
│    ├─ parent_id = NULL           (original)                                 │
│    ├─ root_id = NULL                                                       │
│    └─ status = 'EM_COMPRA'                                                  │
│                                                                             │
│  compra_itens [200]:                                                        │
│    ├─ venda_item_id = 1          ← FK ✅                                     │
│    ├─ quantidade = 100                                                      │
│    └─ status = 'PENDENTE'                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ EVENT: NFe #1 arrives with only 60 units                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Received: 60 units                                                         │
│  Expected: 100 units                                                       │
│  Difference: 40 units pending                                               │
│                                                                             │
│  ✅ SYSTEM DETECTS SPLIT AUTOMATICALLY                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ SYSTEM ACTION: Create split for first 60                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Step 1: Update original venda_item to 60                                   │
│                                                                             │
│    UPDATE venda_itens                                                      │
│    SET quantidade = 60          ← Reduce from 100 to 60                     │
│    WHERE id = 1;                                                           │
│                                                                             │
│    ✅ Keep parent_id = NULL, root_id = NULL                                 │
│    ✅ Keep history: can see it was updated                                  │
│                                                                             │
│  Step 2: Create child for remaining 40                                      │
│                                                                             │
│    INSERT INTO venda_itens (                                               │
│      venda_id = 100,            ← Same order                                │
│      produto_id = 50,                                                      │
│      fornecedor_id = 2,         ← SAME (not supplier split, partial NFe)    │
│      quantidade = 40,           ← The remainder                             │
│                                                                             │
│      parent_id = 1,             ← POINTS TO ORIGINAL ✅                     │
│      root_id = 1,               ← PART OF SAME GROUP ✅                     │
│      split_reason = 'PARTIAL_NFE',  ← EXPLAINS WHY ✅                       │
│                                                                             │
│      status = 'PENDENTE',       ← Still waiting for 40                      │
│      origem = 'COMPRA'                                                     │
│    );                                                                      │
│    → venda_itens.id = 2                                                     │
│                                                                             │
│  ✅ CLEAR HIERARCHY:                                                        │
│     venda_itens [1] (60 units) - parent_id=NULL, root_id=NULL              │
│       └─ venda_itens [2] (40 units) - parent_id=1, root_id=1 (CHILD)       │
│                                                                             │
│  ✅ PRESERVES HISTORY:                                                      │
│     Audit trail shows:                                                     │
│       - Item created with 100                                              │
│       - Item updated to 60 (when NFe arrived partial)                       │
│       - Child created for 40                                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ SYSTEM ACTION: Update compra_items for 60                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  UPDATE compra_itens                                                       │
│  SET quantidade = 60            ← Matches new venda_item quantity          │
│  WHERE id = 200;                                                           │
│                                                                             │
│  ✅ Automatically stays linked to venda_item 1 (FK)                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ SYSTEM ACTION: Create compra_item for remaining 40                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO compra_itens (                                                 │
│    compra_id = 50,              ← Same purchase (still waiting)             │
│    produto_id = 50,                                                        │
│    venda_item_id = 2,           ← FK to CHILD venda_item ✅                 │
│    quantidade = 40,             ← The remainder                             │
│    parent_id = 200,             ← LINKS TO ORIGINAL ✅                      │
│    root_id = 200,               ← SAME ROOT ✅                              │
│    split_reason = 'PARTIAL_NFE',                                           │
│    status = 'PENDENTE'          ← Still waiting                             │
│  );                                                                         │
│  → compra_itens.id = 201                                                    │
│                                                                             │
│  ✅ Clear relationship:                                                     │
│     compra_itens [200] (60) - RECEIVED IN NFe #1                           │
│     compra_itens [201] (40) - WAITING FOR NFe #2                           │
│                                                                             │
│  ✅ Can query: Which items are still pending?                               │
│     SELECT * FROM compra_itens WHERE status = 'PENDENTE'                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ IMPORT NFe #1 (60 units)                                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO nfes (...);                                                    │
│  INSERT INTO nfe_itens (                                                    │
│    compra_item_id = 200,        ← FK ✅ (first 60)                          │
│    quantidade = 60,                                                        │
│    dados = {...}                ← Fiscal JSONB                              │
│  );                                                                         │
│  → nfe_itens.id = 1000                                                      │
│                                                                             │
│  INSERT INTO estoques (                                                     │
│    nfe_item_id = 1000,          ← FK ✅                                     │
│    compra_item_id = 200,        ← FK ✅                                     │
│    quantidade_original = 60,                                               │
│    quantidade_disponivel = 60,                                             │
│    data_entrada = '2025-01-14 10:00:00',  ← TIMESTAMP ✅                    │
│    status = 'DISPONIVEL'                                                   │
│  );                                                                         │
│  → estoques.id = 777                                                        │
│                                                                             │
│  ✅ CLEAR STATE:                                                            │
│     60 units received, 40 still pending                                    │
│     venda_item [1] ready for consumption (60)                              │
│     venda_item [2] waiting for rest (40)                                   │
│                                                                             │
│  Update statuses:                                                           │
│                                                                             │
│    UPDATE compra_itens SET status = 'FATURADO' WHERE id = 200;             │
│    UPDATE venda_itens SET status = 'ESTOQUE' WHERE id = 1;                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ WAITING FOR 40 UNITS (State is clear!) ✅                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  At any point, can query:                                                  │
│                                                                             │
│    "What's pending for this venda?"                                        │
│                                                                             │
│    SELECT * FROM venda_itens                                               │
│    WHERE venda_id = 100 AND status IN ('PENDENTE', 'EM_COMPRA')            │
│    → Shows venda_itens [2] (40 units pending)                              │
│                                                                             │
│    "What's being purchased?"                                               │
│                                                                             │
│    SELECT * FROM compra_itens                                              │
│    WHERE compra_id = 50 AND status = 'PENDENTE'                            │
│    → Shows compra_itens [201] (40 units, waiting for NFe)                  │
│                                                                             │
│    "Show me the full order with all pieces:"                               │
│                                                                             │
│    SELECT * FROM venda_itens                                               │
│    WHERE venda_id = 100 OR root_id = 1                                     │
│    ORDER BY id                                                             │
│    → Shows:                                                                │
│       venda_itens [1]: 60 units (ESTOQUE) - received                       │
│       venda_itens [2]: 40 units (PENDENTE) - waiting                       │
│                                                                             │
│  ✅ PERFECT VISIBILITY                                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ EVENT: NFe #2 arrives 2 weeks later with 40 units                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  System is READY for this:                                                  │
│    - venda_item [2] waiting for 40                                          │
│    - compra_item [201] waiting for NFe                                      │
│    - No ambiguity, no confusion                                             │
│                                                                             │
│  INSERT INTO nfes (...);                                                    │
│  INSERT INTO nfe_itens (                                                    │
│    compra_item_id = 201,        ← FK ✅ (matches the 40-unit purchase)     │
│    quantidade = 40,                                                        │
│    dados = {...}                ← Fiscal JSONB                              │
│  );                                                                         │
│  → nfe_itens.id = 1001                                                      │
│                                                                             │
│  INSERT INTO estoques (                                                     │
│    nfe_item_id = 1001,          ← FK ✅                                     │
│    compra_item_id = 201,        ← FK ✅ (second purchase item)              │
│    quantidade_original = 40,                                               │
│    quantidade_disponivel = 40,                                             │
│    data_entrada = '2025-01-28 14:30:00',  ← TIMESTAMP ✅ (FIFO later)       │
│    status = 'DISPONIVEL'                                                   │
│  );                                                                         │
│  → estoques.id = 888                                                        │
│                                                                             │
│  Update statuses:                                                           │
│                                                                             │
│    UPDATE compra_itens SET status = 'FATURADO' WHERE id = 201;             │
│    UPDATE venda_itens SET status = 'ESTOQUE' WHERE id = 2;                 │
│                                                                             │
│  ✅ NOW ORDER IS COMPLETE:                                                  │
│     venda_item [1]: 60 units (ESTOQUE)     ← From NFe #1 (Jan 14)          │
│     venda_item [2]: 40 units (ESTOQUE)     ← From NFe #2 (Jan 28)          │
│     Total: 100 units (original order)                                      │
│                                                                             │
│  ✅ FIFO VISIBLE:                                                           │
│     estoques [777]: data_entrada = '2025-01-14 10:00' (FIFO first)          │
│     estoques [888]: data_entrada = '2025-01-28 14:30' (FIFO second)         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ PAIRING STOCKS (consume in FIFO order)                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  For venda_item [1] (60 units from first NFe):                              │
│                                                                             │
│    INSERT INTO estoque_consumos (                                           │
│      venda_item_id = 1,         ← 60 units from first NFe                   │
│      estoque_id = 777,          ← Jan 14 arrival (FIFO!)                    │
│      quantidade = 60                                                       │
│    );                                                                      │
│                                                                             │
│  For venda_item [2] (40 units from second NFe):                             │
│                                                                             │
│    INSERT INTO estoque_consumos (                                           │
│      venda_item_id = 2,         ← 40 units from second NFe                  │
│      estoque_id = 888,          ← Jan 28 arrival (FIFO!)                    │
│      quantidade = 40                                                       │
│    );                                                                      │
│                                                                             │
│  ✅ FIFO IS CORRECT:                                                        │
│     Consumed in order: estoque 777 (Jan 14) THEN estoque 888 (Jan 28)      │
│     Perfect FIFO implementation                                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

SUMMARY: NEW SYSTEM ADVANTAGES WITH PARTIAL NFe

  ✅ Detected partial shipment automatically
  ✅ Created split with clear parent_id/root_id
  ✅ Preserved history (can audit what happened)
  ✅ Tracked "40 pending" explicitly (not lost)
  ✅ FIFO support via timestamps
  ✅ Clear state during wait (what's received, what's pending)
  ✅ When second NFe arrives, system was ready
  ✅ No data inconsistency
  ✅ Can query pending items easily
  ✅ Perfect FIFO consumption when complete
```

---

## 4. Stock Consumption Flow Comparison

### OLD SYSTEM: Stock Consumption (Broken & Risky)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ AVAILABLE STOCKS IN ESTOQUE                                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  estoque [777]:                                                             │
│    ├─ idProduto = 50 (Porcelanato)                                          │
│    ├─ lote = 'T01'                                                          │
│    ├─ quantidade = 60                                                       │
│    ├─ saldoEstoque = 60                                                     │
│    ├─ data_entrada = ??? (NO TIMESTAMP!) ⚠️⚠️⚠️                              │
│    └─ fornecedor = 'ACME Corp' (STRING)                                     │
│                                                                             │
│  estoque [888]:                                                             │
│    ├─ idProduto = 50 (Porcelanato)                                          │
│    ├─ lote = 'T02'                                                          │
│    ├─ quantidade = 100                                                      │
│    ├─ saldoEstoque = 100                                                    │
│    ├─ data_entrada = ??? (NO TIMESTAMP!) ⚠️⚠️⚠️                              │
│    └─ fornecedor = 'ACME Corp' (STRING)                                     │
│                                                                             │
│  VENDA ITEM:                                                                │
│  venda_has_produto2 [999]:                                                  │
│    ├─ idProduto = 50                                                        │
│    ├─ quantidade = 100                                                      │
│    └─ fornecedor = 'ACME Corp' (STRING)                                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ PROBLEM 1: No FIFO information                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Which lote arrived first? T01 or T02?                                      │
│  ❌ NO data_entrada column (no timestamp)                                    │
│  ❌ Can't ORDER BY anything to determine FIFO                                │
│  ❌ FIFO selection = GUESSING                                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ PROBLEM 2: Manual selection (error-prone)                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  User must decide (in their head):                                          │
│    "Probably T01 arrived first? I'll use 777."                              │
│                                                                             │
│  But they're GUESSING.                                                      │
│  If they're wrong:                                                          │
│    → FIFO is violated                                                       │
│    → Batch consistency issues (wrong tone/caliber sent to customer)         │
│    → No audit trail of WHY they chose wrong estoque                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP: User selects estoque 777 (60 units) ⚠️ Might be wrong!               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO estoque_has_consumo (                                          │
│    idEstoque = 777,              ← USER SELECTED (not FIFO logic)           │
│    idVendaProduto2 = 999,                                                  │
│    quantidade = 100,             ← ❌ WRONG! estoque only has 60!           │
│    valor_unitario_consumo = 35.00,                                         │
│    valor_total_consumo = 3500.00,                                          │
│                                                                             │
│    -- 30+ fiscal columns DUPLICATED ⚠️⚠️⚠️ --                                │
│    ncm = '69072100',                                                       │
│    cfop = '5102',                                                          │
│    ... ALL repeated from estoque ...                                       │
│  );                                                                         │
│                                                                             │
│  ⚠️ IMMEDIATE PROBLEM: Quantity mismatch!                                   │
│     Trying to consume 100 units from estoque with only 60!                  │
│                                                                             │
│  ⚠️ NO DB CONSTRAINT TO PREVENT THIS                                        │
│     Application must check: IF consumed_qty > stock_available               │
│     But what if app has bug? 💥                                              │
│                                                                             │
│  ⚠️ FISCAL DATA TRIPLICATION:                                               │
│     1. estoque [777]              ← inbound from ACME                      │
│     2. estoque_has_consumo [1500] ← outbound to customer                   │
│     3. future nfe saída           ← will have it AGAIN                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ PROBLEM 3: What about the other 40 units?                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  venda_has_produto2 needs 100 but estoque 777 only has 60.                  │
│  Still need 40 units from somewhere.                                        │
│                                                                             │
│  ❌ System should use estoque 888 (100 available)                            │
│  ❌ But which 40 from 888? The whole quantity?                               │
│  ❌ What if multiple customers are buying from 888?                          │
│  ❌ How to split 100 between multiple venda items?                           │
│                                                                             │
│  ❌ NO CLEAR PATTERN                                                        │
│  ❌ Product.idEstoque only stores ONE estoque!                              │
│  ❌ But we need to LINK venda item to MULTIPLE estoques                      │
│                                                                             │
│  ❌ BROKEN DESIGN:                                                          │
│     Product can only reference one estoque (product.idEstoque)             │
│     But one venda item might need multiple estoques (lote variation)        │
│     Impossible to handle correctly!                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

SUMMARY: OLD SYSTEM CONSUMPTION PROBLEMS

  ❌ No FIFO info (no timestamps on estoque)
  ❌ Manual selection = guessing = FIFO violations
  ❌ No quantity validation at DB level
  ❌ Fiscal data triplication (30+ columns repeated)
  ❌ Product.idEstoque only ONE (can't handle multiple lotes)
  ❌ No constraint: 1:1 pairing between venda item and estoque
  ❌ No audit trail: who selected which estoque, when, why
  ❌ Reports are unreliable (fiscal duplication causes confusion)
```

---

### NEW SYSTEM: Stock Consumption (Proper & Auditable) ✅

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ AVAILABLE STOCKS IN ESTOQUES (with FIFO info) ✅                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  estoques [777]:                                                            │
│    ├─ produto_id = 50 (FK ✅)                                               │
│    ├─ fornecedor_id = 2 (FK ✅)                                             │
│    ├─ lote = 'T01'                                                          │
│    ├─ quantidade_original = 60                                              │
│    ├─ quantidade_disponivel = 60                                            │
│    ├─ data_entrada = '2025-01-14 09:00:00' ← FIFO TIMESTAMP ✅              │
│    ├─ custo_unitario = 35.00                                               │
│    ├─ status = 'DISPONIVEL'                                                 │
│    └─ NO fiscal columns (those are in nfe_itens.dados) ✅                   │
│                                                                             │
│  estoques [888]:                                                            │
│    ├─ produto_id = 50 (FK ✅)                                               │
│    ├─ fornecedor_id = 2 (FK ✅)                                             │
│    ├─ lote = 'T02'                                                          │
│    ├─ quantidade_original = 100                                             │
│    ├─ quantidade_disponivel = 100                                           │
│    ├─ data_entrada = '2025-01-14 15:00:00' ← LATER TIMESTAMP (FIFO 2nd) ✅  │
│    ├─ custo_unitario = 35.00                                               │
│    ├─ status = 'DISPONIVEL'                                                 │
│    └─ NO fiscal columns ✅                                                   │
│                                                                             │
│  VENDA ITEM:                                                                │
│  venda_itens [1]:                                                           │
│    ├─ produto_id = 50 (FK ✅)                                               │
│    ├─ fornecedor_id = 2 (FK ✅)                                             │
│    ├─ quantidade = 100                                                      │
│    └─ status = 'ESTOQUE'                                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 1: Query available stock in FIFO order ✅                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  System shows user available stock:                                         │
│                                                                             │
│    SELECT *                                                                │
│    FROM estoques                                                           │
│    WHERE produto_id = 50                                                   │
│      AND fornecedor_id = 2                                                 │
│      AND quantidade_disponivel > 0                                         │
│    ORDER BY data_entrada ASC;                                              │
│                                                                             │
│  Result (in FIFO order):                                                    │
│    1. estoque [777]: 60 units, lote T01 (arrived Jan 14 09:00) ← FIFO      │
│    2. estoque [888]: 100 units, lote T02 (arrived Jan 14 15:00) ← FIFO 2nd  │
│                                                                             │
│  ✅ FIFO order is CLEAR and VISIBLE                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 2: User selects estoque with guidance ✅                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  UI displays:                                                               │
│                                                                             │
│    FIFO Suggestion: estoque 777 (T01, arrived first)                       │
│                                                                             │
│    Available options:                                                      │
│      [ ] estoque 777 (60 units) - Arrived 09:00 - RECOMMENDED (FIFO)       │
│      [ ] estoque 888 (100 units) - Arrived 15:00 - WARNING: Later batch    │
│                                                                             │
│  User can either:                                                           │
│    a) Select estoque 777 (FIFO) ✅                                          │
│    b) Override and select estoque 888 (if batch consistency matters)        │
│                                                                             │
│  ⚠️ Either way: selection is EXPLICIT and AUDITABLE                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 3: Create estoque_consumo (1:1 pairing with validation) ✅             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  User selects: estoque 777 (60 units)                                       │
│  venda_item needs: 100 units                                                │
│                                                                             │
│  INSERT INTO estoque_consumos (                                             │
│    venda_item_id = 1,           ← Link to venda item                        │
│    estoque_id = 777,            ← Link to stock                             │
│    quantidade = 60,             ← MUST MATCH estoque qty ✅                 │
│    custo_unitario = 35.00,      ← Snapshot from estoque                     │
│    custo_total = 2100.00,       ← GENERATED ALWAYS (60*35) ✅               │
│    motivo = 'VENDA',            ← Why consumed (VENDA, AJUSTE, etc.)        │
│    is_estornado = FALSE,        ← Not reversed                              │
│    created_by = :user_id,       ← WHO paired it ✅ AUDIT!                   │
│  );                                                                         │
│                                                                             │
│  BEFORE INSERT TRIGGER FIRES ✅ (DB-level validation):                      │
│    ✓ quantidade (60) == venda_item.quantidade (60)? YES ✅                  │
│    ✓ estoque.quantidade_disponivel (60) >= 60? YES ✅                       │
│    ✓ venda_item.produto_id == estoque.produto_id? YES ✅                    │
│    ✓ venda_item.fornecedor_id == estoque.fornecedor_id? YES ✅              │
│    ✓ venda_item status allows pairing? YES ✅                               │
│                                                                             │
│  If ANY check fails → Database error, no insertion ✅                       │
│  (App can't bypass DB constraints)                                          │
│                                                                             │
│  AFTER INSERT TRIGGER FIRES ✅ (Auto-update):                               │
│    1. estoques [777] updated:                                               │
│       quantidade_disponivel = 60 - 60 = 0                                   │
│       status = 'CONSUMIDO'                                                 │
│    2. venda_itens [1] updated:                                              │
│       status = 'ENTREGUE'   (or continues in delivery flow)                 │
│                                                                             │
│  → estoque_consumos.id = 999                                                │
│                                                                             │
│  ✅ CLEAN:                                                                  │
│     - No fiscal duplication (fiscal in nfe_itens.dados)                    │
│     - 1:1 pairing enforced by DB                                            │
│     - Constraints checked automatically                                     │
│     - Audit trail: user, time, quantity, reason                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 4: Handle remaining 40 units (automatic split) ✅                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Problem: venda_item needs 100, but estoque 777 only has 60.                │
│  Still need: 40 units                                                       │
│                                                                             │
│  ✅ SYSTEM DETECTS THIS AUTOMATICALLY:                                      │
│     Consumed: 60, Needed: 100, Remaining: 40                                │
│                                                                             │
│  System creates child venda_item:                                           │
│                                                                             │
│    INSERT INTO venda_itens (                                               │
│      venda_id = 100,            ← Same venda                                │
│      produto_id = 50,                                                      │
│      fornecedor_id = 2,                                                    │
│      quantidade = 40,           ← The remainder                             │
│      parent_id = 1,             ← Link to original                          │
│      root_id = 1,               ← Same root                                 │
│      split_reason = 'PARTIAL_STOCK',  ← Explains the split                 │
│      status = 'PENDENTE',       ← Still waiting for 40                      │
│    );                                                                      │
│    → venda_itens.id = 2                                                     │
│                                                                             │
│  ✅ FRAMEWORK HANDLES THIS:                                                 │
│     No manual intervention needed (unless user wants to override)           │
│     Child item created with parent/root links                              │
│     Clear hierarchy of splits                                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 5: Continue pairing child item with next estoque ✅                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Now select estoque for remaining 40 units:                                 │
│                                                                             │
│    SELECT *                                                                │
│    FROM estoques                                                           │
│    WHERE produto_id = 50                                                   │
│      AND fornecedor_id = 2                                                 │
│      AND quantidade_disponivel > 0                                         │
│      AND id NOT IN (777)        ← Exclude already-used estoque              │
│    ORDER BY data_entrada ASC;                                              │
│                                                                             │
│  Result:                                                                    │
│    estoque [888]: 100 units, lote T02 (FIFO next)                          │
│                                                                             │
│  Insert second estoque_consumos:                                            │
│                                                                             │
│    INSERT INTO estoque_consumos (                                           │
│      venda_item_id = 2,         ← CHILD venda item                          │
│      estoque_id = 888,          ← Second estoque (FIFO next)                │
│      quantidade = 40,           ← Match venda_item [2] qty                  │
│      custo_unitario = 35.00,                                               │
│    );                                                                      │
│                                                                             │
│  All DB validations fire again ✅                                            │
│  estoque [888] updated: quantidade_disponivel = 100 - 40 = 60              │
│                                                                             │
│  ✅ PERFECT FIFO:                                                           │
│     First 60 from estoque 777 (Jan 14 09:00) ← consumed first              │
│     Next 40 from estoque 888 (Jan 14 15:00) ← consumed second              │
│                                                                             │
│     FIFO respected, batch consistency maintained!                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

SUMMARY: NEW SYSTEM CONSUMPTION ADVANTAGES

  ✅ FIFO info visible (data_entrada timestamp)
  ✅ UI suggests FIFO selection (but user can override if needed)
  ✅ Quantity validation at DB level (can't over-consume)
  ✅ NO fiscal duplication (in nfe_itens.dados only)
  ✅ 1:1 pairing enforced (UNIQUE constraint)
  ✅ Multiple estoques per venda_item (no product.idEstoque limit)
  ✅ Automatic split detection (child items created)
  ✅ Clear audit trail (user, time, reason, estoque selected)
  ✅ Reversible via estorno (is_estornado soft delete)
  ✅ Reports are accurate (no duplication)
```

---

## 5. Key Problems & Improvements Summary

### Problems in OLD System (8 Critical Issues)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. TWO-LEVEL TABLE MESS (L1/L2)                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  venda_has_produto (L1)      ← Purpose unclear                              │
│    ├─ venda_has_produto2 (L2) ← THE ACTUAL DATA                             │
│                                                                             │
│  Same pattern in:                                                           │
│    pedido_fornecedor (L1)      ← Same confusion                             │
│    └─ pedido_fornecedor_has_produto2 (L2)                                   │
│                                                                             │
│  ❌ Always join L1 → L2 (slow)                                              │
│  ❌ Sync problems (keep L1/L2 in sync)                                      │
│  ❌ Unclear separation of concerns                                          │
│  ❌ idRelacionado creates confusing chains                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. STRING MAGIC (fornecedor VARCHAR)                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  fornecedor = 'ACME Corp' (VARCHAR in 9 tables!) ⚠️                         │
│                                                                             │
│  Appears in:                                                                │
│    venda_has_produto2.fornecedor                                            │
│    estoque.fornecedor                                                       │
│    estoque_has_consumo.fornecedor                                           │
│    pedido_fornecedor_has_produto2.fornecedor                                │
│    compra_avulsa.fornecedor                                                 │
│    ... and 4 more tables ...                                                │
│                                                                             │
│  ❌ Typos ('ACME Corp' vs 'ACE Corp' vs 'Acme Corp')                        │
│  ❌ Update nightmare (change name = update 9 tables)                        │
│  ❌ No referential integrity (DB can't validate)                            │
│  ❌ Orphaned records possible (supplier doesn't exist)                      │
│  ❌ Reports incomplete (spelling variations = multiple suppliers)           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. MEGA-PRODUCT TABLE (100+ columns!)                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  produto table contains:                                                    │
│    - Basic data (id, description, code)                                     │
│    - Pricing (old precos, no versioning)                                    │
│    - Flags (*Upd for tracking changes)                                      │
│    - Calculated fields (estoqueRestante)                                    │
│    - Historical values (oldPrecoVenda)                                      │
│    - idEstoque (ONE only!) ← Limits stock tracking                          │
│                                                                             │
│  ❌ Impossible to version prices (overwrite old data)                       │
│  ❌ Multiple locations impossible (one idEstoque only)                      │
│  ❌ Calculated fields out-of-sync                                           │
│  ❌ Hard to maintain (so many columns)                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. FISCAL COLUMN EXPLOSION (30+ columns)                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  estoque table has 30+ fiscal columns:                                      │
│    ncm, nve, extipi, cest, cfop                                             │
│    tipoICMS, orig, cstICMS, modBC, vBC, pICMS, vICMS                       │
│    modBCST, pMVAST, vBCST, pICMSST, vICMSST                                │
│    cEnq, cstIPI, vBCIPI, pIPI, vIPI                                        │
│    cstPIS, vBCPIS, pPIS, vPIS                                              │
│    cstCOFINS, vBCCOFINS, pCOFINS, vCOFINS                                  │
│    ... and more                                                             │
│                                                                             │
│  estoque_has_consumo ALSO has same 30+ columns                              │
│    → Data is TRIPLED (stored 3 times!)                                      │
│                                                                             │
│  ❌ Duplication (same data in 2 tables)                                     │
│  ❌ Inconsistency risk (which copy is right?)                               │
│  ❌ Hard to maintain (tax changes = update everywhere)                      │
│  ❌ Inflexible (reform tributária IBS/CBS in 2026 will break this)          │
│  ❌ Future NFe saída adds THIRD copy of same data                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 5. NO SPLIT FRAMEWORK                                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  When supplier can't fulfill order:                                         │
│    - Manual intervention (no pattern)                                       │
│    - idRelacionado unclear semantics                                        │
│    - No split_reason (can't explain why)                                    │
│    - No parent/child clarity                                                │
│                                                                             │
│  When NFe partial (40 of 100):                                              │
│    - Ambiguous: Update original or create new?                              │
│    - Lost history (overwrite quantity)                                      │
│    - No tracking of "40 pending"                                            │
│    - State becomes inconsistent                                             │
│                                                                             │
│  ❌ Error-prone (no framework = inconsistency)                              │
│  ❌ Hard to query "all items from original order"                           │
│  ❌ Manual workarounds needed                                               │
│  ❌ Reports are wrong                                                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 6. BROKEN FIFO                                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  estoque has NO data_entrada timestamp!                                     │
│                                                                             │
│  Result:                                                                    │
│    - Can't ORDER BY to determine FIFO                                       │
│    - Manual selection = guessing = errors                                   │
│    - product.idEstoque (ONE only) ← can't handle multiple lotes             │
│                                                                             │
│  For ceramics (batch variation in tone, caliber):                           │
│    - FIFO critical for consistency                                          │
│    - But system can't enforce it                                            │
│    - User selects wrong batch → customer dissatisfied                       │
│                                                                             │
│  ❌ No FIFO support at all                                                  │
│  ❌ Manual selection impossible                                             │
│  ❌ Batch consistency issues                                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 7. NO AUDITORIA (No Audit Trail)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Some *Upd flags exist but no real audit_log table                          │
│                                                                             │
│  Missing:                                                                   │
│    - Who changed what (no user tracking)                                    │
│    - When changed (no timestamp)                                            │
│    - Why changed (no reason)                                                │
│    - Previous values (no old_values tracking)                               │
│                                                                             │
│  ❌ Can't answer: "Who deleted this customer?"                              │
│  ❌ Can't answer: "When was price last changed?"                            │
│  ❌ Can't answer: "Why was estoque split?"                                  │
│  ❌ Regulatory non-compliance (LGPD, fiscal requirements)                   │
│  ❌ Debugging is nightmare                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ 8. NO DB-LEVEL INTEGRITY                                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  All rules enforced by APPLICATION (risky!)                                 │
│                                                                             │
│  Missing:                                                                   │
│    - CHECK constraints (quantities > 0)                                     │
│    - FK constraints (can create orphaned records)                           │
│    - UNIQUE constraints (1:1 relationships not enforced)                    │
│    - Triggers (auto-consistency)                                            │
│                                                                             │
│  If app has bug:                                                            │
│    - Negative quantities possible                                           │
│    - Orphaned records (FK references non-existent)                          │
│    - Duplicate pairings (estoque linked twice)                              │
│    - Invalid status transitions                                             │
│    - Data corruption                                                        │
│                                                                             │
│  ❌ Data integrity depends on app (fragile)                                 │
│  ❌ No safety net (DB doesn't validate)                                     │
│  ❌ Manual enforcement = inevitable mistakes                                │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

### Improvements in NEW System (Summary Table)

```
┌──────────────────────────┬────────────────────┬──────────────────────────┐
│ Aspect                   │ OLD SYSTEM ❌       │ NEW SYSTEM ✅             │
├──────────────────────────┼────────────────────┼──────────────────────────┤
│ Venda Items              │ 2 tables (L1/L2)   │ 1 table (clean)          │
│ Join Complexity          │ Always L1→L2       │ Direct access            │
│ Supplier Reference       │ VARCHAR string     │ FK to fornecedores       │
│ Supplier Change Impact   │ Update 9 tables    │ Update 1 record          │
│ Product Table Size       │ 100+ columns       │ 3 normalized tables      │
│ Price Versioning        │ No (overwrites)    │ Yes (temporal)           │
│ Fiscal Columns          │ 30 in estoque      │ 0 in estoques            │
│ Fiscal Data Storage     │ In columns         │ In nfe_itens JSONB       │
│ Split Handling          │ No framework       │ parent_id/root_id        │
│ Split Semantics         │ Ambiguous          │ Clear hierarchy          │
│ Split Reason Field      │ None               │ split_reason ENUM        │
│ FIFO Capability         │ Impossible         │ Via data_entrada         │
│ FIFO Visibility         │ Manual selection   │ FIFO suggestion in UI    │
│ Stock Pairing           │ 1:N broken         │ 1:1 guaranteed           │
│ Pairing Validation      │ App-level          │ DB-level (triggers)      │
│ Audit Trail             │ None               │ audit_log table          │
│ User Tracking           │ No                 │ Yes (created_by)         │
│ DB Constraints          │ None/minimal       │ Comprehensive            │
│ FK Integrity            │ App-dependent      │ DB-enforced              │
│ Status Management       │ VARCHAR strings    │ ENUMs (type-safe)        │
│ Status Transitions      │ No validation      │ Triggers enforce         │
│ Query Simplicity        │ Complex joins      │ Simple WHERE             │
│ Query Performance       │ Slow (joins)       │ Fast (indexes)           │
│ Data Consistency Risk   │ Very High          │ Very Low                 │
│ Scalability             │ Poor (mega-tables) │ Good (partitioning-ready)│
│ Tax Reform Flexible     │ Rigid columns      │ JSONB extensible         │
│ Soft Deletes            │ Hard deletes       │ is_estornado flag        │
│ History Preservation    │ Lost               │ Preserved (audit_log)    │
│ Bug Tolerance           │ Data corruption    │ DB rejects invalid       │
│ Maintenance Effort      │ High               │ Low                      │
└──────────────────────────┴────────────────────┴──────────────────────────┘
```

---

## Final Summary

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       MIGRATION IMPACT COMPARISON                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│ OLD SYSTEM:                                                                 │
│   • Complexity: Very High (L1/L2 mess, 30+ fiscal columns, no FK)          │
│   • Reliability: Very Low (string references, no constraints)              │
│   • Maintainability: Difficult (many tables, duplication)                  │
│   • Scalability: Poor (mega-tables, no partitioning support)                │
│   • Future-proof: No (rigid schema, inflexible)                            │
│   • Bug Risk: Very High (app-dependent integrity)                          │
│   • Audit: None (no tracking)                                              │
│   • Time to Query: Slow (complex joins)                                    │
│                                                                             │
│ NEW SYSTEM:                                                                 │
│   • Complexity: Low (single tables, clear hierarchy)                        │
│   • Reliability: Very High (FK constraints, triggers, ENUMs)                │
│   • Maintainability: Easy (clear patterns, normal forms)                   │
│   • Scalability: Good (normalized, partitioning-ready)                      │
│   • Future-proof: Yes (JSONB for tax changes, extensible)                   │
│   • Bug Risk: Very Low (DB enforces rules)                                  │
│   • Audit: Complete (audit_log, user tracking, timestamps)                  │
│   • Time to Query: Fast (direct access, good indexes)                       │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

CONCLUSION:

  The new schema:
    ✅ Eliminates the 1:N:N complexity through proper normalization
    ✅ Replaces string references with FKs (referential integrity)
    ✅ Splits fiscal data into dedicated nfe_itens table (JSONB)
    ✅ Implements parent_id/root_id for clear split handling
    ✅ Adds data_entrada for proper FIFO support
    ✅ Enforces 1:1 pairing with unique constraints
    ✅ Adds comprehensive audit trail
    ✅ Uses ENUMs for type-safe status fields
    ✅ Employs triggers for automatic consistency
    ✅ Enables simple, fast queries

  Result: A robust, maintainable, future-proof system!
```

---

**End of Part 3 - Complete ASCII Documentation**

All three parts are now available:
- **Part 1**: Architecture Overview & Data Models
- **Part 2**: Scenario 1 (Simple Sale) & Scenario 2 (Supplier Split)
- **Part 3**: Scenario 3 (Partial NFe), Stock Consumption Flow, & Summary

Total: ~2000 lines of detailed ASCII diagrams and explanations!
