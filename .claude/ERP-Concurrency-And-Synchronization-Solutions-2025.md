# ERP Staccato - Soluções para Concorrência e Sincronização 2025

## Resumo Executivo

Este documento aborda duas questões críticas identificadas no ERP Staccato que devem ser resolvidas no novo schema:

1. **Controle de Concorrência**: Problema de atualizações perdidas quando múltiplos usuários editam o mesmo registro
2. **Sincronização de Dados**: Problema de dessincronização de status e referências entre entidades relacionadas

## 🎯 **Índice de Soluções**

1. [Problema 1: Controle de Concorrência](#1-problema-1-controle-de-concorrência)
2. [Solução 1: Optimistic Locking](#2-solução-1-optimistic-locking)
3. [Problema 2: Sincronização de Dados](#3-problema-2-sincronização-de-dados)
4. [Solução 2: Event-Driven Synchronization](#4-solução-2-event-driven-synchronization)
5. [Implementação no Novo Schema](#5-implementação-no-novo-schema)
6. [Código C++ de Exemplo](#6-código-c-de-exemplo)
7. [Testes e Validação](#7-testes-e-validação)

---

## 1. **Problema 1: Controle de Concorrência**

### 1.1 Cenário Problemático Atual

```
⏰ 10:00 - Usuário A abre cadastro do Cliente XYZ
          Cliente: "Escritório & Cia", Telefone: "(11) 1234-5678", Email: "antigo@empresa.com"

⏰ 10:05 - Usuário B abre o MESMO cadastro do Cliente XYZ
          Cliente: "Escritório & Cia", Telefone: "(11) 1234-5678", Email: "antigo@empresa.com"

⏰ 10:10 - Usuário B atualiza email para "novo@empresa.com" e SALVA
          ✅ BD: Cliente: "Escritório & Cia", Telefone: "(11) 1234-5678", Email: "novo@empresa.com"

⏰ 10:15 - Usuário A atualiza telefone para "(11) 9999-8888" e SALVA
          ❌ BD: Cliente: "Escritório & Cia", Telefone: "(11) 9999-8888", Email: "antigo@empresa.com"

🚨 RESULTADO: Email atualizado pelo Usuário B foi PERDIDO!
```

### 1.2 Impacto do Problema

- **Perda de Dados**: Atualizações importantes são silenciosamente perdidas
- **Inconsistência**: Dados ficam em estado inválido
- **Frustração do Usuário**: Trabalho perdido sem notificação
- **Problemas de Auditoria**: Histórico de mudanças incorreto
- **Compliance**: Violação de requisitos de integridade

### 1.3 Onde Acontece No Sistema Atual

```sql
-- ❌ PROBLEMA: UPDATE sem verificação de concorrência
UPDATE cliente
SET nome = 'Novo Nome',
    telefone = '(11) 9999-8888',
    lastUpdated = NOW()
WHERE idCliente = 123;

-- Não há verificação se o registro foi modificado por outro usuário!
```

---

## 2. **Solução 1: Optimistic Locking**

### 2.1 Conceito

**Optimistic Locking** assume que conflitos são raros e verifica apenas no momento do salvamento se o registro foi modificado por outro usuário.

### 2.2 Implementação com Versionamento

#### 2.2.1 Adicionar Colunas de Controle

```sql
-- ✅ SOLUÇÃO: Adicionar controle de versão em TODAS as tabelas
ALTER TABLE clientes ADD COLUMN versao INTEGER DEFAULT 1;
ALTER TABLE fornecedores ADD COLUMN versao INTEGER DEFAULT 1;
ALTER TABLE produtos ADD COLUMN versao INTEGER DEFAULT 1;
ALTER TABLE vendas ADD COLUMN versao INTEGER DEFAULT 1;
ALTER TABLE pedidos_compra ADD COLUMN versao INTEGER DEFAULT 1;
-- ... todas as demais tabelas

-- Trigger para incrementar versão automaticamente
CREATE OR REPLACE FUNCTION incrementar_versao()
RETURNS TRIGGER AS $$
BEGIN
    NEW.versao = OLD.versao + 1;
    NEW.atualizado_em = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Aplicar trigger em todas as tabelas
CREATE TRIGGER trigger_versao_clientes
    BEFORE UPDATE ON clientes
    FOR EACH ROW
    EXECUTE FUNCTION incrementar_versao();

CREATE TRIGGER trigger_versao_fornecedores
    BEFORE UPDATE ON fornecedores
    FOR EACH ROW
    EXECUTE FUNCTION incrementar_versao();
-- ... aplicar em todas as tabelas
```

#### 2.2.2 Query Segura com Verificação de Versão

```sql
-- ✅ SOLUÇÃO: UPDATE com verificação de concorrência
UPDATE clientes
SET nome = 'Novo Nome',
    telefone = '(11) 9999-8888'
WHERE id = 'c1a2b3c4-d5e6-7890-abcd-ef1234567890'
    AND versao = 5  -- ⭐ VERIFICAÇÃO CRÍTICA
RETURNING versao;

-- Se a query não afetar nenhuma linha = CONFLITO DE CONCORRÊNCIA!
-- Se retornar versao = 6 = SUCESSO
```

#### 2.2.3 Tratamento de Conflitos

```sql
-- ✅ Função para detectar e resolver conflitos
CREATE OR REPLACE FUNCTION atualizar_com_verificacao_concorrencia(
    p_id UUID,
    p_versao_esperada INTEGER,
    p_novos_dados JSONB
) RETURNS TABLE(
    sucesso BOOLEAN,
    nova_versao INTEGER,
    dados_atuais JSONB,
    mensagem TEXT
) AS $$
DECLARE
    v_linhas_afetadas INTEGER;
    v_nova_versao INTEGER;
    v_dados_atuais JSONB;
BEGIN
    -- Tentar atualizar com verificação de versão
    UPDATE clientes
    SET nome = p_novos_dados->>'nome',
        telefone = p_novos_dados->>'telefone',
        email = p_novos_dados->>'email'
    WHERE id = p_id
        AND versao = p_versao_esperada
    RETURNING versao INTO v_nova_versao;

    GET DIAGNOSTICS v_linhas_afetadas = ROW_COUNT;

    IF v_linhas_afetadas = 0 THEN
        -- CONFLITO DETECTADO: Buscar dados atuais
        SELECT
            TRUE as sucesso,
            versao,
            jsonb_build_object(
                'nome', nome,
                'telefone', telefone,
                'email', email,
                'atualizado_em', atualizado_em
            ),
            'CONFLITO: Registro foi modificado por outro usuário'
        INTO sucesso, nova_versao, dados_atuais, mensagem
        FROM clientes
        WHERE id = p_id;

        RETURN QUERY SELECT FALSE, nova_versao, dados_atuais, mensagem;
    ELSE
        -- SUCESSO
        RETURN QUERY SELECT TRUE, v_nova_versao, NULL::JSONB, 'Atualização realizada com sucesso';
    END IF;
END;
$$ LANGUAGE plpgsql;
```

### 2.3 Exemplo de Uso

```sql
-- Usuário A e B abrem o mesmo cliente (versão 5)
SELECT id, nome, telefone, email, versao
FROM clientes
WHERE id = 'c1a2b3c4-d5e6-7890-abcd-ef1234567890';

-- Resultado: versao = 5

-- ✅ Usuário B salva primeiro (versão 5 → 6)
SELECT * FROM atualizar_com_verificacao_concorrencia(
    'c1a2b3c4-d5e6-7890-abcd-ef1234567890',
    5, -- versão esperada
    '{"nome": "Escritório & Cia", "telefone": "(11) 1234-5678", "email": "novo@empresa.com"}'::jsonb
);

-- Resultado: sucesso=TRUE, nova_versao=6

-- ❌ Usuário A tenta salvar com versão antiga (5, mas atual é 6)
SELECT * FROM atualizar_com_verificacao_concorrencia(
    'c1a2b3c4-d5e6-7890-abcd-ef1234567890',
    5, -- versão antiga!
    '{"nome": "Escritório & Cia", "telefone": "(11) 9999-8888", "email": "antigo@empresa.com"}'::jsonb
);

-- Resultado: sucesso=FALSE, mensagem='CONFLITO: Registro foi modificado por outro usuário'
-- dados_atuais={'nome': 'Escritório & Cia', 'email': 'novo@empresa.com', ...}
```

---

## 3. **Problema 2: Sincronização de Dados**

### 3.1 Cenários Problemáticos

#### 3.1.1 Status Dessincronizado

```
🛒 Pedido Cliente: "Em Recebimento"
📦 Pedido Fornecedor: "Confirmado"

❌ Operação X altera Pedido Cliente para "Cancelado"
✅ Pedido Cliente: "Cancelado"
❌ Pedido Fornecedor: "Confirmado" (INCONSISTENTE!)

🚨 PROBLEMA: Estados incompatíveis entre entidades relacionadas
```

#### 3.1.2 Referências Órfãs

```sql
-- Estado inicial
-- cliente_order.supplier_order_id = 123
-- supplier_order.id = 123, client_order_id = 456

-- ❌ Operação remove link de um lado só
UPDATE itens_venda SET id_pedido_compra = NULL WHERE id = '456';

-- Resultado INCONSISTENTE:
-- itens_venda.id_pedido_compra = NULL
-- pedidos_compra.id_item_venda = '456' (ÓRFÃO!)
```

### 3.2 Casos Reais no ERP

#### 3.2.1 Fluxo de Vendas → Compras

```
Venda → Item Venda → Origem Atendimento (tipo=compra) → Pedido Compra

❌ Problemas:
1. Venda cancelada, mas Pedido Compra fica "Confirmado"
2. Pedido Compra recebido, mas Origem Atendimento fica "Pendente"
3. Item removido da Venda, mas Pedido Compra fica ativo
```

#### 3.2.2 Fluxo NFe ↔ Vendas/Compras

```
Venda → NFe Saída
Compra → NFe Entrada

❌ Problemas:
1. NFe cancelada, mas Venda fica "Faturada"
2. Venda cancelada, mas NFe fica "Autorizada"
3. Produto alterado na Venda, mas NFe mantém dados antigos
```

---

## 4. **Solução 2: Event-Driven Synchronization**

### 4.1 Conceito

Implementar um sistema de eventos que automaticamente sincroniza estados relacionados quando uma entidade muda.

### 4.2 Implementação com Triggers e Funções

#### 4.2.1 Sistema de Eventos

```sql
-- ✅ Tabela de eventos do sistema
CREATE TABLE eventos_sistema (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    tipo_evento VARCHAR(50) NOT NULL,
    entidade_origem VARCHAR(50) NOT NULL,
    id_entidade_origem UUID NOT NULL,
    dados_evento JSONB NOT NULL,
    processado BOOLEAN DEFAULT FALSE,
    criado_em TIMESTAMP DEFAULT NOW(),
    processado_em TIMESTAMP NULL
);

-- Índices para performance
CREATE INDEX idx_eventos_nao_processados ON eventos_sistema(processado, criado_em) WHERE NOT processado;
CREATE INDEX idx_eventos_entidade ON eventos_sistema(entidade_origem, id_entidade_origem);
```

#### 4.2.2 Triggers para Capturar Mudanças

```sql
-- ✅ Função genérica para publicar eventos
CREATE OR REPLACE FUNCTION publicar_evento(
    p_tipo_evento VARCHAR,
    p_entidade VARCHAR,
    p_id_entidade UUID,
    p_dados JSONB DEFAULT '{}'::jsonb
) RETURNS UUID AS $$
DECLARE
    v_evento_id UUID;
BEGIN
    INSERT INTO eventos_sistema (tipo_evento, entidade_origem, id_entidade_origem, dados_evento)
    VALUES (p_tipo_evento, p_entidade, p_id_entidade, p_dados)
    RETURNING id INTO v_evento_id;

    RETURN v_evento_id;
END;
$$ LANGUAGE plpgsql;

-- ✅ Trigger para vendas
CREATE OR REPLACE FUNCTION trigger_sincronizar_venda()
RETURNS TRIGGER AS $$
BEGIN
    -- Venda cancelada
    IF NEW.status = 'cancelada' AND OLD.status != 'cancelada' THEN
        PERFORM publicar_evento(
            'venda_cancelada',
            'vendas',
            NEW.id,
            jsonb_build_object(
                'id_venda', NEW.id,
                'status_anterior', OLD.status,
                'status_novo', NEW.status
            )
        );
    END IF;

    -- Venda entregue
    IF NEW.status = 'entregue' AND OLD.status != 'entregue' THEN
        PERFORM publicar_evento(
            'venda_entregue',
            'vendas',
            NEW.id,
            jsonb_build_object(
                'id_venda', NEW.id,
                'data_entrega', NEW.data_entrega_realizada
            )
        );
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trigger_sync_vendas
    AFTER UPDATE ON vendas
    FOR EACH ROW
    EXECUTE FUNCTION trigger_sincronizar_venda();
```

#### 4.2.3 Processadores de Eventos

```sql
-- ✅ Função para processar eventos de venda cancelada
CREATE OR REPLACE FUNCTION processar_venda_cancelada(p_evento_id UUID)
RETURNS BOOLEAN AS $$
DECLARE
    v_evento eventos_sistema%ROWTYPE;
    v_id_venda UUID;
BEGIN
    -- Buscar dados do evento
    SELECT * INTO v_evento FROM eventos_sistema WHERE id = p_evento_id;
    v_id_venda := (v_evento.dados_evento->>'id_venda')::UUID;

    -- 1. Cancelar origens de atendimento relacionadas
    UPDATE origens_atendimento
    SET status = 'cancelada',
        atualizado_em = NOW()
    WHERE id_item_venda IN (
        SELECT id FROM itens_venda WHERE id_venda = v_id_venda
    )
    AND status NOT IN ('cancelada', 'entregue');

    -- 2. Cancelar pedidos de compra relacionados (se não foram recebidos)
    UPDATE pedidos_compra
    SET status = 'cancelado',
        atualizado_em = NOW()
    WHERE id IN (
        SELECT DISTINCT oa.id_referencia_compra
        FROM origens_atendimento oa
        JOIN itens_venda iv ON oa.id_item_venda = iv.id
        WHERE iv.id_venda = v_id_venda
        AND oa.tipo_origem = 'compra'
    )
    AND status NOT IN ('recebido', 'cancelado');

    -- 3. Liberar reservas de estoque
    WITH estoque_a_liberar AS (
        SELECT
            oa.id_referencia_estoque as id_produto,
            SUM(oa.quantidade_alocada) as quantidade_liberar
        FROM origens_atendimento oa
        JOIN itens_venda iv ON oa.id_item_venda = iv.id
        WHERE iv.id_venda = v_id_venda
        AND oa.tipo_origem = 'estoque'
        GROUP BY oa.id_referencia_estoque
    )
    UPDATE saldos_estoque
    SET quantidade_disponivel = quantidade_disponivel + eal.quantidade_liberar,
        quantidade_reservada = quantidade_reservada - eal.quantidade_liberar,
        atualizado_em = NOW()
    FROM estoque_a_liberar eal
    WHERE saldos_estoque.id_produto = eal.id_produto;

    -- 4. Cancelar NFe se ainda não foi enviada
    UPDATE nfes
    SET status_sefaz = 'cancelada',
        atualizado_em = NOW()
    WHERE id IN (SELECT id_nfe_saida FROM vendas WHERE id = v_id_venda)
    AND status_sefaz IN ('rascunho', 'pendente');

    -- Marcar evento como processado
    UPDATE eventos_sistema
    SET processado = TRUE, processado_em = NOW()
    WHERE id = p_evento_id;

    RETURN TRUE;

EXCEPTION WHEN OTHERS THEN
    -- Log do erro
    INSERT INTO log_erros (evento_id, erro, criado_em)
    VALUES (p_evento_id, SQLERRM, NOW());

    RETURN FALSE;
END;
$$ LANGUAGE plpgsql;
```

#### 4.2.4 Processador Principal de Eventos

```sql
-- ✅ Função para processar todos os eventos pendentes
CREATE OR REPLACE FUNCTION processar_eventos_pendentes()
RETURNS INTEGER AS $$
DECLARE
    v_evento eventos_sistema%ROWTYPE;
    v_processados INTEGER := 0;
    v_sucesso BOOLEAN;
BEGIN
    -- Processar eventos em ordem cronológica
    FOR v_evento IN
        SELECT * FROM eventos_sistema
        WHERE NOT processado
        ORDER BY criado_em
        LIMIT 100 -- Processar em lotes
    LOOP
        v_sucesso := FALSE;

        -- Dispatcher de eventos
        CASE v_evento.tipo_evento
            WHEN 'venda_cancelada' THEN
                v_sucesso := processar_venda_cancelada(v_evento.id);
            WHEN 'venda_entregue' THEN
                v_sucesso := processar_venda_entregue(v_evento.id);
            WHEN 'pedido_compra_recebido' THEN
                v_sucesso := processar_pedido_recebido(v_evento.id);
            WHEN 'estoque_atualizado' THEN
                v_sucesso := processar_estoque_atualizado(v_evento.id);
            ELSE
                -- Evento desconhecido
                UPDATE eventos_sistema
                SET processado = TRUE, processado_em = NOW()
                WHERE id = v_evento.id;
                v_sucesso := TRUE;
        END CASE;

        IF v_sucesso THEN
            v_processados := v_processados + 1;
        END IF;
    END LOOP;

    RETURN v_processados;
END;
$$ LANGUAGE plpgsql;
```

---

## 5. **Implementação no Novo Schema**

### 5.1 Tabelas Base com Controle de Concorrência

```sql
-- ✅ Template para todas as tabelas do novo schema
CREATE TABLE vendas (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    numero VARCHAR(20) UNIQUE NOT NULL,
    id_cliente UUID NOT NULL REFERENCES clientes(id),
    id_vendedor UUID NOT NULL REFERENCES usuarios(id),

    -- Dados do negócio
    data_venda TIMESTAMP NOT NULL DEFAULT NOW(),
    status VARCHAR(20) NOT NULL DEFAULT 'pendente',
    valor_total DECIMAL(15,2) NOT NULL DEFAULT 0,

    -- ⭐ CONTROLE DE CONCORRÊNCIA
    versao INTEGER NOT NULL DEFAULT 1,

    -- ⭐ AUDITORIA
    criado_em TIMESTAMP NOT NULL DEFAULT NOW(),
    atualizado_em TIMESTAMP NOT NULL DEFAULT NOW(),
    criado_por UUID REFERENCES usuarios(id),
    atualizado_por UUID REFERENCES usuarios(id),

    -- ⭐ SOFT DELETE
    deletado BOOLEAN DEFAULT FALSE,
    deletado_em TIMESTAMP NULL,
    deletado_por UUID REFERENCES usuarios(id)
);

-- ✅ Trigger de versionamento
CREATE TRIGGER trigger_versao_vendas
    BEFORE UPDATE ON vendas
    FOR EACH ROW
    EXECUTE FUNCTION incrementar_versao();

-- ✅ Trigger de sincronização
CREATE TRIGGER trigger_sync_vendas
    AFTER UPDATE ON vendas
    FOR EACH ROW
    EXECUTE FUNCTION trigger_sincronizar_venda();
```

### 5.2 Constraints para Integridade Referencial

```sql
-- ✅ Função para verificar integridade de estados
CREATE OR REPLACE FUNCTION verificar_estados_consistentes()
RETURNS TRIGGER AS $$
BEGIN
    -- Verificar se venda cancelada não tem origens ativas
    IF NEW.status = 'cancelada' THEN
        IF EXISTS (
            SELECT 1 FROM origens_atendimento oa
            JOIN itens_venda iv ON oa.id_item_venda = iv.id
            WHERE iv.id_venda = NEW.id
            AND oa.status NOT IN ('cancelada', 'entregue')
        ) THEN
            RAISE EXCEPTION 'Não é possível cancelar venda com origens de atendimento ativas';
        END IF;
    END IF;

    -- Verificar se venda entregue tem todas as origens entregues
    IF NEW.status = 'entregue' THEN
        IF EXISTS (
            SELECT 1 FROM origens_atendimento oa
            JOIN itens_venda iv ON oa.id_item_venda = iv.id
            WHERE iv.id_venda = NEW.id
            AND oa.status != 'entregue'
        ) THEN
            RAISE EXCEPTION 'Não é possível marcar venda como entregue com origens pendentes';
        END IF;
    END IF;

    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

-- Aplicar trigger de validação
CREATE TRIGGER trigger_validar_vendas
    BEFORE UPDATE ON vendas
    FOR EACH ROW
    EXECUTE FUNCTION verificar_estados_consistentes();
```

### 5.3 Foreign Keys com Cascade Apropriado

```sql
-- ✅ Foreign keys que mantêm integridade
ALTER TABLE itens_venda
ADD CONSTRAINT fk_itens_venda_venda
FOREIGN KEY (id_venda) REFERENCES vendas(id)
ON DELETE CASCADE -- Se venda for deletada, deletar itens
ON UPDATE CASCADE; -- Se ID da venda mudar, atualizar itens

ALTER TABLE origens_atendimento
ADD CONSTRAINT fk_origens_item_venda
FOREIGN KEY (id_item_venda) REFERENCES itens_venda(id)
ON DELETE CASCADE
ON UPDATE CASCADE;

-- ✅ Índices para performance das foreign keys
CREATE INDEX idx_itens_venda_venda ON itens_venda(id_venda);
CREATE INDEX idx_origens_atendimento_item ON origens_atendimento(id_item_venda);
CREATE INDEX idx_origens_atendimento_compra ON origens_atendimento(id_referencia_compra);
```

---

## 6. **Código C++ de Exemplo**

### 6.1 Classe Base com Optimistic Locking

```cpp
// ✅ Classe base para entidades com controle de concorrência
class EntityBase {
protected:
    QString id;
    int version;
    QDateTime createdAt;
    QDateTime updatedAt;
    QString createdBy;
    QString updatedBy;
    bool deleted = false;

public:
    virtual bool save() {
        SqlQuery query;

        if (isNewRecord()) {
            return insertRecord(query);
        } else {
            return updateRecord(query);
        }
    }

private:
    bool updateRecord(SqlQuery& query) {
        // ✅ UPDATE com verificação de versão
        query.prepare(
            "UPDATE " + getTableName() + " "
            "SET " + getUpdateFields() + ", "
            "    atualizado_em = NOW(), "
            "    atualizado_por = ? "
            "WHERE id = ? AND versao = ? "
            "RETURNING versao"
        );

        // Bind dos valores dos campos
        bindUpdateValues(query);
        query.addBindValue(getCurrentUserId());
        query.addBindValue(id);
        query.addBindValue(version); // ⭐ VERIFICAÇÃO CRÍTICA

        if (!query.exec()) {
            throw RuntimeException("Erro executando query: " + query.lastError().text());
        }

        if (!query.next()) {
            // ❌ CONFLITO DE CONCORRÊNCIA DETECTADO
            handleConcurrencyConflict();
            return false;
        }

        // ✅ SUCESSO: Atualizar versão local
        version = query.value("versao").toInt();
        return true;
    }

    void handleConcurrencyConflict() {
        // Buscar dados atuais do banco
        SqlQuery queryCheck;
        queryCheck.prepare(
            "SELECT versao, atualizado_em, atualizado_por, " +
            getAllFields() + " "
            "FROM " + getTableName() + " "
            "WHERE id = ?"
        );
        queryCheck.addBindValue(id);
        queryCheck.exec();

        if (queryCheck.next()) {
            int currentVersion = queryCheck.value("versao").toInt();
            QDateTime lastUpdate = queryCheck.value("atualizado_em").toDateTime();
            QString lastUser = queryCheck.value("atualizado_por").toString();

            // ✅ Mostrar diálogo de conflito para o usuário
            showConflictDialog(currentVersion, lastUpdate, lastUser, queryCheck);
        }
    }

    void showConflictDialog(int currentVersion, QDateTime lastUpdate,
                           QString lastUser, SqlQuery& currentData) {
        ConflictResolutionDialog dialog;
        dialog.setLocalData(this);
        dialog.setServerData(currentData);
        dialog.setConflictInfo(currentVersion, lastUpdate, lastUser);

        auto result = dialog.exec();

        switch (result) {
            case ConflictResolutionDialog::UseServer:
                // Descartar mudanças locais, usar dados do servidor
                loadFromQuery(currentData);
                break;

            case ConflictResolutionDialog::UseLocal:
                // Forçar mudanças locais (atualizar com versão atual)
                version = currentVersion;
                save(); // Tentar novamente
                break;

            case ConflictResolutionDialog::Merge:
                // Permitir usuário resolver campo por campo
                showMergeDialog(currentData);
                break;

            case ConflictResolutionDialog::Cancel:
                // Cancelar operação
                break;
        }
    }
};
```

### 6.2 Implementação para Venda

```cpp
// ✅ Implementação específica para Vendas
class Venda : public EntityBase {
private:
    QString numero;
    QString idCliente;
    QString idVendedor;
    QDateTime dataVenda;
    QString status;
    double valorTotal;

public:
    bool setStatus(const QString& novoStatus) {
        // ✅ Validar transição de status
        if (!isValidStatusTransition(status, novoStatus)) {
            qApp->enqueueError(
                QString("Transição de status inválida: %1 → %2")
                .arg(status, novoStatus)
            );
            return false;
        }

        QString statusAnterior = status;
        status = novoStatus;

        // ✅ Salvar com controle de concorrência
        if (!save()) {
            // Reverter em caso de erro
            status = statusAnterior;
            return false;
        }

        // ✅ Disparar eventos de sincronização
        processStatusChange(statusAnterior, novoStatus);
        return true;
    }

private:
    QString getTableName() const override { return "vendas"; }

    QString getUpdateFields() const override {
        return "numero = ?, id_cliente = ?, id_vendedor = ?, "
               "data_venda = ?, status = ?, valor_total = ?";
    }

    void bindUpdateValues(SqlQuery& query) override {
        query.addBindValue(numero);
        query.addBindValue(idCliente);
        query.addBindValue(idVendedor);
        query.addBindValue(dataVenda);
        query.addBindValue(status);
        query.addBindValue(valorTotal);
    }

    bool isValidStatusTransition(const QString& from, const QString& to) {
        // ✅ Máquina de estados válidas
        static QMap<QString, QStringList> validTransitions = {
            {"pendente", {"confirmada", "cancelada"}},
            {"confirmada", {"em_separacao", "cancelada"}},
            {"em_separacao", {"pronta_entrega", "confirmada"}},
            {"pronta_entrega", {"em_entrega", "em_separacao"}},
            {"em_entrega", {"entregue", "pronta_entrega"}},
            {"entregue", {}}, // Estado final
            {"cancelada", {}} // Estado final
        };

        return validTransitions[from].contains(to);
    }

    void processStatusChange(const QString& from, const QString& to) {
        // ✅ Processar mudanças que afetam outras entidades
        if (to == "cancelada") {
            // Cancelar origens de atendimento
            cancelarOrigensAtendimento();
            // Liberar reservas de estoque
            liberarReservasEstoque();
            // Cancelar pedidos de compra relacionados
            cancelarPedidosCompraRelacionados();
        }

        if (to == "entregue") {
            // Dar baixa no estoque
            baixarEstoque();
            // Gerar movimentos financeiros
            gerarContasReceber();
        }
    }
};
```

### 6.3 Sistema de Eventos em C++

```cpp
// ✅ Sistema de eventos assíncrono
class EventProcessor : public QObject {
    Q_OBJECT

private:
    QTimer* eventTimer;

public:
    EventProcessor(QObject* parent = nullptr) : QObject(parent) {
        eventTimer = new QTimer(this);
        connect(eventTimer, &QTimer::timeout, this, &EventProcessor::processEvents);
        eventTimer->start(5000); // Processar a cada 5 segundos
    }

private slots:
    void processEvents() {
        SqlQuery query;
        query.prepare(
            "SELECT * FROM eventos_sistema "
            "WHERE NOT processado "
            "ORDER BY criado_em "
            "LIMIT 10"
        );

        if (!query.exec()) {
            qApp->enqueueError("Erro processando eventos: " + query.lastError().text());
            return;
        }

        while (query.next()) {
            processEvent(query);
        }
    }

    void processEvent(SqlQuery& query) {
        QString tipoEvento = query.value("tipo_evento").toString();
        QString entidadeOrigem = query.value("entidade_origem").toString();
        QString idEntidade = query.value("id_entidade_origem").toString();
        QString dadosJson = query.value("dados_evento").toString();

        try {
            if (tipoEvento == "venda_cancelada") {
                processVendaCancelada(idEntidade, dadosJson);
            } else if (tipoEvento == "pedido_recebido") {
                processPedidoRecebido(idEntidade, dadosJson);
            }
            // ... outros eventos

            // Marcar como processado
            markEventProcessed(query.value("id").toString());

        } catch (const std::exception& e) {
            qApp->enqueueError(
                QString("Erro processando evento %1: %2")
                .arg(tipoEvento, e.what())
            );
        }
    }

    void processVendaCancelada(const QString& idVenda, const QString& dados) {
        qApp->startTransaction("Processar cancelamento de venda");

        try {
            // 1. Cancelar origens de atendimento
            SqlQuery queryOrigens;
            queryOrigens.prepare(
                "UPDATE origens_atendimento "
                "SET status = 'cancelada', atualizado_em = NOW() "
                "WHERE id_item_venda IN ("
                "    SELECT id FROM itens_venda WHERE id_venda = ?"
                ") AND status NOT IN ('cancelada', 'entregue')"
            );
            queryOrigens.addBindValue(idVenda);
            queryOrigens.exec();

            // 2. Liberar reservas de estoque
            SqlQuery queryEstoque;
            queryEstoque.prepare(
                "UPDATE saldos_estoque "
                "SET quantidade_disponivel = quantidade_disponivel + reserva.quantidade, "
                "    quantidade_reservada = quantidade_reservada - reserva.quantidade "
                "FROM ("
                "    SELECT oa.id_referencia_estoque as id_produto, "
                "           SUM(oa.quantidade_alocada) as quantidade "
                "    FROM origens_atendimento oa "
                "    JOIN itens_venda iv ON oa.id_item_venda = iv.id "
                "    WHERE iv.id_venda = ? AND oa.tipo_origem = 'estoque' "
                "    GROUP BY oa.id_referencia_estoque"
                ") reserva "
                "WHERE saldos_estoque.id_produto = reserva.id_produto"
            );
            queryEstoque.addBindValue(idVenda);
            queryEstoque.exec();

            qApp->endTransaction();

        } catch (...) {
            qApp->rollbackTransaction("Erro cancelando venda");
            throw;
        }
    }
};
```

---

## 7. **Testes e Validação**

### 7.1 Testes de Concorrência

```sql
-- ✅ Script de teste para optimistic locking
DO $$
DECLARE
    v_cliente_id UUID := gen_random_uuid();
    v_versao_inicial INTEGER;
    v_resultado1 BOOLEAN;
    v_resultado2 BOOLEAN;
BEGIN
    -- Setup: Criar cliente teste
    INSERT INTO clientes (id, nome, telefone, email, versao)
    VALUES (v_cliente_id, 'Cliente Teste', '(11) 1111-1111', 'teste@teste.com', 1);

    -- Simular dois usuários abrindo o mesmo registro
    SELECT versao INTO v_versao_inicial FROM clientes WHERE id = v_cliente_id;

    RAISE NOTICE 'Versão inicial: %', v_versao_inicial;

    -- Usuário 1 salva primeiro (deve suceder)
    SELECT sucesso INTO v_resultado1 FROM atualizar_com_verificacao_concorrencia(
        v_cliente_id,
        v_versao_inicial,
        '{"nome": "Cliente Teste", "telefone": "(11) 2222-2222", "email": "teste@teste.com"}'::jsonb
    );

    RAISE NOTICE 'Usuário 1 resultado: %', v_resultado1;

    -- Usuário 2 tenta salvar com versão antiga (deve falhar)
    SELECT sucesso INTO v_resultado2 FROM atualizar_com_verificacao_concorrencia(
        v_cliente_id,
        v_versao_inicial, -- versão antiga!
        '{"nome": "Cliente Teste", "telefone": "(11) 1111-1111", "email": "novo@teste.com"}'::jsonb
    );

    RAISE NOTICE 'Usuário 2 resultado: %', v_resultado2;

    -- Verificar resultado
    IF v_resultado1 = TRUE AND v_resultado2 = FALSE THEN
        RAISE NOTICE '✅ TESTE PASSOU: Optimistic locking funcionando';
    ELSE
        RAISE NOTICE '❌ TESTE FALHOU: Optimistic locking com problema';
    END IF;

    -- Cleanup
    DELETE FROM clientes WHERE id = v_cliente_id;
END $$;
```

### 7.2 Testes de Sincronização

```sql
-- ✅ Script de teste para sincronização de eventos
DO $$
DECLARE
    v_venda_id UUID := gen_random_uuid();
    v_item_id UUID := gen_random_uuid();
    v_origem_id UUID := gen_random_uuid();
    v_eventos_antes INTEGER;
    v_eventos_depois INTEGER;
BEGIN
    -- Contar eventos antes
    SELECT COUNT(*) INTO v_eventos_antes FROM eventos_sistema WHERE NOT processado;

    -- Setup: Criar estrutura de teste
    INSERT INTO vendas (id, numero, id_cliente, id_vendedor, status)
    VALUES (v_venda_id, 'TEST001', gen_random_uuid(), gen_random_uuid(), 'confirmada');

    INSERT INTO itens_venda (id, id_venda, id_produto, quantidade)
    VALUES (v_item_id, v_venda_id, gen_random_uuid(), 5);

    INSERT INTO origens_atendimento (id, id_item_venda, tipo_origem, quantidade_alocada, status)
    VALUES (v_origem_id, v_item_id, 'estoque', 5, 'disponivel');

    -- Ação: Cancelar venda (deve gerar evento)
    UPDATE vendas SET status = 'cancelada' WHERE id = v_venda_id;

    -- Verificar se evento foi criado
    SELECT COUNT(*) INTO v_eventos_depois FROM eventos_sistema
    WHERE NOT processado AND tipo_evento = 'venda_cancelada';

    IF v_eventos_depois > v_eventos_antes THEN
        RAISE NOTICE '✅ TESTE PASSOU: Evento criado automaticamente';

        -- Processar eventos
        PERFORM processar_eventos_pendentes();

        -- Verificar sincronização
        IF EXISTS (
            SELECT 1 FROM origens_atendimento
            WHERE id = v_origem_id AND status = 'cancelada'
        ) THEN
            RAISE NOTICE '✅ TESTE PASSOU: Sincronização automática funcionando';
        ELSE
            RAISE NOTICE '❌ TESTE FALHOU: Sincronização não aconteceu';
        END IF;
    ELSE
        RAISE NOTICE '❌ TESTE FALHOU: Evento não foi criado';
    END IF;

    -- Cleanup
    DELETE FROM origens_atendimento WHERE id = v_origem_id;
    DELETE FROM itens_venda WHERE id = v_item_id;
    DELETE FROM vendas WHERE id = v_venda_id;
    DELETE FROM eventos_sistema WHERE entidade_origem = 'vendas' AND id_entidade_origem = v_venda_id;
END $$;
```

---

## 8. **Conclusão e Benefícios**

### 8.1 Problemas Resolvidos

#### ✅ **Controle de Concorrência**
- **Detecção Automática**: Conflitos detectados no momento do save
- **Resolução Guiada**: Interface para resolver conflitos
- **Integridade Garantida**: Impossível perder atualizações silenciosamente
- **Auditoria Completa**: Histórico de todas as mudanças

#### ✅ **Sincronização de Dados**
- **Automática**: Eventos disparados automaticamente por triggers
- **Eventual Consistency**: Dados sincronizados assincronamente
- **Resiliente**: Tratamento de erros e retry automático
- **Rastreável**: Log completo de todos os eventos

### 8.2 Métricas de Melhoria

| **Problema** | **Antes (Schema Atual)** | **Depois (Nova Solução)** |
|--------------|---------------------------|----------------------------|
| **Lost Updates** | 🔴 Frequentes e silenciosos | 🟢 Impossíveis (detectados automaticamente) |
| **Data Inconsistency** | 🔴 Manual para detectar | 🟢 Prevenção automática com triggers |
| **Status Desync** | 🔴 Comum e difícil de corrigir | 🟢 Sincronização automática em tempo real |
| **Orphaned References** | 🔴 Foreign keys quebradas | 🟢 Cascade constraints + eventos |
| **Debug Complexity** | 🔴 Extremamente difícil | 🟢 Log de eventos rastreável |
| **Performance Impact** | 🔴 Queries lentas para validar | 🟢 Triggers otimizados + processamento assíncrono |

### 8.3 Implementação Recomendada

#### **Fase 1: Fundação**
1. Implementar sistema de versionamento em todas as tabelas
2. Criar sistema básico de eventos
3. Implementar classe base C++ com optimistic locking

#### **Fase 2: Triggers e Sincronização**
4. Implementar triggers para entidades críticas (vendas, compras)
5. Criar processadores de eventos principais
6. Implementar validações de estado

#### **Fase 3: Interface de Usuário**
7. Criar diálogos de resolução de conflitos
8. Implementar notificações de sincronização
9. Adicionar dashboards de monitoramento

#### **Fase 4: Monitoramento e Otimização**
10. Implementar métricas de performance
11. Otimizar processamento de eventos
12. Adicionar alertas automáticos

**🚀 Com estas soluções, o ERP terá controle de concorrência robusto e sincronização de dados automática, eliminando os dois problemas críticos identificados!**

---

*Documento técnico para controle de concorrência e sincronização - ERP Staccato 2025*