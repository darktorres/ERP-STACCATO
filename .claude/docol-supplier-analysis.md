# Análise de Pedidos do Fornecedor DOCOL

**Data da análise:** 2026-05-05
**Fonte:** banco `staccato` em `nuvem.staccatorevestimentos.com.br`
**Janela:** `created >= '2025-01-01'` (apenas 3 registros foram criados em 2026 e são re-entradas manuais com datas antigas)

## 1. Resumo executivo

A relação operacional com DOCOL entrou em colapso em 2025: queda de **88% em linhas** e **80% em valor** vs 2024, com **60% de cancelamento** em todas as linhas geradas. A causa-raiz, confirmada pelos followups, é a **descontinuação de SKUs pela DOCOL** somada a lead times longos (até 96 dias). Em paralelo, há um problema financeiro grave: **R$ 284 mil em títulos a pagar para DOCOL emitidos em 2025 estão pendentes e vencidos**, com apenas 1 título de R$ 3.578 marcado como pago.

## 2. Volume ano a ano

| Ano  | Linhas | OCs | Valor (R$)   |
| ---- | -----: | --: | -----------: |
| 2019 |    348 | 103 |    164.457,16 |
| 2020 |    529 | 103 |    343.071,00 |
| 2021 |    563 |  89 |    412.704,46 |
| 2022 |    546 | 134 |    414.144,95 |
| 2023 |  1.417 | 268 |  1.154.937,47 |
| 2024 |  1.739 | 256 |  1.473.603,03 |
| 2025 |    204 |  72 |    289.740,49 |
| 2026 |      3 |   3 |        931,62 |

Tabela base: `pedido_fornecedor_has_produto2`. A queda 2024→2025 representa redução de R$ 1,18 milhões em compras.

## 3. Status dos pedidos 2025

Todas as 204 linhas estão com `statusFinanceiro = PENDENTE`.

| Status         | Linhas (%)  | OCs | Valor (R$)   |
| -------------- | ----------: | --: | -----------: |
| **CANCELADO**  | 123 (60,3%) |  13 | **183.944,92** |
| ESTOQUE        |  47 (23,0%) |  44 |     83.296,01 |
| ENTREGUE       |  29 (14,2%) |  20 |     17.619,69 |
| ENTREGA AGEND. |   5 ( 2,5%) |   4 |      4.879,87 |

## 4. Distribuição mensal (2025+)

| Mês     | Linhas | OCs | Valor      | Cancelados | Valor cancelado |
| ------- | -----: | --: | ---------: | ---------: | --------------: |
| 2025-01 |     84 |  11 | 124.843,05 |     **76** |     108.080,70 |
| 2025-02 |     26 |  12 |  25.073,01 |         12 |      11.832,37 |
| 2025-03 |     30 |  12 |  48.459,06 |         20 |      34.052,40 |
| 2025-04 |     25 |  19 |  36.329,10 |          6 |       4.619,36 |
| 2025-05 |      9 |   8 |   5.807,69 |          0 |              — |
| 2025-06 |      2 |   2 |   2.874,98 |          0 |              — |
| 2025-07 |      5 |   5 |  11.011,83 |          1 |       5.425,38 |
| 2025-08 |      3 |   2 |   2.924,00 |          0 |              — |
| 2025-09 |      7 |   2 |  12.998,65 |          5 |      10.515,30 |
| 2025-10 |      6 |   3 |   4.161,44 |          2 |       2.103,06 |
| 2025-11 |      3 |   3 |   8.486,93 |          1 |       7.316,36 |
| 2025-12 |      4 |   3 |   6.770,75 |          0 |              — |
| 2026-02 |      2 |   2 |     622,89 |          0 |              — |
| 2026-04 |      1 |   1 |     308,73 |          0 |              — |

Janeiro/2025 concentra **62% das cancelações de todo o período** (76 de 123) e **59% do valor cancelado** (R$ 108k de R$ 184k). De maio/2025 em diante o volume mensal nunca passa de 9 linhas.

## 5. Padrão crítico: cancelamento atinge apenas pedidos sem venda

| Origem                  | Linhas | Cancel. | Estoque | Entregue | Valor cancelado |
| ----------------------- | -----: | ------: | ------: | -------: | --------------: |
| Vinculadas a venda      |     35 |       0 |       0 |       35 |              0 |
| **Sem venda (estoque)** |    172 | **123** |      47 |        2 | **183.944,92** |

**Pedidos vinculados a vendas reais executam 100%**. Cancelamento é fenômeno exclusivo do fluxo de reposição/estoque especulativo. Em particular, **100 das 123 cancelações nem chegaram a gerar OC** (`ordemCompra IS NULL`), totalizando R$ 137.270,89 — listas planejadas que foram abandonadas antes da emissão.

## 6. Top OCs (2025+)

| OC      | Linhas | Valor     | Cancel | Ent | Estq | Abertura   | Venda exemplo |
| ------- | -----: | --------: | -----: | --: | ---: | ---------- | ------------- |
| `NULL`  |    100 | 137.270,89 |    100 |   0 |    0 | 2025-01-07 | —             |
| 296709  |      8 |  12.867,35 |      8 |   0 |    0 | 2025-03-18 | —             |
| 295792  |      1 |  10.727,10 |      0 |   0 |    1 | 2025-01-13 | —             |
| 295842  |      2 |   7.716,44 |      1 |   1 |    0 | 2025-01-21 | GABR-241162   |
| 303085  |      1 |   7.316,36 |      1 |   0 |    0 | 2025-11-17 | —             |
| 300079  |      1 |   5.910,45 |      0 |   0 |    1 | 2025-04-08 | —             |
| 301848  |      1 |   5.425,38 |      0 |   1 |    0 | 2025-07-30 | MATR-250104   |
| 301791  |      1 |   5.425,38 |      1 |   0 |    0 | 2025-07-25 | —             |
| 296324  |      4 |   5.252,00 |      0 |   3 |    1 | 2025-02-28 | GABR-241132   |
| 303367  |      1 |   4.760,88 |      0 |   0 |    1 | 2025-12-16 | —             |

## 7. Top produtos cancelados

| Cód comercial | Descrição                              | Linhas | Cancel | Valor    |
| ------------- | -------------------------------------- | -----: | -----: | -------: |
| 015597BH      | MIST MO COZ ME DVITALIS +O3 INOX ESC   |      5 |      5 | 36.581,80 |
| 00961306      | PT TO DOCOLFLAT CR                     |     10 |     10 | 27.362,82 |
| 00888244      | CHU DOCOLHEAVEN Q200 NIQ ESC           |      4 |      4 | 15.074,98 |
| 00920870      | MIST LAV ME LISS 290 GRAF ESC          |      2 |      1 | 14.302,80 |
| 00905657      | MIST MONO COZ ME DOCOLSPICE CR/BK      |      7 |      7 | 12.618,36 |
| 00535506      | ACAB (DE) 3/4 PROVENCE CR              |      3 |      2 | 10.413,61 |
| 009342CE      | DUCMAN C30 C DESV ON                   |      7 |      7 |  7.721,15 |
| 00931106      | ACAB (DE) 3/4MM CR                     |      5 |      5 |  7.433,75 |

## 8. Lead time operacional

Quando pedidos DOCOL fluem (não cancelados), execução é boa:

- Atraso médio de confirmação: **7,2 dias**
- Atraso médio de recebimento: **0 dias** (no prazo)
- Lead time médio compra → recebimento: **26,7 dias**
- Apenas 2 pedidos não cancelados sem `dataRealReceb`

## 9. Cadeia financeira: pedido → NFe → pagamento

| Etapa                                | Tabela                              |        Volume | Valor (R$) |
| ------------------------------------ | ----------------------------------- | ------------: | ---------: |
| Pedidos abertos (todos status)       | `pedido_fornecedor_has_produto2`    | 207 ln/76 OCs | 290.672,11 |
| Pedidos não cancelados               | idem                                |       84 ln  | 106.727,18 |
| NFe entrada autorizada (2025)        | `nfe` (ENTRADA, emitente=%DOCOL%)   |   117 chaves | 298.895,29 |
| Conta a pagar emitida 2025+          | `conta_a_pagar_has_pagamento`       |   114 títulos | 289.414,10 |
| Conta a pagar **PAGA**               | idem                                |   **1 título** |   3.577,84 |
| Conta a pagar **CONFERIDO**          | idem                                |    2 títulos |   1.707,11 |
| Conta a pagar **PENDENTE/VENCIDA** ⚠️ | idem                                | **109 títulos** | **284.129,13** |

Cruzando via `idCompra` (`conta_a_pagar_has_idcompra`):

- 72 OCs com `idCompra` preenchido → 64 cruzaram, gerando 170 parcelas
- 88 parcelas PAGAS (R$ 197.421,36) — tipicamente compras anteriores a 2025
- 74 parcelas PENDENTES, **todas vencidas** (vencimento entre 2025-04-15 e 2025-10-03), valor R$ 192.856,31
- 5 parcelas CANCELADAS (R$ 18.601,29)

## 10. Followups confirmam descontinuação de SKUs

23 anotações em 12 OCs. Trechos representativos:

- OC 296709 (2025-05-02): _"VERIFICANDO COM A DOCOL A POSSIBILIDADE DE FATURAR, POIS É **DESCONTINUADO** E JÁ PROCURAMOS F[ORNECEDOR]"_
- OC 293157 (2025-05-02): _"EM CONTATO COM A DOCOL TENTANDO A SOLUÇÃO, PREVISÃO PARA MÊS 05"_
- OC 290205 (2024-01-22): _"ESTIMADO 96 DIAS CORRIDOS, APÓS 13/12/2023"_ — lead time de 3 meses
- OC 293692 (2025-03-18): _"AGUARDANDO FATURAMENTO PARA POSSIVEL USO"_

Padrão: descontinuação de itens pela DOCOL + lead times estendidos. Não é uma decisão comercial unilateral da Staccato; é uma redução de catálogo do próprio fornecedor.

## 11. Cruzamento com vendas e orçamentos

| Métrica                                                           |  Valor |
| ----------------------------------------------------------------- | -----: |
| Vendas distintas que geraram pedidos DOCOL desde 2025             |     28 |
| Range das vendas vinculadas                                       | 2024-05-29 a 2026-04-27 |
| Total dessas vendas (não só DOCOL)                                | R$ 1.166.779,88 |
| Vendas no histórico com DOCOL nos `fornecedores`                  |    370 |
| Orçamentos com DOCOL nos `fornecedores`                           |  2.013 |

A DOCOL aparece como componente pequeno (R$ 1k–5k) de vendas maiores. A taxa de conversão "orçamento com DOCOL → venda com pedido DOCOL" é baixíssima (≈1%).

## 12. Estoque vinculado

`estoque_has_compra` registra **55 entradas em estoque** para 49 idCompras DOCOL de 2025+. Compatível com os 47 pedidos em status ESTOQUE — 2 idCompras geraram múltiplas entradas (split de carga ou lote).

## 13. Discrepâncias detectadas

### 13.1 NFe (R$ 298k) vs OC não cancelado (R$ 107k) — 2,8× maior
Hipóteses:
1. NFes contemplam OCs históricas (anteriores a 2025) ainda em faturamento
2. NFe inclui ICMS/ST/IPI/frete enquanto `preco` do pedido é líquido
3. NFes autorizadas para OCs que foram canceladas a posteriori sem cancelar a NFe

Próximo passo: cruzar `view_ordemcompra_nfe` por chave de acesso.

### 13.2 `pedido_fornecedor_has_produto` (p1) vs `_produto2` (p2)

| Tabela | Linhas 2025+ | Valor      |
| ------ | -----------: | ---------: |
| p1     |          171 | 265.432,10 |
| p2     |          204 | 289.740,49 |

p2 tem 33 linhas a mais (principalmente em ENTREGUE: 32 vs 9). Investigar se p1 é histórico/write-once ou se há falha de sincronização. As views (`view_fornecedor_compra`, `view_compras_financeiro`) leem de p1, então relatórios podem estar subnotificando.

### 13.3 R$ 284k a pagar vencido para DOCOL
Status PENDENTE com vencimentos entre abril e outubro de 2025. Cenários possíveis:
- A) Títulos foram pagos via banco mas nunca reconciliados no ERP
- B) Pagamento à DOCOL travado (disputa, devolução pendente, restrição financeira)
- C) Títulos duplicados que precisam ser cancelados

Auditoria com financeiro é obrigatória.

## 14. Ações recomendadas

1. **Financeiro — prioridade alta**: auditar os 109 títulos PENDENTES vencidos (R$ 284k) com a DOCOL. Conferir extratos bancários para identificar pagamentos não conciliados.
2. **Compras**: confirmar fornecedor substituto consolidado para os top SKUs descontinuados (DVITALIS, DOCOLFLAT, DOCOLHEAVEN, DOCOLSPICE).
3. **Sistema**: investigar as 100 linhas canceladas sem OC (R$ 137k). Entender origem do fluxo (importação manual, planejamento de reposição) e travar/auditar a geração.
4. **Engenharia ERP**: validar paridade entre `pedido_fornecedor_has_produto` e `pedido_fornecedor_has_produto2`. Decidir qual é fonte canônica e ajustar views se necessário.
5. **Fiscal**: cruzar as 117 NFe de entrada DOCOL via `view_ordemcompra_nfe` para identificar NFes órfãs de OC ou OCs faturadas e canceladas.

## 15. Consultas SQL utilizadas

```sql
-- Volume ano a ano
SELECT YEAR(created) ano, COUNT(*) linhas, COUNT(DISTINCT ordemCompra) ocs, ROUND(SUM(preco),2) valor
FROM pedido_fornecedor_has_produto2
WHERE fornecedor='DOCOL'
GROUP BY YEAR(created) ORDER BY ano;

-- Status 2025
SELECT status, COUNT(*) linhas, COUNT(DISTINCT ordemCompra) ocs, ROUND(SUM(preco),2) valor
FROM pedido_fornecedor_has_produto2
WHERE fornecedor='DOCOL' AND created>='2025-01-01'
GROUP BY status;

-- Origem (vinculada vs especulativa)
SELECT
  CASE WHEN idVenda IS NULL THEN 'sem_venda' ELSE 'com_venda' END origem,
  status, COUNT(*) linhas, ROUND(SUM(preco),2) valor
FROM pedido_fornecedor_has_produto2
WHERE fornecedor='DOCOL' AND created>='2025-01-01'
GROUP BY origem, status;

-- Pagamentos pendentes vencidos
SELECT CASE WHEN dataPagamento<CURDATE() THEN 'VENCIDO' ELSE 'A_VENCER' END situacao,
       COUNT(*) qtd, ROUND(SUM(valor),2) valor
FROM conta_a_pagar_has_pagamento
WHERE status='PENDENTE'
  AND idPagamento IN (
    SELECT DISTINCT capi.idPagamento
    FROM conta_a_pagar_has_idcompra capi
    JOIN pedido_fornecedor_has_produto2 p ON capi.idCompra=CAST(p.idCompra AS CHAR)
    WHERE p.fornecedor='DOCOL' AND p.created>='2025-01-01'
  )
GROUP BY situacao;

-- Reconciliação NFe vs pedido vs pago
SELECT 'comprado_nao_cancelado' tipo, ROUND(SUM(preco),2) valor, COUNT(*) qtd
FROM pedido_fornecedor_has_produto2
WHERE fornecedor='DOCOL' AND created>='2025-01-01' AND status<>'CANCELADO'
UNION ALL
SELECT 'nfe_entrada_autorizada', ROUND(SUM(valor),2), COUNT(*)
FROM nfe WHERE tipo='ENTRADA' AND emitente LIKE '%DOCOL%' AND status='AUTORIZADA'
  AND dataHoraEmissao>='2025-01-01' AND dataHoraEmissao<'2026-01-01'
UNION ALL
SELECT 'pago_em_2025', ROUND(SUM(valor),2), COUNT(*)
FROM conta_a_pagar_has_pagamento
WHERE status='PAGO' AND contraParte LIKE '%DOCOL%' AND dataPagamento>='2025-01-01';
```
