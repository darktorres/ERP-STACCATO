# Correção de Consumo de Estoque FIFO - Exploração Detalhada

> Status: **Análise**
> Última atualização: 2025-12-27
> Foco: Corrigir consumo de estoque para seguir First-In-First-Out corretamente

---

## Sumário

1. [Problema Atual](#1-problema-atual)
2. [Análise de Causa Raiz](#2-análise-de-causa-raiz)
3. [Solução Proposta](#3-solução-proposta)
4. [Detalhes de Implementação](#4-detalhes-de-implementação)
5. [Casos de Borda](#5-casos-de-borda)
6. [Estratégia de Migração](#6-estratégia-de-migração)

---

## 1. Problema Atual

### 1.1 O Que FIFO Significa

**First-In-First-Out (FIFO)**: Estoque mais antigo deve ser consumido primeiro.

```text
Recebido 01 Jan:  100 unidades @ R$10,00  ← Deve ser consumido PRIMEIRO
Recebido 15 Jan:   50 unidades @ R$12,00  ← Deve ser consumido SEGUNDO
Recebido 30 Jan:   75 unidades @ R$11,00  ← Deve ser consumido TERCEIRO
```

### 1.2 Por Que Importa

| Problema                 | Impacto                                                    |
| ------------------------ | ---------------------------------------------------------- |
| **Valoração de estoque** | CMV calculado incorretamente                               |
| **Produtos perecíveis**  | Estoque antigo nunca usado, expira                         |
| **Compliance fiscal**    | Lei tributária brasileira assume FIFO para alguns cálculos |
| **Trilha de auditoria**  | Não consegue rastrear qual lote foi vendido                |
| **Rotação de estoque**   | Estoque antigo acumula                                     |

### 1.3 Comportamento Atual

O sistema **NÃO** implementa FIFO corretamente:

```cpp
// venda.cpp:1046 - Consumo de estoque atual
query.prepare(
    "SELECT p.idEstoque, vp2.idVendaProduto2, vp2.quant "
    "FROM venda_has_produto2 vp2 "
    "LEFT JOIN produto p ON vp2.idProduto = p.idProduto "
    "WHERE vp2.idVenda = :idVenda AND vp2.estoque > 0"
);
```

**Problema**: Usa `p.idEstoque` - um ID de estoque único pré-definido armazenado na tabela `produto`.

---

## 2. Análise de Causa Raiz

### 2.1 A Coluna `produto.idEstoque`

A tabela `produto` tem uma coluna `idEstoque` que:

- Aponta para UM registro de estoque específico
- É definida manualmente (ou definida pela última importação de NFe)
- Não tem lógica FIFO

```sql
-- Atual: produto aponta para um estoque específico
produto (idProduto=123, idEstoque=456)
        +-- estoque (idEstoque=456)  -- Apenas este, independente da idade

-- Múltiplos registros de estoque existem, mas apenas um está vinculado
estoque (idEstoque=455, idProduto=123, data_entrada='2025-01-01')  -- MAIS ANTIGO, ignorado!
estoque (idEstoque=456, idProduto=123, data_entrada='2025-01-15')  -- Usado (por acaso)
estoque (idEstoque=457, idProduto=123, data_entrada='2025-01-30')  -- MAIS NOVO, ignorado!
```

### 2.2 Quando Estoque é Consumido

**Caminho 1: Via Importação de NFe** (`importarxml.cpp`)

- Estoque É corretamente vinculado a compra/venda específica
- Cria `estoque_has_consumo` com `idEstoque` específico
- Isso está OK - é determinístico

**Caminho 2: Via Venda de Estoque** (`venda.cpp:criarConsumos`)

- Usa `produto.idEstoque` (o problema!)
- Sem ORDER BY, sem seleção FIFO
- Apenas pega o que está pré-definido

### 2.3 Fluxo de Código

```text
Cliente compra produto do estoque existente:
    |
    +-- Venda criada com vp2.estoque > 0
    |
    +-- Venda::criarConsumos() chamado
    |
    +-- Query: SELECT p.idEstoque ...
    |       |
    |       +-- Obtém ÚNICO idEstoque da tabela produto
    |           (Sem consideração de FIFO!)
    |
    +-- Estoque::criarConsumo(idEstoque, quantidade)
            |
            +-- Consome daquele ÚNICO registro de estoque
```

### 2.4 Problemas Relacionados

1. **Sem seleção automática de estoque**: Usuário deve definir manualmente `produto.idEstoque`
2. **Um estoque por produto**: Não consegue facilmente consumir de múltiplos lotes
3. **Sem rastreamento de lote**: Rastreabilidade perdida para NFe/lote original

---

## 3. Solução Proposta

### 3.1 Visão Geral

Substituir `produto.idEstoque` único por **seleção FIFO dinâmica**:

```sql
-- NOVO: Query de estoque mais antigo disponível
SELECT id, quantidade_disponivel, custo_unitario, data_entrada
FROM estoques
WHERE produto_id = :produto_id
  AND loja_id = :loja_id
  AND quantidade_disponivel > 0
ORDER BY data_entrada ASC  -- FIFO: mais antigo primeiro
FOR UPDATE;  -- Travar para segurança de concorrência
```

### 3.2 Mudanças Chave

| Aspecto            | Atual                          | Proposto                      |
| ------------------ | ------------------------------ | ----------------------------- |
| Seleção de estoque | Manual via `produto.idEstoque` | FIFO automático               |
| Múltiplos lotes    | Não                            | Sim - consumir de múltiplos   |
| Travamento         | Nenhum                         | `FOR UPDATE` durante consumo  |
| Rastreabilidade    | Perdida                        | Rastreamento completo de lote |

### 3.3 Novo Schema

```sql
-- Remover produto.idEstoque (não mais necessário)
ALTER TABLE produtos DROP COLUMN idEstoque;

-- Garantir que estoque tem índices adequados
CREATE INDEX idx_estoques_fifo
    ON estoques(produto_id, loja_id, data_entrada)
    WHERE quantidade_disponivel > 0;

-- Adicionar rastreamento de lote
ALTER TABLE estoques ADD COLUMN lote VARCHAR(50);
ALTER TABLE estoques ADD COLUMN data_validade DATE;
ALTER TABLE estoques ADD COLUMN data_entrada TIMESTAMP DEFAULT NOW();
```

---

## 4. Detalhes de Implementação

### 4.1 Função PostgreSQL

```sql
CREATE OR REPLACE FUNCTION consumir_estoque_fifo(
    p_produto_id INTEGER,
    p_loja_id INTEGER,
    p_quantidade DECIMAL,
    p_venda_item_id INTEGER,
    p_motivo VARCHAR DEFAULT 'VENDA'
) RETURNS TABLE (
    estoque_id INTEGER,
    quantidade_consumida DECIMAL,
    custo_unitario DECIMAL
) AS $$
DECLARE
    v_restante DECIMAL := p_quantidade;
    v_estoque RECORD;
    v_consumir DECIMAL;
BEGIN
    -- Travar e iterar pelo estoque disponível (ordem FIFO)
    FOR v_estoque IN
        SELECT id, quantidade_disponivel, custo_unitario
        FROM estoques
        WHERE produto_id = p_produto_id
          AND loja_id = p_loja_id
          AND quantidade_disponivel > 0
        ORDER BY data_entrada ASC  -- FIFO
        FOR UPDATE
    LOOP
        EXIT WHEN v_restante <= 0;

        -- Calcular quanto tirar deste lote
        v_consumir := LEAST(v_restante, v_estoque.quantidade_disponivel);

        -- Atualizar estoque
        UPDATE estoques
        SET quantidade_disponivel = quantidade_disponivel - v_consumir,
            updated_at = NOW()
        WHERE id = v_estoque.id;

        -- Criar registro de consumo
        INSERT INTO estoque_consumos (
            estoque_id, venda_item_id, quantidade,
            custo_unitario, motivo, created_at
        ) VALUES (
            v_estoque.id, p_venda_item_id, v_consumir,
            v_estoque.custo_unitario, p_motivo, NOW()
        );

        -- Retornar info do lote consumido
        estoque_id := v_estoque.id;
        quantidade_consumida := v_consumir;
        custo_unitario := v_estoque.custo_unitario;
        RETURN NEXT;

        v_restante := v_restante - v_consumir;
    END LOOP;

    -- Verificar se atendemos o pedido inteiro
    IF v_restante > 0 THEN
        RAISE EXCEPTION 'Estoque insuficiente. Faltam % unidades', v_restante;
    END IF;
END;
$$ LANGUAGE plpgsql;
```

### 4.2 Serviço Laravel

```php
<?php

namespace App\Services\Estoque;

use App\Models\Estoque;
use App\Models\EstoqueConsumo;
use App\Models\VendaItem;
use Illuminate\Support\Collection;
use Illuminate\Support\Facades\DB;

class EstoqueConsumoService
{
    /**
     * Consumir estoque usando FIFO
     *
     * @param int $produtoId
     * @param int $lojaId
     * @param float $quantidade
     * @param int|null $vendaItemId
     * @param string $motivo
     * @return Collection<EstoqueConsumo>
     * @throws \Exception
     */
    public function consumirFifo(
        int $produtoId,
        int $lojaId,
        float $quantidade,
        ?int $vendaItemId = null,
        string $motivo = 'VENDA'
    ): Collection {
        return DB::transaction(function () use ($produtoId, $lojaId, $quantidade, $vendaItemId, $motivo) {
            $consumos = collect();
            $restante = $quantidade;

            // Obter estoque disponível em ordem FIFO com lock
            $estoques = Estoque::where('produto_id', $produtoId)
                ->where('loja_id', $lojaId)
                ->where('quantidade_disponivel', '>', 0)
                ->orderBy('data_entrada', 'asc')  // FIFO
                ->lockForUpdate()
                ->get();

            foreach ($estoques as $estoque) {
                if ($restante <= 0) break;

                $consumir = min($restante, $estoque->quantidade_disponivel);

                // Atualizar estoque
                $estoque->decrement('quantidade_disponivel', $consumir);

                // Criar registro de consumo
                $consumo = EstoqueConsumo::create([
                    'estoque_id' => $estoque->id,
                    'venda_item_id' => $vendaItemId,
                    'quantidade' => $consumir,
                    'custo_unitario' => $estoque->custo_unitario,
                    'motivo' => $motivo,
                ]);

                $consumos->push($consumo);
                $restante -= $consumir;
            }

            if ($restante > 0) {
                throw new \Exception(
                    "Estoque insuficiente para produto {$produtoId}. Faltam {$restante} unidades."
                );
            }

            return $consumos;
        });
    }

    /**
     * Verificar estoque disponível (preview FIFO)
     */
    public function verificarDisponibilidade(int $produtoId, int $lojaId): array
    {
        $estoques = Estoque::where('produto_id', $produtoId)
            ->where('loja_id', $lojaId)
            ->where('quantidade_disponivel', '>', 0)
            ->orderBy('data_entrada', 'asc')
            ->get();

        return [
            'total_disponivel' => $estoques->sum('quantidade_disponivel'),
            'custo_medio' => $estoques->avg('custo_unitario'),
            'lotes' => $estoques->map(fn($e) => [
                'id' => $e->id,
                'quantidade' => $e->quantidade_disponivel,
                'custo' => $e->custo_unitario,
                'data_entrada' => $e->data_entrada,
                'lote' => $e->lote,
            ])->toArray(),
        ];
    }

    /**
     * Reverter consumo (para devoluções)
     */
    public function estornarConsumo(EstoqueConsumo $consumo): void
    {
        DB::transaction(function () use ($consumo) {
            // Retornar quantidade ao estoque
            Estoque::where('id', $consumo->estoque_id)
                ->increment('quantidade_disponivel', $consumo->quantidade);

            // Marcar consumo como revertido
            $consumo->update([
                'estornado' => true,
                'estornado_at' => now(),
            ]);
        });
    }
}
```

### 4.3 VendaService Atualizado

```php
<?php

namespace App\Services\Venda;

use App\Models\Venda;
use App\Models\VendaItem;
use App\Services\Estoque\EstoqueConsumoService;

class VendaService
{
    public function __construct(
        private EstoqueConsumoService $estoqueService
    ) {}

    /**
     * Criar consumos de estoque para itens de venda marcados como de estoque
     */
    public function criarConsumos(Venda $venda): void
    {
        $itensEstoque = $venda->itens()
            ->where('origem', 'ESTOQUE')  // Apenas itens de estoque existente
            ->whereNull('consumido_at')   // Ainda não consumido
            ->get();

        foreach ($itensEstoque as $item) {
            $consumos = $this->estoqueService->consumirFifo(
                produtoId: $item->produto_id,
                lojaId: $venda->loja_id,
                quantidade: $item->quantidade,
                vendaItemId: $item->id,
                motivo: 'VENDA'
            );

            // Marcar item como consumido
            $item->update([
                'consumido_at' => now(),
                'custo_real' => $consumos->sum(fn($c) => $c->quantidade * $c->custo_unitario),
            ]);
        }
    }
}
```

### 4.4 Mudanças de Schema

```sql
-- Nova tabela de consumo (substitui estoque_has_consumo)
CREATE TABLE estoque_consumos (
    id SERIAL PRIMARY KEY,
    estoque_id INTEGER NOT NULL REFERENCES estoques(id),
    venda_item_id INTEGER REFERENCES venda_itens(id),
    compra_item_id INTEGER REFERENCES compra_itens(id),

    quantidade DECIMAL(15,4) NOT NULL,
    custo_unitario DECIMAL(15,4) NOT NULL,
    motivo VARCHAR(50) NOT NULL,  -- VENDA, AJUSTE, QUEBRA, TRANSFERENCIA

    -- Rastreamento de reversão
    estornado BOOLEAN DEFAULT FALSE,
    estornado_at TIMESTAMP,
    estornado_por INTEGER REFERENCES usuarios(id),

    -- Auditoria
    created_at TIMESTAMP DEFAULT NOW(),
    created_by INTEGER REFERENCES usuarios(id)
);

CREATE INDEX idx_consumos_estoque ON estoque_consumos(estoque_id);
CREATE INDEX idx_consumos_venda ON estoque_consumos(venda_item_id);
```

---

## 5. Casos de Borda

### 5.1 Estoque Insuficiente

```php
// Atual: Falha silenciosa ou erro críptico
// Proposto: Exceção clara com detalhes

try {
    $this->estoqueService->consumirFifo($produtoId, $lojaId, 100);
} catch (EstoqueInsuficienteException $e) {
    // $e->getQuantidadeFaltante() = 25
    // $e->getQuantidadeDisponivel() = 75
    // Mostrar mensagem amigável ao usuário
}
```

### 5.2 Consumo Concorrente

```php
// Lock FOR UPDATE previne condições de corrida
// Se duas vendas tentam consumir o mesmo estoque simultaneamente:
// - Primeira transação trava as linhas
// - Segunda transação espera
// - Após primeira fazer commit, segunda vê quantidades atualizadas
```

### 5.3 Múltiplos Armazéns (Lojas)

```php
// FIFO por armazém
$consumos = $this->estoqueService->consumirFifo(
    produtoId: $produtoId,
    lojaId: $venda->loja_id,  // Apenas deste armazém
    quantidade: $quantidade
);

// Transferência entre armazéns se necessário
$this->estoqueService->transferir(
    produtoId: $produtoId,
    lojaOrigem: 1,
    lojaDestino: 2,
    quantidade: 50
);
```

### 5.4 Consumo de Lote Específico

```php
// Às vezes precisa lote específico (problema de qualidade, pedido do cliente)
public function consumirLoteEspecifico(
    int $estoqueId,
    float $quantidade,
    int $vendaItemId
): EstoqueConsumo {
    // Ignorar FIFO para lote específico
    $estoque = Estoque::lockForUpdate()->findOrFail($estoqueId);

    if ($estoque->quantidade_disponivel < $quantidade) {
        throw new EstoqueInsuficienteException();
    }

    $estoque->decrement('quantidade_disponivel', $quantidade);

    return EstoqueConsumo::create([
        'estoque_id' => $estoqueId,
        'venda_item_id' => $vendaItemId,
        'quantidade' => $quantidade,
        'custo_unitario' => $estoque->custo_unitario,
        'motivo' => 'VENDA_LOTE_ESPECIFICO',
    ]);
}
```

### 5.5 Prioridade por Data de Validade

```php
// FEFO: First-Expired, First-Out (para perecíveis)
public function consumirFefo(int $produtoId, int $lojaId, float $quantidade): Collection
{
    $estoques = Estoque::where('produto_id', $produtoId)
        ->where('loja_id', $lojaId)
        ->where('quantidade_disponivel', '>', 0)
        ->orderByRaw('COALESCE(data_validade, DATE "9999-12-31") ASC')  // FEFO
        ->orderBy('data_entrada', 'asc')  // Depois FIFO
        ->lockForUpdate()
        ->get();

    // ... resto igual ao FIFO
}
```

---

## 6. Estratégia de Migração

### Fase 1: Adicionar Novas Colunas (Sem Quebra)

```sql
-- Adicionar data_entrada se faltando
ALTER TABLE estoque ADD COLUMN IF NOT EXISTS data_entrada TIMESTAMP;

-- Popular a partir de data da NFe para registros existentes
UPDATE estoque e
SET data_entrada = n.dataEmissao
FROM nfe n
WHERE e.idNFe = n.idNFe
  AND e.data_entrada IS NULL;

-- Padrão para qualquer restante
UPDATE estoque
SET data_entrada = created_at
WHERE data_entrada IS NULL;

-- Tornar obrigatório daqui pra frente
ALTER TABLE estoque ALTER COLUMN data_entrada SET NOT NULL;
ALTER TABLE estoque ALTER COLUMN data_entrada SET DEFAULT NOW();
```

### Fase 2: Criar Novo Serviço (Paralelo)

```php
// Criar novo serviço junto com código antigo
class EstoqueConsumoService { ... }

// Feature flag para rollout gradual
if (config('features.fifo_consumption')) {
    $this->newService->consumirFifo(...);
} else {
    $this->legacyConsumption(...);
}
```

### Fase 3: Migrar Lógica de Consumo

```php
// Substituir equivalente de venda.cpp criarConsumos()
// Antigo: Usa produto.idEstoque
// Novo: Usa EstoqueConsumoService::consumirFifo()
```

### Fase 4: Remover produto.idEstoque

```sql
-- Após todo código migrado
ALTER TABLE produto DROP COLUMN idEstoque;
```

---

## Resumo

### Problema

- `produto.idEstoque` aponta para UM registro de estoque (sem FIFO)
- Sem seleção automática de estoque
- Estoque mais antigo pode nunca ser consumido

### Solução

- Remover `produto.idEstoque`
- Adicionar `estoques.data_entrada` para ordenação FIFO
- Criar `EstoqueConsumoService` com query FIFO
- Travar linhas durante consumo para prevenir corridas
- Suportar múltiplos lotes por consumo

### Benefícios

- Compliance FIFO adequado
- Rastreabilidade de lote
- Valoração de estoque correta
- Consumo seguro para concorrência
- Flexível (FIFO, FEFO, lote específico)

---

## Documentos Relacionados

- [03-melhorias.md](./03-melhorias.md) - Lista completa de melhorias
- [../negocios/02-fluxos-estoque.md](../negocios/02-fluxos-estoque.md) - Análise de fluxo de estoque
- [04-simplificacao-l1l2.md](./04-simplificacao-l1l2.md) - Achatamento de tabelas (afeta consumo)
