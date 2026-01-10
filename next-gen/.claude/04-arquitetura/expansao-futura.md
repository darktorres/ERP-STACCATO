# Roadmap de Expansao Futura

> Documento consolidando recomendacoes de expansao para todos os modulos do ERP.
> Criado: 2026-01-10
> Status: Planejamento

---

## Indice

1. [Visao Geral](#visao-geral)
2. [Prioridade P0 - Critico](#prioridade-p0---critico)
3. [Prioridade P1 - Alta](#prioridade-p1---alta)
4. [Prioridade P2 - Media](#prioridade-p2---media)
5. [Prioridade P3 - Baixa](#prioridade-p3---baixa)
6. [Expansao por Modulo](#expansao-por-modulo)
7. [Infraestrutura Cross-Cutting](#infraestrutura-cross-cutting)
8. [Matriz de Priorizacao](#matriz-de-priorizacao)

---

## Visao Geral

### Estado Atual dos Modulos

| Modulo | Status | Documentacao |
|--------|--------|--------------|
| Cadastros | Basico | cadastros.md |
| Compras | Completo | compras.md |
| Estoque | Completo + Event Sourcing | estoque.md |
| Financeiro | Expandido (8 fases) | financeiro.md |
| Vendas | Expandido (desconto progressivo) | vendas.md |
| NFe | Completo | nfe.md |
| Logistica | Basico | logistica.md |
| Notificacoes | Completo | notificacoes.md |
| Relatorios | Expandido (7 fases) | relatorios.md |

### Gaps Identificados

```
CRITICOS (Seguranca/Compliance):
- Audit trail apenas no estoque
- RBAC basico (apenas tipos de usuario)
- Sem LGPD compliance
- Sem idempotencia em operacoes financeiras

FUNCIONAIS (Negocios):
- Compras sem cotacao/requisicao
- Estoque sem WMS lite
- Vendas sem metas/comissao complexa
- Logistica sem roteirizacao
- NFe sem NFSe/CTe/MDFe

INFRAESTRUTURA:
- Sem sistema de anexos
- Sem comentarios/notas
- Sem tags/labels
- Sem busca full-text
```

---

## Prioridade P0 - Critico

### 1. Audit Trail / Change Log

> Sistema de rastreamento de mudancas para todos os modulos.

#### Problema Atual
- Apenas `estoque` tem event sourcing
- Demais modulos nao rastreiam quem fez o que
- Impossivel auditar mudancas em dados criticos

#### Solucao

```sql
-- =====================================================
-- AUDIT TRAIL - Schema
-- =====================================================

-- Campos padrao em TODAS as tabelas
-- Adicionar via migration em cada tabela existente
ALTER TABLE {tabela} ADD COLUMN IF NOT EXISTS created_by BIGINT REFERENCES usuarios(id);
ALTER TABLE {tabela} ADD COLUMN IF NOT EXISTS updated_by BIGINT REFERENCES usuarios(id);
ALTER TABLE {tabela} ADD COLUMN IF NOT EXISTS deleted_by BIGINT REFERENCES usuarios(id);

-- Historico de mudancas (field-level)
CREATE TABLE audit_log (
    id BIGSERIAL PRIMARY KEY,

    -- Identificacao
    entidade_tipo VARCHAR(50) NOT NULL,     -- 'cliente', 'produto', 'venda'
    entidade_id BIGINT NOT NULL,

    -- Acao
    acao VARCHAR(20) NOT NULL,              -- 'CREATE', 'UPDATE', 'DELETE', 'RESTORE'

    -- Mudancas (para UPDATE)
    campo VARCHAR(100),                      -- NULL para CREATE/DELETE
    valor_anterior TEXT,
    valor_novo TEXT,

    -- Contexto
    usuario_id BIGINT REFERENCES usuarios(id),
    ip_address INET,
    user_agent TEXT,

    -- Metadata
    motivo TEXT,                            -- Justificativa opcional
    request_id UUID,                        -- Correlacao de request

    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_audit_entidade ON audit_log(entidade_tipo, entidade_id);
CREATE INDEX idx_audit_usuario ON audit_log(usuario_id);
CREATE INDEX idx_audit_data ON audit_log(created_at);

-- Particionar por mes para performance
CREATE TABLE audit_log_2026_01 PARTITION OF audit_log
    FOR VALUES FROM ('2026-01-01') TO ('2026-02-01');
```

#### Laravel Implementation

```php
// app/Traits/Auditable.php
trait Auditable
{
    public static function bootAuditable(): void
    {
        static::creating(function ($model) {
            $model->created_by = auth()->id();
        });

        static::updating(function ($model) {
            $model->updated_by = auth()->id();

            // Log field changes
            foreach ($model->getDirty() as $field => $newValue) {
                AuditLog::create([
                    'entidade_tipo' => $model->getTable(),
                    'entidade_id' => $model->id,
                    'acao' => 'UPDATE',
                    'campo' => $field,
                    'valor_anterior' => $model->getOriginal($field),
                    'valor_novo' => $newValue,
                    'usuario_id' => auth()->id(),
                    'ip_address' => request()->ip(),
                ]);
            }
        });

        static::deleting(function ($model) {
            $model->deleted_by = auth()->id();
            $model->saveQuietly();
        });
    }
}

// Usar em todos os models
class Cliente extends Model
{
    use SoftDeletes, Auditable;
}
```

#### Tabelas a Adicionar Campos

```
ALTA PRIORIDADE:
- clientes
- fornecedores
- produtos
- orcamentos
- vendas
- compras
- financeiro_parcelas
- usuarios

MEDIA PRIORIDADE:
- orcamento_itens
- venda_itens
- compra_itens
- estoque_lotes
- nfes

BAIXA PRIORIDADE:
- configuracoes
- notificacoes
- relatorios_execucoes
```

---

### 2. RBAC - Controle de Acesso Granular

> Sistema de permissoes por modulo/acao/escopo.

#### Problema Atual
- Usuarios tem apenas `tipo` (VENDEDOR, GERENTE, etc.)
- Sem controle granular por funcionalidade
- Impossivel dar acesso parcial a um modulo

#### Solucao

```sql
-- =====================================================
-- RBAC - Schema
-- =====================================================

-- Permissoes disponiveis no sistema
CREATE TABLE permissoes (
    id BIGSERIAL PRIMARY KEY,
    codigo VARCHAR(100) NOT NULL UNIQUE,    -- 'VENDAS.DESCONTO.APROVAR'
    modulo VARCHAR(50) NOT NULL,            -- 'VENDAS'
    acao VARCHAR(50) NOT NULL,              -- 'DESCONTO.APROVAR'
    descricao TEXT,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Perfis de acesso (roles)
CREATE TABLE perfis (
    id BIGSERIAL PRIMARY KEY,
    nome VARCHAR(50) NOT NULL UNIQUE,       -- 'Vendedor', 'Gerente Loja', 'Diretor'
    descricao TEXT,
    nivel INTEGER DEFAULT 0,                -- Hierarquia (maior = mais poder)
    ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Permissoes por perfil
CREATE TABLE perfil_permissoes (
    perfil_id BIGINT REFERENCES perfis(id) ON DELETE CASCADE,
    permissao_id BIGINT REFERENCES permissoes(id) ON DELETE CASCADE,

    -- Escopo opcional
    escopo_tipo VARCHAR(50),                -- 'loja', 'regiao', 'all'
    escopo_valor TEXT,                      -- ID da loja, regiao, ou NULL

    PRIMARY KEY (perfil_id, permissao_id, COALESCE(escopo_tipo, ''), COALESCE(escopo_valor, ''))
);

-- Usuarios podem ter multiplos perfis
CREATE TABLE usuario_perfis (
    usuario_id BIGINT REFERENCES usuarios(id) ON DELETE CASCADE,
    perfil_id BIGINT REFERENCES perfis(id) ON DELETE CASCADE,

    -- Escopo do perfil para este usuario
    loja_id BIGINT REFERENCES lojas(id),    -- NULL = todas as lojas

    atribuido_por BIGINT REFERENCES usuarios(id),
    atribuido_em TIMESTAMP DEFAULT NOW(),

    PRIMARY KEY (usuario_id, perfil_id, COALESCE(loja_id, 0))
);

-- Permissoes especificas por usuario (override)
CREATE TABLE usuario_permissoes (
    usuario_id BIGINT REFERENCES usuarios(id) ON DELETE CASCADE,
    permissao_id BIGINT REFERENCES permissoes(id) ON DELETE CASCADE,

    concedida BOOLEAN NOT NULL,             -- TRUE = permite, FALSE = nega (override)
    loja_id BIGINT REFERENCES lojas(id),

    atribuido_por BIGINT REFERENCES usuarios(id),
    atribuido_em TIMESTAMP DEFAULT NOW(),
    motivo TEXT,

    PRIMARY KEY (usuario_id, permissao_id, COALESCE(loja_id, 0))
);

CREATE INDEX idx_usuario_perfis ON usuario_perfis(usuario_id);
CREATE INDEX idx_perfil_permissoes ON perfil_permissoes(perfil_id);
```

#### Permissoes por Modulo

```sql
-- Seed de permissoes
INSERT INTO permissoes (codigo, modulo, acao, descricao) VALUES
-- CADASTROS
('CADASTROS.CLIENTE.VISUALIZAR', 'CADASTROS', 'CLIENTE.VISUALIZAR', 'Ver clientes'),
('CADASTROS.CLIENTE.CRIAR', 'CADASTROS', 'CLIENTE.CRIAR', 'Criar clientes'),
('CADASTROS.CLIENTE.EDITAR', 'CADASTROS', 'CLIENTE.EDITAR', 'Editar clientes'),
('CADASTROS.CLIENTE.EXCLUIR', 'CADASTROS', 'CLIENTE.EXCLUIR', 'Excluir clientes'),
('CADASTROS.CLIENTE.LIMITE_CREDITO', 'CADASTROS', 'CLIENTE.LIMITE_CREDITO', 'Alterar limite de credito'),

('CADASTROS.PRODUTO.VISUALIZAR', 'CADASTROS', 'PRODUTO.VISUALIZAR', 'Ver produtos'),
('CADASTROS.PRODUTO.CRIAR', 'CADASTROS', 'PRODUTO.CRIAR', 'Criar produtos'),
('CADASTROS.PRODUTO.EDITAR', 'CADASTROS', 'PRODUTO.EDITAR', 'Editar produtos'),
('CADASTROS.PRODUTO.CUSTO', 'CADASTROS', 'PRODUTO.CUSTO', 'Ver/editar custo do produto'),
('CADASTROS.PRODUTO.PRECO', 'CADASTROS', 'PRODUTO.PRECO', 'Alterar preco de venda'),

-- VENDAS
('VENDAS.ORCAMENTO.VISUALIZAR', 'VENDAS', 'ORCAMENTO.VISUALIZAR', 'Ver orcamentos'),
('VENDAS.ORCAMENTO.CRIAR', 'VENDAS', 'ORCAMENTO.CRIAR', 'Criar orcamentos'),
('VENDAS.ORCAMENTO.EDITAR', 'VENDAS', 'ORCAMENTO.EDITAR', 'Editar orcamentos'),
('VENDAS.ORCAMENTO.CANCELAR', 'VENDAS', 'ORCAMENTO.CANCELAR', 'Cancelar orcamentos'),

('VENDAS.VENDA.VISUALIZAR', 'VENDAS', 'VENDA.VISUALIZAR', 'Ver vendas'),
('VENDAS.VENDA.CRIAR', 'VENDAS', 'VENDA.CRIAR', 'Criar vendas'),
('VENDAS.VENDA.CANCELAR', 'VENDAS', 'VENDA.CANCELAR', 'Cancelar vendas'),

('VENDAS.DESCONTO.APLICAR', 'VENDAS', 'DESCONTO.APLICAR', 'Aplicar desconto ate seu limite'),
('VENDAS.DESCONTO.APROVAR', 'VENDAS', 'DESCONTO.APROVAR', 'Aprovar descontos de outros'),
('VENDAS.DESCONTO.ILIMITADO', 'VENDAS', 'DESCONTO.ILIMITADO', 'Aplicar qualquer desconto'),

-- COMPRAS
('COMPRAS.PEDIDO.VISUALIZAR', 'COMPRAS', 'PEDIDO.VISUALIZAR', 'Ver pedidos de compra'),
('COMPRAS.PEDIDO.CRIAR', 'COMPRAS', 'PEDIDO.CRIAR', 'Criar pedidos de compra'),
('COMPRAS.PEDIDO.CONFIRMAR', 'COMPRAS', 'PEDIDO.CONFIRMAR', 'Confirmar pedidos'),
('COMPRAS.PEDIDO.CANCELAR', 'COMPRAS', 'PEDIDO.CANCELAR', 'Cancelar pedidos'),
('COMPRAS.PEDIDO.APROVAR', 'COMPRAS', 'PEDIDO.APROVAR', 'Aprovar pedidos acima do limite'),

-- ESTOQUE
('ESTOQUE.SALDO.VISUALIZAR', 'ESTOQUE', 'SALDO.VISUALIZAR', 'Ver saldos de estoque'),
('ESTOQUE.CUSTO.VISUALIZAR', 'ESTOQUE', 'CUSTO.VISUALIZAR', 'Ver custos de estoque'),
('ESTOQUE.AJUSTE.CRIAR', 'ESTOQUE', 'AJUSTE.CRIAR', 'Fazer ajustes de estoque'),
('ESTOQUE.TRANSFERENCIA.CRIAR', 'ESTOQUE', 'TRANSFERENCIA.CRIAR', 'Transferir entre lojas'),

-- FINANCEIRO
('FINANCEIRO.RECEBER.VISUALIZAR', 'FINANCEIRO', 'RECEBER.VISUALIZAR', 'Ver contas a receber'),
('FINANCEIRO.RECEBER.BAIXAR', 'FINANCEIRO', 'RECEBER.BAIXAR', 'Baixar recebimentos'),
('FINANCEIRO.PAGAR.VISUALIZAR', 'FINANCEIRO', 'PAGAR.VISUALIZAR', 'Ver contas a pagar'),
('FINANCEIRO.PAGAR.BAIXAR', 'FINANCEIRO', 'PAGAR.BAIXAR', 'Efetuar pagamentos'),
('FINANCEIRO.PAGAR.APROVAR', 'FINANCEIRO', 'PAGAR.APROVAR', 'Aprovar pagamentos'),

-- NFE
('NFE.EMITIR', 'NFE', 'EMITIR', 'Emitir notas fiscais'),
('NFE.CANCELAR', 'NFE', 'CANCELAR', 'Cancelar notas fiscais'),
('NFE.INUTILIZAR', 'NFE', 'INUTILIZAR', 'Inutilizar numeracao'),

-- RELATORIOS
('RELATORIOS.VENDAS', 'RELATORIOS', 'VENDAS', 'Relatorios de vendas'),
('RELATORIOS.FINANCEIRO', 'RELATORIOS', 'FINANCEIRO', 'Relatorios financeiros'),
('RELATORIOS.ESTOQUE', 'RELATORIOS', 'ESTOQUE', 'Relatorios de estoque'),
('RELATORIOS.CUSTOS', 'RELATORIOS', 'CUSTOS', 'Relatorios com custos/margens'),

-- ADMIN
('ADMIN.USUARIOS', 'ADMIN', 'USUARIOS', 'Gerenciar usuarios'),
('ADMIN.PERMISSOES', 'ADMIN', 'PERMISSOES', 'Gerenciar permissoes'),
('ADMIN.CONFIGURACOES', 'ADMIN', 'CONFIGURACOES', 'Alterar configuracoes do sistema');
```

#### Perfis Padrao

```sql
-- Perfis padrao
INSERT INTO perfis (nome, descricao, nivel) VALUES
('Vendedor', 'Acesso basico a vendas', 10),
('Vendedor Senior', 'Vendedor com mais autonomia', 20),
('Caixa', 'Operacoes de caixa e recebimento', 15),
('Estoquista', 'Operacoes de estoque', 15),
('Comprador', 'Gestao de compras', 25),
('Gerente Loja', 'Gestao completa da loja', 50),
('Gerente Regional', 'Gestao de multiplas lojas', 70),
('Financeiro', 'Operacoes financeiras', 40),
('Diretor', 'Acesso total exceto admin', 90),
('Administrador', 'Acesso total ao sistema', 100);

-- Permissoes do Vendedor
INSERT INTO perfil_permissoes (perfil_id, permissao_id)
SELECT
    (SELECT id FROM perfis WHERE nome = 'Vendedor'),
    id
FROM permissoes
WHERE codigo IN (
    'CADASTROS.CLIENTE.VISUALIZAR',
    'CADASTROS.CLIENTE.CRIAR',
    'CADASTROS.PRODUTO.VISUALIZAR',
    'VENDAS.ORCAMENTO.VISUALIZAR',
    'VENDAS.ORCAMENTO.CRIAR',
    'VENDAS.ORCAMENTO.EDITAR',
    'VENDAS.VENDA.VISUALIZAR',
    'VENDAS.VENDA.CRIAR',
    'VENDAS.DESCONTO.APLICAR',
    'ESTOQUE.SALDO.VISUALIZAR'
);

-- Permissoes do Gerente (herda Vendedor + extras)
INSERT INTO perfil_permissoes (perfil_id, permissao_id)
SELECT
    (SELECT id FROM perfis WHERE nome = 'Gerente Loja'),
    id
FROM permissoes
WHERE codigo NOT IN ('ADMIN.USUARIOS', 'ADMIN.PERMISSOES');
```

#### Laravel Gate/Policy

```php
// app/Services/PermissaoService.php
class PermissaoService
{
    private array $cache = [];

    public function usuarioTemPermissao(int $usuarioId, string $codigo, ?int $lojaId = null): bool
    {
        $cacheKey = "{$usuarioId}:{$codigo}:{$lojaId}";

        if (isset($this->cache[$cacheKey])) {
            return $this->cache[$cacheKey];
        }

        // 1. Verificar override especifico do usuario
        $override = UsuarioPermissao::where('usuario_id', $usuarioId)
            ->whereHas('permissao', fn($q) => $q->where('codigo', $codigo))
            ->where(fn($q) => $q->whereNull('loja_id')->orWhere('loja_id', $lojaId))
            ->orderByRaw('loja_id IS NULL')  // Especifico primeiro
            ->first();

        if ($override) {
            return $this->cache[$cacheKey] = $override->concedida;
        }

        // 2. Verificar perfis do usuario
        $temPermissao = UsuarioPerfil::where('usuario_id', $usuarioId)
            ->where(fn($q) => $q->whereNull('loja_id')->orWhere('loja_id', $lojaId))
            ->whereHas('perfil.permissoes', fn($q) => $q->where('codigo', $codigo))
            ->exists();

        return $this->cache[$cacheKey] = $temPermissao;
    }

    public function listarPermissoesUsuario(int $usuarioId, ?int $lojaId = null): array
    {
        // Retorna lista de codigos de permissao
    }
}

// app/Providers/AuthServiceProvider.php
Gate::before(function ($user, $ability) {
    $permissaoService = app(PermissaoService::class);
    $lojaId = session('loja_id');

    return $permissaoService->usuarioTemPermissao($user->id, $ability, $lojaId);
});

// Uso nos controllers
public function aprovarDesconto(DescontoAprovacao $aprovacao)
{
    $this->authorize('VENDAS.DESCONTO.APROVAR');
    // ...
}
```

---

### 3. Idempotencia para Operacoes Financeiras

> Prevenir duplicacao em operacoes criticas.

#### Problema Atual
- Retry de pagamento pode gerar duplicata
- CNAB pode ser processado duas vezes
- Sem protecao contra double-submit

#### Solucao

```sql
-- Tabela de idempotencia
CREATE TABLE idempotency_keys (
    key VARCHAR(255) PRIMARY KEY,

    -- Request original
    request_hash VARCHAR(64) NOT NULL,      -- SHA256 do body

    -- Resposta armazenada
    response_status INTEGER,
    response_body JSONB,

    -- Controle
    locked_at TIMESTAMP,                    -- Para processamento em andamento
    completed_at TIMESTAMP,
    expires_at TIMESTAMP DEFAULT NOW() + INTERVAL '24 hours',

    created_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_idempotency_expires ON idempotency_keys(expires_at)
    WHERE completed_at IS NOT NULL;

-- Cleanup automatico
CREATE OR REPLACE FUNCTION fn_cleanup_idempotency_keys()
RETURNS void AS $$
BEGIN
    DELETE FROM idempotency_keys WHERE expires_at < NOW();
END;
$$ LANGUAGE plpgsql;
```

```php
// app/Http/Middleware/IdempotencyMiddleware.php
class IdempotencyMiddleware
{
    public function handle(Request $request, Closure $next)
    {
        $idempotencyKey = $request->header('Idempotency-Key');

        if (!$idempotencyKey) {
            return $next($request);
        }

        $requestHash = hash('sha256', $request->getContent());

        // Verificar se ja existe
        $existing = IdempotencyKey::find($idempotencyKey);

        if ($existing) {
            // Verificar se request e identico
            if ($existing->request_hash !== $requestHash) {
                return response()->json([
                    'error' => 'Idempotency key reused with different request'
                ], 422);
            }

            // Se ja completou, retornar resposta armazenada
            if ($existing->completed_at) {
                return response($existing->response_body, $existing->response_status);
            }

            // Se ainda esta processando, retornar 409
            if ($existing->locked_at && $existing->locked_at > now()->subMinutes(5)) {
                return response()->json([
                    'error' => 'Request is being processed'
                ], 409);
            }
        }

        // Criar/atualizar lock
        IdempotencyKey::updateOrCreate(
            ['key' => $idempotencyKey],
            ['request_hash' => $requestHash, 'locked_at' => now()]
        );

        // Processar request
        $response = $next($request);

        // Armazenar resposta
        IdempotencyKey::where('key', $idempotencyKey)->update([
            'response_status' => $response->getStatusCode(),
            'response_body' => $response->getContent(),
            'completed_at' => now(),
            'locked_at' => null,
        ]);

        return $response;
    }
}

// Aplicar em rotas financeiras
Route::middleware(['auth', 'idempotency'])->group(function () {
    Route::post('financeiro/parcelas/{parcela}/baixar', ...);
    Route::post('cnab/processar', ...);
    Route::post('pix/cobranca', ...);
});
```

---

### 4. Workflow de Aprovacao Generico

> Reutilizar padrao de aprovacao para multiplos contextos.

#### Contextos que Precisam de Aprovacao

| Contexto | Gatilho | Aprovador |
|----------|---------|-----------|
| Desconto acima do limite | desconto > limite_usuario | Gerente/Diretor |
| Compra acima de valor | compra.total > X | Gerente/Diretor |
| Cancelamento de venda | venda.cancelar() | Gerente |
| Devolucao/troca | devolucao.criar() | Gerente |
| Ajuste de estoque | ajuste.valor > X | Gerente |
| Alteracao de preco | produto.preco changed | Gerente |
| Limite de credito | cliente.limite changed | Diretor |
| Pagamento agendado | pagamento.valor > X | Diretor |

#### Schema Generico

```sql
-- =====================================================
-- WORKFLOW DE APROVACAO GENERICO
-- =====================================================

CREATE TYPE aprovacao_status AS ENUM ('PENDENTE', 'APROVADO', 'NEGADO', 'EXPIRADO', 'CANCELADO');

-- Tipos de aprovacao configurados
CREATE TABLE aprovacao_tipos (
    id BIGSERIAL PRIMARY KEY,
    codigo VARCHAR(50) NOT NULL UNIQUE,     -- 'DESCONTO', 'COMPRA_GRANDE', 'CANCELAMENTO'
    nome VARCHAR(100) NOT NULL,
    descricao TEXT,

    -- Configuracao
    entidade_tipo VARCHAR(50) NOT NULL,     -- 'orcamento', 'venda', 'compra', 'produto'

    -- Regras de gatilho (JSONB para flexibilidade)
    regras JSONB NOT NULL,
    /*
    Exemplos:
    {"campo": "desconto_percentual", "operador": ">", "valor_referencia": "usuario.limite_desconto"}
    {"campo": "total", "operador": ">", "valor": 50000}
    {"acao": "cancelar"}
    */

    -- Quem pode aprovar
    aprovador_permissao VARCHAR(100),       -- Codigo da permissao necessaria
    aprovador_perfil_minimo INTEGER,        -- Nivel minimo do perfil

    -- Configuracoes
    expira_em_horas INTEGER DEFAULT 24,
    permite_aprovacao_parcial BOOLEAN DEFAULT FALSE,
    notificar_canais TEXT[] DEFAULT ARRAY['IN_APP'],

    ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Solicitacoes de aprovacao
CREATE TABLE aprovacoes (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT gen_random_uuid() UNIQUE,

    -- Tipo e entidade
    tipo_id BIGINT NOT NULL REFERENCES aprovacao_tipos(id),
    entidade_tipo VARCHAR(50) NOT NULL,
    entidade_id BIGINT NOT NULL,

    -- Solicitante
    solicitante_id BIGINT NOT NULL REFERENCES usuarios(id),
    loja_id BIGINT REFERENCES lojas(id),

    -- Dados da solicitacao
    dados_solicitacao JSONB NOT NULL,       -- Snapshot dos dados no momento
    justificativa TEXT,

    -- Aprovacao
    status aprovacao_status NOT NULL DEFAULT 'PENDENTE',
    aprovador_id BIGINT REFERENCES usuarios(id),

    -- Resposta
    dados_resposta JSONB,                   -- Valores alternativos aprovados
    observacao_resposta TEXT,
    respondido_em TIMESTAMP,

    -- Controle
    expira_em TIMESTAMP,
    notificacao_id BIGINT REFERENCES notificacoes(id),

    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

CREATE INDEX idx_aprovacoes_status ON aprovacoes(status) WHERE status = 'PENDENTE';
CREATE INDEX idx_aprovacoes_tipo ON aprovacoes(tipo_id, status);
CREATE INDEX idx_aprovacoes_entidade ON aprovacoes(entidade_tipo, entidade_id);
CREATE INDEX idx_aprovacoes_solicitante ON aprovacoes(solicitante_id);

-- Historico de aprovacoes (para entidades)
CREATE TABLE aprovacoes_historico (
    id BIGSERIAL PRIMARY KEY,
    aprovacao_id BIGINT NOT NULL REFERENCES aprovacoes(id),

    acao VARCHAR(20) NOT NULL,              -- 'CRIADO', 'APROVADO', 'NEGADO', 'EXPIRADO'
    usuario_id BIGINT REFERENCES usuarios(id),

    dados JSONB,
    created_at TIMESTAMP DEFAULT NOW()
);
```

#### Tipos de Aprovacao Pre-Configurados

```sql
INSERT INTO aprovacao_tipos (codigo, nome, entidade_tipo, regras, aprovador_permissao, expira_em_horas) VALUES
('DESCONTO_EXCEDENTE', 'Desconto acima do limite', 'orcamento',
 '{"campo": "desconto_percentual", "operador": ">", "valor_referencia": "usuario.limite_desconto"}',
 'VENDAS.DESCONTO.APROVAR', 24),

('COMPRA_GRANDE', 'Compra acima de R$ 50.000', 'compra',
 '{"campo": "total", "operador": ">", "valor": 50000}',
 'COMPRAS.PEDIDO.APROVAR', 48),

('CANCELAMENTO_VENDA', 'Cancelamento de venda', 'venda',
 '{"acao": "cancelar"}',
 'VENDAS.VENDA.CANCELAR', 24),

('DEVOLUCAO', 'Devolucao de mercadoria', 'devolucao',
 '{"acao": "criar"}',
 'VENDAS.DEVOLUCAO.APROVAR', 24),

('AJUSTE_ESTOQUE_GRANDE', 'Ajuste de estoque > R$ 5.000', 'ajuste_estoque',
 '{"campo": "valor_total", "operador": ">", "valor": 5000}',
 'ESTOQUE.AJUSTE.APROVAR', 24),

('ALTERACAO_PRECO', 'Alteracao de preco de produto', 'produto',
 '{"campo": "preco", "operador": "changed"}',
 'CADASTROS.PRODUTO.PRECO', 48),

('LIMITE_CREDITO', 'Alteracao de limite de credito', 'cliente',
 '{"campo": "limite_credito", "operador": "changed"}',
 'CADASTROS.CLIENTE.LIMITE_CREDITO', 48),

('PAGAMENTO_GRANDE', 'Pagamento acima de R$ 100.000', 'pagamento',
 '{"campo": "valor", "operador": ">", "valor": 100000}',
 'FINANCEIRO.PAGAR.APROVAR', 24);
```

#### Service Generico

```php
// app/Services/AprovacaoService.php
class AprovacaoService
{
    public function __construct(
        private NotificacaoService $notificacaoService,
    ) {}

    /**
     * Verificar se acao precisa de aprovacao
     */
    public function precisaAprovacao(
        string $tipoEntidade,
        Model $entidade,
        string $acao,
        array $dadosNovos = []
    ): ?AprovacaoTipo {
        $tipos = AprovacaoTipo::where('entidade_tipo', $tipoEntidade)
            ->where('ativo', true)
            ->get();

        foreach ($tipos as $tipo) {
            if ($this->avaliarRegras($tipo->regras, $entidade, $acao, $dadosNovos)) {
                // Verificar se usuario atual ja tem permissao
                if (!auth()->user()->can($tipo->aprovador_permissao)) {
                    return $tipo;
                }
            }
        }

        return null;
    }

    /**
     * Criar solicitacao de aprovacao
     */
    public function solicitar(
        AprovacaoTipo $tipo,
        Model $entidade,
        array $dadosSolicitacao,
        ?string $justificativa = null
    ): Aprovacao {
        $aprovacao = DB::transaction(function () use ($tipo, $entidade, $dadosSolicitacao, $justificativa) {
            $aprovacao = Aprovacao::create([
                'tipo_id' => $tipo->id,
                'entidade_tipo' => $entidade->getTable(),
                'entidade_id' => $entidade->id,
                'solicitante_id' => auth()->id(),
                'loja_id' => $entidade->loja_id ?? session('loja_id'),
                'dados_solicitacao' => $dadosSolicitacao,
                'justificativa' => $justificativa,
                'status' => 'PENDENTE',
                'expira_em' => now()->addHours($tipo->expira_em_horas),
            ]);

            // Encontrar aprovadores
            $aprovadores = $this->encontrarAprovadores($tipo, $aprovacao->loja_id);

            // Notificar
            $notificacao = $this->notificacaoService->criar(
                tipo: 'APROVACAO_PENDENTE',
                titulo: "Aprovacao pendente: {$tipo->nome}",
                mensagem: $this->formatarMensagem($tipo, $entidade, $dadosSolicitacao),
                acaoUrl: "/aprovacoes/{$aprovacao->uuid}",
                entidadeTipo: 'aprovacao',
                entidadeId: $aprovacao->id,
                usuariosIds: $aprovadores->pluck('id')->toArray(),
            );

            $aprovacao->update(['notificacao_id' => $notificacao->id]);

            // Historico
            AprovacaoHistorico::create([
                'aprovacao_id' => $aprovacao->id,
                'acao' => 'CRIADO',
                'usuario_id' => auth()->id(),
                'dados' => $dadosSolicitacao,
            ]);

            return $aprovacao;
        });

        return $aprovacao;
    }

    /**
     * Aprovar solicitacao
     */
    public function aprovar(
        Aprovacao $aprovacao,
        ?array $dadosAlternativos = null,
        ?string $observacao = null
    ): Aprovacao {
        // Validar permissao
        $this->validarPermissaoAprovador($aprovacao);

        return DB::transaction(function () use ($aprovacao, $dadosAlternativos, $observacao) {
            $aprovacao->update([
                'status' => 'APROVADO',
                'aprovador_id' => auth()->id(),
                'dados_resposta' => $dadosAlternativos,
                'observacao_resposta' => $observacao,
                'respondido_em' => now(),
            ]);

            // Historico
            AprovacaoHistorico::create([
                'aprovacao_id' => $aprovacao->id,
                'acao' => 'APROVADO',
                'usuario_id' => auth()->id(),
                'dados' => $dadosAlternativos,
            ]);

            // Notificar solicitante
            $this->notificacaoService->criar(
                tipo: 'APROVACAO_APROVADA',
                titulo: "Solicitacao aprovada",
                mensagem: "Sua solicitacao de {$aprovacao->tipo->nome} foi aprovada.",
                acaoUrl: $this->getUrlEntidade($aprovacao),
                usuariosIds: [$aprovacao->solicitante_id],
            );

            // Disparar evento para aplicar a acao
            event(new AprovacaoAprovada($aprovacao));

            return $aprovacao;
        });
    }

    /**
     * Negar solicitacao
     */
    public function negar(
        Aprovacao $aprovacao,
        string $motivo
    ): Aprovacao {
        // Similar ao aprovar...
    }
}
```

---

## Prioridade P1 - Alta

### 5. Expansao do Modulo Compras

```
REQUISICAO DE COMPRA
├── Funcionario solicita item
├── Aprovacao por nivel (valor)
├── Gera pedido de compra apos aprovacao
└── Tracking de status

COTACAO (RFQ)
├── Criar cotacao para multiplos fornecedores
├── Receber propostas
├── Comparativo automatico
├── Selecionar vencedor
└── Gerar pedido de compra

SUPPLIER SCORING
├── Prazo de entrega (pontualidade)
├── Qualidade (devolucoes, defeitos)
├── Preco (competitividade)
├── Condicoes de pagamento
└── Score geral calculado

PONTO DE REPOSICAO
├── Estoque minimo por produto/loja
├── Estoque de seguranca
├── Lead time do fornecedor
├── Sugestao automatica de compra
└── Geracao automatica de pedido (opcional)

CONTRATOS
├── Framework agreements com fornecedores
├── Precos negociados
├── Quantidades minimas/maximas
├── Vigencia
└── Vinculo com pedidos
```

### 6. Expansao do Modulo Estoque

```
INVENTARIO CICLICO
├── Definir zonas/categorias para contagem
├── Gerar agenda de contagem
├── App mobile para contagem
├── Reconciliacao de diferencas
├── Ajuste com aprovacao
└── Relatorio de acuracidade

WMS LITE (Localizacoes)
├── Estrutura: Deposito > Corredor > Prateleira > Posicao
├── Enderecamento de produtos
├── Sugestao de localizacao (put-away)
├── Sugestao de picking (FIFO/FEFO)
├── Mapa visual do deposito
└── Otimizacao de armazenagem

SERIAL NUMBERS
├── Produtos que exigem rastreamento individual
├── Registro de serial na entrada
├── Vinculo serial -> lote -> NF entrada
├── Vinculo serial -> venda -> cliente
├── Historico do serial (garantia, manutencao)

KIT/COMPOSICAO
├── Produto pai com lista de componentes
├── Baixa automatica dos componentes na venda
├── Montagem/desmontagem de kits
├── Custo calculado dos componentes
└── Estoque do kit vs componentes

CONSIGNACAO
├── Estoque em consignacao (nosso no cliente)
├── Estoque consignado (do fornecedor em nos)
├── Controle de propriedade
├── Faturamento por consumo
└── Devolucao de consignado

TRANSFERENCIA ENTRE LOJAS
├── Solicitacao de transferencia
├── Aprovacao da loja origem
├── Geracao de NF de transferencia
├── Tracking do transporte
├── Recebimento na loja destino
└── Conferencia e ajustes
```

### 7. Expansao do Modulo Vendas

```
METAS DE VENDEDOR
├── Meta por periodo (mensal, semanal)
├── Meta por valor ou quantidade
├── Meta por produto/categoria
├── Acompanhamento em tempo real
├── Dashboard de metas
└── Alertas de atingimento

COMISSAO COMPLEXA
├── Percentual base por vendedor
├── Bonus por margem (escalonado)
├── Bonus por meta atingida
├── Comissao por produto/categoria
├── Split de comissao (venda conjunta)
├── Calculo retroativo por cancelamento/devolucao
└── Relatorio de comissao

LEAD/PROSPECT
├── Cadastro de leads (pre-cliente)
├── Origem do lead (indicacao, site, evento)
├── Pipeline de conversao
├── Atribuicao a vendedor
├── Follow-up automatico
├── Conversao para cliente/orcamento
└── Metricas de conversao

TABELA DE PRECO
├── Multiplas tabelas (Balcao, Atacado, VIP)
├── Vinculo cliente -> tabela
├── Preco por tabela
├── Desconto maximo por tabela
├── Vigencia de tabela
└── Promocoes temporarias

RECORRENCIA
├── Pedido recorrente (assinatura)
├── Frequencia (semanal, mensal)
├── Geracao automatica de pedido
├── Cobranca automatica
├── Gestao de pausas/cancelamentos
└── Renovacao automatica
```

### 8. Expansao do Modulo Logistica

```
ROTEIRIZACAO
├── Agrupamento de entregas por regiao
├── Otimizacao de rota (Google Maps API)
├── Capacidade do veiculo
├── Janela de entrega do cliente
├── Reotimizacao em tempo real
└── Custo estimado por rota

RASTREAMENTO REAL-TIME
├── App do motorista com GPS
├── Status de entrega em tempo real
├── ETA para cliente
├── Notificacao push para cliente
├── Historico de posicoes
└── Dashboard de frota

COMPROVANTE DE ENTREGA
├── Captura de foto
├── Assinatura digital
├── Registro de quem recebeu
├── Observacoes de entrega
├── Sincronizacao offline
└── Anexo automatico ao pedido

LOGISTICA REVERSA
├── Solicitacao de devolucao/troca
├── Geracao de codigo de autorizacao
├── Agendamento de coleta
├── Recebimento e inspecao
├── Reintegracao ao estoque ou descarte
└── Credito/estorno para cliente

CALCULO DE FRETE
├── Regras por regiao/CEP
├── Regras por peso/volume
├── Regras por valor do pedido
├── Frete gratis condicional
├── Integracao com transportadoras
├── Cotacao em tempo real
└── Escolha pelo cliente
```

### 9. Expansao do Modulo NFe

```
OUTROS DOCUMENTOS FISCAIS:

NFSe (Nota Fiscal de Servico)
├── Integracao com prefeituras
├── RPS -> NFSe
├── Cancelamento
├── Consulta

CTe (Conhecimento de Transporte)
├── Emissao para transporte proprio
├── Vinculo com NFe
├── Manifesto

MDFe (Manifesto de Documentos Fiscais)
├── Agrupamento de CTe/NFe
├── Encerramento
├── Eventos

NFCe (Cupom Fiscal)
├── PDV integrado
├── Contingencia offline
├── SAT (SP) / MFe (CE)
```

---

## Prioridade P2 - Media

### 10. Sistema de Anexos

```sql
CREATE TABLE anexos (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT gen_random_uuid() UNIQUE,

    -- Vinculo polimorfico
    entidade_tipo VARCHAR(50) NOT NULL,
    entidade_id BIGINT NOT NULL,

    -- Arquivo
    nome_original VARCHAR(255) NOT NULL,
    nome_storage VARCHAR(255) NOT NULL,     -- Nome no storage (UUID)
    extensao VARCHAR(20),
    tipo_mime VARCHAR(100),
    tamanho_bytes BIGINT,

    -- Storage
    storage_disk VARCHAR(50) DEFAULT 'local', -- 'local', 's3', 'azure'
    storage_path TEXT NOT NULL,

    -- Metadata
    categoria VARCHAR(50),                  -- 'documento', 'foto', 'comprovante'
    descricao TEXT,

    -- Controle
    uploaded_by BIGINT REFERENCES usuarios(id),
    created_at TIMESTAMP DEFAULT NOW(),
    deleted_at TIMESTAMP
);

CREATE INDEX idx_anexos_entidade ON anexos(entidade_tipo, entidade_id);
CREATE INDEX idx_anexos_categoria ON anexos(categoria);
```

### 11. Sistema de Comentarios

```sql
CREATE TABLE comentarios (
    id BIGSERIAL PRIMARY KEY,

    -- Vinculo polimorfico
    entidade_tipo VARCHAR(50) NOT NULL,
    entidade_id BIGINT NOT NULL,

    -- Conteudo
    texto TEXT NOT NULL,

    -- Visibilidade
    interno BOOLEAN DEFAULT TRUE,           -- FALSE = visivel para cliente

    -- Mencoes
    mencoes_usuarios BIGINT[],              -- IDs de usuarios mencionados

    -- Controle
    usuario_id BIGINT NOT NULL REFERENCES usuarios(id),
    editado_em TIMESTAMP,
    created_at TIMESTAMP DEFAULT NOW(),
    deleted_at TIMESTAMP
);

CREATE INDEX idx_comentarios_entidade ON comentarios(entidade_tipo, entidade_id);
```

### 12. Sistema de Tags

```sql
CREATE TABLE tags (
    id BIGSERIAL PRIMARY KEY,
    nome VARCHAR(50) NOT NULL,
    slug VARCHAR(50) NOT NULL UNIQUE,
    cor VARCHAR(7),                         -- '#FF5733'

    -- Onde pode ser usada
    entidade_tipos TEXT[] NOT NULL,         -- ['cliente', 'produto', 'venda']

    -- Controle
    ativo BOOLEAN DEFAULT TRUE,
    created_at TIMESTAMP DEFAULT NOW()
);

CREATE TABLE entidade_tags (
    entidade_tipo VARCHAR(50) NOT NULL,
    entidade_id BIGINT NOT NULL,
    tag_id BIGINT NOT NULL REFERENCES tags(id) ON DELETE CASCADE,

    atribuido_por BIGINT REFERENCES usuarios(id),
    created_at TIMESTAMP DEFAULT NOW(),

    PRIMARY KEY (entidade_tipo, entidade_id, tag_id)
);

CREATE INDEX idx_entidade_tags ON entidade_tags(entidade_tipo, entidade_id);
CREATE INDEX idx_tag_entidades ON entidade_tags(tag_id);
```

### 13. Conformidade LGPD

```sql
-- Consentimentos
CREATE TABLE lgpd_consentimentos (
    id BIGSERIAL PRIMARY KEY,

    cliente_id BIGINT REFERENCES clientes(id),

    tipo VARCHAR(50) NOT NULL,              -- 'marketing_email', 'dados_terceiros', 'cookies'
    consentido BOOLEAN NOT NULL,

    -- Contexto
    ip_address INET,
    user_agent TEXT,
    origem VARCHAR(100),                    -- 'cadastro', 'checkout', 'modal_site'

    -- Versao do termo
    termo_versao VARCHAR(20),
    termo_hash VARCHAR(64),

    created_at TIMESTAMP DEFAULT NOW()
);

-- Solicitacoes de direitos
CREATE TABLE lgpd_solicitacoes (
    id BIGSERIAL PRIMARY KEY,
    uuid UUID NOT NULL DEFAULT gen_random_uuid() UNIQUE,

    cliente_id BIGINT REFERENCES clientes(id),
    email VARCHAR(255),                     -- Se cliente nao cadastrado

    tipo VARCHAR(50) NOT NULL,              -- 'EXPORTAR', 'EXCLUIR', 'REVOGAR', 'RETIFICAR'

    status VARCHAR(20) NOT NULL DEFAULT 'PENDENTE',

    -- Dados da solicitacao
    dados_solicitados JSONB,
    justificativa TEXT,

    -- Processamento
    processado_por BIGINT REFERENCES usuarios(id),
    processado_em TIMESTAMP,
    resultado JSONB,                        -- Link para download, etc

    -- Prazos (LGPD = 15 dias)
    prazo_resposta DATE,

    created_at TIMESTAMP DEFAULT NOW()
);

-- Log de acesso a dados pessoais
CREATE TABLE lgpd_acesso_log (
    id BIGSERIAL PRIMARY KEY,

    usuario_id BIGINT REFERENCES usuarios(id),
    cliente_id BIGINT REFERENCES clientes(id),

    tipo_acesso VARCHAR(50),                -- 'VISUALIZOU', 'EXPORTOU', 'EDITOU'
    campos_acessados TEXT[],

    ip_address INET,
    created_at TIMESTAMP DEFAULT NOW()
);
```

---

## Prioridade P3 - Baixa

### 14. Busca Full-Text

```sql
-- Usando PostgreSQL FTS
CREATE TABLE search_index (
    id BIGSERIAL PRIMARY KEY,

    entidade_tipo VARCHAR(50) NOT NULL,
    entidade_id BIGINT NOT NULL,

    -- Conteudo para busca
    titulo TEXT,
    conteudo TEXT,

    -- Vector de busca
    search_vector TSVECTOR,

    -- Metadata para filtros
    metadata JSONB,

    updated_at TIMESTAMP DEFAULT NOW(),

    UNIQUE (entidade_tipo, entidade_id)
);

CREATE INDEX idx_search_vector ON search_index USING GIN(search_vector);
CREATE INDEX idx_search_metadata ON search_index USING GIN(metadata);

-- Trigger para atualizar vector
CREATE FUNCTION fn_update_search_vector()
RETURNS TRIGGER AS $$
BEGIN
    NEW.search_vector :=
        setweight(to_tsvector('portuguese', COALESCE(NEW.titulo, '')), 'A') ||
        setweight(to_tsvector('portuguese', COALESCE(NEW.conteudo, '')), 'B');
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER tr_search_vector
BEFORE INSERT OR UPDATE ON search_index
FOR EACH ROW EXECUTE FUNCTION fn_update_search_vector();

-- Query de busca
-- SELECT * FROM search_index
-- WHERE search_vector @@ plainto_tsquery('portuguese', 'termo busca')
-- ORDER BY ts_rank(search_vector, plainto_tsquery('portuguese', 'termo busca')) DESC;
```

---

## Matriz de Priorizacao

| # | Feature | Prioridade | Esforco | Impacto | Dependencias |
|---|---------|------------|---------|---------|--------------|
| 1 | Audit Trail | P0 | M | Alto | - |
| 2 | RBAC Permissions | P0 | M | Alto | - |
| 3 | Idempotencia | P0 | B | Critico | - |
| 4 | Workflow Aprovacao | P0 | M | Alto | Notificacoes |
| 5 | Compras Expansion | P1 | A | Alto | Aprovacao |
| 6 | Estoque Expansion | P1 | A | Alto | - |
| 7 | Vendas Expansion | P1 | A | Alto | Aprovacao |
| 8 | Logistica Expansion | P1 | A | Medio | - |
| 9 | NFe Expansion | P1 | A | Medio | - |
| 10 | Anexos | P2 | B | Medio | - |
| 11 | Comentarios | P2 | B | Medio | - |
| 12 | Tags | P2 | B | Baixo | - |
| 13 | LGPD | P2 | M | Alto | Audit Trail |
| 14 | Full-Text Search | P3 | A | Medio | - |

**Legenda Esforco:** B = Baixo (< 1 semana), M = Medio (1-3 semanas), A = Alto (> 3 semanas)

---

## Proximos Passos Recomendados

1. **Fase Imediata (P0)**
   - Implementar Audit Trail em todas as tabelas
   - Implementar RBAC com permissoes granulares
   - Adicionar idempotencia nas rotas financeiras
   - Generalizar workflow de aprovacao

2. **Fase Curto Prazo (P1)**
   - Escolher UM modulo para expandir primeiro (recomendo Compras)
   - Implementar requisicao de compra + cotacao
   - Expandir estoque com inventario ciclico

3. **Fase Medio Prazo (P2)**
   - Implementar anexos e comentarios (cross-cutting)
   - LGPD compliance
   - Expansao dos demais modulos

4. **Fase Longo Prazo (P3)**
   - Full-text search
   - Documentos fiscais adicionais
   - Features avanadas

---

## Documentos Relacionados

- [modulos/cadastros.md](./modulos/cadastros.md)
- [modulos/compras.md](./modulos/compras.md)
- [modulos/estoque.md](./modulos/estoque.md)
- [modulos/financeiro.md](./modulos/financeiro.md)
- [modulos/vendas.md](./modulos/vendas.md)
- [modulos/nfe.md](./modulos/nfe.md)
- [modulos/logistica.md](./modulos/logistica.md)
- [modulos/notificacoes.md](./modulos/notificacoes.md)
- [modulos/relatorios.md](./modulos/relatorios.md)
