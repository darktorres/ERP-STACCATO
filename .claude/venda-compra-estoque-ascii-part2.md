# Venda → Pedido Fornecedor → Estoque: Old vs New (ASCII Version)

## Part 2: Three Detailed Scenarios

---

## Scenario 1: Simple Sale (No Splits)

### OLD SYSTEM: Simple Sale Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 1: USER ACTION                                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Create venda for customer ACME                                             │
│  Add 100 units of Porcelanato (cost R$35/unit, sell R$45/unit)              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 2: CREATE VENDA (cabeçalho)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO venda (                                                        │
│    idCliente = 1,              ← ACME customer                              │
│    dataVenda = '2025-01-15',                                                │
│    total = 4500.00                                                          │
│  );                                                                         │
│                                                                             │
│  → venda.idVenda = 100                                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 3: CREATE L1 (venda_has_produto)                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO venda_has_produto (                                            │
│    idVenda = 100                 ← Link to venda                            │
│  );                                                                         │
│                                                                             │
│  → venda_has_produto.idVendaProduto = 1                                     │
│                                                                             │
│  ⚠️ Why this table? Unclear purpose...                                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 4: CREATE L2 (venda_has_produto2) - THE ACTUAL DATA                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO venda_has_produto2 (                                           │
│    idVendaProduto = 1,           ← Link to L1                               │
│    idProduto = 50,               ← Porcelanato                              │
│    quantidade = 100,                                                       │
│    valor_unitario = 45.00,                                                 │
│    descricaoProduto = 'Porcelanato Polido 60x60',  ← DESNORMALIZED ⚠️      │
│    fornecedor = 'ACME Corp',     ← MAGIC STRING ⚠️                          │
│    desconto = 0,                                                           │
│    valor_total = 4500.00                                                   │
│  );                                                                         │
│                                                                             │
│  → venda_has_produto2.idVendaProduto2 = 999                                 │
│                                                                             │
│  ⚠️ ALL product data is here in L2                                          │
│  ⚠️ L1 seems to be just a middle-man layer                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 5: DETERMINE ORIGIN (in application code)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Query: Is this product available in estoque?                               │
│                                                                             │
│    SELECT COUNT(*) FROM estoque                                            │
│    WHERE idProduto = 50 AND saldoEstoque > 0;                              │
│                                                                             │
│  IF yes:                                                                   │
│    → consume from existing stock (estoque_has_consumo)                     │
│  ELSE IF no:                                                               │
│    → create purchase order (pedido_fornecedor)                             │
│                                                                             │
│  Result: Must create purchase (no existing stock)                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 6: CREATE PURCHASE (pedido_fornecedor)                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Problem: How to get fornecedor_id?                                        │
│                                                                             │
│    SELECT idFornecedor FROM fornecedor                                     │
│    WHERE razao_social = 'ACME Corp';  ← PARSE STRING!                      │
│                                                                             │
│  ⚠️ This is error-prone!                                                    │
│  ⚠️ What if string doesn't match exactly?                                   │
│                                                                             │
│  Assume found idFornecedor = 2                                              │
│                                                                             │
│  INSERT INTO pedido_fornecedor (                                            │
│    idFornecedor = 2,              ← Finally have ID                         │
│    dataPedido = '2025-01-15',                                               │
│    status = 'PENDENTE'                                                     │
│  );                                                                         │
│                                                                             │
│  → pedido_fornecedor.idPedido = 50                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 7: CREATE PURCHASE ITEMS (L1 + L2)                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO pedido_fornecedor_has_produto (                                │
│    idPedido = 50                 ← Link to purchase                         │
│  );                                                                         │
│  → idPedidoProduto = 1                                                      │
│                                                                             │
│  INSERT INTO pedido_fornecedor_has_produto2 (                               │
│    idPedidoProduto = 1,           ← Link to L1                              │
│    idProduto = 50,                ← Porcelanato                             │
│    quantidade = 100,                                                       │
│    valor_unitario = 35.00,        ← Cost (less than sell price)             │
│    fornecedor = 'ACME Corp',      ← STRING AGAIN ⚠️⚠️                       │
│    status = 'PENDENTE'                                                     │
│  );                                                                         │
│  → idPedidoProduto2 = 200                                                   │
│                                                                             │
│  ⚠️ Same L1/L2 mess in purchases!                                           │
│  ⚠️ STRING 'ACME Corp' appears again (data duplication)                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 8: WAIT FOR NFe FROM SUPPLIER                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Days pass... Supplier ACME sends NFe with 100 units                        │
│                                                                             │
│  NFe XML arrives via email                                                  │
│  Application must parse XML manually (ACBr library)                         │
│  Extract key data from XML                                                  │
│                                                                             │
│  Updated purchase status:                                                  │
│                                                                             │
│    UPDATE pedido_fornecedor_has_produto2                                   │
│    SET status = 'FATURADO'                                                 │
│    WHERE idPedidoProduto2 = 200;                                           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 9: IMPORT NFe INTO ESTOQUE                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO estoque (                                                      │
│    idProduto = 50,                ← Porcelanato                             │
│    fornecedor = 'ACME Corp',      ← STRING AGAIN ⚠️⚠️⚠️                     │
│    quantidade = 100,                                                       │
│    saldoEstoque = 100,            ← Current stock                           │
│    valor_unitario = 35.00,        ← Cost                                    │
│    valor_total = 3500.00,                                                  │
│                                                                             │
│    -- 30+ fiscal columns --                                                │
│    ncm = '69072100',                                                       │
│    cfop = '1102',                                                          │
│    cstICMS = '00',                                                         │
│    orig = '0',                                                             │
│    modBC = '3',                                                            │
│    vBC = 3500.00,                                                          │
│    pICMS = 18.00,                                                          │
│    vICMS = 630.00,                                                         │
│    ... 20+ more fiscal fields ...                                          │
│                                                                             │
│    -- NO data_entrada timestamp! ⚠️ Can't do FIFO --                       │
│  );                                                                         │
│                                                                             │
│  → estoque.idEstoque = 888                                                  │
│                                                                             │
│  ⚠️ ALL fiscal data is in estoque                                           │
│  ⚠️ When sale creates consumption, fiscal duplication incoming...           │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 10: CONSUME STOCK (estoque_has_consumo)                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Now we have stock available. Link venda to estoque.                        │
│                                                                             │
│  ⚠️ Problem: How to link venda_has_produto2 to estoque?                      │
│  There's NO DIRECT FK!                                                     │
│                                                                             │
│  INSERT INTO estoque_has_consumo (                                          │
│    idEstoque = 888,               ← From estoque                            │
│    idVendaProduto2 = 999,         ← From venda_has_produto2                 │
│    quantidade = 100,                                                       │
│    valor_unitario_consumo = 35.00,← Cost                                    │
│    valor_total_consumo = 3500.00,                                           │
│                                                                             │
│    -- 30+ fiscal columns AGAIN! ⚠️⚠️⚠️ --                                   │
│    ncm = '69072100',                                                       │
│    cfop = '5102',            ← Now different CFOP (outbound)                │
│    cstICMS = '00',                                                         │
│    ... duplicated again ...                                                │
│  );                                                                         │
│                                                                             │
│  → estoque_has_consumo.idEstoqueConsumo = 1500                              │
│                                                                             │
│  ⚠️ Same fiscal data appears 3 times:                                       │
│     1. estoque (inbound from supplier)                                      │
│     2. estoque_has_consumo (outbound to customer)                           │
│     3. future NFe saída (will have it again)                                │
│                                                                             │
│  Update estoque balance:                                                   │
│                                                                             │
│    UPDATE estoque                                                          │
│    SET saldoEstoque = 0        ← Was 100, now 0                             │
│    WHERE idEstoque = 888;                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ SUMMARY: Data Created                                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  venda [1]                                                                  │
│    ├─ venda_has_produto [1]                                                 │
│    │   └─ venda_has_produto2 [999]  ← THE ACTUAL DATA                      │
│    │       └─ (links to) pedido_fornecedor_has_produto2 [200]              │
│    │           └─ (links to) estoque [888]                                 │
│    │               └─ estoque_has_consumo [1500]  ← BACK TO VENDA          │
│    │                                                                       │
│    └─ (eventually) nfe saída ← With SAME fiscal data AGAIN                 │
│                                                                             │
│  Complexity: 7 tables touched, 3 levels deep (L1/L2, then more L1/L2),      │
│             Multiple string references, Data duplication, Hard to query    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

KEY PROBLEMS IN THIS SIMPLE CASE:

  ❌ L1/L2 duplication in venda AND pedido_fornecedor
  ❌ STRING 'ACME Corp' appears in 4 places (venda_has_produto2, pedido_fornecedor_has_produto2, estoque, estoque_has_consumo)
  ❌ Fiscal columns duplicated (estoque + estoque_has_consumo)
  ❌ No data_entrada timestamp (can't do FIFO)
  ❌ Complex query to get venda items (must join L1 + L2)
  ❌ Hard to track which estoque links to which venda item
  ❌ No clear parent-child relationship for future splits
```

---

### NEW SYSTEM: Simple Sale Flow

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 1: USER ACTION                                                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Create venda for customer ACME                                             │
│  Add 100 units of Porcelanato (cost R$35/unit, sell R$45/unit)              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 2: CREATE VENDA (cabeçalho)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO vendas (                                                       │
│    loja_id = 1,                  ← Store                                    │
│    cliente_id = 1,               ← ACME customer                            │
│    data_emissao = '2025-01-15',                                             │
│    total = 4500.00                                                          │
│  );                                                                         │
│                                                                             │
│  → vendas.id = 100                                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 3: CREATE ITEM (single table!) ✅                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO venda_itens (                                                  │
│    venda_id = 100,               ← Link to venda                            │
│    produto_id = 50,              ← FK to produtos ✅                        │
│    fornecedor_id = 2,            ← FK to fornecedores ✅                    │
│    quantidade = 100,                                                       │
│    valor_unitario = 45.00,                                                 │
│    descricao_produto = 'Porcelanato Polido 60x60',  ← snapshot of product  │
│    codigo_comercial = 'POR-60X60-POL',             ← snapshot              │
│    valor_total = 4500.00,                                                  │
│                                                                             │
│    origem = 'COMPRA',            ← Need to order ✅                         │
│    parent_id = NULL,             ← This is original (no split yet)         │
│    root_id = NULL,               ← No hierarchy yet                         │
│    status = 'PENDENTE',          ← ENUM, not string                        │
│  );                                                                         │
│                                                                             │
│  → venda_itens.id = 1                                                       │
│                                                                             │
│  ✅ That's it! One row, all data, all FKs validated by DB.                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 4: CREATE PURCHASE (automatic, one table) ✅                           │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Since origem = 'COMPRA', system creates purchase order.                    │
│                                                                             │
│  INSERT INTO compras (                                                      │
│    loja_id = 1,                                                            │
│    fornecedor_id = 2,            ← FK ✅ (from venda_itens)                 │
│    venda_id = 100,               ← Link back to original venda              │
│    data_emissao = '2025-01-15',                                             │
│    status = 'PENDENTE'            ← ENUM                                    │
│  );                                                                         │
│                                                                             │
│  → compras.id = 50                                                          │
│                                                                             │
│  ✅ Clean, no L1/L2 mess                                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 5: CREATE PURCHASE ITEM (linked to venda_item) ✅                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO compra_itens (                                                 │
│    compra_id = 50,               ← FK to compras                            │
│    produto_id = 50,              ← FK to produtos                           │
│    venda_item_id = 1,            ← FK to venda_itens ✅ DIRECT LINK         │
│    quantidade = 100,                                                       │
│    valor_unitario = 35.00,       ← Cost from supplier                       │
│    valor_total = 3500.00,                                                  │
│    parent_id = NULL,             ← Can split later if needed                │
│    root_id = NULL,                                                         │
│    status = 'PENDENTE'                                                     │
│  );                                                                         │
│                                                                             │
│  → compra_itens.id = 200                                                    │
│                                                                             │
│  ✅ Single table, all FKs, clear hierarchy structure ready                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 6: WAIT FOR NFe FROM SUPPLIER                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Days pass... Supplier ACME sends NFe with 100 units                        │
│                                                                             │
│  NFe XML arrives via email                                                  │
│  Application parses XML (ACBr library)                                      │
│  Creates nfes + nfe_itens records                                           │
│                                                                             │
│  Updated purchase status:                                                  │
│                                                                             │
│    UPDATE compra_itens                                                     │
│    SET status = 'FATURADO'                                                 │
│    WHERE id = 200;                                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 7: CREATE NFe RECORDS (clean structure) ✅                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO nfes (                                                         │
│    loja_id = 1,                                                            │
│    tipo = 'ENTRADA',             ← ENUM                                     │
│    numero = 123456,                                                        │
│    serie = 1,                                                              │
│    chave = '35250112345678901234567890123456789012345678',                 │
│    emitente_id = 2,              ← FK to fornecedores                       │
│    destinatario_id = NULL,       ← NULL (we're receiving)                  │
│    valor_total = 3500.00,                                                  │
│    status = 'AUTORIZADA',        ← ENUM                                     │
│    data_emissao = '2025-01-14'                                              │
│  );                                                                         │
│                                                                             │
│  → nfes.id = 1000                                                           │
│                                                                             │
│  INSERT INTO nfe_itens (                                                    │
│    nfe_id = 1000,                                                          │
│    numero_item = 1,                                                        │
│    produto_id = 50,                                                        │
│    compra_item_id = 200,         ← FK ✅ LINKS TO PURCHASE                  │
│    dados = {JSONB},              ← ALL fiscal data in JSONB ✅              │
│  );                                                                         │
│                                                                             │
│  → nfe_itens.id = 1000                                                      │
│                                                                             │
│  Example JSONB dados:                                                      │
│  {                                                                         │
│    "cfop": "1102",                                                         │
│    "ncm": "69072100",                                                      │
│    "quantidade": 100,                                                      │
│    "valor_unitario": 35.00,                                                │
│    "valor_total": 3500.00,                                                 │
│    "icms": {                                                               │
│      "cst": "00",                                                          │
│      "origem": "0",                                                        │
│      "vBC": 3500.00,                                                       │
│      "aliquota": 18.00,                                                    │
│      "valor": 630.00                                                       │
│    },                                                                      │
│    ... other fiscal data ...                                               │
│  }                                                                         │
│                                                                             │
│  ✅ ALL fiscal data in ONE JSONB, not spread across columns                 │
│  ✅ Easy to extend for future tax reforms (IBS/CBS)                         │
│  ✅ No duplication across multiple tables                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 8: CREATE ESTOQUE (clean!) ✅                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO estoques (                                                     │
│    loja_id = 1,                                                            │
│    produto_id = 50,              ← FK ✅                                    │
│    fornecedor_id = 2,            ← FK ✅                                    │
│    nfe_entrada_id = 1000,        ← FK ✅                                    │
│    nfe_item_id = 1000,           ← FK ✅                                    │
│    compra_item_id = 200,         ← FK ✅                                    │
│                                                                             │
│    quantidade_original = 100,                                              │
│    quantidade_disponivel = 100,  ← Will decrease when consumed             │
│    custo_unitario = 35.00,                                                 │
│    custo_total = 3500.00,                                                  │
│    lote = 'ACME-2025-01-14',     ← Batch identifier                         │
│    data_validade = '2026-01-14', ← Expiration                               │
│    data_entrada = '2025-01-14 10:30:00',  ← TIMESTAMP FOR FIFO ✅           │
│    status = 'DISPONIVEL'         ← ENUM                                     │
│  );                                                                         │
│                                                                             │
│  → estoques.id = 777                                                        │
│                                                                             │
│  ✅ Clean table, only quantity/location data                                │
│  ✅ NO fiscal columns (those are in nfe_itens.dados)                        │
│  ✅ data_entrada timestamp enables FIFO                                     │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP 9: PAIR STOCK TO VENDA ITEM (1:1 link) ✅                             │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  User manually selects which estoque to use for this venda_item.            │
│  (Seleção manual: important for products like ceramics with batch           │
│   variation in tone, caliber, etc.)                                        │
│                                                                             │
│  INSERT INTO estoque_consumos (                                             │
│    venda_item_id = 1,            ← FK ✅ (from venda_itens)                 │
│    estoque_id = 777,             ← FK ✅ (from estoques)                    │
│    quantidade = 100,             ← Must match venda_item.quantidade        │
│    custo_unitario = 35.00,       ← Snapshot at consumption time             │
│    custo_total = 3500.00,        ← GENERATED ALWAYS (calc'd)                │
│    motivo = 'VENDA',             ← ENUM: why was stock consumed?           │
│    is_estornado = FALSE,         ← Not reversed yet                         │
│    created_by = :user_id,        ← Audit: who paired it                    │
│  );                                                                         │
│                                                                             │
│  → estoque_consumos.id = 999                                                │
│                                                                             │
│  DB TRIGGERS FIRE:                                                          │
│    1. Validate: quantidade matches, estoque has enough, same product, etc. │
│    2. Update estoque: quantidade_disponivel -= 100                          │
│    3. Update venda_itens: status = 'ESTOQUE'                                │
│                                                                             │
│  ✅ ONE record, all FKs, all constraints checked by DB                      │
│  ✅ NO fiscal duplication (fiscal is in nfe_itens.dados)                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ SUMMARY: Data Created                                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Tables touched:                                                            │
│    ✅ vendas [100]                                                          │
│    ✅ venda_itens [1]           (one row, all data)                         │
│    ✅ compras [50]              (one row)                                   │
│    ✅ compra_itens [200]        (one row, links to venda_item)              │
│    ✅ nfes [1000]               (one row)                                   │
│    ✅ nfe_itens [1000]          (one row, fiscal in JSONB)                  │
│    ✅ estoques [777]            (one row, clean)                            │
│    ✅ estoque_consumos [999]    (one row, 1:1 link)                         │
│                                                                             │
│  Total: 8 tables, all with clear relationships and FKs                     │
│  Complexity: LOW (linear flow, no L1/L2 duplication)                       │
│  Data integrity: HIGH (enforced by DB constraints and triggers)            │
│                                                                             │
│  Queries are simple:                                                       │
│    SELECT * FROM venda_itens WHERE venda_id = 100;   ← Direct!             │
│    SELECT * FROM estoques WHERE data_entrada > '2025-01-14';  ← FIFO sort  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

KEY ADVANTAGES IN THIS SIMPLE CASE:

  ✅ No L1/L2 mess (single venda_itens table)
  ✅ No string magic (fornecedor_id FK instead of VARCHAR)
  ✅ Fiscal data isolated (nfe_itens.dados JSONB, not duplicated)
  ✅ FIFO support (data_entrada timestamp)
  ✅ 1:1 pairing enforced (UNIQUE constraint)
  ✅ Simple queries (direct access to venda_itens)
  ✅ Clear hierarchy ready (parent_id/root_id for future splits)
  ✅ DB-level integrity (triggers, constraints, FKs)
  ✅ Audit trail (created_by, timestamps)
```

---

## Scenario 2: Sale with Supplier Split

**Context**: Customer orders Porcelanato from ACME, but ACME only supplies 60 units.
The remaining 40 must come from supplier BRICKS.

### OLD SYSTEM: Supplier Split (Messy & Error-Prone)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ PROBLEM: Supplier can't fulfill full order                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Original order:  100 units from ACME Corp                                  │
│  ACME can supply: 60 units only                                              │
│  Remaining:       40 units from another supplier (BRICKS)                    │
│                                                                             │
│  ❌ OLD SYSTEM: No framework for this situation!                             │
│  How should we handle it?                                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

EXISTING STATE (from Scenario 1):

  venda_has_produto2 [999]:
    ├─ idVendaProduto2 = 999
    ├─ idProduto = 50
    ├─ quantidade = 100
    ├─ fornecedor = 'ACME Corp'
    ├─ idRelacionado = NULL
    └─ status = ?

┌─────────────────────────────────────────────────────────────────────────────┐
│ APPROACH 1: Update original, create new row (common BAD practice)            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Step 1: Reduce original to 60                                             │
│                                                                             │
│    UPDATE venda_has_produto2                                               │
│    SET quantidade = 60            ← Changed from 100 to 60                  │
│    WHERE idVendaProduto2 = 999;                                            │
│                                                                             │
│  Step 2: Create new row for remaining 40                                   │
│                                                                             │
│    INSERT INTO venda_has_produto2 (                                        │
│      idVendaProduto = 1,           ← Still same L1                          │
│      idProduto = 50,                                                       │
│      quantidade = 40,               ← The remainder                         │
│      fornecedor = 'BRICKS',        ← Different supplier                    │
│      idRelacionado = 999           ← ??? Points to original?                │
│    );                                                                      │
│    → idVendaProduto2 = 1000                                                 │
│                                                                             │
│  ⚠️ PROBLEM 1: idRelacionado semantics are unclear!                         │
│     - Is 999 the parent? Then 1000 is child?                                │
│     - Or are they siblings (both children of same sale)?                    │
│     - When querying: "get all items from original order", what do we find?  │
│     - No standard interpretation!                                           │
│                                                                             │
│  ⚠️ PROBLEM 2: No split_reason!                                             │
│     Later: Why did this split happen?                                       │
│     → Have to infer from context (hard to debug)                            │
│                                                                             │
│  ⚠️ PROBLEM 3: Lost information!                                            │
│     When we updated quantity from 100 to 60:                                │
│     → We overwrote the ORIGINAL quantity                                    │
│     → If status already had a value, we didn't change it (inconsistent)     │
│     → Can't reconstruct audit trail                                         │
│                                                                             │
│  ⚠️ PROBLEM 4: Multiple purchase orders confusion!                          │
│     Row 999 (60 units) → pedido_fornecedor_has_produto2 [200] (ACME)       │
│     Row 1000 (40 units) → pedido_fornecedor_has_produto2 [???] (BRICKS)   │
│     But both link to same L1 row (venda_has_produto 1)                     │
│     Nightmare to query: Which purchase goes with which L2 row?             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ CREATING SECOND PURCHASE (for BRICKS)                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Must find BRICKS furnecedor_id (string parsing again):                     │
│                                                                             │
│    SELECT idFornecedor FROM fornecedor                                     │
│    WHERE razao_social = 'BRICKS';  ← Search by string                      │
│                                                                             │
│  Assume found: idFornecedor = 5                                              │
│                                                                             │
│  INSERT INTO pedido_fornecedor (idFornecedor = 5, ...);                     │
│  → idPedido = 51                                                            │
│                                                                             │
│  INSERT INTO pedido_fornecedor_has_produto (...);                           │
│  → idPedidoProduto = 2                                                      │
│                                                                             │
│  INSERT INTO pedido_fornecedor_has_produto2 (                               │
│    idPedidoProduto = 2,                                                    │
│    idProduto = 50,                                                         │
│    quantidade = 40,                                                        │
│    valor_unitario = 36.00,        ← Different price from different supplier│
│    fornecedor = 'BRICKS',         ← String again                            │
│    status = 'PENDENTE'                                                     │
│  );                                                                         │
│  → idPedidoProduto2 = 201                                                   │
│                                                                             │
│  ⚠️ Now we have 2 purchase orders (ACME + BRICKS) for same venda item!      │
│  ⚠️ How does application know both relate to item 999 (or split)?           │
│  ⚠️ No explicit relationship between pedido and venda_has_produto2!        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ WHEN BRICKS NFe ARRIVES (40 units)                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO estoque (                                                      │
│    idProduto = 50,                                                         │
│    fornecedor = 'BRICKS',         ← String! (must match exactly)           │
│    quantidade = 40,                                                        │
│    ... 30+ fiscal columns ...                                              │
│  );                                                                         │
│  → idEstoque = 889                                                          │
│                                                                             │
│  Now we have 2 estoques:                                                    │
│    estoque 888: 60 units from ACME (data_entrada = ?)                      │
│    estoque 889: 40 units from BRICKS (data_entrada = ?)                    │
│                                                                             │
│  ⚠️ FIFO Broken!                                                            │
│     No timestamp, can't tell which arrived first                            │
│     No ORDER BY clause possible                                            │
│     Manual selection? (error-prone)                                        │
│                                                                             │
├─────────────────────────────────────────────────────────────────────────────┤
│ FINAL NIGHTMARE: Consuming both stocks                                      │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  Which venda_has_produto2 row gets which estoque?                          │
│                                                                             │
│  Option 1: Original split into 60 + 40, consume separately:                │
│    estoque_has_consumo [1] ← idEstoque=888, idVendaProduto2=999           │
│    estoque_has_consumo [2] ← idEstoque=889, idVendaProduto2=1000          │
│                                                                             │
│  But what if we later discover split was actually a temporary solution,    │
│  and we want to "undo" it? No clear way!                                   │
│                                                                             │
│  ⚠️ STATE IS AMBIGUOUS                                                      │
│  ⚠️ NO PARENT-CHILD RELATIONSHIP ENFORCED                                    │
│  ⚠️ HARD TO QUERY "all items from original order"                           │
│  ⚠️ CAN'T RECONSTRUCT WHAT HAPPENED                                         │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

SUMMARY OF OLD SYSTEM PROBLEMS WITH SPLITS:

  ❌ No framework (application must decide how to handle)
  ❌ idRelacionado semantics unclear (parent? sibling? both?)
  ❌ No split_reason field (can't explain why split happened)
  ❌ Lost history when updating quantities
  ❌ String references require exact matches
  ❌ Multiple purchases created but no explicit link to split items
  ❌ No FIFO support (missing timestamps)
  ❌ Querying all items from "original order" is complex
  ❌ Very error-prone and inconsistent
```

---

### NEW SYSTEM: Supplier Split (Clean & Automatic) ✅

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ STARTING STATE (from Scenario 1)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  venda_itens [1]:                                                           │
│    ├─ venda_id = 100                                                       │
│    ├─ produto_id = 50                                                      │
│    ├─ fornecedor_id = 2 (ACME) ← FK                                        │
│    ├─ quantidade = 100                                                     │
│    ├─ parent_id = NULL         ← Original (no splits yet)                   │
│    ├─ root_id = NULL                                                       │
│    ├─ status = 'EM_COMPRA'                                                 │
│    └─ origem = 'COMPRA'                                                    │
│                                                                             │
│  compra_itens [200]:                                                        │
│    ├─ compra_id = 50                                                       │
│    ├─ venda_item_id = 1        ← FK link ✅                                 │
│    ├─ quantidade = 100                                                     │
│    └─ status = 'PENDENTE'                                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ EVENT: ACME CONFIRMS: "Can only supply 60 units"                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  System detects: 100 ordered, only 60 from ACME                             │
│  → Automatically triggers SPLIT logic ✅                                     │
│                                                                             │
│  Step 1: Update original venda_item to 60 (reduce quantity)                │
│                                                                             │
│    UPDATE venda_itens                                                      │
│    SET quantidade = 60,         ← Was 100                                   │
│        status = 'EM_COMPRA'     ← Stays in compra flow                     │
│    WHERE id = 1;                                                           │
│                                                                             │
│  Step 2: Create child venda_item for remaining 40 ✅ (NEW ITEM)             │
│                                                                             │
│    INSERT INTO venda_itens (                                               │
│      venda_id = 100,            ← Same venda                                │
│      produto_id = 50,           ← Same product                              │
│      fornecedor_id = 5,         ← DIFFERENT! (BRICKS) ✅ FK                 │
│      quantidade = 40,           ← The remainder                             │
│      valor_unitario = 45.00,    ← Snapshot of current price                 │
│                                                                             │
│      parent_id = 1,             ← POINTS TO ORIGINAL ✅ (clear hierarchy)    │
│      root_id = 1,               ← BOTH FROM SAME ROOT ✅                     │
│      split_reason = 'SUPPLIER_SPLIT',  ← EXPLAINS WHY ✅                    │
│                                                                             │
│      status = 'PENDENTE',       ← Fresh start for this split                │
│      origem = 'COMPRA'                                                     │
│    );                                                                      │
│    → venda_itens.id = 2                                                     │
│                                                                             │
│  ✅ CLEAN HIERARCHY:                                                        │
│     venda_itens [1] (60 units, ACME)                                       │
│       ├─ parent_id = NULL, root_id = NULL   (ORIGINAL)                     │
│       └─ split_reason = NULL                                              │
│                                                                             │
│     venda_itens [2] (40 units, BRICKS)                                     │
│       ├─ parent_id = 1, root_id = 1        (CHILD of 1)                   │
│       └─ split_reason = 'SUPPLIER_SPLIT'   (WHY it split)                 │
│                                                                             │
│  ✅ Query all items from original order:                                    │
│     SELECT * FROM venda_itens WHERE id = 1 OR root_id = 1;                 │
│     → Returns items 1 (60) and 2 (40) = 100 total ✅                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP: Update original compra_item to 60                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  UPDATE compra_itens                                                       │
│  SET quantidade = 60             ← Match updated venda_item                 │
│  WHERE id = 200;                                                           │
│                                                                             │
│  ✅ Automatically stays linked to venda_item 1 (FK enforces)                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP: Create NEW compra for BRICKS (second supplier)                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO compras (                                                      │
│    loja_id = 1,                                                            │
│    fornecedor_id = 5,           ← BRICKS (FK ✅)                            │
│    venda_id = 100,              ← Same venda                                │
│    status = 'PENDENTE'                                                     │
│  );                                                                         │
│  → compras.id = 51                                                          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ STEP: Create compra_item for BRICKS (40 units)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO compra_itens (                                                 │
│    compra_id = 51,              ← NEW compra for BRICKS                     │
│    produto_id = 50,                                                        │
│    venda_item_id = 2,           ← FK to CHILD venda_item ✅                 │
│    quantidade = 40,                                                        │
│    valor_unitario = 36.00,      ← Different price from BRICKS               │
│    parent_id = NULL,            ← Can have its own splits later             │
│    root_id = NULL,                                                         │
│    status = 'PENDENTE'                                                     │
│  );                                                                         │
│  → compra_itens.id = 201                                                    │
│                                                                             │
│  ✅ CLEAR MAPPING:                                                          │
│     venda_itens [1] (60, ACME) ← compra_itens [200]                        │
│     venda_itens [2] (40, BRICKS) ← compra_itens [201]                      │
│                                                                             │
│  ✅ If venda_item 2 is later split again, compra_items [201] can have       │
│     its own parent_id/root_id                                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ WHEN ACME NFe ARRIVES (60 units)                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO nfes (...);                                                    │
│  INSERT INTO nfe_itens (                                                    │
│    compra_item_id = 200,        ← FK ✅ (links to compra_itens 200)        │
│    dados = {...}                ← Fiscal data in JSONB                      │
│  );                                                                         │
│                                                                             │
│  INSERT INTO estoques (                                                     │
│    nfe_item_id = X,             ← FK ✅                                     │
│    compra_item_id = 200,        ← FK ✅                                     │
│    quantidade_original = 60,                                               │
│    data_entrada = '2025-01-14 10:00:00',  ← TIMESTAMP ✅ (FIFO)             │
│    status = 'DISPONIVEL'                                                   │
│  );                                                                         │
│  → estoques.id = 777                                                        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ WHEN BRICKS NFe ARRIVES (40 units)                                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  INSERT INTO nfes (...);                                                    │
│  INSERT INTO nfe_itens (                                                    │
│    compra_item_id = 201,        ← FK ✅ (links to compra_itens 201)        │
│    dados = {...}                ← Fiscal data in JSONB                      │
│  );                                                                         │
│                                                                             │
│  INSERT INTO estoques (                                                     │
│    nfe_item_id = Y,             ← FK ✅                                     │
│    compra_item_id = 201,        ← FK ✅                                     │
│    quantidade_original = 40,                                               │
│    data_entrada = '2025-01-15 14:00:00',  ← TIMESTAMP ✅ (FIFO)             │
│    status = 'DISPONIVEL'                                                   │
│  );                                                                         │
│  → estoques.id = 888                                                        │
│                                                                             │
│  ✅ Different timestamp = FIFO order is clear                               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────────────────┐
│ PAIRING STOCKS (manual selection by user)                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  For venda_itens [1] (60 units from ACME):                                  │
│                                                                             │
│    INSERT INTO estoque_consumos (                                           │
│      venda_item_id = 1,         ← Links to item 1 (60)                      │
│      estoque_id = 777,          ← ACME stock (from first NFe)               │
│      quantidade = 60,                                                      │
│      motivo = 'VENDA'                                                      │
│    );                                                                      │
│                                                                             │
│  For venda_itens [2] (40 units from BRICKS):                                │
│                                                                             │
│    INSERT INTO estoque_consumos (                                           │
│      venda_item_id = 2,         ← Links to item 2 (40)                      │
│      estoque_id = 888,          ← BRICKS stock (from second NFe)            │
│      quantidade = 40,                                                      │
│      motivo = 'VENDA'                                                      │
│    );                                                                      │
│                                                                             │
│  ✅ PERFECT 1:1 MAPPING:                                                    │
│     venda_item 1 (60) ← estoque 777 (ACME)                                  │
│     venda_item 2 (40) ← estoque 888 (BRICKS)                                │
│                                                                             │
│  ✅ Clear audit trail:                                                      │
│     Original order split because ACME couldn't supply 100                  │
│     60 came from ACME, 40 came from BRICKS                                  │
│     Both suppliers are now identified, tracked, costed correctly            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘

SUMMARY: NEW SYSTEM ADVANTAGES FOR SPLITS

  ✅ Framework provided (parent_id/root_id hierarchy)
  ✅ split_reason explains WHY (SUPPLIER_SPLIT, PARTIAL_NFE, etc.)
  ✅ Query all items: WHERE id = 1 OR root_id = 1
  ✅ History preserved (don't overwrite quantity)
  ✅ Foreign key integrity (no string matching)
  ✅ FIFO support (timestamps on estoques)
  ✅ Clear supplier mapping (one compra per supplier)
  ✅ 1:1 pairing (each venda_item → exact estoque)
  ✅ Automatic split detection and handling
  ✅ Extensible (splits can be nested further if needed)
```

---

**End of Part 2**

Next up in Part 3:
- Scenario 3: Partial NFe (Split Across Shipments)
- Stock Consumption Flow (Old vs New)
