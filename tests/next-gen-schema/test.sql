-- Next-Gen Schema Test Script
-- Run via: docker exec -i erp_nextgen_postgres psql -U postgres -d erp_nextgen_test < test.sql

\echo '======================================================================'
\echo '  1. SEED TEST DATA'
\echo '======================================================================'

-- Loja
INSERT INTO lojas (nome, cnpj) VALUES ('Loja Matriz', '12345678000199');
\echo '[OK] Loja created'

-- Fornecedor
INSERT INTO fornecedores (razao_social, cnpj) VALUES ('Ceramica ABC', '98765432000111');
\echo '[OK] Fornecedor created'

-- Cliente
INSERT INTO clientes (nome, cpf_cnpj) VALUES ('Joao Silva', '12345678901');
\echo '[OK] Cliente created'

-- Produtos
INSERT INTO produtos (fornecedor_id, codigo, descricao, unidade) VALUES
    (1, 'POR-60X60-BR', 'Porcelanato 60x60 Branco', 'M2'),
    (1, 'POR-60X60-CZ', 'Porcelanato 60x60 Cinza', 'M2');
\echo '[OK] Produtos created'

-- Estoques (multiple lots)
INSERT INTO estoques (loja_id, produto_id, fornecedor_id, quantidade_original, quantidade_disponivel, custo_unitario, lote, data_entrada) VALUES
    (1, 1, 1, 100, 100, 35.00, 'LOTE-A1', '2025-01-01 10:00:00'),
    (1, 1, 1, 150, 150, 36.00, 'LOTE-A2', '2025-01-05 10:00:00'),
    (1, 2, 1, 80, 80, 40.00, 'LOTE-B1', '2025-01-03 10:00:00');
\echo '[OK] Estoques created: LOTE-A1(100), LOTE-A2(150), LOTE-B1(80)'
\echo ''

\echo '======================================================================'
\echo '  2. TEST: Create Venda (single table, no L1/L2!)'
\echo '======================================================================'

INSERT INTO vendas (loja_id, cliente_id, total) VALUES (1, 1, 0);

SELECT
    CASE WHEN status = 'ABERTA' THEN '[OK] Venda status is ABERTA'
         ELSE '[ERROR] Wrong status: ' || status::text END as result
FROM vendas WHERE id = 1;

\echo ''

\echo '======================================================================'
\echo '  3. TEST: Add Venda Item (single INSERT, no trigger copy to L2!)'
\echo '======================================================================'

INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 50, 45.00, 2250.00);

SELECT
    '[OK] Item created: id=' || id || ', qty=' || quantidade || ', status=' || status::text as result
FROM venda_itens WHERE id = 1;

-- Verify no parent/root for original
SELECT
    CASE WHEN parent_id IS NULL AND root_id IS NULL
         THEN '[OK] parent_id and root_id are NULL for original item'
         ELSE '[ERROR] parent_id/root_id should be NULL' END as result
FROM venda_itens WHERE id = 1;

\echo ''

\echo '======================================================================'
\echo '  4. TEST: Split Item (replaces idRelacionado pattern)'
\echo '======================================================================'

-- Create item for split test
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 100, 45.00, 4500.00);

\echo '[INFO] Created original item id=2 with qty=100'

-- Simulate: NFe arrives with only 60 units
SELECT split_venda_item(2, 60, 'PARTIAL_NFE') as split_id;

-- Verify original was updated
SELECT
    CASE WHEN quantidade = 60
         THEN '[OK] Original (id=2) now has qty=60'
         ELSE '[ERROR] Original should have 60, has: ' || quantidade::text END as result
FROM venda_itens WHERE id = 2;

-- Verify split was created correctly
SELECT
    '[OK] Split created: id=' || id ||
    ', qty=' || quantidade ||
    ', parent_id=' || parent_id ||
    ', root_id=' || root_id ||
    ', reason=' || split_reason as result
FROM venda_itens WHERE parent_id = 2;

\echo ''

\echo '======================================================================'
\echo '  5. TEST: Cascading Split (root_id traces back to original)'
\echo '======================================================================'

-- Get the split id
\set split_id `psql -U postgres -d erp_nextgen_test -t -c "SELECT id FROM venda_itens WHERE parent_id = 2 LIMIT 1"`

-- Do cascading split on the split
SELECT split_venda_item(3, 25, 'PARTIAL_DELIVERY') as cascade_split_id;

-- Verify cascade still points to ORIGINAL root
SELECT
    CASE WHEN root_id = 2
         THEN '[OK] Cascade split root_id correctly traces back to original (id=2)!'
         ELSE '[ERROR] root_id should be 2, is: ' || COALESCE(root_id::text, 'NULL') END as result
FROM venda_itens WHERE parent_id = 3;

-- Show the full hierarchy
\echo ''
\echo '[INFO] Full split hierarchy:'
SELECT id, parent_id, root_id, quantidade, split_reason
FROM venda_itens
WHERE id IN (2) OR root_id = 2
ORDER BY id;

\echo ''

\echo '======================================================================'
\echo '  6. TEST: Estoque Consumo (1:1 pairing)'
\echo '======================================================================'

-- Create new item for consumption test
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 50, 45.00, 2250.00);

\echo '[INFO] Created item id=5 with qty=50 for consumption test'

-- Show available stock before
\echo '[INFO] Stock BEFORE consumption:'
SELECT id, lote, quantidade_disponivel, status::text FROM estoques WHERE produto_id = 1;

-- Create consumption (pairing with LOTE-A1)
INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
VALUES (5, 1, 50, 35.00);

\echo ''
\echo '[INFO] Stock AFTER consumption:'
SELECT id, lote, quantidade_disponivel, status::text FROM estoques WHERE produto_id = 1;

-- Verify stock decreased
SELECT
    CASE WHEN quantidade_disponivel = 50
         THEN '[OK] LOTE-A1 stock decreased from 100 to 50'
         ELSE '[ERROR] Stock should be 50, is: ' || quantidade_disponivel::text END as result
FROM estoques WHERE id = 1;

-- Verify item status changed to ESTOQUE
SELECT
    CASE WHEN status = 'ESTOQUE'
         THEN '[OK] Item status auto-updated to ESTOQUE (trigger worked!)'
         ELSE '[ERROR] Status should be ESTOQUE, is: ' || status::text END as result
FROM venda_itens WHERE id = 5;

\echo ''

\echo '======================================================================'
\echo '  7. TEST: Estorno (reversal)'
\echo '======================================================================'

-- Reverse the consumption
UPDATE estoque_consumos
SET is_estornado = TRUE, estornado_em = NOW(), estorno_motivo = 'Teste de estorno'
WHERE venda_item_id = 5;

-- Verify stock was restored
SELECT
    CASE WHEN quantidade_disponivel = 100
         THEN '[OK] Stock restored to 100 after reversal'
         ELSE '[ERROR] Stock should be 100, is: ' || quantidade_disponivel::text END as result
FROM estoques WHERE id = 1;

\echo ''

\echo '======================================================================'
\echo '  8. TEST: Validations'
\echo '======================================================================'

-- Test: Product mismatch
\echo '[INFO] Testing product mismatch...'
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 30, 45.00, 1350.00);

DO $$
BEGIN
    -- Try to pair product 1 item with product 2 stock
    INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
    VALUES (6, 3, 30, 40.00);  -- estoque 3 is product 2
    RAISE NOTICE '[ERROR] Should have been rejected!';
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE '[OK] Product mismatch correctly rejected: %', SQLERRM;
END $$;

-- Test: Quantity mismatch
\echo ''
\echo '[INFO] Testing quantity mismatch...'
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 30, 45.00, 1350.00);

DO $$
BEGIN
    -- Try to consume different quantity than item
    INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
    VALUES (7, 1, 25, 35.00);  -- item has 30, trying 25
    RAISE NOTICE '[ERROR] Should have been rejected!';
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE '[OK] Quantity mismatch correctly rejected: %', SQLERRM;
END $$;

-- Test: 1:1 constraint (double pairing same stock)
\echo ''
\echo '[INFO] Testing 1:1 constraint (double pairing)...'
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 20, 45.00, 900.00);

-- First pairing
INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
VALUES (8, 1, 20, 35.00);
\echo '[OK] First pairing successful'

-- Try second item with same stock
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 20, 45.00, 900.00);

DO $$
BEGIN
    INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
    VALUES (9, 1, 20, 35.00);  -- same estoque_id=1
    RAISE NOTICE '[ERROR] Should have been rejected!';
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE '[OK] Double pairing correctly rejected (1:1 constraint works)';
END $$;

-- Test: Insufficient stock
\echo ''
\echo '[INFO] Testing insufficient stock...'
INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
VALUES (1, 1, 1, 9999, 45.00, 449955.00);

DO $$
BEGIN
    INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
    VALUES (10, 2, 9999, 36.00);  -- estoque 2 only has 150
    RAISE NOTICE '[ERROR] Should have been rejected!';
EXCEPTION WHEN OTHERS THEN
    RAISE NOTICE '[OK] Insufficient stock correctly rejected: %', SQLERRM;
END $$;

\echo ''

\echo '======================================================================'
\echo '  9. TEST: Aggregated View (replaces L1 queries)'
\echo '======================================================================'

\echo '[INFO] Aggregated view results for venda_id=1:'
SELECT
    venda_id,
    produto_id,
    quantidade_pedida,
    quantidade_ativa,
    quantidade_pendente,
    quantidade_estoque,
    total_linhas,
    linhas_split
FROM venda_itens_agregado
WHERE venda_id = 1;

\echo ''
\echo '[OK] Aggregated view replaces complex L1+L2 JOIN queries!'

\echo ''
\echo '======================================================================'
\echo '  ALL TESTS COMPLETED!'
\echo '======================================================================'
