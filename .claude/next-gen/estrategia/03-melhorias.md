# Melhorias de Fluxo e Schema

> Status: **Brainstorming**
> Última atualização: 2025-12-27
> Propósito: Identificar pontos de dor e oportunidades de melhoria para a migração web

---

## Sumário

1. [Pontos de Dor Atuais](#1-pontos-de-dor-atuais)
2. [Opções de Melhoria](#2-opções-de-melhoria)
3. [Novas Capacidades](#3-novas-capacidades)
4. [Matriz de Prioridade](#4-matriz-de-prioridade)
5. [Decisões Necessárias](#5-decisões-necessárias)

---

## 1. Pontos de Dor Atuais

### 1.1 Tabelas de Dois Níveis (Arquitetura L1/L2)

**Problema**: O sistema usa tabelas pareadas para vendas e compras:

- `venda_has_produto` (L1) + `venda_has_produto2` (L2)
- `pedido_fornecedor_has_produto` (L1) + `pedido_fornecedor_has_produto2` (L2)

**Problemas**:

- Difícil raciocinar sobre qual nível consultar
- Problemas de sincronização entre níveis
- Triggers de banco de dados necessários para manter consistência
- JOINs complexos para queries simples
- Links auto-referenciais `idRelacionado` para splits adicionam mais complexidade

**Propósito Atual**:

| Aspecto       | Nível 1          | Nível 2                       |
| ------------- | ---------------- | ----------------------------- |
| Propósito     | O que foi pedido | Como está sendo atendido      |
| Granularidade | Agregado         | Por-entrega/por-NFe           |
| Status        | Status do pedido | Status de atendimento do item |

**Causa Raiz**: Projetado para lidar com entregas parciais e divisões de pedidos, mas a implementação ficou complexa.

---

### 1.2 FIFO Não Implementado Corretamente

**Problema**: Consumo de estoque não segue First-In-First-Out corretamente.

**Código Atual** (simplificado):

```cpp
// Apenas pega qualquer idEstoque pré-definido no produto
query.exec("SELECT * FROM estoque WHERE idEstoque = " + produto.idEstoque);
```

**Deveria Ser**:

```sql
SELECT * FROM estoque
WHERE produto_id = :produto_id
  AND quantidade_disponivel > 0
ORDER BY data_entrada ASC  -- FIFO: mais antigo primeiro
LIMIT 1
```

**Impacto**:

- Valoração de estoque incorreta
- Estoque mais antigo pode nunca ser consumido
- Problemas de auditoria/compliance para produtos perecíveis

---

### 1.3 Nomes de Fornecedor Desnormalizados

**Problema**: Nome do fornecedor armazenado como VARCHAR em múltiplas tabelas ao invés de FK.

**Tabelas Afetadas**:

| Tabela                           | Coluna       |
| -------------------------------- | ------------ |
| `venda_has_produto2`             | `fornecedor` |
| `estoque`                        | `fornecedor` |
| `estoque_has_consumo`            | `fornecedor` |
| `compra_avulsa`                  | `fornecedor` |
| `pedido_fornecedor_has_produto2` | `fornecedor` |

**Impacto**:

- Se fornecedor muda de nome, precisa atualizar 5+ tabelas
- Sem integridade referencial
- Dados inconsistentes possíveis (erros de digitação, variações)
- Não consegue facilmente consultar "todas as transações do fornecedor X"

---

### 1.4 Fluxo de Devolução Incompleto

**Problema**: O fluxo de devoluções tem múltiplos bugs e funcionalidades faltando.

**Problemas Encontrados**:

1. **Sem NFe Devolução automática**: Deveria gerar nota de devolução para fornecedor
2. **Registros financeiros errados**: Marcado como `RECEBIDO` imediatamente ao invés de pendente
3. **Observação vazia**: Nenhum motivo capturado para a devolução
4. **Estoque não revertido corretamente**: Manipulação do campo `restante` é frágil
5. **Reversão de comissão incompleta**: Lógica de clawback de RT tem casos de borda

**Evidência no Código**:

```cpp
// De devolucao.cpp - TODOs encontrados
// TODO: gerar NFe de devolução
// TODO: verificar se precisa estornar financeiro
```

---

### 1.5 Status como Strings Mágicas

**Problema**: Valores de status são strings hardcoded em todo o codebase.

**Exemplos**:

```cpp
if (status == "PENDENTE") ...
if (status == "EM ENTREGA") ...
if (status == "PEND. APROV.") ...
```

**Problemas**:

- Erros de digitação causam bugs silenciosos
- Sem verificação em tempo de compilação
- Conjuntos de status diferentes para tabelas diferentes (inconsistente)
- Difícil encontrar todos os lugares que verificam um status

**Variações de Status Encontradas**:

| Tabela                           | Status                                                             |
| -------------------------------- | ------------------------------------------------------------------ |
| `venda_has_produto2`             | PENDENTE, ESTOQUE, ENTREGA AGEND., EM ENTREGA, ENTREGUE, DEVOLVIDO |
| `pedido_fornecedor_has_produto2` | PENDENTE, CONFIRMADO, FATURADO, EM COLETA, EM RECEBIMENTO, ESTOQUE |
| `conta_a_pagar`                  | PENDENTE, CONFERIDO, AGENDADO, PAGO, CANCELADO                     |
| `nfe`                            | NOTA PENDENTE, AUTORIZADA, CANCELADA, DENEGADA                     |

---

### 1.6 Mega-Tabela: `produto` (100+ Colunas)

**Problema**: A tabela `produto` cresceu para 100+ colunas misturando diferentes preocupações.

**Categorias de Colunas**:

| Categoria                    | Colunas de Exemplo                        | Quantidade |
| ---------------------------- | ----------------------------------------- | ---------- |
| Dados principais             | descricao, codComercial, codBarras        | ~10        |
| Preços                       | custo, precoVenda, markup, oldPrecoVenda  | ~8         |
| Impostos                     | ncm, cst, icms, st, mva, ipi, pis, cofins | ~15        |
| Reforma Tributária (IBS/CBS) | cClassTribIBSCBS, pAliqEfetIBSUF, etc.    | ~35        |
| Estoque                      | estoqueRestante, quantCaixa, temLote      | ~5         |
| Flags de rastreamento        | colunas \*Upd para cada campo             | ~20+       |
| Dimensões                    | m2, altura, largura, profundidade         | ~5         |

**Problemas**:

- Difícil de manter
- Muitas colunas NULL para campos irrelevantes
- Colunas de impostos mudam com legislação (reforma 2025 adicionou 35 colunas)
- Flags de rastreamento poluem a tabela

---

### 1.7 Sem Trilha de Auditoria

**Problema**: Rastreamento limitado de quem mudou o quê e quando.

**Estado Atual**:

- Algumas flags booleanas `*Upd` existem
- Sem rastreamento de usuário
- Sem timestamp das mudanças
- Sem preservação de valor antigo

**Impacto**:

- Não consegue responder "quem mudou este preço?"
- Não consegue reconstruir estado histórico
- Problemas de compliance/auditoria

---

### 1.8 Senhas Fracas

**Problema**: Muitos usuários têm senhas fracas como `1234`, `senha`, `123456`.

**Riscos**:
- Acesso não autorizado ao sistema
- Comprometimento de dados sensíveis (financeiro, clientes)
- Sem política de senha mínima no sistema atual

**Evidência**: Senhas comuns encontradas em produção incluem variações de `1234`, nomes próprios, e palavras simples.

---

### 1.9 Complexidade de Tabelas de Junção

**Problema**: Múltiplas tabelas de junção com propósitos sobrepostos.

**Tabelas de Junção Atuais**:

```text
estoque_has_compra      - Vincula estoque ao pedido de compra
estoque_has_consumo     - Vincula estoque ao pedido de venda
conta_a_pagar_has_idcompra - Vincula pagamento a compra
veiculo_has_produto     - Vincula entrega a produtos
```

**Problema**: Difícil rastrear a cadeia completa de pedido do cliente → compra → NFe → estoque → entrega.

---

## 2. Opções de Melhoria

### 2.1 Simplificar Tabelas L1/L2

#### Opção A: Achatar para Tabela Única

```sql
CREATE TABLE venda_itens (
    id SERIAL PRIMARY KEY,
    venda_id INTEGER REFERENCES vendas(id),
    parent_item_id INTEGER REFERENCES venda_itens(id), -- Para splits
    produto_id INTEGER,
    quantidade_pedida DECIMAL,
    quantidade_entregue DECIMAL,
    status venda_item_status,
    -- Todos os outros campos...
);
```

- Prós: Queries mais simples, sem problemas de sincronização
- Contras: Precisa lidar com splits via auto-referência

#### Opção B: Manter Apenas L2, Derivar L1

```sql
-- L2 é a fonte da verdade
CREATE TABLE venda_itens (...);

-- L1 é uma view/materialized view
CREATE VIEW venda_itens_agregado AS
SELECT venda_id, produto_id, SUM(quantidade) as total
FROM venda_itens
GROUP BY venda_id, produto_id;
```

- Prós: Fonte única da verdade, L1 sempre consistente
- Contras: Overhead de agregação

#### Opção C: Event Sourcing

```sql
CREATE TABLE venda_item_events (
    id SERIAL,
    venda_item_id INTEGER,
    event_type VARCHAR, -- CREATED, SPLIT, DELIVERED, RETURNED
    payload JSONB,
    created_at TIMESTAMP
);
```

- Prós: Histórico completo, pode reproduzir estado
- Contras: Mais complexo, precisa CQRS

---

### 2.2 Corrigir Consumo FIFO

**Correção Simples**:

```sql
-- Query FIFO correta
SELECT id, quantidade_disponivel
FROM estoques
WHERE produto_id = :produto_id
  AND loja_id = :loja_id
  AND quantidade_disponivel > 0
ORDER BY data_entrada ASC
FOR UPDATE;  -- Travar para consumo
```

#### Melhor: Serviço de Consumo

```php
class EstoqueConsumoService
{
    public function consumir(int $produtoId, float $quantidade): Collection
    {
        $consumidos = collect();
        $restante = $quantidade;

        $estoques = Estoque::where('produto_id', $produtoId)
            ->where('quantidade_disponivel', '>', 0)
            ->orderBy('data_entrada', 'asc')  // FIFO
            ->lockForUpdate()
            ->get();

        foreach ($estoques as $estoque) {
            if ($restante <= 0) break;

            $consumir = min($restante, $estoque->quantidade_disponivel);
            $estoque->decrement('quantidade_disponivel', $consumir);

            $consumidos->push([
                'estoque_id' => $estoque->id,
                'quantidade' => $consumir,
                'custo' => $estoque->custo_unitario,
            ]);

            $restante -= $consumir;
        }

        return $consumidos;
    }
}
```

---

### 2.3 Normalizar Referências de Fornecedor

**Migração**:

```sql
-- Adicionar colunas FK
ALTER TABLE venda_has_produto2 ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);
ALTER TABLE estoque ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);
-- etc.

-- Popular a partir de nomes existentes
UPDATE venda_has_produto2 v
SET fornecedor_id = f.id
FROM fornecedores f
WHERE v.fornecedor = f.razao_social;

-- Eventualmente remover colunas VARCHAR
ALTER TABLE venda_has_produto2 DROP COLUMN fornecedor;
```

---

### 2.4 Corrigir Fluxo de Devoluções

**Fluxo Completo de Devoluções**:

```text
1. Usuário inicia devolução
   +-- Capturar motivo (observação obrigatória)
   +-- Validar quantidades
   +-- Verificar se está dentro do prazo de devolução

2. Reversão de estoque
   +-- Criar registro de consumo negativo
   +-- Atualizar estoque.quantidade_disponivel
   +-- Registrar qual consumo original está sendo revertido

3. Reversão financeira
   +-- Criar nota de crédito (conta_a_receber com valor negativo)
   +-- NÃO marcar como RECEBIDO imediatamente
   +-- Vincular aos registros de pagamento originais

4. NFe Devolução
   +-- Gerar XML de NFe de devolução
   +-- Referenciar NFe original (chave)
   +-- Submeter ao SEFAZ

5. Reversão de comissão
   +-- Calcular clawback proporcional de RT
   +-- Criar conta_a_pagar para vendedor
   +-- Agendar para próximo ciclo de pagamento
```

---

### 2.5 Dividir Tabela Produto

**Estrutura Proposta**:

```sql
-- Apenas dados principais do produto
CREATE TABLE produtos (
    id SERIAL PRIMARY KEY,
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    cod_comercial VARCHAR(100),
    descricao VARCHAR(500),
    unidade VARCHAR(10),
    ativo BOOLEAN DEFAULT true
);

-- Preços versionados
CREATE TABLE produto_precos (
    id SERIAL PRIMARY KEY,
    produto_id INTEGER REFERENCES produtos(id),
    custo DECIMAL(15,2),
    preco_venda DECIMAL(15,2),
    vigencia_inicio DATE,
    vigencia_fim DATE
);

-- Configuração de impostos (JSONB para flexibilidade)
CREATE TABLE produto_tributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id),
    ncm_id INTEGER REFERENCES ncms(id),
    config JSONB  -- {icms: {...}, ipi: {...}, ibs: {...}}
);

-- Atributos flexíveis
CREATE TABLE produto_atributos (
    produto_id INTEGER PRIMARY KEY REFERENCES produtos(id),
    atributos JSONB  -- {m2: 1.5, cor: "branco", ...}
);
```

---

### 2.6 Redesenhar Tratamento de Status

**ENUMs PostgreSQL**:

```sql
CREATE TYPE venda_item_status AS ENUM (
    'PENDENTE',
    'ESTOQUE',
    'ENTREGA_AGENDADA',
    'EM_ENTREGA',
    'ENTREGUE',
    'DEVOLVIDO',
    'CANCELADO'
);

CREATE TYPE compra_status AS ENUM (
    'PENDENTE',
    'CONFIRMADO',
    'FATURADO',
    'EM_COLETA',
    'EM_RECEBIMENTO',
    'RECEBIDO',
    'CANCELADO'
);
```

**Enums PHP**:

```php
enum VendaItemStatus: string
{
    case PENDENTE = 'PENDENTE';
    case ESTOQUE = 'ESTOQUE';
    case ENTREGA_AGENDADA = 'ENTREGA_AGENDADA';
    // ...

    public function canTransitionTo(self $new): bool
    {
        return match($this) {
            self::PENDENTE => in_array($new, [self::ESTOQUE, self::CANCELADO]),
            self::ESTOQUE => in_array($new, [self::ENTREGA_AGENDADA, self::CANCELADO]),
            // ...
        };
    }
}
```

---

### 2.7 Implementar Regras de Senha

**Requisitos Mínimos**:

| Regra | Requisito |
|-------|-----------|
| Tamanho mínimo | 8 caracteres |
| Maiúscula | Pelo menos 1 |
| Minúscula | Pelo menos 1 |
| Número | Pelo menos 1 |
| Especial | Opcional |
| Senhas comuns | Bloqueadas |

**Lista de Senhas Bloqueadas**:

```php
$blockedPasswords = [
    '12345678', '123456789', '1234567890',
    'password', 'senha', 'senha123',
    'qwerty', 'qwertyui', 'qwerty123',
    'admin', 'admin123', 'administrator',
    'staccato', 'staccato123',  // nome da empresa
];
```

**Implementação Laravel**:

```php
// app/Rules/StrongPassword.php
class StrongPassword implements ValidationRule
{
    public function validate(string $attribute, mixed $value, Closure $fail): void
    {
        if (strlen($value) < 8) {
            $fail('A senha deve ter no mínimo 8 caracteres.');
            return;
        }

        if (!preg_match('/[A-Z]/', $value)) {
            $fail('A senha deve conter pelo menos uma letra maiúscula.');
            return;
        }

        if (!preg_match('/[a-z]/', $value)) {
            $fail('A senha deve conter pelo menos uma letra minúscula.');
            return;
        }

        if (!preg_match('/[0-9]/', $value)) {
            $fail('A senha deve conter pelo menos um número.');
            return;
        }

        if ($this->isCommonPassword($value)) {
            $fail('Esta senha é muito comum. Escolha uma senha mais forte.');
            return;
        }
    }
}
```

**Validação com Laravel Password**:

```php
use Illuminate\Validation\Rules\Password;

$request->validate([
    'password' => [
        'required',
        'confirmed',
        Password::min(8)
            ->mixedCase()
            ->numbers()
            ->uncompromised(),  // verifica no HaveIBeenPwned
    ],
]);
```

**Migração de Usuários Existentes**:

```php
// Forçar troca de senha no próximo login
Schema::table('usuarios', function (Blueprint $table) {
    $table->boolean('must_change_password')->default(false);
    $table->timestamp('password_changed_at')->nullable();
});
```

**Middleware para Forçar Troca**:

```php
class EnsurePasswordChanged
{
    public function handle(Request $request, Closure $next)
    {
        if (auth()->user()->must_change_password) {
            return redirect()->route('password.change')
                ->with('warning', 'Você precisa alterar sua senha.');
        }
        return $next($request);
    }
}
```

**Feedback Visual (Vue)**:

```vue
<template>
  <div class="password-strength">
    <div class="bar" :style="{ width: strength + '%' }"></div>
    <span>{{ strengthText }}</span>
  </div>
</template>

<script setup>
const strength = computed(() => {
  let score = 0;
  if (password.length >= 8) score += 25;
  if (/[A-Z]/.test(password)) score += 25;
  if (/[a-z]/.test(password)) score += 25;
  if (/[0-9]/.test(password)) score += 25;
  return score;
});
</script>
```

---

## 3. Novas Capacidades

A migração habilita funcionalidades não possíveis no sistema atual:

### 3.1 Trilha de Auditoria Adequada

- Rastrear todas as mudanças com usuário, timestamp, valores antigos/novos
- Consultar estado histórico em qualquer ponto no tempo
- Log pronto para compliance

### 3.2 Queries Temporais

- "Qual era o nível de estoque em 31 de dezembro?"
- "Qual era o preço do produto X no mês passado?"
- Relatórios point-in-time para auditorias

### 3.3 Busca Melhor

- Full-text search do PostgreSQL com stemming em português
- Correspondência fuzzy para nomes de produtos
- Busca facetada (por fornecedor, categoria, faixa de preço)

### 3.4 Atualizações em Tempo Real

- Notificações WebSocket para mudanças de status
- Atualizações de dashboard ao vivo
- Colaboração multi-usuário sem refresh

### 3.5 Design API-First

- Possibilidade de app mobile
- Integrações com terceiros
- Notificações via webhook

### 3.6 Processamento em Background

- Processamento de NFe em filas
- Geração de CNAB assíncrona
- Geração de relatórios sem bloquear UI

---

## 4. Matriz de Prioridade

| #   | Melhoria                        | Impacto | Complexidade | Prioridade |
| --- | ------------------------------- | ------- | ------------ | ---------- |
| 1   | Corrigir consumo FIFO           | Médio   | Baixa        | **Alta**   |
| 2   | Normalizar refs de fornecedor   | Médio   | Média        | **Alta**   |
| 3   | Redesenhar tratamento de status | Baixo   | Baixa        | **Alta**   |
| 4   | Adicionar trilha de auditoria   | Alto    | Média        | **Alta**   |
| 5   | **Regras de senha**             | Alto    | Baixa        | **Alta**   |
| 6   | Corrigir fluxo de devoluções    | Médio   | Média        | **Média**  |
| 7   | Dividir tabela produto          | Médio   | Média        | **Média**  |
| 8   | Simplificar tabelas L1/L2       | Alto    | Alta         | **Média**  |
| 9   | Adicionar queries temporais     | Médio   | Alta         | **Baixa**  |

### Ordem Recomendada

**Fase 1 - Ganhos Rápidos** (implementar imediatamente):

- Correção FIFO
- ENUMs de status
- Normalização FK de fornecedor
- Regras de senha (segurança)

**Fase 2 - Fundação** (durante migração):

- Trilha de auditoria
- Dividir tabela produto
- Correção de fluxo de devoluções

**Fase 3 - Arquitetura** (planejamento cuidadoso):

- Simplificação L1/L2
- Queries temporais

---

## 5. Decisões Necessárias

### Decisão 1: Estratégia L1/L2

- [ ] Opção A: Achatar para tabela única
- [ ] Opção B: Manter apenas L2, derivar L1
- [ ] Opção C: Event sourcing
- [ ] Opção D: Manter estrutura atual (apenas limpar)

### Decisão 2: Método de Consumo de Estoque

- [ ] FIFO simples por data
- [ ] FIFO por data + localização (bloco do galpão)
- [ ] Configurável (FIFO/LIFO/lote específico)

### Decisão 3: Escopo da Trilha de Auditoria

- [ ] Todas as tabelas
- [ ] Apenas tabelas críticas (vendas, compras, estoque, financeiro)
- [ ] Configurável por tabela

### Decisão 4: NFe de Devoluções

- [ ] Gerar automaticamente na confirmação de devolução
- [ ] Geração manual com dados pré-preenchidos
- [ ] Opcional (algumas devoluções podem não precisar de NFe)

---

## Documentos Relacionados

- [negocios/02-fluxos-estoque.md](../negocios/02-fluxos-estoque.md) - Análise de fluxo de estoque atual
- [negocios/01-visao-geral-fluxos.md](../negocios/01-visao-geral-fluxos.md) - Documentação completa de fluxos
- [tecnico/02-banco-dados.md](../tecnico/02-banco-dados.md) - Proposta de schema de banco de dados
- [tecnico/04-infraestrutura.md](../tecnico/04-infraestrutura.md) - Arquitetura de auditoria/temporal
