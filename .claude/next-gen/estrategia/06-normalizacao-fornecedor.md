# Normalização de Referências de Fornecedor

> Status: **Análise**
> Última atualização: 2025-12-27
> Foco: Substituir nomes de fornecedor desnormalizados por referências FK adequadas

---

## Sumário

1. [Problema Atual](#1-problema-atual)
2. [Tabelas Afetadas](#2-tabelas-afetadas)
3. [Análise de Impacto](#3-análise-de-impacto)
4. [Solução Proposta](#4-solução-proposta)
5. [Estratégia de Migração](#5-estratégia-de-migração)
6. [Mudanças de Código](#6-mudanças-de-código)

---

## 1. Problema Atual

### 1.1 O Que Está Errado

Nome do fornecedor é armazenado como **VARCHAR** em múltiplas tabelas ao invés de **FK para fornecedor**:

```sql
-- Atual: Nome duplicado em todo lugar
venda_has_produto2 (fornecedor VARCHAR)     -- "ACME Corp"
estoque (fornecedor VARCHAR)                 -- "ACME Corp"
estoque_has_consumo (fornecedor VARCHAR)     -- "ACME Corp"
pedido_fornecedor_has_produto2 (fornecedor VARCHAR) -- "ACME Corp"
produto (fornecedor VARCHAR)                 -- "ACME Corp"

-- Deveria ser: Referência FK
venda_has_produto2 (fornecedor_id INT FK)   -- 123
estoque (fornecedor_id INT FK)              -- 123
```

### 1.2 Por Que é um Problema

| Problema | Impacto |
|----------|---------|
| **Inconsistência de dados** | Erros de digitação, variações ("ACME Corp" vs "Acme Corp") |
| **Pesadelo de renomeação** | Se fornecedor muda de nome, precisa atualizar 5+ tabelas |
| **Sem integridade referencial** | Pode ter referências órfãs |
| **Ineficiência de query** | Comparação de string mais lenta que INT |
| **Desperdício de armazenamento** | VARCHAR repetido vs único INT FK |
| **Sem cascade** | Deletar fornecedor deixa registros órfãos |

### 1.3 Exemplo Real do Código

```cpp
// inputdialogproduto.cpp:215 - Buscando por nome (frágil!)
query.bindValue(":razaoSocial", modelPedidoFornecedor.data(0, "fornecedor"));

// Se nome tem erro de digitação ou variação, isso falha silenciosamente
```

---

## 2. Tabelas Afetadas

### 2.1 Tabelas com Fornecedor Desnormalizado

| Tabela | Coluna | Contagem de Uso no Código |
|--------|--------|---------------------------|
| `produto` | `fornecedor` | ~15 arquivos |
| `venda_has_produto` | `fornecedor` | ~10 arquivos |
| `venda_has_produto2` | `fornecedor` | ~20 arquivos |
| `pedido_fornecedor_has_produto` | `fornecedor` | ~8 arquivos |
| `pedido_fornecedor_has_produto2` | `fornecedor` | ~12 arquivos |
| `estoque` | `fornecedor` | ~8 arquivos |
| `estoque_has_consumo` | `fornecedor` | ~5 arquivos |
| `compra_avulsa` | `fornecedor` | ~3 arquivos |
| `orcamento_has_produto` | `fornecedor` | ~5 arquivos |

**Total**: ~9 tabelas, ~85+ referências de código

### 2.2 Tabelas Que Já Tem FK

```sql
-- Estas estão corretas
produto.idFornecedor → fornecedor.idFornecedor
pedido_fornecedor.idFornecedor → fornecedor.idFornecedor
```

**Problema**: Tanto `idFornecedor` (FK) QUANTO `fornecedor` (VARCHAR) existem!

---

## 3. Análise de Impacto

### 3.1 Problemas de Dados Atuais

```sql
-- Encontrar inconsistências
SELECT DISTINCT p.fornecedor, f.razaoSocial
FROM produto p
LEFT JOIN fornecedor f ON p.idFornecedor = f.idFornecedor
WHERE p.fornecedor != f.razaoSocial
  AND p.fornecedor IS NOT NULL;

-- Problemas comuns encontrados:
-- "TRAMONTINA " vs "TRAMONTINA" (espaço no final)
-- "Tok&Stok" vs "TOK&STOK" (diferença de caixa)
-- Nome antigo vs nome novo após renomeação
```

### 3.2 Padrões de Código a Corrigir

**Padrão 1: Definindo nome do fornecedor manualmente**
```cpp
// Atual: Copia string do nome
modelEstoque.setData(newRow, "fornecedor", xml.xNome);

// Deveria ser: Usar FK
modelEstoque.setData(newRow, "fornecedor_id", fornecedorId);
```

**Padrão 2: Comparando por nome**
```cpp
// Atual: Comparação de string
if (modelItem.data(0, "fornecedor").toString() == "ATELIER STACCATO") { ... }

// Deveria ser: Comparar FK ou usar constante
if (modelItem.data(0, "fornecedor_id").toInt() == ATELIER_STACCATO_ID) { ... }
```

**Padrão 3: Agrupando por fornecedor**
```cpp
// Atual: Agrupar por nome (lento, propenso a erros)
for (row : rows) { fornecedores << modelItem.data(row, "fornecedor").toString(); }

// Deveria ser: Agrupar por FK
for (row : rows) { fornecedorIds << modelItem.data(row, "fornecedor_id").toInt(); }
```

---

## 4. Solução Proposta

### 4.1 Novo Schema

```sql
-- Manter tabela fornecedor como está (já normalizada)
-- fornecedor (idFornecedor PK, razaoSocial, cnpj, ...)

-- Adicionar colunas FK nas tabelas afetadas
ALTER TABLE venda_has_produto2
    ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);

ALTER TABLE estoque
    ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);

ALTER TABLE estoque_has_consumo
    ADD COLUMN fornecedor_id INTEGER REFERENCES fornecedores(id);

-- etc para outras tabelas

-- Criar índices
CREATE INDEX idx_vhp2_fornecedor ON venda_has_produto2(fornecedor_id);
CREATE INDEX idx_estoque_fornecedor ON estoque(fornecedor_id);
```

### 4.2 Schema PostgreSQL (Sistema Novo)

```sql
-- Tabela de fornecedores
CREATE TABLE fornecedores (
    id SERIAL PRIMARY KEY,
    razao_social VARCHAR(200) NOT NULL,
    nome_fantasia VARCHAR(200),
    cnpj VARCHAR(18) UNIQUE,
    inscricao_estadual VARCHAR(20),

    -- Contato
    email VARCHAR(200),
    telefone VARCHAR(20),

    -- Dados bancários
    banco VARCHAR(100),
    agencia VARCHAR(20),
    conta VARCHAR(20),

    -- Regras de negócio
    comissao_percentual DECIMAL(5,2) DEFAULT 0,
    frete_pago_loja BOOLEAN DEFAULT FALSE,
    representacao BOOLEAN DEFAULT FALSE,

    -- Status
    ativo BOOLEAN DEFAULT TRUE,

    -- Auditoria
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);

-- Todas as tabelas relacionadas usam FK
CREATE TABLE venda_itens (
    -- ...
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),
    -- Removido: fornecedor VARCHAR
);

CREATE TABLE estoques (
    -- ...
    fornecedor_id INTEGER NOT NULL REFERENCES fornecedores(id),
    -- Removido: fornecedor VARCHAR
);
```

### 4.3 View para Compatibilidade com Versões Anteriores

```sql
-- Durante transição, criar view que inclui nome do fornecedor
CREATE VIEW venda_itens_com_fornecedor AS
SELECT
    vi.*,
    f.razao_social as fornecedor,
    f.nome_fantasia as fornecedor_fantasia
FROM venda_itens vi
JOIN fornecedores f ON vi.fornecedor_id = f.id;
```

---

## 5. Estratégia de Migração

### Fase 1: Adicionar Colunas FK (Sem Quebra)

```sql
-- Adicionar colunas FK nullables
ALTER TABLE venda_has_produto2
    ADD COLUMN fornecedor_id INTEGER;

ALTER TABLE estoque
    ADD COLUMN fornecedor_id INTEGER;

-- Popular a partir dos nomes existentes
UPDATE venda_has_produto2 vp
SET fornecedor_id = f.idFornecedor
FROM fornecedor f
WHERE UPPER(TRIM(vp.fornecedor)) = UPPER(TRIM(f.razaoSocial));

UPDATE estoque e
SET fornecedor_id = f.idFornecedor
FROM fornecedor f
WHERE UPPER(TRIM(e.fornecedor)) = UPPER(TRIM(f.razaoSocial));

-- Verificar registros não correspondidos
SELECT DISTINCT fornecedor
FROM venda_has_produto2
WHERE fornecedor_id IS NULL
  AND fornecedor IS NOT NULL;
```

### Fase 2: Tratar Nomes Não Correspondidos

```sql
-- Opção A: Criar fornecedores faltantes
INSERT INTO fornecedor (razaoSocial)
SELECT DISTINCT vp.fornecedor
FROM venda_has_produto2 vp
WHERE vp.fornecedor_id IS NULL
  AND vp.fornecedor IS NOT NULL
  AND NOT EXISTS (
      SELECT 1 FROM fornecedor f
      WHERE UPPER(TRIM(f.razaoSocial)) = UPPER(TRIM(vp.fornecedor))
  );

-- Opção B: Revisão manual de variações
-- Exportar lista para correspondência manual
COPY (
    SELECT DISTINCT fornecedor, COUNT(*) as count
    FROM venda_has_produto2
    WHERE fornecedor_id IS NULL
    GROUP BY fornecedor
    ORDER BY count DESC
) TO '/tmp/fornecedores_nao_correspondidos.csv' CSV HEADER;
```

### Fase 3: Adicionar Constraints

```sql
-- Após todos os dados migrados
ALTER TABLE venda_has_produto2
    ALTER COLUMN fornecedor_id SET NOT NULL,
    ADD CONSTRAINT fk_vhp2_fornecedor
        FOREIGN KEY (fornecedor_id)
        REFERENCES fornecedor(idFornecedor);

-- Criar índice
CREATE INDEX idx_vhp2_fornecedor ON venda_has_produto2(fornecedor_id);
```

### Fase 4: Escrita Dupla na Aplicação

```php
// Durante transição: escrever em ambas colunas
class VendaItemService
{
    public function create(array $data): VendaItem
    {
        return VendaItem::create([
            'fornecedor_id' => $data['fornecedor_id'],
            // Manter escrita na coluna antiga para compatibilidade
            'fornecedor' => Fornecedor::find($data['fornecedor_id'])->razao_social,
            // ...
        ]);
    }
}
```

### Fase 5: Remover Colunas Antigas

```sql
-- Após todo código migrado
ALTER TABLE venda_has_produto2 DROP COLUMN fornecedor;
ALTER TABLE estoque DROP COLUMN fornecedor;
-- etc.
```

---

## 6. Mudanças de Código

### 6.1 Modelos Laravel

```php
<?php

namespace App\Models;

class VendaItem extends Model
{
    protected $fillable = [
        'venda_id',
        'produto_id',
        'fornecedor_id',  // FK ao invés de nome
        'quantidade',
        // ...
    ];

    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }

    // Accessor para compatibilidade com versões anteriores
    public function getFornecedorNomeAttribute(): string
    {
        return $this->fornecedor->razao_social;
    }
}

class Estoque extends Model
{
    public function fornecedor(): BelongsTo
    {
        return $this->belongsTo(Fornecedor::class);
    }
}

class Fornecedor extends Model
{
    public function produtos(): HasMany
    {
        return $this->hasMany(Produto::class);
    }

    public function vendaItens(): HasMany
    {
        return $this->hasMany(VendaItem::class);
    }

    public function estoques(): HasMany
    {
        return $this->hasMany(Estoque::class);
    }

    // Helper: buscar por nome (para migração/importação)
    public static function findByName(string $name): ?self
    {
        return static::whereRaw(
            'UPPER(TRIM(razao_social)) = ?',
            [strtoupper(trim($name))]
        )->first();
    }

    // Helper: buscar ou criar por nome
    public static function findOrCreateByName(string $name): self
    {
        return static::findByName($name)
            ?? static::create(['razao_social' => trim($name)]);
    }
}
```

### 6.2 Serviço de Importação (NFe)

```php
<?php

namespace App\Services\NFe;

class NfeImportService
{
    public function importarEstoque(NfeXml $xml): Estoque
    {
        // Obter ou criar fornecedor por CNPJ (preferido) ou nome
        $fornecedor = Fornecedor::where('cnpj', $xml->emitente->cnpj)->first()
            ?? Fornecedor::findOrCreateByName($xml->emitente->razaoSocial);

        return Estoque::create([
            'fornecedor_id' => $fornecedor->id,  // FK!
            'quantidade' => $xml->quantidade,
            // ...
        ]);
    }
}
```

### 6.3 Exemplos de Query

```php
// Antigo: Agrupar por nome (lento, propenso a erros)
$vendas->groupBy('fornecedor');

// Novo: Agrupar por FK (rápido, confiável)
$vendas->groupBy('fornecedor_id');

// Antigo: Filtrar por nome
VendaItem::where('fornecedor', 'ACME Corp')->get();

// Novo: Filtrar por FK
VendaItem::where('fornecedor_id', $acmeId)->get();

// Ou com relacionamento
VendaItem::whereHas('fornecedor', fn($q) =>
    $q->where('razao_social', 'like', '%ACME%')
)->get();

// Eager load do nome do fornecedor
VendaItem::with('fornecedor:id,razao_social')->get();
```

### 6.4 Exibição na UI

```php
// Template Blade
{{ $item->fornecedor->razao_social }}

// Ou com accessor
{{ $item->fornecedor_nome }}

// Vue/Inertia
<td>{{ item.fornecedor.razao_social }}</td>
```

---

## 7. Casos Especiais

### 7.1 Verificação "ATELIER STACCATO"

```cpp
// Atual: Comparação de string mágica
if (modelItem.data(0, "fornecedor").toString() == "ATELIER STACCATO") { ... }
```

```php
// Novo: Usar constante ou config
class Fornecedor extends Model
{
    // IDs de fornecedores conhecidos
    public const ATELIER_STACCATO_ID = 1;  // Ou do config

    public function isAtelierStaccato(): bool
    {
        return $this->id === self::ATELIER_STACCATO_ID;
    }
}

// Uso
if ($item->fornecedor->isAtelierStaccato()) { ... }
```

### 7.2 Nome do Fornecedor em Relatórios

```php
// Para relatórios/exportações, fazer join do nome do fornecedor
$items = VendaItem::query()
    ->select('venda_itens.*', 'f.razao_social as fornecedor_nome')
    ->join('fornecedores as f', 'f.id', '=', 'venda_itens.fornecedor_id')
    ->get();
```

### 7.3 Dados Históricos

```php
// Para fins de auditoria, pode querer fazer snapshot do nome do fornecedor no momento da transação
CREATE TABLE venda_itens (
    fornecedor_id INTEGER REFERENCES fornecedores(id),
    fornecedor_nome_snapshot VARCHAR(200),  -- Nome no momento da venda
);

// Definir na criação
$item = VendaItem::create([
    'fornecedor_id' => $fornecedor->id,
    'fornecedor_nome_snapshot' => $fornecedor->razao_social,
]);
```

---

## 8. Resumo de Benefícios

| Aspecto | Antes | Depois |
|---------|-------|--------|
| **Armazenamento** | VARCHAR repetido em 9 tabelas | Único INT FK |
| **Renomeação** | Atualizar 9 tabelas manualmente | Atualizar 1 tabela |
| **Integridade** | Nenhuma (órfãos possíveis) | Constraint FK |
| **Velocidade de query** | Comparação de string | Comparação de INT |
| **Erros de digitação** | Quebram queries silenciosamente | Impossíveis |
| **Joins** | Por string (lento) | Por FK (rápido) |

---

## Documentos Relacionados

- [03-improvements.md](./03-improvements.md) - Lista completa de melhorias
- [04-l1l2-simplification.md](./04-l1l2-simplification.md) - Achatamento de tabelas (inclui fornecedor_id)
- [../business/04-cadastros-flows.md](../business/04-cadastros-flows.md) - Fluxo de cadastro de fornecedor
