<?php
/**
 * Demo do Novo Schema
 * Aplicação web interativa para testar a tabela unificada venda_itens
 */

session_start();
error_reporting(E_ALL);

$config = [
    'host' => 'postgres',
    'port' => 5432,
    'dbname' => 'erp_nextgen_test',
    'user' => 'postgres',
    'password' => 'postgres',
];

// Conexão com banco de dados
function getDb(): PDO {
    global $config;
    static $db = null;
    if ($db === null) {
        $dsn = sprintf("pgsql:host=%s;port=%d;dbname=%s", $config['host'], $config['port'], $config['dbname']);
        $db = new PDO($dsn, $config['user'], $config['password'], [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        ]);
    }
    return $db;
}

// Inicializar banco na primeira execução
function initDatabase(): void {
    $db = getDb();
    $check = $db->query("SELECT to_regclass('public.venda_itens')")->fetchColumn();
    if (!$check) {
        $schema = file_get_contents(__DIR__ . '/../schema.sql');
        $db->exec($schema);

        // Dados iniciais
        $db->exec("INSERT INTO lojas (nome, cnpj) VALUES ('Loja Matriz', '12345678000199')");
        $db->exec("INSERT INTO fornecedores (razao_social, cnpj) VALUES ('Ceramica ABC', '98765432000111')");
        $db->exec("INSERT INTO clientes (nome, cpf_cnpj) VALUES ('João Silva', '12345678901')");
        $db->exec("INSERT INTO clientes (nome, cpf_cnpj) VALUES ('Maria Santos', '98765432109')");
        $db->exec("INSERT INTO produtos (fornecedor_id, codigo, descricao, unidade) VALUES
            (1, 'POR-60X60-BR', 'Porcelanato 60x60 Branco', 'M2'),
            (1, 'POR-60X60-CZ', 'Porcelanato 60x60 Cinza', 'M2'),
            (1, 'CER-30X30-BG', 'Cerâmica 30x30 Bege', 'M2')");
        $db->exec("INSERT INTO estoques (loja_id, produto_id, fornecedor_id, quantidade_original, quantidade_disponivel, custo_unitario, lote, data_entrada) VALUES
            (1, 1, 1, 100, 100, 35.00, 'LOTE-A1', '2025-01-01'),
            (1, 1, 1, 80, 80, 36.00, 'LOTE-A2', '2025-01-05'),
            (1, 2, 1, 120, 120, 38.00, 'LOTE-B1', '2025-01-03'),
            (1, 3, 1, 200, 200, 25.00, 'LOTE-C1', '2025-01-02')");
    }
}

// Processar ações AJAX
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_POST['action'])) {
    header('Content-Type: application/json');

    try {
        initDatabase();
        $db = getDb();
        $result = ['success' => true];

        switch ($_POST['action']) {
            case 'create_venda':
                $stmt = $db->prepare("INSERT INTO vendas (loja_id, cliente_id) VALUES (1, ?) RETURNING id, status");
                $stmt->execute([$_POST['cliente_id']]);
                $result['data'] = $stmt->fetch();
                $result['message'] = "Venda #{$result['data']['id']} criada com status '{$result['data']['status']}' (ENUM!)";
                break;

            case 'add_item':
                $total = (float)$_POST['quantidade'] * (float)$_POST['valor_unitario'];
                $stmt = $db->prepare("
                    INSERT INTO venda_itens (venda_id, produto_id, fornecedor_id, quantidade, valor_unitario, valor_total)
                    VALUES (?, ?, 1, ?, ?, ?)
                    RETURNING id, status
                ");
                $stmt->execute([$_POST['venda_id'], $_POST['produto_id'], $_POST['quantidade'], $_POST['valor_unitario'], $total]);
                $result['data'] = $stmt->fetch();
                $result['message'] = "Item #{$result['data']['id']} adicionado - INSERT único, sem trigger L2!";
                break;

            case 'split_item':
                $stmt = $db->prepare("SELECT split_venda_item(?, ?, ?)");
                $stmt->execute([$_POST['item_id'], $_POST['quantidade_manter'], $_POST['split_reason']]);
                $splitId = $stmt->fetchColumn();

                $split = $db->query("SELECT * FROM venda_itens WHERE id = $splitId")->fetch();
                $result['data'] = $split;
                $result['message'] = "Divisão criada! ID: $splitId, parent_id: {$split['parent_id']}, root_id: {$split['root_id']}";
                break;

            case 'pair_stock':
                $item = $db->query("SELECT * FROM venda_itens WHERE id = {$_POST['item_id']}")->fetch();
                $estoque = $db->query("SELECT * FROM estoques WHERE id = {$_POST['estoque_id']}")->fetch();

                $stmt = $db->prepare("
                    INSERT INTO estoque_consumos (venda_item_id, estoque_id, quantidade, custo_unitario)
                    VALUES (?, ?, ?, ?)
                ");
                $stmt->execute([$_POST['item_id'], $_POST['estoque_id'], $item['quantidade'], $estoque['custo_unitario']]);

                $updatedItem = $db->query("SELECT status FROM venda_itens WHERE id = {$_POST['item_id']}")->fetch();
                $updatedStock = $db->query("SELECT quantidade_disponivel FROM estoques WHERE id = {$_POST['estoque_id']}")->fetch();

                $result['message'] = "Pareamento criado! Status do item: {$updatedItem['status']}, Estoque disponível: {$updatedStock['quantidade_disponivel']}";
                break;

            case 'reverse_consumption':
                $stmt = $db->prepare("
                    UPDATE estoque_consumos
                    SET is_estornado = TRUE, estornado_em = NOW(), estorno_motivo = ?
                    WHERE id = ? AND NOT is_estornado
                ");
                $stmt->execute([$_POST['motivo'], $_POST['consumo_id']]);
                $result['message'] = "Estorno realizado! Estoque restaurado.";
                break;

            case 'get_vendas':
                $result['data'] = $db->query("
                    SELECT v.id, c.nome as cliente, v.status, v.total, v.created_at
                    FROM vendas v JOIN clientes c ON c.id = v.cliente_id
                    ORDER BY v.id DESC
                ")->fetchAll();
                break;

            case 'get_items':
                $vendaId = (int)$_POST['venda_id'];
                $result['data'] = $db->query("
                    SELECT vi.*, p.codigo as produto_codigo, p.descricao as produto_nome
                    FROM venda_itens vi
                    JOIN produtos p ON p.id = vi.produto_id
                    WHERE vi.venda_id = $vendaId
                    ORDER BY vi.id
                ")->fetchAll();
                break;

            case 'get_estoques':
                $produtoId = isset($_POST['produto_id']) ? (int)$_POST['produto_id'] : 0;
                $where = $produtoId ? "WHERE e.produto_id = $produtoId" : "";
                $result['data'] = $db->query("
                    SELECT e.*, p.codigo as produto_codigo
                    FROM estoques e
                    JOIN produtos p ON p.id = e.produto_id
                    $where
                    ORDER BY e.produto_id, e.data_entrada
                ")->fetchAll();
                break;

            case 'get_consumos':
                $result['data'] = $db->query("
                    SELECT ec.*, e.lote, vi.id as item_id
                    FROM estoque_consumos ec
                    JOIN estoques e ON e.id = ec.estoque_id
                    JOIN venda_itens vi ON vi.id = ec.venda_item_id
                    WHERE NOT ec.is_estornado
                    ORDER BY ec.id DESC
                ")->fetchAll();
                break;

            case 'get_hierarchy':
                $itemId = (int)$_POST['item_id'];
                $result['data'] = $db->query("
                    SELECT vi.*, p.codigo as produto_codigo
                    FROM venda_itens vi
                    JOIN produtos p ON p.id = vi.produto_id
                    WHERE vi.id = $itemId OR vi.root_id = $itemId
                    ORDER BY vi.id
                ")->fetchAll();
                break;

            case 'get_aggregated':
                $result['data'] = $db->query("SELECT * FROM venda_itens_agregado ORDER BY venda_id, produto_id")->fetchAll();
                break;

            case 'get_clientes':
                $result['data'] = $db->query("SELECT id, nome FROM clientes")->fetchAll();
                break;

            case 'get_produtos':
                $result['data'] = $db->query("SELECT id, codigo, descricao FROM produtos")->fetchAll();
                break;

            case 'get_available_stocks':
                $produtoId = (int)$_POST['produto_id'];
                $result['data'] = $db->query("
                    SELECT e.id, e.lote, e.quantidade_disponivel, e.custo_unitario
                    FROM estoques e
                    WHERE e.produto_id = $produtoId
                      AND e.quantidade_disponivel > 0
                      AND e.id NOT IN (SELECT estoque_id FROM estoque_consumos WHERE NOT is_estornado)
                    ORDER BY e.data_entrada
                ")->fetchAll();
                break;

            case 'reset_database':
                $schema = file_get_contents(__DIR__ . '/../schema.sql');
                $db->exec($schema);
                $db->exec("INSERT INTO lojas (nome, cnpj) VALUES ('Loja Matriz', '12345678000199')");
                $db->exec("INSERT INTO fornecedores (razao_social, cnpj) VALUES ('Ceramica ABC', '98765432000111')");
                $db->exec("INSERT INTO clientes (nome, cpf_cnpj) VALUES ('João Silva', '12345678901'), ('Maria Santos', '98765432109')");
                $db->exec("INSERT INTO produtos (fornecedor_id, codigo, descricao, unidade) VALUES
                    (1, 'POR-60X60-BR', 'Porcelanato 60x60 Branco', 'M2'),
                    (1, 'POR-60X60-CZ', 'Porcelanato 60x60 Cinza', 'M2'),
                    (1, 'CER-30X30-BG', 'Cerâmica 30x30 Bege', 'M2')");
                $db->exec("INSERT INTO estoques (loja_id, produto_id, fornecedor_id, quantidade_original, quantidade_disponivel, custo_unitario, lote, data_entrada) VALUES
                    (1, 1, 1, 100, 100, 35.00, 'LOTE-A1', '2025-01-01'),
                    (1, 1, 1, 80, 80, 36.00, 'LOTE-A2', '2025-01-05'),
                    (1, 2, 1, 120, 120, 38.00, 'LOTE-B1', '2025-01-03'),
                    (1, 3, 1, 200, 200, 25.00, 'LOTE-C1', '2025-01-02')");
                $result['message'] = "Banco de dados resetado!";
                break;
        }

        echo json_encode($result);
    } catch (Exception $e) {
        echo json_encode(['success' => false, 'error' => $e->getMessage()]);
    }
    exit;
}

// Inicializar na carga da página
try {
    initDatabase();
    $dbStatus = 'Conectado';
} catch (Exception $e) {
    $dbStatus = 'Erro: ' . $e->getMessage();
}
?>
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Demo do Novo Schema</title>
    <style>
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #1a1a2e; color: #eee; }

        .header { background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); padding: 20px; text-align: center; }
        .header h1 { font-size: 1.8em; margin-bottom: 5px; }
        .header p { opacity: 0.9; }

        .container { display: grid; grid-template-columns: 300px 1fr; min-height: calc(100vh - 80px); }

        .sidebar { background: #16213e; padding: 20px; border-right: 1px solid #0f3460; }
        .sidebar h3 { color: #667eea; margin-bottom: 15px; font-size: 0.9em; text-transform: uppercase; letter-spacing: 1px; }
        .sidebar button { width: 100%; padding: 12px 15px; margin-bottom: 8px; background: #0f3460; border: none; color: #eee; border-radius: 8px; cursor: pointer; text-align: left; transition: all 0.2s; }
        .sidebar button:hover { background: #667eea; transform: translateX(5px); }
        .sidebar button.active { background: #667eea; }
        .sidebar hr { border: none; border-top: 1px solid #0f3460; margin: 20px 0; }

        .main { padding: 20px; overflow-y: auto; }

        .card { background: #16213e; border-radius: 12px; padding: 20px; margin-bottom: 20px; }
        .card h2 { color: #667eea; margin-bottom: 15px; display: flex; align-items: center; gap: 10px; }
        .card h2 .badge { background: #764ba2; padding: 3px 10px; border-radius: 20px; font-size: 0.6em; }

        table { width: 100%; border-collapse: collapse; }
        th, td { padding: 10px 12px; text-align: left; border-bottom: 1px solid #0f3460; }
        th { background: #0f3460; color: #667eea; font-weight: 600; font-size: 0.85em; text-transform: uppercase; }
        tr:hover { background: rgba(102, 126, 234, 0.1); }

        .status { padding: 4px 10px; border-radius: 20px; font-size: 0.8em; font-weight: 600; }
        .status-pendente { background: #ffc107; color: #000; }
        .status-estoque { background: #28a745; color: #fff; }
        .status-entregue { background: #17a2b8; color: #fff; }
        .status-cancelado { background: #dc3545; color: #fff; }
        .status-aberta { background: #667eea; color: #fff; }

        .form-group { margin-bottom: 15px; }
        .form-group label { display: block; margin-bottom: 5px; color: #aaa; font-size: 0.9em; }
        .form-group input, .form-group select { width: 100%; padding: 10px; background: #0f3460; border: 1px solid #1a1a2e; color: #eee; border-radius: 6px; }
        .form-group input:focus, .form-group select:focus { outline: none; border-color: #667eea; }

        .btn { padding: 10px 20px; border: none; border-radius: 6px; cursor: pointer; font-weight: 600; transition: all 0.2s; }
        .btn-primary { background: #667eea; color: #fff; }
        .btn-primary:hover { background: #764ba2; }
        .btn-danger { background: #dc3545; color: #fff; }
        .btn-sm { padding: 5px 10px; font-size: 0.85em; }

        .alert { padding: 15px; border-radius: 8px; margin-bottom: 15px; }
        .alert-success { background: rgba(40, 167, 69, 0.2); border: 1px solid #28a745; }
        .alert-error { background: rgba(220, 53, 69, 0.2); border: 1px solid #dc3545; }
        .alert-info { background: rgba(102, 126, 234, 0.2); border: 1px solid #667eea; }

        .highlight { background: #764ba2; color: #fff; padding: 2px 8px; border-radius: 4px; font-family: monospace; }

        .comparison { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; margin: 20px 0; }
        .comparison .old { background: rgba(220, 53, 69, 0.1); border: 1px solid #dc3545; border-radius: 8px; padding: 15px; }
        .comparison .new { background: rgba(40, 167, 69, 0.1); border: 1px solid #28a745; border-radius: 8px; padding: 15px; }
        .comparison h4 { margin-bottom: 10px; }
        .comparison code { display: block; background: #0f3460; padding: 10px; border-radius: 4px; font-size: 0.85em; white-space: pre-wrap; }

        .tree { margin-left: 20px; }
        .tree-item { padding: 8px; margin: 5px 0; background: #0f3460; border-radius: 6px; border-left: 3px solid #667eea; }
        .tree-item.split { margin-left: 30px; border-left-color: #ffc107; }
        .tree-item.cascade { margin-left: 60px; border-left-color: #dc3545; }

        #message { position: fixed; top: 20px; right: 20px; z-index: 1000; max-width: 400px; }

        .empty { text-align: center; padding: 40px; color: #666; }

        .grid-2 { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>Demo do Novo Schema</h1>
        <p>Tabela venda_itens unificada substituindo arquitetura L1/L2</p>
    </div>

    <div id="message"></div>

    <div class="container">
        <div class="sidebar">
            <h3>Visualizar Dados</h3>
            <button onclick="showVendas()">Vendas</button>
            <button onclick="showEstoques()">Estoques</button>
            <button onclick="showConsumos()">Consumos Ativos</button>

            <hr>
            <h3>Operações</h3>
            <button onclick="showCreateVenda()">+ Nova Venda</button>
            <button onclick="showAddItem()">+ Adicionar Item</button>
            <button onclick="showSplit()">Dividir Item</button>
            <button onclick="showPairing()">Parear Estoque</button>
            <button onclick="showReversal()">Estornar</button>

            <hr>
            <h3>Análise</h3>
            <button onclick="showAggregated()">Visão Agregada</button>
            <button onclick="showHierarchy()">Hierarquia de Divisão</button>

            <hr>
            <h3>Ferramentas</h3>
            <button onclick="resetDatabase()" class="btn-danger">Resetar Banco</button>
        </div>

        <div class="main" id="content">
            <div class="card">
                <h2>Bem-vindo! <span class="badge">BD: <?= $dbStatus ?></span></h2>

                <div class="alert alert-info">
                    <strong>O que este demo demonstra:</strong><br>
                    A nova tabela unificada <code>venda_itens</code> que substitui a arquitetura L1/L2
                    (<code>venda_has_produto</code> + <code>venda_has_produto2</code>).
                </div>

                <div class="comparison">
                    <div class="old">
                        <h4>Antigo (L1/L2)</h4>
                        <code>venda_has_produto (L1)
  ↓ trigger copia para
venda_has_produto2 (L2)
  ↓ cadeias de idRelacionado
Rastreamento de divisão complexo</code>
                    </div>
                    <div class="new">
                        <h4>Novo (Unificado)</h4>
                        <code>venda_itens
  • parent_id → pai direto
  • root_id → item original
  • Query simples:
    WHERE id=X OR root_id=X</code>
                    </div>
                </div>

                <h3 style="margin: 20px 0 10px;">Principais Melhorias:</h3>
                <ul style="margin-left: 20px; line-height: 2;">
                    <li><strong>Tabela única</strong> - Sem problemas de sincronização entre L1/L2</li>
                    <li><strong>Status ENUM</strong> - Tipagem segura, sem strings mágicas</li>
                    <li><strong>parent_id/root_id</strong> - Hierarquia de divisão clara</li>
                    <li><strong>estoque_consumos 1:1</strong> - Garantido no nível do banco</li>
                    <li><strong>Triggers</strong> - Atualização automática de estoque e status</li>
                </ul>
            </div>
        </div>
    </div>

    <script>
        function api(action, data = {}) {
            return fetch('', {
                method: 'POST',
                headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                body: new URLSearchParams({action, ...data})
            }).then(r => r.json());
        }

        function showMessage(msg, type = 'success') {
            const div = document.getElementById('message');
            div.innerHTML = `<div class="alert alert-${type}">${msg}</div>`;
            setTimeout(() => div.innerHTML = '', 5000);
        }

        function statusClass(status) {
            const s = status.toLowerCase();
            if (s.includes('pendente')) return 'status-pendente';
            if (s.includes('estoque')) return 'status-estoque';
            if (s.includes('entreg')) return 'status-entregue';
            if (s.includes('cancel')) return 'status-cancelado';
            if (s.includes('aberta')) return 'status-aberta';
            return '';
        }

        async function showVendas() {
            const {data} = await api('get_vendas');
            let html = `<div class="card">
                <h2>Vendas</h2>
                <table>
                    <tr><th>ID</th><th>Cliente</th><th>Status</th><th>Total</th><th>Ações</th></tr>`;

            if (data.length === 0) {
                html += `<tr><td colspan="5" class="empty">Nenhuma venda. Crie uma!</td></tr>`;
            } else {
                for (const v of data) {
                    html += `<tr>
                        <td>#${v.id}</td>
                        <td>${v.cliente}</td>
                        <td><span class="status ${statusClass(v.status)}">${v.status}</span></td>
                        <td>R$ ${parseFloat(v.total).toFixed(2)}</td>
                        <td><button class="btn btn-sm btn-primary" onclick="showVendaItems(${v.id})">Ver Itens</button></td>
                    </tr>`;
                }
            }
            html += `</table></div>`;
            document.getElementById('content').innerHTML = html;
        }

        async function showVendaItems(vendaId) {
            const {data} = await api('get_items', {venda_id: vendaId});
            let html = `<div class="card">
                <h2>Itens da Venda #${vendaId} <span class="badge">Tabela Única!</span></h2>
                <div class="alert alert-info">
                    <strong>Nota:</strong> Estes itens estão em UMA única tabela (venda_itens),
                    não mais em L1+L2. Os campos <code>parent_id</code> e <code>root_id</code>
                    rastreiam divisões.
                </div>
                <table>
                    <tr><th>ID</th><th>Produto</th><th>Qtd</th><th>Status</th><th>Parent</th><th>Root</th><th>Motivo Divisão</th></tr>`;

            if (data.length === 0) {
                html += `<tr><td colspan="7" class="empty">Nenhum item</td></tr>`;
            } else {
                for (const i of data) {
                    html += `<tr>
                        <td>#${i.id}</td>
                        <td>${i.produto_codigo}</td>
                        <td>${parseFloat(i.quantidade).toFixed(2)}</td>
                        <td><span class="status ${statusClass(i.status)}">${i.status}</span></td>
                        <td>${i.parent_id || '-'}</td>
                        <td>${i.root_id || '-'}</td>
                        <td>${i.split_reason || '-'}</td>
                    </tr>`;
                }
            }
            html += `</table>
                <p style="margin-top:15px;color:#888;">
                    → <code>parent_id=NULL</code> = item original<br>
                    → <code>root_id</code> SEMPRE aponta para o original (mesmo em divisões cascateadas!)
                </p>
            </div>`;
            document.getElementById('content').innerHTML = html;
        }

        async function showEstoques() {
            const {data} = await api('get_estoques');
            let html = `<div class="card">
                <h2>Estoques</h2>
                <table>
                    <tr><th>ID</th><th>Produto</th><th>Lote</th><th>Original</th><th>Disponível</th><th>Custo</th><th>Status</th></tr>`;

            for (const e of data) {
                html += `<tr>
                    <td>#${e.id}</td>
                    <td>${e.produto_codigo}</td>
                    <td><span class="highlight">${e.lote}</span></td>
                    <td>${parseFloat(e.quantidade_original).toFixed(2)}</td>
                    <td><strong>${parseFloat(e.quantidade_disponivel).toFixed(2)}</strong></td>
                    <td>R$ ${parseFloat(e.custo_unitario).toFixed(2)}</td>
                    <td><span class="status ${statusClass(e.status)}">${e.status}</span></td>
                </tr>`;
            }
            html += `</table></div>`;
            document.getElementById('content').innerHTML = html;
        }

        async function showConsumos() {
            const {data} = await api('get_consumos');
            let html = `<div class="card">
                <h2>Consumos Ativos (1:1)</h2>
                <div class="alert alert-info">
                    Cada <code>venda_item</code> só pode ter UM consumo ativo (constraint 1:1).<br>
                    Cada <code>estoque</code> só pode ser consumido por UM item.
                </div>
                <table>
                    <tr><th>ID</th><th>Item</th><th>Estoque</th><th>Lote</th><th>Qtd</th><th>Ações</th></tr>`;

            if (data.length === 0) {
                html += `<tr><td colspan="6" class="empty">Nenhum consumo ativo</td></tr>`;
            } else {
                for (const c of data) {
                    html += `<tr>
                        <td>#${c.id}</td>
                        <td>Item #${c.item_id}</td>
                        <td>Estoque #${c.estoque_id}</td>
                        <td><span class="highlight">${c.lote}</span></td>
                        <td>${parseFloat(c.quantidade).toFixed(2)}</td>
                        <td><button class="btn btn-sm btn-danger" onclick="reverseConsumo(${c.id})">Estornar</button></td>
                    </tr>`;
                }
            }
            html += `</table></div>`;
            document.getElementById('content').innerHTML = html;
        }

        async function showCreateVenda() {
            const {data: clientes} = await api('get_clientes');
            let options = clientes.map(c => `<option value="${c.id}">${c.nome}</option>`).join('');

            document.getElementById('content').innerHTML = `<div class="card">
                <h2>+ Nova Venda</h2>
                <div class="form-group">
                    <label>Cliente</label>
                    <select id="cliente_id">${options}</select>
                </div>
                <button class="btn btn-primary" onclick="createVenda()">Criar Venda</button>
                <p style="margin-top:15px;color:#888;">
                    → Status inicial será <code>ABERTA</code> (PostgreSQL ENUM, não string!)
                </p>
            </div>`;
        }

        async function createVenda() {
            const cliente_id = document.getElementById('cliente_id').value;
            const result = await api('create_venda', {cliente_id});
            if (result.success) {
                showMessage(result.message);
                showVendas();
            } else {
                showMessage(result.error, 'error');
            }
        }

        async function showAddItem() {
            const {data: vendas} = await api('get_vendas');
            const {data: produtos} = await api('get_produtos');

            if (vendas.length === 0) {
                document.getElementById('content').innerHTML = `<div class="card">
                    <h2>+ Adicionar Item</h2>
                    <div class="alert alert-error">Crie uma venda primeiro!</div>
                </div>`;
                return;
            }

            let vendaOpts = vendas.map(v => `<option value="${v.id}">Venda #${v.id} - ${v.cliente}</option>`).join('');
            let prodOpts = produtos.map(p => `<option value="${p.id}">${p.codigo} - ${p.descricao}</option>`).join('');

            document.getElementById('content').innerHTML = `<div class="card">
                <h2>+ Adicionar Item à Venda</h2>
                <div class="alert alert-info">
                    <strong>INSERT único!</strong> Não há mais trigger copiando para L2.
                </div>
                <div class="grid-2">
                    <div class="form-group">
                        <label>Venda</label>
                        <select id="venda_id">${vendaOpts}</select>
                    </div>
                    <div class="form-group">
                        <label>Produto</label>
                        <select id="produto_id">${prodOpts}</select>
                    </div>
                    <div class="form-group">
                        <label>Quantidade</label>
                        <input type="number" id="quantidade" value="100" step="0.01">
                    </div>
                    <div class="form-group">
                        <label>Valor Unitário</label>
                        <input type="number" id="valor_unitario" value="45.00" step="0.01">
                    </div>
                </div>
                <button class="btn btn-primary" onclick="addItem()">Adicionar Item</button>
            </div>`;
        }

        async function addItem() {
            const result = await api('add_item', {
                venda_id: document.getElementById('venda_id').value,
                produto_id: document.getElementById('produto_id').value,
                quantidade: document.getElementById('quantidade').value,
                valor_unitario: document.getElementById('valor_unitario').value
            });
            if (result.success) {
                showMessage(result.message);
                showVendaItems(document.getElementById('venda_id').value);
            } else {
                showMessage(result.error, 'error');
            }
        }

        async function showSplit() {
            const {data: vendas} = await api('get_vendas');
            if (vendas.length === 0) {
                document.getElementById('content').innerHTML = `<div class="card"><div class="alert alert-error">Crie uma venda com itens primeiro!</div></div>`;
                return;
            }

            document.getElementById('content').innerHTML = `<div class="card">
                <h2>Dividir Item</h2>
                <div class="alert alert-info">
                    <strong>Simula:</strong> NFe chega com menos quantidade que o pedido.<br>
                    O sistema divide o item em dois: um com a quantidade recebida, outro pendente.
                </div>
                <div class="comparison">
                    <div class="old">
                        <h4>Antigo: idRelacionado</h4>
                        <code>Item 100 (original)
Item 101 (idRelacionado=100)
Item 102 (idRelacionado=101)
  → Precisa query recursiva!</code>
                    </div>
                    <div class="new">
                        <h4>Novo: parent_id + root_id</h4>
                        <code>Item 1 (parent=NULL, root=NULL)
Item 2 (parent=1, root=1)
Item 3 (parent=2, root=1)
  → root_id SEMPRE = original!</code>
                    </div>
                </div>
                <div class="form-group">
                    <label>ID do Item</label>
                    <input type="number" id="split_item_id" placeholder="ID do item para dividir">
                </div>
                <div class="form-group">
                    <label>Quantidade a MANTER (resto vai para divisão)</label>
                    <input type="number" id="split_quantidade" value="60" step="0.01">
                </div>
                <div class="form-group">
                    <label>Motivo da Divisão</label>
                    <select id="split_reason">
                        <option value="PARTIAL_NFE">NFE_PARCIAL - NFe com menos quantidade</option>
                        <option value="PARTIAL_DELIVERY">ENTREGA_PARCIAL - Entrega parcial</option>
                        <option value="BROKEN">QUEBRADO - Itens quebrados</option>
                    </select>
                </div>
                <button class="btn btn-primary" onclick="doSplit()">Executar Divisão</button>
            </div>`;
        }

        async function doSplit() {
            const result = await api('split_item', {
                item_id: document.getElementById('split_item_id').value,
                quantidade_manter: document.getElementById('split_quantidade').value,
                split_reason: document.getElementById('split_reason').value
            });
            if (result.success) {
                showMessage(result.message);
                showHierarchyFor(document.getElementById('split_item_id').value);
            } else {
                showMessage(result.error, 'error');
            }
        }

        async function showPairing() {
            document.getElementById('content').innerHTML = `<div class="card">
                <h2>Parear Item com Estoque</h2>
                <div class="alert alert-info">
                    <strong>Pareamento 1:1:</strong> Cada item só pode ser pareado com UM estoque.<br>
                    Triggers atualizam automaticamente: estoque.quantidade_disponivel e item.status → ESTOQUE
                </div>
                <div class="form-group">
                    <label>ID do Item</label>
                    <input type="number" id="pair_item_id" placeholder="ID do item" onchange="loadAvailableStocks()">
                </div>
                <div id="stock_options"></div>
                <button class="btn btn-primary" onclick="doPairing()">Parear</button>
            </div>`;
        }

        async function loadAvailableStocks() {
            const {data: estoques} = await api('get_estoques');
            const available = estoques.filter(e => parseFloat(e.quantidade_disponivel) > 0);

            let html = `<div class="form-group"><label>Estoque Disponível</label><select id="pair_estoque_id">`;
            for (const e of available) {
                html += `<option value="${e.id}">${e.produto_codigo} - ${e.lote} (${e.quantidade_disponivel} disp.)</option>`;
            }
            html += `</select></div>`;
            document.getElementById('stock_options').innerHTML = html;
        }

        async function doPairing() {
            const result = await api('pair_stock', {
                item_id: document.getElementById('pair_item_id').value,
                estoque_id: document.getElementById('pair_estoque_id').value
            });
            if (result.success) {
                showMessage(result.message);
                showConsumos();
            } else {
                showMessage(result.error, 'error');
            }
        }

        async function showReversal() {
            const {data} = await api('get_consumos');
            if (data.length === 0) {
                document.getElementById('content').innerHTML = `<div class="card"><div class="alert alert-info">Nenhum consumo ativo para estornar.</div></div>`;
                return;
            }

            let options = data.map(c => `<option value="${c.id}">Consumo #${c.id} - Item #${c.item_id} ↔ ${c.lote}</option>`).join('');

            document.getElementById('content').innerHTML = `<div class="card">
                <h2>Estornar Consumo</h2>
                <div class="alert alert-info">
                    O estorno marca <code>is_estornado=TRUE</code> (mantém histórico) e restaura o estoque via trigger.
                </div>
                <div class="form-group">
                    <label>Consumo</label>
                    <select id="consumo_id">${options}</select>
                </div>
                <div class="form-group">
                    <label>Motivo</label>
                    <input type="text" id="estorno_motivo" placeholder="Motivo do estorno">
                </div>
                <button class="btn btn-danger" onclick="doReversal()">Estornar</button>
            </div>`;
        }

        async function doReversal() {
            const result = await api('reverse_consumption', {
                consumo_id: document.getElementById('consumo_id').value,
                motivo: document.getElementById('estorno_motivo').value
            });
            if (result.success) {
                showMessage(result.message);
                showEstoques();
            } else {
                showMessage(result.error, 'error');
            }
        }

        async function reverseConsumo(id) {
            if (!confirm('Estornar este consumo?')) return;
            const result = await api('reverse_consumption', {consumo_id: id, motivo: 'Estorno manual'});
            if (result.success) {
                showMessage(result.message);
                showConsumos();
            } else {
                showMessage(result.error, 'error');
            }
        }

        async function showAggregated() {
            const {data} = await api('get_aggregated');
            let html = `<div class="card">
                <h2>Visão Agregada <span class="badge">Substitui L1!</span></h2>
                <div class="alert alert-info">
                    Esta VIEW agrega todas as divisões de volta para mostrar quantidades por pedido original.<br>
                    <strong>Substitui</strong> as queries complexas de L1+L2 JOIN.
                </div>
                <table>
                    <tr><th>Venda</th><th>Produto</th><th>Pedida</th><th>Ativa</th><th>Pendente</th><th>Estoque</th><th>Linhas</th><th>Divisões</th></tr>`;

            if (data.length === 0) {
                html += `<tr><td colspan="8" class="empty">Sem dados</td></tr>`;
            } else {
                for (const r of data) {
                    html += `<tr>
                        <td>#${r.venda_id}</td>
                        <td>${r.produto_id}</td>
                        <td>${r.quantidade_pedida || '-'}</td>
                        <td>${r.quantidade_ativa || '-'}</td>
                        <td>${r.quantidade_pendente || '-'}</td>
                        <td>${r.quantidade_estoque || '-'}</td>
                        <td>${r.total_linhas}</td>
                        <td>${r.linhas_split}</td>
                    </tr>`;
                }
            }
            html += `</table></div>`;
            document.getElementById('content').innerHTML = html;
        }

        async function showHierarchy() {
            document.getElementById('content').innerHTML = `<div class="card">
                <h2>Hierarquia de Divisão</h2>
                <div class="alert alert-info">
                    Digite o ID de um item original para ver toda a árvore de divisões.<br>
                    Query simples: <code>WHERE id = X OR root_id = X</code>
                </div>
                <div class="form-group">
                    <label>ID do Item (original)</label>
                    <input type="number" id="hierarchy_item_id" placeholder="ID do item">
                </div>
                <button class="btn btn-primary" onclick="showHierarchyFor(document.getElementById('hierarchy_item_id').value)">Ver Hierarquia</button>
                <div id="hierarchy_result"></div>
            </div>`;
        }

        async function showHierarchyFor(itemId) {
            const {data} = await api('get_hierarchy', {item_id: itemId});

            let html = `<div style="margin-top:20px;">
                <h3>Hierarquia do Item #${itemId}</h3>
                <div class="tree">`;

            for (const i of data) {
                let cls = 'tree-item';
                if (i.parent_id && i.root_id == i.parent_id) cls += ' split';
                else if (i.parent_id) cls += ' cascade';

                html += `<div class="${cls}">
                    <strong>#${i.id}</strong> - ${i.produto_codigo}<br>
                    Qtd: ${parseFloat(i.quantidade).toFixed(2)} | Status: <span class="status ${statusClass(i.status)}">${i.status}</span><br>
                    <small>parent_id: ${i.parent_id || 'NULL'} | root_id: ${i.root_id || 'NULL'} | motivo: ${i.split_reason || '-'}</small>
                </div>`;
            }

            html += `</div>
                <p style="margin-top:15px;color:#888;">
                    → Todos os itens com <code>root_id=${itemId}</code> são descendentes do original<br>
                    → Não precisa de CTE recursivo como com idRelacionado!
                </p>
            </div>`;

            document.getElementById('hierarchy_result').innerHTML = html;
        }

        async function resetDatabase() {
            if (!confirm('Resetar banco de dados? Todos os dados serão perdidos.')) return;
            const result = await api('reset_database');
            showMessage(result.message);
            location.reload();
        }
    </script>
</body>
</html>
