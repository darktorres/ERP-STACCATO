# Análise de Pedidos do Fornecedor DOCOL

**Data da análise:** 2026-05-05
**Fonte:** banco `staccato` em `nuvem.staccatorevestimentos.com.br`
**Janela:** `created >= '2025-01-01'` (apenas 3 registros foram criados em 2026 e são re-entradas manuais com datas antigas)

## 1. Resumo executivo

A relação operacional com DOCOL entrou em colapso em 2025: queda de **88% em linhas** e **80% em valor** vs 2024, com **60% de cancelamento** em todas as linhas geradas em p2 (execução). A análise inicial superficial concluiu que cancelações eram em "reposição especulativa", mas o cruzamento com `pedido_fornecedor_has_produto` (p1, pedido master) revela uma realidade muito mais grave: **111 das 123 cancelações afetaram 21 vendas reais já entregues**, totalizando R$ 168 mil de rupturas que precisaram de substituição por DECA, MEKAL e FRANKE.

Em paralelo, há um problema financeiro grave: **R$ 277 mil em 108 títulos a pagar para DOCOL têm vencimento idêntico (2025-04-15)**, somente 1 marcado como pago, com 20 títulos tendo vencimento anterior à emissão e 4 NFes gerando títulos duplicados — fortíssimo indício de falha de conciliação ou bug em massa.

## 2. Modelo de dados: p1 vs p2

| Tabela | Papel | Comportamento |
|---|---|---|
| `pedido_fornecedor_has_produto` (**p1**) | Pedido master / intenção original | Imutável quanto ao vínculo com a venda. Reflete o item conforme planejado. |
| `pedido_fornecedor_has_produto2` (**p2**) | Execução / desmembramento | Mesmas linhas, atualizadas conforme fluxo (status, idCompra, datas reais). Cancelamentos podem **desvincular o idVenda**. |

A relação formal por `idRelacionado` está parcialmente quebrada no banco inteiro: dos 161.432 registros em p2, 38.004 têm `idRelacionado` preenchido — **dos quais apenas 21.905 (58%) apontam para um `idPedido1` válido**. Os outros 16.099 são links órfãos. O par real (mesma idCompra+codComercial+preço) é a chave prática de cruzamento.

**Implicação crítica para análise**: ao cancelar, p2 zera `idVenda` mas p1 mantém. Conclusões baseadas só em p2 distorcem a origem dos cancelamentos (parecem "sem venda" quando na verdade vinham de vendas reais).

## 3. Volume ano a ano (p2)

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

A queda 2024→2025 representa redução de R$ 1,18 milhões em compras DOCOL.

## 4. Comparação p1 vs p2 (DOCOL 2025+)

| Categoria              | p1 linhas | p1 valor   | p2 linhas | p2 valor   |
| ---------------------- | --------: | ---------: | --------: | ---------: |
| com_venda — CANCELADO  |   **111** | **168.000** |     0     |     0      |
| com_venda — ENTREGUE   |        3  |   7.256    |    30     |  17.479    |
| com_venda — ENTREGA AGEND. |    0  |     0      |     5     |   4.880    |
| sem_venda — CANCELADO  |        6  |   6.528    |   123     | 183.945    |
| sem_venda — ENTREGUE   |        6  |   4.125    |     2     |   1.073    |
| sem_venda — ENTREGA AGEND. |    1  |     187    |     0     |     0      |
| sem_venda — ESTOQUE    |       44  |  79.336    |    47     |  83.296    |
| **Total**              | **171** | **265.432** |  **207**  | **290.672** |

**Inversão completa**: 111 cancelamentos com idVenda em p1 viraram 123 cancelamentos sem idVenda em p2. A diferença de 12 vem de pedidos com idCompra preenchido (cancelados pós-faturamento, ver §10).

## 5. Status dos pedidos 2025 (p2)

Todas as 204 linhas com `statusFinanceiro = PENDENTE`:

| Status         | Linhas (%)  | OCs | Valor (R$)   |
| -------------- | ----------: | --: | -----------: |
| **CANCELADO**  | 123 (60,3%) |  13 | **183.944,92** |
| ESTOQUE        |  47 (23,0%) |  44 |     83.296,01 |
| ENTREGUE       |  29 (14,2%) |  20 |     17.619,69 |
| ENTREGA AGEND. |   5 ( 2,5%) |   4 |      4.879,87 |

## 6. Distribuição mensal (2025+)

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

## 7. Impacto real das cancelações (cruzamento com vendas via p1)

Os **111 cancelamentos com vínculo de venda em p1 (R$ 168.000)** atingiram **21 vendas reais distintas** entre 2024-04-26 e 2025-11-17. Top vendas afetadas:

| Venda        | Linhas canc. | Valor canc. | Status venda  | Total venda    | Substitutos pedidos                         |
| ------------ | -----------: | ----------: | ------------- | -------------: | ------------------------------------------- |
| GABR-241162  |            4 |   29.265,44 | ENTREGUE      |     20.544,98* |  (substituídos)                              |
| SD&D-240589  |           18 |   28.914,25 | ENTREGUE      |     70.000,00  |                                              |
| JUND-240538  |           13 |   22.718,66 | ENTREGUE      |     93.000,00  | ARELL, DECA, FRANKE, MEKAL, ZEN              |
| GABR-241132  |           12 |   20.846,20 | ENTREGUE      |    185.845,00  | DECA, MEKAL, RALO LINEAR                     |
| JUND-240480  |           14 |   11.176,11 | ENTREGUE      |     25.167,00  |                                              |
| GABR-241019  |            9 |   10.728,45 | ENTREGUE      |     59.270,00  | AQUECE METAIS, DECA                          |
| PACB-240270  |            6 |    7.467,82 | ENTREGUE      |     32.876,95  |                                              |
| MATR-250185  |            1 |    7.316,36 | ENTREGUE      |     14.539,19  |                                              |
| GABR-240808  |            8 |    6.510,18 | ENTREGUE      |     17.299,00  |                                              |
| MATR-250104  |            1 |    5.425,38 | ENTREGUE      |     10.862,28  |                                              |
| ECKO-240129  |            3 |    3.812,38 | ENTREGA AGEND.|     90.000,00  | DECA, MEKAL                                  |
| JUND-240474  |            7*|    1.848,88 | ENTREGUE      |     70.000,00  | ARELL, DECA, FRANKE, SABBIA, VISCARDI        |

*\* GABR-241162: valor cancelado DOCOL (R$ 29k) > total da venda (R$ 20k); ver §13.4.*

**20 das 21 vendas estão ENTREGUES**, indicando que os itens DOCOL cancelados foram **substituídos** ou removidos da venda. Substitutos recorrentes: **DECA, MEKAL, FRANKE**.

## 8. Padrão dos cancelamentos: timing e clusters

As 100 linhas canceladas sem OC (R$ 137.271) têm padrão temporal específico:

- 35 produtos distintos, 26 dias distintos, 100 linhas
- **70 das 100 em janeiro/2025** (R$ 94.520)
- Clusters de criação: 17 linhas em 2025-01-08 17h, 10 em 2025-01-07 16h, 7 em 2025-01-29 17h
- Todas com `idVenda=NULL` em p2 (mas p1 mantém vínculo de 111 dessas com vendas)
- 23 linhas adicionais foram canceladas APÓS terem `idCompra` preenchido (ver §10)

## 9. SKUs descontinuados (top 15)

Itens com cancelamento total ou quase total em 2025:

| Cód comercial | Descrição                              | Total hist. | Linhas 2025 | Canc 2025 | Ent 2025 | Valor 2025 |
| ------------- | -------------------------------------- | ----------: | ----------: | --------: | -------: | ---------: |
| 00961306      | PORTA TOALHA BASTAO 600MM DOCOLFLAT    |          36 |          10 |        10 |        0 |  27.362,82 |
| 00960906      | CABIDE DOCOL 960906 FLAT CR            |          73 |          11 |         9 |        1 |   5.824,44 |
| 00905657      | MIST MONO COZ ME DOCOLSPICE CR/BK      |          17 |           8 |         7 |        1 |  13.669,89 |
| 009342CE      | DUCHA MANUAL C30 COM DESVIADOR — ON    |           9 |           7 |         7 |        0 |   7.721,15 |
| 00926444      | ACAB DB 3/4" MIX&MATCH NIQ ESC         |          18 |          10 |         7 |        1 |   6.528,10 |
| 012048CE      | CHU NOVO TECHNOSHOWER ON               |          17 |           7 |         7 |        0 |   3.454,96 |
| 015597BH      | MIST MO COZ ME DVITALIS +O3 INOX ESC   |           8 |           5 |         5 |        0 |  36.581,80 |
| 00931106      | ACAB (DE) 3/4" MIX&MATCH CR            |          22 |           5 |         5 |        0 |   7.433,75 |
| 00968966      | BACIA CONV SV SIF LIFT WH/CR           |          20 |           6 |         5 |        0 |   5.765,55 |
| 00322672      | SIFAO LAV TOP OURO ESC                 |           7 |           5 |         5 |        0 |   1.921,04 |
| 00796526      | SIFAO UNIVERSAL EXTENSIVEL 72CM WH     |         222 |          15 |         5 |        9 |     136,78 |
| 00888244      | CHU DOCOLHEAVEN Q200 NIQ ESC           |           8 |           4 |         4 |        0 |  15.074,98 |
| 00925072      | TOR LAV ME BA NEW EDGE OURO ESC        |           8 |           5 |         4 |        1 |   7.153,30 |

A linha **00796526** (Sifão Universal) é interessante: 222 compras históricas, ainda 9 entregas em 2025 — não está descontinuado, mas teve 5 cancelamentos pontuais. Os demais top SKUs têm padrão de descontinuação clara (cancelamento > 50%).

## 10. Cancelamentos pós-faturamento (com idCompra)

| Categoria                                | Linhas | Valor      |
| ---------------------------------------- | -----: | ---------: |
| Cancelados antes de virar compra (`idCompra IS NULL`) |    100 | 137.270,89 |
| **Cancelados APÓS virar compra (`idCompra` preenchido)** | **23** | **46.674,03** |

Dos 23 cancelados pós-faturamento, **31 NFes autorizadas e 45 entradas em estoque foram registradas** antes do cancelamento. Significa que produtos foram recebidos no estoque, depois marcados como cancelados — provável devolução ou erro de status. Investigação contábil necessária.

## 11. Lead time operacional

Quando pedidos DOCOL fluem (não cancelados), execução é boa:

- Atraso médio de confirmação: **7,2 dias**
- Atraso médio de recebimento: **0 dias** (no prazo)
- Lead time médio compra → recebimento: **26,7 dias**
- Apenas 2 pedidos não cancelados sem `dataRealReceb`

## 12. Cadeia financeira: pedido → NFe → pagamento

| Etapa                                | Tabela                              |        Volume | Valor (R$) |
| ------------------------------------ | ----------------------------------- | ------------: | ---------: |
| Pedidos abertos (todos status)       | `pedido_fornecedor_has_produto2`    | 207 ln/76 OCs | 290.672,11 |
| Pedidos não cancelados               | idem                                |       84 ln  | 106.727,18 |
| NFe entrada autorizada (2025)        | `nfe` (ENTRADA, emitente=%DOCOL%)   |   117 chaves | 298.895,29 |
| Conta a pagar emitida 2025+          | `conta_a_pagar_has_pagamento`       |   114 títulos | 289.414,10 |
| Conta a pagar **PAGA**               | idem                                |   **1 título** |   3.577,84 |
| Conta a pagar **CONFERIDO**          | idem                                |    2 títulos |   1.707,11 |
| Conta a pagar **PENDENTE/VENCIDA** ⚠️ | idem                                | **109 títulos** | **284.129,13** |

## 13. Pontos críticos investigados

### 13.1 ⚠️ Vencimento padronizado em 2025-04-15 (R$ 277k em 108 títulos)

Distribuição de vencimentos dos títulos PENDENTES emitidos em 2025+:

| Vencimento  | Qtd | Valor      |
| ----------- | --: | ---------: |
| 2025-04-15  | **108** | **276.944,57** |
| 2025-10-03  |   1 |   7.184,56 |

**108 títulos com vencimento no mesmo dia** não é padrão de cobrança real. Inclui boletos com `dataEmissao` em junho/2025 mas vencimento em abril/2025 — impossível operacionalmente. Hipóteses:

1. **Bug de sistema** (mais provável): processo automático sobrescreveu `dataPagamento` para `2025-04-15` em massa.
2. **Pagamento real ocorrido** em 2025-04-15 via remessa CNAB consolidada, mas sem reconciliação dos títulos individuais.
3. **Renegociação em lote** que padronizou vencimento mas não atualizou status pago.

**Investigação obrigatória com financeiro**.

### 13.2 ⚠️ Vencimento anterior à emissão

**20 títulos (R$ 56.987,12)** têm `dataPagamento < dataEmissao`. Inconsistência de dados. Exemplo: idPagamento 306362, emissão 2025-06-18, vencimento 2025-04-15.

### 13.3 ⚠️ Títulos duplicados por NFe

| idNFe  | Títulos | Valor total | idPagamentos     |
| ------ | -----: | ----------: | ---------------- |
| 104186 |      2 |   12.461,50 | 296811, 297765   |
| 104921 |      2 |    4.543,10 | 298359, 301183   |
| 104142 |      2 |    2.034,84 | 296679, 297764   |
| 103991 |      2 |      648,80 | 296339, 296613   |

4 NFes geraram 2 títulos cada (R$ 19.688,24 duplicados).

### 13.4 NFe órfãs

5 NFes autorizadas DOCOL 2025 sem vinculação a OC alguma:

| idNFe  | numeroNFe | Emissão    | Valor    |
| ------ | --------- | ---------- | -------: |
| 103281 | 000091339 | 2025-01-21 | 2.503,83 |
| 103465 | 000096324 | 2025-01-27 |   772,76 |
| 104519 | 000124988 | 2025-02-26 | 6.847,60 |
| 105207 | 000152728 | 2025-03-18 |   283,90 |
| 105954 | 000179797 | 2025-04-06 |   887,66 |
| **Total** |        |            | **11.295,75** |

NFe 104519 (R$ 6.847,60) tem o mesmo valor da única "NFe sem ordem" cobrada em conta_a_pagar — investigar se é fatura extraordinária ou faturamento sem pedido prévio.

### 13.5 Discrepância NFe (R$ 298k) vs OC não cancelado (R$ 107k)

A diferença se explica:
- 112 das 117 NFes 2025 fartam OCs criadas **em 2024** (não 2025) — o filtro `created>='2025-01-01'` em p2 excluía elas.
- Considerando todas as OCs faturadas pelas 112 NFes: 446 linhas em p2 (R$ 474.534,47), das quais 238 ENTREGUES (R$ 223.191) e 92 CANCELADAS (R$ 122.941).
- Saldo: NFes (R$ 283.635) ≈ ENTREGUES (R$ 223.191) + impostos/frete ≈ valor reconciliável. Sem gap real.

### 13.6 Paridade p1 vs p2

| Categoria | p1 | p2 |
| --- | ---: | ---: |
| Linhas DOCOL 2025+ | 171 | 207 |
| Valor | 265.432,10 | 290.672,11 |
| Diferença | — | +36 linhas (+R$ 25.240) |

p2 tem 36 linhas a mais. Investigar se são desmembramentos legítimos (ex: split de coleta) ou inserções diretas em p2 sem registro master. As views `view_fornecedor_compra` e `view_compras_financeiro` leem de p1 — relatórios baseados nelas subnotificam o universo executado.

## 14. Cruzamento com vendas e orçamentos

| Métrica                                                           |  Valor |
| ----------------------------------------------------------------- | -----: |
| Vendas distintas que geraram pedidos DOCOL desde 2025             |     28 |
| Range das vendas vinculadas                                       | 2024-05-29 a 2026-04-27 |
| Total dessas vendas (não só DOCOL)                                | R$ 1.166.779,88 |
| Vendas no histórico com DOCOL nos `fornecedores`                  |    370 |
| Orçamentos com DOCOL nos `fornecedores`                           |  2.013 |

A DOCOL aparece como componente pequeno (R$ 1k–5k) de vendas maiores. A taxa de conversão "orçamento com DOCOL → venda com pedido DOCOL" é baixíssima (≈1%).

## 15. Estoque vinculado

`estoque_has_compra` registra **55 entradas em estoque** para 49 idCompras DOCOL de 2025+. Compatível com os 47 pedidos em status ESTOQUE — 2 idCompras geraram múltiplas entradas (split de carga ou lote).

## 16. Followups confirmam descontinuação de SKUs

23 anotações em 12 OCs. Trechos representativos:

- OC 296709 (2025-05-02): _"VERIFICANDO COM A DOCOL A POSSIBILIDADE DE FATURAR, POIS É **DESCONTINUADO** E JÁ PROCURAMOS F[ORNECEDOR]"_
- OC 293157 (2025-05-02): _"EM CONTATO COM A DOCOL TENTANDO A SOLUÇÃO, PREVISÃO PARA MÊS 05"_
- OC 290205 (2024-01-22): _"ESTIMADO 96 DIAS CORRIDOS, APÓS 13/12/2023"_ — lead time de 3 meses
- OC 293692 (2025-03-18): _"AGUARDANDO FATURAMENTO PARA POSSIVEL USO"_

Padrão: descontinuação de itens pela DOCOL + lead times estendidos. **Não é uma decisão comercial unilateral da Staccato; é uma redução de catálogo do próprio fornecedor**, com efeito cascata: vendas reais sem item → cancelamento → substituição emergencial por DECA/MEKAL/FRANKE.

## 17. Ações recomendadas (priorizadas)

| Prioridade | Ação | Responsável | Valor em risco |
| --- | --- | --- | ---: |
| 🔴 P0 | Auditar 108 títulos vencendo em 2025-04-15 — bug ou pagamento não conciliado? | Financeiro + TI | R$ 276.945 |
| 🔴 P0 | Investigar 23 cancelamentos pós-faturamento — devolução ou erro? Ajustar NFe se devolução. | Compras + Fiscal | R$ 46.674 |
| 🟠 P1 | Corrigir 20 títulos com vencimento < emissão | Financeiro | R$ 56.987 |
| 🟠 P1 | Cancelar/consolidar 4 títulos duplicados por NFe | Financeiro | R$ 19.688 |
| 🟠 P1 | Reconciliar 5 NFes órfãs (R$ 11.295) com OC ou criar lançamento manual | Fiscal + Compras | R$ 11.295 |
| 🟡 P2 | Mapear catálogo DOCOL vivo vs descontinuado para evitar repetir cancelamentos | Compras | preventivo |
| 🟡 P2 | Validar paridade p1↔p2 e corrigir 16k registros com `idRelacionado` órfão | Engenharia ERP | sistêmico |
| 🟡 P2 | Atualizar `view_fornecedor_compra` e `view_compras_financeiro` para ler de p2 (estado real) | Engenharia ERP | sistêmico |

## 18. Consultas SQL utilizadas

```sql
-- Comparação p1 vs p2 (revela inversão sem_venda/com_venda)
SELECT 'p1' tab,
  CASE WHEN idVenda IS NULL THEN 'sem_venda' ELSE 'com_venda' END origem,
  status, COUNT(*) qtd, ROUND(SUM(preco),2) valor
FROM pedido_fornecedor_has_produto WHERE fornecedor='DOCOL' AND created>='2025-01-01'
GROUP BY origem, status
UNION ALL
SELECT 'p2', CASE WHEN idVenda IS NULL THEN 'sem_venda' ELSE 'com_venda' END,
  status, COUNT(*), ROUND(SUM(preco),2)
FROM pedido_fornecedor_has_produto2 WHERE fornecedor='DOCOL' AND created>='2025-01-01'
GROUP BY origem, status;

-- Vendas reais afetadas pelos cancelamentos DOCOL (via p1)
SELECT idVenda, COUNT(*) linhas_canc, ROUND(SUM(preco),2) valor_canc
FROM pedido_fornecedor_has_produto
WHERE fornecedor='DOCOL' AND status='CANCELADO'
  AND created>='2025-01-01' AND idVenda IS NOT NULL
GROUP BY idVenda ORDER BY valor_canc DESC;

-- Anomalia 2025-04-15
SELECT dataPagamento, COUNT(*) qtd, ROUND(SUM(valor),2) valor
FROM conta_a_pagar_has_pagamento
WHERE contraParte IN ('DOCOL','DOCOL INDUSTRIA E COMERCIO LTDA')
  AND status='PENDENTE' AND dataEmissao>='2025-01-01'
GROUP BY dataPagamento;

-- Títulos duplicados por NFe
SELECT idNFe, COUNT(*) qtd_titulos, ROUND(SUM(valor),2) valor_total,
       GROUP_CONCAT(idPagamento) ids
FROM conta_a_pagar_has_pagamento
WHERE contraParte IN ('DOCOL','DOCOL INDUSTRIA E COMERCIO LTDA')
  AND status='PENDENTE' AND dataEmissao>='2025-01-01'
GROUP BY idNFe HAVING COUNT(*) > 1;

-- Vencimento anterior à emissão
SELECT COUNT(*) qtd, ROUND(SUM(valor),2) valor
FROM conta_a_pagar_has_pagamento
WHERE contraParte IN ('DOCOL','DOCOL INDUSTRIA E COMERCIO LTDA')
  AND status='PENDENTE' AND dataPagamento<dataEmissao;

-- NFe órfãs (não vinculadas a OC)
SELECT n.idNFe, n.numeroNFe, DATE(n.dataHoraEmissao) emissao, ROUND(n.valor,2) valor
FROM nfe n
LEFT JOIN view_ordemcompra_nfe voc ON voc.idNFe = n.idNFe
WHERE n.tipo='ENTRADA' AND n.emitente LIKE '%DOCOL%' AND n.status='AUTORIZADA'
  AND n.dataHoraEmissao>='2025-01-01' AND voc.ordemCompra IS NULL;

-- Cancelamentos pós-faturamento (com idCompra)
SELECT
  CASE WHEN idCompra IS NULL THEN 'antes_faturar' ELSE 'apos_faturar' END categoria,
  COUNT(*) linhas, ROUND(SUM(preco),2) valor
FROM pedido_fornecedor_has_produto2
WHERE fornecedor='DOCOL' AND status='CANCELADO' AND created>='2025-01-01'
GROUP BY categoria;

-- Substitutos das vendas afetadas
SELECT v.idVenda,
  COUNT(DISTINCT CASE WHEN p.fornecedor='DOCOL' THEN p.idPedido2 END) docol_p2,
  GROUP_CONCAT(DISTINCT CASE WHEN p.fornecedor<>'DOCOL' THEN p.fornecedor END) substitutos
FROM venda v
JOIN pedido_fornecedor_has_produto2 p ON p.idVenda=v.idVenda
WHERE v.idVenda IN (
  SELECT DISTINCT idVenda FROM pedido_fornecedor_has_produto
  WHERE fornecedor='DOCOL' AND status='CANCELADO' AND created>='2025-01-01' AND idVenda IS NOT NULL
)
GROUP BY v.idVenda;
```
