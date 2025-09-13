# Problema de Lock Prolongado - ImportaProdutos

## Descrição do Problema

A classe `ImportaProdutos` mantém uma transação de banco de dados aberta durante **todo o tempo** em que a janela está aberta, causando locks desnecessários na tabela `produto`. Isso impede outros usuários/processos de acessarem a tabela enquanto a importação está sendo configurada.

## ⚠️ DESCOBERTA: Por Que a Transação É Necessária

Após análise completa do código, descobri que **a transação longa não é apenas por descuido**, mas sim **tecnicamente necessária** devido a 4 motivos específicos:

## Localização do Problema

### Início da Transação
```cpp
// src/importaprodutos.cpp:46
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        qApp->startTransaction("ImportaProdutos::importaTabela"); // ← PROBLEMA: Transação inicia aqui
        
        processarArquivo();
    } catch (std::exception &) {
        close();
        throw;
    }
}
```

### Fim da Transação
```cpp
// src/importaprodutos.cpp:984
void ImportaProdutos::on_pushButtonSalvar_clicked() {
    // ... código de validação ...
    
    try {
        salvar();
    } catch (std::exception &) {
        close();
        throw;
    }

    qApp->endTransaction(); // ← PROBLEMA: Transação só termina aqui
    
    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}
```

### Rollback no Fechamento
```cpp
// src/importaprodutos.cpp:1014-1018
void ImportaProdutos::closeEvent(QCloseEvent *event) {
    if (qApp->getInTransaction()) { 
        qApp->rollbackTransaction(""); // ← Rollback se janela for fechada
    }
    
    QDialog::closeEvent(event);
}
```

## Impacto do Problema

1. **Concorrência Bloqueada**: Outros usuários não conseguem modificar produtos enquanto ImportaProdutos está aberto
2. **Deadlocks Potenciais**: Pode causar deadlocks com outras operações simultâneas
3. **Performance do Sistema**: Degrada performance geral do banco de dados
4. **Timeout de Transações**: Pode exceder timeouts configurados no MySQL/MariaDB

## Cenário Problemático

```
1. Usuário abre ImportaProdutos                    → Transação INICIA
2. Usuário seleciona arquivo Excel                 → Transação ATIVA
3. Usuário configura validade                      → Transação ATIVA  
4. Sistema processa arquivo (pode demorar minutos) → Transação ATIVA
5. Usuário revisa dados na tela                    → Transação ATIVA
6. Usuário clica "Salvar" ou fecha janela          → Transação FINALIZA
```

**Tempo total com lock: Pode ser de 10+ minutos para importações grandes**

## Operações Afetadas Durante o Lock

Durante o período de transação ativa, as seguintes operações ficam bloqueadas:

- Outras importações de produtos
- Atualizações de preços
- Cadastro manual de produtos
- Operações de descontinuação em massa
- Relatórios que acessam tabela produto
- Sincronizações automáticas

## Motivos Técnicos para a Transação Longa

### **1. Operações SQL Diretas Durante o Processamento**

#### **a) Inserção/Atualização de Fornecedores**
**Localização**: `buscarCadastrarFornecedor()` (linha 931-934) e `cadastraFornecedores()` (linha 383-387)

```cpp
// ❌ INSERÇÃO de novos fornecedores durante processamento
if (not queryFornecedor.first()) {
    queryFornecedor.prepare("INSERT INTO fornecedor (razaoSocial) VALUES (:razaoSocial)");
    queryFornecedor.bindValue(":razaoSocial", m_fornecedor);
    if (not queryFornecedor.exec()) { throw RuntimeException("Erro cadastrando fornecedor: " + queryFornecedor.lastError().text()); }
    return queryFornecedor.lastInsertId().toInt();
}

// ❌ ATUALIZAÇÃO da validade dos produtos para cada fornecedor
SqlQuery queryFornecedor;
queryFornecedor.prepare("UPDATE fornecedor SET validadeProdutos = :validade WHERE razaoSocial = :razaoSocial");
queryFornecedor.bindValue(":validade", (validade == -1) ? QVariant() : qApp->serverDate().addDays(validade));
queryFornecedor.bindValue(":razaoSocial", fornecedor);
if (not queryFornecedor.exec()) { throw RuntimeException("Erro salvando validade: " + queryFornecedor.lastError().text()); }
```

#### **b) Marcação Massiva de Produtos como Descontinuados**
**Localização**: `marcaTodosProdutosDescontinuados()` (linha 404-406)

```cpp
// ❌ UPDATE MASSIVO que pode afetar milhares de produtos
if (not query.exec("UPDATE produto SET descontinuado = TRUE WHERE idFornecedor IN (" + idsFornecedor + ") AND estoque = FALSE AND promocao = " + QString::number(static_cast<int>(tipo)))) {
    throw RuntimeException("Erro marcando produtos descontinuados: " + query.lastError().text());
}
```

#### **c) Atualização de Representação (pode ser chamada durante processamento)**
**Localização**: `on_checkBoxRepresentacao_toggled()` (linha 1025-1027)

```cpp
// ❌ UPDATE na tabela fornecedor durante interface
if (not query.exec("UPDATE fornecedor SET representacao = " + QString(checked ? "TRUE" : "FALSE") + " WHERE idFornecedor IN (" + idsFornecedor + ")")) {
    throw RuntimeException("Erro guardando 'Representacao' em Fornecedor: " + query.lastError().text());
}
```

### **2. SqlTableModel com OnManualSubmit**

**Localização**: `sqltablemodel.cpp:165`

```cpp
void SqlTableModel::setTable(const QString &tableName) {
    QSqlTableModel::setTable(tableName);
    setEditStrategy(QSqlTableModel::OnManualSubmit); // ← PROBLEMA CRÍTICO
    setFilter("0");
}
```

**Implicações**:
- ❌ Todas as operações `insertRow()` e `setData()` ficam **apenas em cache**
- ❌ Milhares de mudanças ficam "pendentes" na memória durante o processamento  
- ❌ Só são commitadas quando `submitAll()` é chamado no final
- ❌ Se a transação for interrompida, **TODAS** essas mudanças são perdidas

### **3. Dependências Relacionais Críticas**

#### **a) Dependência Fornecedor → Produto**
```cpp
// Produtos dependem dos IDs de fornecedores que podem ser criados durante o processamento
const int idFornecedor = buscarCadastrarFornecedor(); // ❌ Pode criar fornecedor
produto.idFornecedor = idFornecedor; // ❌ Produto usa esse ID
```

#### **b) Lógica de Descontinuação Complexa**
```cpp
// 1. PRIMEIRO: Marca TODOS os produtos como descontinuados
marcaTodosProdutosDescontinuados(); 

// 2. DEPOIS: Durante processamento, desmarca apenas os que estão na importação
marcaProdutoNaoDescontinuado(row); // linha 726-730
```

**❌ Se não houver transação**: Produtos ficariam incorretamente marcados como descontinuados se o processo falhar no meio.

### **4. Necessidade de Rollback Atômico Complexo**

**Cenário de falha sem transação longa**:
1. ✅ Novos fornecedores são inseridos
2. ✅ Validade é atualizada na tabela fornecedor  
3. ✅ Produtos são marcados como descontinuados
4. ✅ 30.000 produtos são processados (ficam em cache do model)
5. ❌ **ERRO** no produto 30.001
6. 🚨 **ESTADO INCONSISTENTE**:
   - Fornecedores ficam cadastrados desnecessariamente
   - Produtos ficam descontinuados incorretamente
   - Cache do model é perdido
   - Sistema fica em estado corrompido

**✅ Com transação longa**:
- Rollback desfaz **TODAS** as operações atomicamente
- Estado volta exatamente como estava antes
- Não há corrupção de dados

## Soluções Propostas

### ❌ Solução 1: Transação Apenas no Salvamento (NÃO FUNCIONARÁ)

~~Mover o início da transação para imediatamente antes do salvamento~~ - **Esta solução não é viável** devido aos motivos técnicos apresentados acima.

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // ✅ REMOVER: qApp->startTransaction("ImportaProdutos::importaTabela");
        
        processarArquivo(); // Sem transação - apenas leitura e preparação
    } catch (std::exception &) {
        close();
        throw;
    }
}

void ImportaProdutos::on_pushButtonSalvar_clicked() {
    if (modelErro.rowCount() > 0) {
        QMessageBox msgBox(QMessageBox::Question, "Atenção!", 
                          "Produtos com erro não serão salvos. Deseja continuar?", 
                          QMessageBox::Yes | QMessageBox::No, this);
        msgBox.button(QMessageBox::Yes)->setText("Continuar");
        msgBox.button(QMessageBox::No)->setText("Voltar");

        if (msgBox.exec() == QMessageBox::No) { return; }
    }

    try {
        qApp->startTransaction("ImportaProdutos::salvar"); // ✅ TRANSAÇÃO INICIA AQUI
        salvar();
        qApp->endTransaction(); // ✅ TRANSAÇÃO TERMINA AQUI
    } catch (std::exception &) {
        qApp->rollbackTransaction(""); // ✅ ROLLBACK EM CASO DE ERRO
        close();
        throw;
    }

    qApp->enqueueInformation("Tabela salva com sucesso!", this);
    close();
}

void ImportaProdutos::closeEvent(QCloseEvent *event) {
    // ✅ REMOVER: Não há mais transação para fazer rollback
    QDialog::closeEvent(event);
}
```

### ✅ Solução 1: Transação com Timeout Configurável (Paliativo)

Se manter transação longa for necessário, configurar timeout apropriado para reduzir deadlocks:

```cpp
void ImportaProdutos::importarTabela() {
    try {
        if (not readFile() or not readValidade()) {
            close();
            return;
        }

        // Configurar timeout maior para importações
        SqlQuery timeoutQuery;
        timeoutQuery.exec("SET SESSION innodb_lock_wait_timeout = 1800"); // 30 minutos
        timeoutQuery.exec("SET SESSION lock_wait_timeout = 1800");
        
        qApp->startTransaction("ImportaProdutos::importaTabela");
        
        processarArquivo();
    } catch (std::exception &) {
        close();
        throw;
    }
}
```

### ⚠️ Solução 2: Transações Menores com Savepoints (Complexa)

Quebrar em múltiplas transações menores:

```cpp
void ImportaProdutos::processarArquivoComTransacoesMenores() {
    // Transação 1: Fornecedores (rápida)
    qApp->startTransaction("ImportaProdutos::fornecedores");
    try {
        cadastraFornecedores(xlsx);
        verificaSeRepresentacao();
        qApp->endTransaction();
    } catch (...) {
        qApp->rollbackTransaction("");
        throw;
    }

    // Transação 2: Descontinuação (rápida) 
    qApp->startTransaction("ImportaProdutos::descontinuacao");
    try {
        marcaTodosProdutosDescontinuados();
        qApp->endTransaction();
    } catch (...) {
        qApp->rollbackTransaction("");
        throw;
    }

    // Processamento sem transação (apenas cache)
    mostraApenasEstesFornecedores();
    // ... processamento de produtos em cache ...

    // Transação 3: Commit final (média)
    qApp->startTransaction("ImportaProdutos::produtos");
    try {
        modelProduto.submitAll();
        salvarPrecos();
        atualizaPrecoEstoque();
        qApp->endTransaction();
    } catch (...) {
        qApp->rollbackTransaction("");
        // PROBLEMA: Como desfazer transações anteriores?
        throw;
    }
}
```

**⚠️ Problema**: Se transação final falhar, como reverter transações anteriores já commitadas?

### ⚠️ Solução 3: Mudar EditStrategy para OnRowChange (Muito Complexa)

```cpp
// Em SqlTableModel::setTable()
setEditStrategy(QSqlTableModel::OnRowChange); // Em vez de OnManualSubmit

// Implicações:
// ✅ Cada produto é salvo imediatamente
// ✅ Transação pode ser menor
// ❌ MUITO MAIS LENTO (commit por linha)
// ❌ Pode quebrar lógica de rollback 
// ❌ Requer refatoração massiva do código
```

### 🚀 Solução 4: Sistema de Compensação (Avançada)

```cpp
class ImportCompensationManager {
private:
    struct UndoOperation {
        QString table;
        QString operation; // INSERT, UPDATE, DELETE  
        QMap<QString, QVariant> oldValues;
        QMap<QString, QVariant> newValues;
        QString whereClause;
    };
    
    QVector<UndoOperation> undoLog;

public:
    void logInsert(const QString& table, const QMap<QString, QVariant>& values) {
        UndoOperation undo;
        undo.table = table;
        undo.operation = "DELETE";
        undo.newValues = values;
        undoLog.append(undo);
    }
    
    void logUpdate(const QString& table, const QString& where, 
                   const QMap<QString, QVariant>& oldVals,
                   const QMap<QString, QVariant>& newVals) {
        UndoOperation undo;
        undo.table = table;
        undo.operation = "UPDATE";
        undo.whereClause = where;
        undo.oldValues = oldVals;
        undo.newValues = newVals;
        undoLog.append(undo);
    }
    
    void rollbackAll() {
        // Reverter operações na ordem inversa
        for (auto it = undoLog.rbegin(); it != undoLog.rend(); ++it) {
            executeUndo(*it);
        }
    }
};

void ImportaProdutos::processarArquivoComCompensacao() {
    ImportCompensationManager compensator;
    
    try {
        // Operações sem transação longa, mas com log de compensação
        cadastraFornecedoresComLog(xlsx, compensator);
        marcaDescontinuadosComLog(compensator);
        processaProdutosComLog(compensator);
    } catch (...) {
        // Rollback manual usando compensação
        compensator.rollbackAll();
        throw;
    }
}
```

## Implementação Recomendada

**Situação Atual**: A transação longa é **tecnicamente necessária** e **funcionalmente correta**.

**Recomendação por Prioridade**:

1. **✅ Curto Prazo - Solução 1**: Implementar timeouts configuráveis para reduzir impacto de deadlocks
2. **⚠️ Médio Prazo**: Considerar Solução 2 (transações menores) se concorrência for crítica
3. **🚀 Longo Prazo**: Refatoração completa da arquitetura (Solução 4) para eliminar dependência de transação longa

**Motivo**: As soluções alternativas são muito complexas e arriscadas. A arquitetura atual funciona corretamente, apenas com impacto de concorrência.

## Conclusão

**A transação longa NÃO é um erro de design**, mas sim uma **necessidade técnica** devido à complexidade arquitetural do sistema:

- ✅ **Operações SQL diretas** durante processamento exigem transação
- ✅ **OnManualSubmit** mantém dados em cache que precisam de rollback atômico  
- ✅ **Dependências relacionais** entre fornecedores e produtos
- ✅ **Lógica de negócio complexa** (descontinuação/reativação) requer atomicidade

**Trade-off Atual**:
- ✅ **Consistência de Dados**: Garantida 100%
- ✅ **Integridade Transacional**: Perfeita
- ❌ **Concorrência**: Limitada durante importações

## Testes Necessários

Para qualquer solução implementada:

1. **Teste de Concorrência**: Abrir múltiplas janelas ImportaProdutos simultaneamente
2. **Teste de Erro**: Forçar erro em diferentes pontos e verificar rollback completo
3. **Teste de Cancelamento**: Fechar janela durante processamento
4. **Teste de Timeout**: Verificar comportamento com timeout configurado
5. **Teste de Integridade**: Verificar que fornecedores/produtos ficam consistentes
6. **Teste de Performance**: Medir tempo de lock e impacto em outras operações