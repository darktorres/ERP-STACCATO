<?php
/**
 * Next-Gen Schema Test Application
 * Tests the new unified venda_itens table that replaces L1/L2
 *
 * Usage: php test.php [setup|seed|test|all]
 */

error_reporting(E_ALL);

class NextGenSchemaTest
{
    private PDO $db;
    private array $config;

    public function __construct()
    {
        $this->config = require __DIR__ . '/config.php';
    }

    public function connect(): void
    {
        $dsn = sprintf(
            "pgsql:host=%s;port=%d;dbname=%s",
            $this->config['host'],
            $this->config['port'],
            $this->config['dbname']
        );

        try {
            $this->db = new PDO($dsn, $this->config['user'], $this->config['password'], [
                PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            ]);
            $this->success("Connected to PostgreSQL");
        } catch (PDOException $e) {
            $this->error("Connection failed: " . $e->getMessage());
            $this->info("Make sure to create database first: CREATE DATABASE {$this->config['dbname']};");
            exit(1);
        }
    }

    public function setup(): void
    {
        $this->header("Setting up schema");

        $sql = file_get_contents(__DIR__ . '/schema.sql');
        $this->db->exec($sql);
        $this->success("Schema created successfully");
    }

    public function seed(): void
    {
        $this->header("Seeding test data");

        // Loja
        $this->db->exec("INSERT INTO lojas (nome, cnpj) VALUES ('Loja Matriz', '12345678000199')");
        $lojaId = $this->db->lastInsertId();
        $this->info("Created loja: $lojaId");

        // Fornecedor
        $this->db->exec("INSERT INTO fornecedores (razao_social, cnpj) VALUES ('Ceramica ABC', '98765432000111')");
        $fornecedorId = $this->db->lastInsertId();
        $this->info("Created fornecedor: $fornecedorId");

        // Cliente
        $this->db->exec("INSERT INTO clientes (nome, cpf_cnpj) VALUES ('Joao Silva', '12345678901')");
        $clienteId = $this->db->lastInsertId();
        $this->info("Created cliente: $clienteId");

        // Produtos
        $stmt = $this->db->prepare("INSERT INTO produtos (fornecedor_id, codigo, descricao, unidade) VALUES (?, ?, ?, ?)");
        $stmt->execute([$fornecedorId, 'POR-60X60-BR', 'Porcelanato 60x60 Branco', 'M2']);
        $produto1Id = $this->db->lastInsertId();
        $stmt->execute([$fornecedorId, 'POR-60X60-CZ', 'Porcelanato 60x60 Cinza', 'M2']);
        $produto2Id = $this->db->lastInsertId();
        $this->info("Created produtos: $produto1Id, $produto2Id");

        // Estoques (multiple lots for testing selection)
        $stmt = $this->db->prepare("INSERT INTO estoques (loja_id, produto_id, fornecedor_id, quantidade_original, quantidade_disponivel, custo_unitario, lote, data_entrada) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
        $stmt->execute([$lojaId, $produto1Id, $fornecedorId, 100, 100, 35.00, 'LOTE-A1', '2025-01-01 10:00:00']);
        $estoque1Id = $this->db->lastInsertId();
        $stmt->execute([$lojaId, $produto1Id, $fornecedorId, 150, 150, 36.00, 'LOTE-A2', '2025-01-05 10:00:00']);
        $estoque2Id = $this->db->lastInsertId();
        $stmt->execute([$lojaId, $produto2Id, $fornecedorId, 80, 80, 40.00, 'LOTE-B1', '2025-01-03 10:00:00']);
        $estoque3Id = $this->db->lastInsertId();
        $this->info("Created estoques: $estoque1Id (100 M2), $estoque2Id (150 M2), $estoque3Id (80 M2)");

        $this->success("Seed data created");
    }

    public function test(): void
    {
        $this->testCreateVenda();
        $this->testAddVendaItem();
        $this->testSplit();
        $this->testEstoqueConsumo();
        $this->testEstorno();
        $this->testValidations();
        $this->testAggregatedView();
    }

    private function testCreateVenda(): void
    {
        $this->header("Test: Create Venda");

        $this->db->exec("INSERT INTO vendas (loja_id, cliente_id, total) VALUES (1, 1, 0)");
        $vendaId = $this->db->lastInsertId();

        $venda = $this->db->query("SELECT * FROM vendas WHERE id = $vendaId")->fetch();
        $this->assert($venda['status'] === 'ABERTA', "Status should be ABERTA");
        $this->success("Venda created with id: $vendaId");
    }

    private function testAddVendaItem(): void
    {
        $this->header("Test: Add Venda Item (single table, no L1/L2)");

        // Add item - notice: single INSERT, no trigger copying to L2!
        $stmt = $this->db->prepare("
            INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
            VALUES (1, 1, 1, 50, 45.00, 2250.00)
            RETURNING id
        ");
        $stmt->execute();
        $itemId = $stmt->fetchColumn();

        $item = $this->db->query("SELECT * FROM venda_itens WHERE id = $itemId")->fetch();

        $this->assert($item['status'] === 'PENDENTE', "Initial status should be PENDENTE");
        $this->assert($item['parent_id'] === null, "parent_id should be NULL for original item");
        $this->assert($item['root_id'] === null, "root_id should be NULL for original item");
        $this->assert((float)$item['quantidade'] === 50.0, "Quantidade should be 50");

        $this->success("Venda item added: id=$itemId, qty=50, status=PENDENTE");
    }

    private function testSplit(): void
    {
        $this->header("Test: Split Item (replaces idRelacionado pattern)");

        // Create a new item for split test
        $this->db->exec("
            INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
            VALUES (1, 1, 1, 100, 45.00, 4500.00)
        ");
        $originalId = $this->db->lastInsertId();
        $this->info("Created original item: id=$originalId, qty=100");

        // Simulate: NFe arrives with only 60 units
        $stmt = $this->db->prepare("SELECT split_venda_item(?, ?, ?)");
        $stmt->execute([$originalId, 60, 'PARTIAL_NFE']);
        $splitId = $stmt->fetchColumn();

        // Verify original was updated
        $original = $this->db->query("SELECT * FROM venda_itens WHERE id = $originalId")->fetch();
        $this->assert((float)$original['quantidade'] === 60.0, "Original should have 60 after split");
        $this->assert((float)$original['valor_total'] === 2700.0, "Original total should be recalculated");

        // Verify split was created
        $split = $this->db->query("SELECT * FROM venda_itens WHERE id = $splitId")->fetch();
        $this->assert((float)$split['quantidade'] === 40.0, "Split should have 40");
        $this->assert((int)$split['parent_id'] === (int)$originalId, "Split parent_id should point to original");
        $this->assert((int)$split['root_id'] === (int)$originalId, "Split root_id should point to original");
        $this->assert($split['split_reason'] === 'PARTIAL_NFE', "Split reason should be PARTIAL_NFE");

        $this->success("Split successful: original=$originalId (60), split=$splitId (40)");

        // Test cascading split
        $this->info("Testing cascading split...");
        $stmt->execute([$splitId, 25, 'PARTIAL_DELIVERY']);
        $cascadeSplitId = $stmt->fetchColumn();

        $cascadeSplit = $this->db->query("SELECT * FROM venda_itens WHERE id = $cascadeSplitId")->fetch();
        $this->assert((int)$cascadeSplit['parent_id'] === (int)$splitId, "Cascade parent should be the split");
        $this->assert((int)$cascadeSplit['root_id'] === (int)$originalId, "Cascade root should STILL be original!");

        $this->success("Cascading split: root_id correctly traces back to original");
    }

    private function testEstoqueConsumo(): void
    {
        $this->header("Test: Estoque Consumo (1:1 pairing)");

        // Create item for consumption test
        $this->db->exec("
            INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
            VALUES (1, 1, 1, 50, 45.00, 2250.00)
        ");
        $itemId = $this->db->lastInsertId();
        $this->info("Created item for consumption: id=$itemId, qty=50");

        // Get available stock
        $estoque = $this->db->query("
            SELECT id, lote, quantidade_disponivel, custo_unitario
            FROM estoques
            WHERE produto_id = 1 AND quantidade_disponivel >= 50
            ORDER BY data_entrada ASC
            LIMIT 1
        ")->fetch();
        $this->info("Selected stock: id={$estoque['id']}, lote={$estoque['lote']}, available={$estoque['quantidade_disponivel']}");

        $qtdAntes = (float)$estoque['quantidade_disponivel'];

        // Create consumption (pairing)
        $stmt = $this->db->prepare("
            INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
            VALUES (?, ?, 50, ?)
        ");
        $stmt->execute([$itemId, $estoque['id'], $estoque['custo_unitario']]);
        $consumoId = $this->db->lastInsertId();

        // Verify stock was updated (trigger)
        $estoqueAtualizado = $this->db->query("SELECT * FROM estoques WHERE id = {$estoque['id']}")->fetch();
        $qtdDepois = (float)$estoqueAtualizado['quantidade_disponivel'];
        $this->assert($qtdDepois === ($qtdAntes - 50), "Stock should decrease by 50");

        // Verify item status was updated (trigger)
        $item = $this->db->query("SELECT * FROM venda_itens WHERE id = $itemId")->fetch();
        $this->assert($item['status'] === 'ESTOQUE', "Item status should be ESTOQUE after consumption");

        $this->success("Consumption created: consumo=$consumoId, stock decreased from $qtdAntes to $qtdDepois");
    }

    private function testEstorno(): void
    {
        $this->header("Test: Estorno (reversal)");

        // Get the last consumption
        $consumo = $this->db->query("
            SELECT ec.*, e.quantidade_disponivel as estoque_disponivel
            FROM estoque_consumos ec
            JOIN estoques e ON e.id = ec.estoque_id
            WHERE NOT ec.is_estornado
            ORDER BY ec.id DESC LIMIT 1
        ")->fetch();

        if (!$consumo) {
            $this->warning("No active consumption to reverse");
            return;
        }

        $qtdAntes = (float)$consumo['estoque_disponivel'];
        $this->info("Reversing consumption: id={$consumo['id']}, qty={$consumo['quantidade']}");

        // Perform reversal
        $stmt = $this->db->prepare("
            UPDATE estoque_consumos
            SET is_estornado = TRUE, estornado_em = NOW(), estorno_motivo = ?
            WHERE id = ?
        ");
        $stmt->execute(['Teste de estorno', $consumo['id']]);

        // Verify stock was restored
        $estoque = $this->db->query("SELECT * FROM estoques WHERE id = {$consumo['estoque_id']}")->fetch();
        $qtdDepois = (float)$estoque['quantidade_disponivel'];
        $expectedQtd = $qtdAntes + (float)$consumo['quantidade'];
        $this->assert($qtdDepois === $expectedQtd, "Stock should be restored to $expectedQtd");

        $this->success("Reversal successful: stock restored from $qtdAntes to $qtdDepois");
    }

    private function testValidations(): void
    {
        $this->header("Test: Validations (constraints and triggers)");

        // Test 1: Product mismatch
        $this->info("Testing product mismatch validation...");
        try {
            $this->db->exec("
                INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
                VALUES (1, 1, 1, 30, 45.00, 1350.00)
            ");
            $itemId = $this->db->lastInsertId();

            // Try to pair with stock of different product
            $estoque = $this->db->query("SELECT id FROM estoques WHERE produto_id = 2 LIMIT 1")->fetch();
            $this->db->exec("
                INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                VALUES ($itemId, {$estoque['id']}, 30, 40.00)
            ");
            $this->error("Should have thrown product mismatch error!");
        } catch (PDOException $e) {
            $this->success("Correctly rejected: " . substr($e->getMessage(), 0, 80));
        }

        // Test 2: Quantity mismatch
        $this->info("Testing quantity mismatch validation...");
        try {
            $this->db->exec("
                INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
                VALUES (1, 1, 1, 30, 45.00, 1350.00)
            ");
            $itemId = $this->db->lastInsertId();

            $estoque = $this->db->query("SELECT id FROM estoques WHERE produto_id = 1 AND quantidade_disponivel >= 30 LIMIT 1")->fetch();
            // Try to consume different quantity
            $this->db->exec("
                INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                VALUES ($itemId, {$estoque['id']}, 25, 35.00)
            ");
            $this->error("Should have thrown quantity mismatch error!");
        } catch (PDOException $e) {
            $this->success("Correctly rejected: " . substr($e->getMessage(), 0, 80));
        }

        // Test 3: 1:1 constraint (double pairing)
        $this->info("Testing 1:1 constraint...");
        try {
            $this->db->exec("
                INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
                VALUES (1, 1, 1, 20, 45.00, 900.00)
            ");
            $itemId = $this->db->lastInsertId();

            $estoque = $this->db->query("SELECT id, custo_unitario FROM estoques WHERE produto_id = 1 AND quantidade_disponivel >= 20 LIMIT 1")->fetch();

            // First pairing - should work
            $this->db->exec("
                INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                VALUES ($itemId, {$estoque['id']}, 20, {$estoque['custo_unitario']})
            ");
            $this->success("First pairing successful");

            // Create another item
            $this->db->exec("
                INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
                VALUES (1, 1, 1, 20, 45.00, 900.00)
            ");
            $itemId2 = $this->db->lastInsertId();

            // Try to pair with same stock - should fail
            $this->db->exec("
                INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                VALUES ($itemId2, {$estoque['id']}, 20, {$estoque['custo_unitario']})
            ");
            $this->error("Should have thrown 1:1 constraint error!");
        } catch (PDOException $e) {
            $this->success("Correctly rejected double pairing: unique constraint");
        }

        // Test 4: Insufficient stock
        $this->info("Testing insufficient stock validation...");
        try {
            $this->db->exec("
                INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
                VALUES (1, 1, 1, 9999, 45.00, 449955.00)
            ");
            $itemId = $this->db->lastInsertId();

            $estoque = $this->db->query("SELECT id, custo_unitario FROM estoques WHERE produto_id = 1 LIMIT 1")->fetch();
            $this->db->exec("
                INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                VALUES ($itemId, {$estoque['id']}, 9999, {$estoque['custo_unitario']})
            ");
            $this->error("Should have thrown insufficient stock error!");
        } catch (PDOException $e) {
            $this->success("Correctly rejected: " . substr($e->getMessage(), 0, 80));
        }
    }

    private function testAggregatedView(): void
    {
        $this->header("Test: Aggregated View (replaces L1 queries)");

        $rows = $this->db->query("SELECT * FROM venda_itens_agregado WHERE venda_id = 1")->fetchAll();

        $this->info("Aggregated view results:");
        echo str_repeat("-", 100) . "\n";
        printf("%-10s %-10s %-15s %-15s %-15s %-15s %-10s\n",
            "venda_id", "produto", "qtd_pedida", "qtd_ativa", "qtd_pendente", "qtd_estoque", "splits");
        echo str_repeat("-", 100) . "\n";

        foreach ($rows as $row) {
            printf("%-10s %-10s %-15s %-15s %-15s %-15s %-10s\n",
                $row['venda_id'],
                $row['produto_id'],
                $row['quantidade_pedida'] ?? 'N/A',
                $row['quantidade_ativa'] ?? 'N/A',
                $row['quantidade_pendente'] ?? 'N/A',
                $row['quantidade_estoque'] ?? 'N/A',
                $row['linhas_split']
            );
        }

        $this->success("Aggregated view working - replaces complex L1+L2 JOINs");
    }

    // Output helpers
    private function header(string $text): void
    {
        echo "\n" . str_repeat("=", 70) . "\n";
        echo "  $text\n";
        echo str_repeat("=", 70) . "\n";
    }

    private function success(string $text): void
    {
        echo "[OK] $text\n";
    }

    private function error(string $text): void
    {
        echo "[ERROR] $text\n";
    }

    private function warning(string $text): void
    {
        echo "[WARN] $text\n";
    }

    private function info(string $text): void
    {
        echo "[INFO] $text\n";
    }

    private function assert(bool $condition, string $message): void
    {
        if (!$condition) {
            throw new Exception("Assertion failed: $message");
        }
    }
}

// Main
$test = new NextGenSchemaTest();
$test->connect();

$command = $argv[1] ?? 'all';

switch ($command) {
    case 'setup':
        $test->setup();
        break;
    case 'seed':
        $test->seed();
        break;
    case 'test':
        $test->test();
        break;
    case 'all':
    default:
        $test->setup();
        $test->seed();
        $test->test();
        break;
}

echo "\n[DONE] All tests completed!\n";
