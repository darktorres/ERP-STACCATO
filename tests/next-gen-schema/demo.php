<?php
/**
 * Interactive Demo: Next-Gen Schema
 * Tests the unified venda_itens table that replaces L1/L2
 */

error_reporting(E_ALL);

class InteractiveDemo
{
    private PDO $db;
    private array $config;

    // ANSI colors
    const GREEN = "\033[32m";
    const RED = "\033[31m";
    const YELLOW = "\033[33m";
    const CYAN = "\033[36m";
    const BOLD = "\033[1m";
    const RESET = "\033[0m";

    public function __construct()
    {
        $this->config = require __DIR__ . '/config.php';
    }

    public function run(): void
    {
        $this->banner();
        $this->connect();
        $this->setupSchema();
        $this->seedData();

        while (true) {
            $this->showMenu();
            $choice = $this->prompt("Choose option");

            switch ($choice) {
                case '1': $this->showVendas(); break;
                case '2': $this->showVendaItens(); break;
                case '3': $this->showEstoques(); break;
                case '4': $this->createVenda(); break;
                case '5': $this->addItemToVenda(); break;
                case '6': $this->splitItem(); break;
                case '7': $this->pairItemWithStock(); break;
                case '8': $this->reverseConsumption(); break;
                case '9': $this->showAggregatedView(); break;
                case '10': $this->showSplitHierarchy(); break;
                case '0':
                case 'q':
                    $this->println("\nBye!", self::CYAN);
                    exit(0);
                default:
                    $this->println("Invalid option", self::RED);
            }

            $this->prompt("\nPress ENTER to continue...");
        }
    }

    private function banner(): void
    {
        echo self::BOLD . self::CYAN;
        echo "\n";
        echo "╔══════════════════════════════════════════════════════════════╗\n";
        echo "║          NEXT-GEN SCHEMA - INTERACTIVE DEMO                  ║\n";
        echo "║                                                              ║\n";
        echo "║   Tests the unified venda_itens table that replaces L1/L2   ║\n";
        echo "╚══════════════════════════════════════════════════════════════╝\n";
        echo self::RESET . "\n";
    }

    private function showMenu(): void
    {
        echo "\n" . self::BOLD . "═══ MENU ═══" . self::RESET . "\n\n";
        echo self::CYAN . " [VIEW DATA]" . self::RESET . "\n";
        echo "  1. Show Vendas\n";
        echo "  2. Show Venda Itens\n";
        echo "  3. Show Estoques\n";
        echo "\n" . self::CYAN . " [OPERATIONS]" . self::RESET . "\n";
        echo "  4. Create new Venda\n";
        echo "  5. Add Item to Venda\n";
        echo "  6. Split Item (simulates partial NFe)\n";
        echo "  7. Pair Item with Stock (1:1 consumption)\n";
        echo "  8. Reverse Consumption (estorno)\n";
        echo "\n" . self::CYAN . " [ANALYSIS]" . self::RESET . "\n";
        echo "  9. Show Aggregated View (replaces L1)\n";
        echo " 10. Show Split Hierarchy (replaces idRelacionado)\n";
        echo "\n  0/q. Exit\n";
    }

    private function connect(): void
    {
        $dsn = sprintf(
            "pgsql:host=%s;port=%d;dbname=%s",
            $this->config['host'],
            $this->config['port'],
            $this->config['dbname']
        );

        $maxRetries = 10;
        for ($i = 0; $i < $maxRetries; $i++) {
            try {
                $this->db = new PDO($dsn, $this->config['user'], $this->config['password'], [
                    PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
                    PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
                ]);
                $this->println("Connected to PostgreSQL", self::GREEN);
                return;
            } catch (PDOException $e) {
                if ($i < $maxRetries - 1) {
                    echo "Waiting for database... (" . ($i + 1) . "/$maxRetries)\n";
                    sleep(2);
                } else {
                    $this->println("Connection failed: " . $e->getMessage(), self::RED);
                    exit(1);
                }
            }
        }
    }

    private function setupSchema(): void
    {
        $this->println("Setting up schema...", self::YELLOW);
        $sql = file_get_contents(__DIR__ . '/schema.sql');
        $this->db->exec($sql);
        $this->println("Schema ready", self::GREEN);
    }

    private function seedData(): void
    {
        $this->println("Seeding test data...", self::YELLOW);

        $this->db->exec("INSERT INTO lojas (nome, cnpj) VALUES ('Loja Matriz', '12345678000199')");
        $this->db->exec("INSERT INTO fornecedores (razao_social, cnpj) VALUES ('Ceramica ABC', '98765432000111')");
        $this->db->exec("INSERT INTO clientes (nome, cpf_cnpj) VALUES ('Joao Silva', '12345678901')");
        $this->db->exec("INSERT INTO clientes (nome, cpf_cnpj) VALUES ('Maria Santos', '98765432109')");

        $this->db->exec("INSERT INTO produtos (fornecedor_id, codigo, descricao, unidade) VALUES
            (1, 'POR-60X60-BR', 'Porcelanato 60x60 Branco', 'M2'),
            (1, 'POR-60X60-CZ', 'Porcelanato 60x60 Cinza', 'M2'),
            (1, 'CER-30X30-BG', 'Ceramica 30x30 Bege', 'M2')");

        $this->db->exec("INSERT INTO estoques (loja_id, produto_id, fornecedor_id, quantidade_original, quantidade_disponivel, custo_unitario, lote, data_entrada) VALUES
            (1, 1, 1, 100, 100, 35.00, 'LOTE-A1', '2025-01-01 10:00:00'),
            (1, 1, 1, 80, 80, 36.00, 'LOTE-A2', '2025-01-05 10:00:00'),
            (1, 2, 1, 120, 120, 38.00, 'LOTE-B1', '2025-01-03 10:00:00'),
            (1, 3, 1, 200, 200, 25.00, 'LOTE-C1', '2025-01-02 10:00:00')");

        $this->println("Test data ready", self::GREEN);
    }

    // =========================================================================
    // VIEW DATA
    // =========================================================================

    private function showVendas(): void
    {
        $this->header("VENDAS");
        $rows = $this->db->query("
            SELECT v.id, c.nome as cliente, v.status, v.total,
                   to_char(v.created_at, 'DD/MM HH24:MI') as criado
            FROM vendas v
            JOIN clientes c ON c.id = v.cliente_id
            ORDER BY v.id
        ")->fetchAll();

        if (empty($rows)) {
            $this->println("No vendas yet. Create one!", self::YELLOW);
            return;
        }

        $this->table(['ID', 'Cliente', 'Status', 'Total', 'Criado'], $rows);
    }

    private function showVendaItens(): void
    {
        $this->header("VENDA ITENS (Single Table - No L1/L2!)");

        $vendaId = $this->prompt("Venda ID (or ENTER for all)");

        $sql = "
            SELECT vi.id, vi.venda_id, p.codigo as produto,
                   vi.quantidade as qty, vi.status,
                   CASE WHEN vi.parent_id IS NULL THEN '-' ELSE vi.parent_id::text END as parent,
                   CASE WHEN vi.root_id IS NULL THEN '-' ELSE vi.root_id::text END as root,
                   COALESCE(vi.split_reason, '-') as split_reason
            FROM venda_itens vi
            JOIN produtos p ON p.id = vi.produto_id
        ";

        if ($vendaId) {
            $sql .= " WHERE vi.venda_id = " . (int)$vendaId;
        }
        $sql .= " ORDER BY vi.venda_id, vi.id";

        $rows = $this->db->query($sql)->fetchAll();

        if (empty($rows)) {
            $this->println("No items found", self::YELLOW);
            return;
        }

        $this->table(['ID', 'Venda', 'Produto', 'Qty', 'Status', 'Parent', 'Root', 'Split Reason'], $rows);

        $this->println("\n→ parent_id/root_id replace the old idRelacionado pattern", self::CYAN);
        $this->println("→ root_id ALWAYS points to original item (no recursive queries!)", self::CYAN);
    }

    private function showEstoques(): void
    {
        $this->header("ESTOQUES");
        $rows = $this->db->query("
            SELECT e.id, p.codigo as produto, e.lote,
                   e.quantidade_original as original,
                   e.quantidade_disponivel as disponivel,
                   e.custo_unitario as custo,
                   e.status
            FROM estoques e
            JOIN produtos p ON p.id = e.produto_id
            ORDER BY e.produto_id, e.data_entrada
        ")->fetchAll();

        $this->table(['ID', 'Produto', 'Lote', 'Original', 'Disponivel', 'Custo', 'Status'], $rows);
    }

    // =========================================================================
    // OPERATIONS
    // =========================================================================

    private function createVenda(): void
    {
        $this->header("CREATE VENDA");

        $clientes = $this->db->query("SELECT id, nome FROM clientes")->fetchAll();
        echo "Clientes:\n";
        foreach ($clientes as $c) {
            echo "  {$c['id']}. {$c['nome']}\n";
        }

        $clienteId = $this->prompt("Cliente ID");
        if (!$clienteId) return;

        $stmt = $this->db->prepare("INSERT INTO vendas (loja_id, cliente_id) VALUES (1, ?) RETURNING id, status");
        $stmt->execute([$clienteId]);
        $venda = $stmt->fetch();

        $this->println("Venda #{$venda['id']} created with status '{$venda['status']}' (ENUM!)", self::GREEN);
    }

    private function addItemToVenda(): void
    {
        $this->header("ADD ITEM TO VENDA");

        $vendaId = $this->prompt("Venda ID");
        if (!$vendaId) return;

        $produtos = $this->db->query("SELECT id, codigo, descricao FROM produtos")->fetchAll();
        echo "Produtos:\n";
        foreach ($produtos as $p) {
            echo "  {$p['id']}. {$p['codigo']} - {$p['descricao']}\n";
        }

        $produtoId = $this->prompt("Produto ID");
        $quantidade = $this->prompt("Quantidade");
        $valorUnit = $this->prompt("Valor Unitario (ex: 45.00)");

        if (!$produtoId || !$quantidade || !$valorUnit) return;

        $total = (float)$quantidade * (float)$valorUnit;

        $stmt = $this->db->prepare("
            INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
            VALUES (?, ?, 1, ?, ?, ?)
            RETURNING id, status
        ");
        $stmt->execute([$vendaId, $produtoId, $quantidade, $valorUnit, $total]);
        $item = $stmt->fetch();

        $this->println("Item #{$item['id']} added with status '{$item['status']}'", self::GREEN);
        $this->println("→ Single INSERT - no trigger copying to L2!", self::CYAN);
    }

    private function splitItem(): void
    {
        $this->header("SPLIT ITEM");
        $this->println("Simulates: NFe arrives with less quantity than ordered", self::YELLOW);

        $itemId = $this->prompt("Item ID to split");
        if (!$itemId) return;

        $item = $this->db->query("SELECT * FROM venda_itens WHERE id = $itemId")->fetch();
        if (!$item) {
            $this->println("Item not found!", self::RED);
            return;
        }

        $this->println("Current quantity: {$item['quantidade']}", self::CYAN);
        $keepQty = $this->prompt("Quantity to KEEP (rest goes to split)");
        if (!$keepQty) return;

        echo "Split reasons:\n";
        echo "  1. PARTIAL_NFE (NFe with less quantity)\n";
        echo "  2. PARTIAL_DELIVERY (partial delivery)\n";
        echo "  3. BROKEN (damaged items)\n";
        $reasonChoice = $this->prompt("Reason (1-3)");

        $reasons = ['1' => 'PARTIAL_NFE', '2' => 'PARTIAL_DELIVERY', '3' => 'BROKEN'];
        $reason = $reasons[$reasonChoice] ?? 'PARTIAL_NFE';

        try {
            $stmt = $this->db->prepare("SELECT split_venda_item(?, ?, ?)");
            $stmt->execute([$itemId, $keepQty, $reason]);
            $splitId = $stmt->fetchColumn();

            $this->println("Split created! New item ID: $splitId", self::GREEN);

            // Show result
            $rows = $this->db->query("
                SELECT id, quantidade, parent_id, root_id, split_reason, status
                FROM venda_itens
                WHERE id IN ($itemId, $splitId)
                ORDER BY id
            ")->fetchAll();

            echo "\nResult:\n";
            $this->table(['ID', 'Qty', 'Parent', 'Root', 'Reason', 'Status'], $rows);

            $this->println("\n→ root_id always points to original (even in cascading splits!)", self::CYAN);

        } catch (PDOException $e) {
            $this->println("Error: " . $e->getMessage(), self::RED);
        }
    }

    private function pairItemWithStock(): void
    {
        $this->header("PAIR ITEM WITH STOCK (1:1 Consumption)");

        $itemId = $this->prompt("Item ID to pair");
        if (!$itemId) return;

        $item = $this->db->query("SELECT * FROM venda_itens WHERE id = $itemId")->fetch();
        if (!$item) {
            $this->println("Item not found!", self::RED);
            return;
        }

        $this->println("Item quantity: {$item['quantidade']}", self::CYAN);
        $this->println("Item product_id: {$item['produto_id']}", self::CYAN);

        // Show available stocks for this product
        $stocks = $this->db->query("
            SELECT e.id, e.lote, e.quantidade_disponivel as disponivel, e.custo_unitario as custo
            FROM estoques e
            WHERE e.produto_id = {$item['produto_id']}
              AND e.quantidade_disponivel > 0
              AND e.id NOT IN (SELECT estoque_id FROM estoque_consumos WHERE NOT is_estornado)
            ORDER BY e.data_entrada
        ")->fetchAll();

        if (empty($stocks)) {
            $this->println("No available stock for this product!", self::RED);
            return;
        }

        echo "\nAvailable stocks (FIFO order):\n";
        $this->table(['ID', 'Lote', 'Disponivel', 'Custo'], $stocks);

        $estoqueId = $this->prompt("Stock ID to use");
        if (!$estoqueId) return;

        $estoque = $this->db->query("SELECT * FROM estoques WHERE id = $estoqueId")->fetch();

        try {
            $stmt = $this->db->prepare("
                INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                VALUES (?, ?, ?, ?)
            ");
            $stmt->execute([$itemId, $estoqueId, $item['quantidade'], $estoque['custo_unitario']]);

            $this->println("Pairing successful!", self::GREEN);

            // Show updated data
            $updatedItem = $this->db->query("SELECT status FROM venda_itens WHERE id = $itemId")->fetch();
            $updatedStock = $this->db->query("SELECT quantidade_disponivel, status FROM estoques WHERE id = $estoqueId")->fetch();

            $this->println("→ Item status auto-updated to: {$updatedItem['status']} (trigger!)", self::CYAN);
            $this->println("→ Stock disponivel now: {$updatedStock['quantidade_disponivel']}, status: {$updatedStock['status']}", self::CYAN);
            $this->println("→ 1:1 constraint: this stock cannot be paired with another item", self::CYAN);

        } catch (PDOException $e) {
            $this->println("Error: " . $e->getMessage(), self::RED);
            $this->println("→ Validation trigger rejected the pairing", self::YELLOW);
        }
    }

    private function reverseConsumption(): void
    {
        $this->header("REVERSE CONSUMPTION (Estorno)");

        $consumos = $this->db->query("
            SELECT ec.id, ec.venda_item_id as item, ec.estoque_id as estoque,
                   ec.quantidade as qty, e.lote
            FROM estoque_consumos ec
            JOIN estoques e ON e.id = ec.estoque_id
            WHERE NOT ec.is_estornado
        ")->fetchAll();

        if (empty($consumos)) {
            $this->println("No active consumptions to reverse", self::YELLOW);
            return;
        }

        echo "Active consumptions:\n";
        $this->table(['ID', 'Item', 'Estoque', 'Qty', 'Lote'], $consumos);

        $consumoId = $this->prompt("Consumption ID to reverse");
        if (!$consumoId) return;

        $motivo = $this->prompt("Reason for reversal");

        $consumo = $this->db->query("SELECT * FROM estoque_consumos WHERE id = $consumoId")->fetch();
        $stockBefore = $this->db->query("SELECT quantidade_disponivel FROM estoques WHERE id = {$consumo['estoque_id']}")->fetchColumn();

        $stmt = $this->db->prepare("
            UPDATE estoque_consumos
            SET is_estornado = TRUE, estornado_em = NOW(), estorno_motivo = ?
            WHERE id = ?
        ");
        $stmt->execute([$motivo, $consumoId]);

        $stockAfter = $this->db->query("SELECT quantidade_disponivel FROM estoques WHERE id = {$consumo['estoque_id']}")->fetchColumn();

        $this->println("Reversal successful!", self::GREEN);
        $this->println("→ Stock restored: $stockBefore → $stockAfter (trigger!)", self::CYAN);
        $this->println("→ Consumption record preserved with is_estornado=TRUE (audit trail)", self::CYAN);
    }

    // =========================================================================
    // ANALYSIS
    // =========================================================================

    private function showAggregatedView(): void
    {
        $this->header("AGGREGATED VIEW (Replaces L1 Queries)");

        $rows = $this->db->query("
            SELECT
                venda_id as venda,
                produto_id as produto,
                quantidade_pedida as pedida,
                quantidade_ativa as ativa,
                quantidade_pendente as pendente,
                quantidade_estoque as estoque,
                total_linhas as linhas,
                linhas_split as splits
            FROM venda_itens_agregado
            ORDER BY venda_id, produto_id
        ")->fetchAll();

        if (empty($rows)) {
            $this->println("No data yet", self::YELLOW);
            return;
        }

        $this->table(['Venda', 'Produto', 'Pedida', 'Ativa', 'Pendente', 'Estoque', 'Linhas', 'Splits'], $rows);

        $this->println("\n→ This VIEW replaces complex L1+L2 JOIN queries!", self::CYAN);
        $this->println("→ Aggregates all splits back to original order quantities", self::CYAN);
    }

    private function showSplitHierarchy(): void
    {
        $this->header("SPLIT HIERARCHY");

        $itemId = $this->prompt("Original Item ID (root)");
        if (!$itemId) return;

        $rows = $this->db->query("
            SELECT
                vi.id,
                vi.quantidade as qty,
                vi.status,
                CASE WHEN vi.parent_id IS NULL THEN '(original)' ELSE '→ parent=' || vi.parent_id END as hierarchy,
                COALESCE(vi.split_reason, '-') as reason
            FROM venda_itens vi
            WHERE vi.id = $itemId OR vi.root_id = $itemId
            ORDER BY vi.id
        ")->fetchAll();

        if (empty($rows)) {
            $this->println("Item not found or has no splits", self::YELLOW);
            return;
        }

        $this->table(['ID', 'Qty', 'Status', 'Hierarchy', 'Reason'], $rows);

        $this->println("\n→ Simple query: WHERE id = X OR root_id = X", self::CYAN);
        $this->println("→ No recursive CTE needed (unlike idRelacionado chains!)", self::CYAN);
    }

    // =========================================================================
    // HELPERS
    // =========================================================================

    private function prompt(string $message): string
    {
        echo self::BOLD . "$message: " . self::RESET;
        return trim(fgets(STDIN));
    }

    private function println(string $text, string $color = ''): void
    {
        echo $color . $text . self::RESET . "\n";
    }

    private function header(string $text): void
    {
        echo "\n" . self::BOLD . self::CYAN;
        echo "╔" . str_repeat("═", strlen($text) + 2) . "╗\n";
        echo "║ $text ║\n";
        echo "╚" . str_repeat("═", strlen($text) + 2) . "╝\n";
        echo self::RESET;
    }

    private function table(array $headers, array $rows): void
    {
        if (empty($rows)) return;

        // Calculate column widths
        $widths = [];
        foreach ($headers as $i => $h) {
            $widths[$i] = strlen($h);
        }
        foreach ($rows as $row) {
            $i = 0;
            foreach ($row as $val) {
                $widths[$i] = max($widths[$i] ?? 0, strlen((string)$val));
                $i++;
            }
        }

        // Print header
        echo self::BOLD;
        foreach ($headers as $i => $h) {
            printf(" %-{$widths[$i]}s ", $h);
        }
        echo self::RESET . "\n";

        // Separator
        foreach ($widths as $w) {
            echo str_repeat("─", $w + 2);
        }
        echo "\n";

        // Rows
        foreach ($rows as $row) {
            $i = 0;
            foreach ($row as $val) {
                printf(" %-{$widths[$i]}s ", $val);
                $i++;
            }
            echo "\n";
        }
    }
}

// Run
$demo = new InteractiveDemo();
$demo->run();
