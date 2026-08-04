# Performance audit — 2026-08-04

## Update 2 (same day): correctness bug found and fixed in Consistência

While working through the "systemic finding" list, `widgetconsistencia.cpp`'s
`view_consistencia_vp_op_quant` (Venda × Orçamento quantity check) turned out to have a **join
fan-out correctness bug**, not just a slow query: it joined `venda_has_produto` to
`orcamento_has_produto` on `(idOrcamento, codComercial)` and summed both sides *after* the join.
Both tables legitimately have multiple rows per product (room-by-room quote lines, confirmed
intentional via real data and `Venda::copiaProdutosOrcamento`, `src/venda.cpp:1679`, which copies
every `orcamento_has_produto` row 1:1 at sale creation). Whenever the row counts differed between
the two sides — which only happens from a real post-creation edit, since `orcamento.cpp` allows
editing line items after the fact and `venda.cpp` does not — the join multiplied both sums by the
other side's row count, corrupting the comparison. The view was over-reporting inconsistencies by
~2.3× (1549 flagged vs. 682 real). Fixed by aggregating each side independently before comparing;
full investigation and before/after evidence in `db/fix_view_consistencia_vp_op_quant.sql`. See
`.claude/performance-audit-tracking.md` for the complete session-by-session log (this report
captures only the original 2026-08-04 pass).

## Update (same day): Findings 1-3 applied, remeasured

Per instruction, pagination (`SqlPaginatedModel` migration, i.e. Findings 5/6's fix) stays out of
scope for this round — only query/index fixes. Applied and remeasured on the local DB:

- **Finding 1** (Estoque `HAVING`→`WHERE`): shipped in `src/sql.cpp` (`Sql::queryEstoque`, param
  renamed `having`→`where` since it's no longer a `HAVING` clause) + `src/widgetestoques.cpp`.
  Remeasured: **1448ms** (matches the 1550ms first measured; the fix doesn't depend on any index).
- **Finding 2** (`estoque.status` index): added locally, documented in
  `db/add_index_estoque_status.sql` (dry-run/apply, matching existing convention — not applied to
  production, that's a manual step same as the other `add_index_*.sql` files). Remeasured with the
  index live: `view_agendar_coleta` 814ms → **11.6ms**, `view_coleta` → **2.1ms**, `view_recebimento`
  → **0.05ms**.
- **Finding 3** (`pedido_fornecedor_has_produto.ordemCompra` index): added locally, documented in
  `db/add_index_pedido_fornecedor_has_produto_ordemcompra.sql`. Remeasured: single-OC lookup went
  from `type=ALL, rows=123168` to `type=ref, key=idx_pf1_ordemcompra, rows=5, Using index`.
- **Finding 4** (`view_compras_financeiro`): **not meaningfully fixed by indexing alone.** The new
  index removes the query's explicit `Sort` step (now reads `pf` pre-sorted via the index), but the
  view has no `WHERE` at all — it aggregates *every* row in `pedido_fornecedor_has_produto` — so an
  index can't reduce what gets scanned. Remeasured: 972ms → 931ms (~4%, within noise). The real fix
  is bounding the row count (pagination or a default status/date filter), both out of scope here.
- **Findings 5 & 6** (Orçamento/Venda): **no index-only fix exists.** Both measured queries are
  `SELECT *` with no `WHERE` at all on tab-open (worst case) — an index cannot speed up a scan that
  reads 100% of rows by definition. Their only available fixes are pagination (out of scope) or
  changing default UI behavior (a status filter that hides closed records by default), which is a
  product decision, not a pure query optimization — left untouched.

See "Suggested prioritization" at the bottom for what's left.


Full-codebase follow-up to the NF-e Entrada/Saída and Financeiro Contas a Pagar/Receber pagination
fixes (`50fe05d7`, `d29c6841`, `1639cf4c`, `e4515291`, `db/add_index_*.sql`). Those fixed one class
of bug — unbounded/full-scan list queries, non-sargable filters, missing indexes — in four screens.
This audit checks whether the same class of bug exists elsewhere, and measures it instead of
guessing.

**Method**: every `SqlTableModel`/`SqlQueryModel`/`SqlPaginatedModel` list-widget instantiation in
`src/*.cpp` was resolved to its backing table/view (142 `setTable()` + 30 `setQuery()` call sites).
Every one of the 143 DB views was cross-referenced against the source tree; 58 are actually queried
by the app (the other 85 — `edu_view_*`, `dexco_*`, `viewexcel*`, `v_rpt_*`, etc. — are legacy/BI
exports with no C++ caller and are out of scope, listed at the end). Each of those 58 views' `SHOW
CREATE VIEW` was scanned for non-sargable functions, `GROUP BY`/`HAVING`, and nesting. The 80 base
tables were sized via `information_schema.tables`. Every flagged query was then run with
`EXPLAIN`/`EXPLAIN ANALYZE` against the local `staccato` DB (a real, if stale — roughly end-of-2025
— copy of production; same schema and indexes), the same instrument already used in
`db/add_index_*.sql`. **Absolute timings here are a lower bound**: production has ~8 more months of
growth on every table below. Treat the ms numbers as "at least this bad today, worse in prod", and
the query-plan shape (full scan vs. index range, `HAVING` vs `WHERE`) as the durable signal.

Two fixes were verified by actually creating the proposed index locally and re-running
`EXPLAIN ANALYZE` (then dropping it — no schema changes were left in place; nothing was applied to
production). All numbers below are real measurements, not estimates.

## Summary of confirmed, measured findings

| # | Screen | Issue | Measured | Fix |
|---|--------|-------|----------|-----|
| 1 | Estoque (main tab) | `HAVING restante > 0` instead of `WHERE` | 6373ms → 1550ms (4.1×) | Move predicate to `WHERE` |
| 2 | Logística Coleta/Recebimento/Agendar Coleta | `estoque.status` has no index | 814ms → 15.9ms (51×) | Add `estoque(status)` index |
| 3 | Compras › Histórico (row click) | `pedido_fornecedor_has_produto.ordemCompra` has no index | full scan, 123168 rows, every click | Add index — exact sibling of already-applied `pedido_fornecedor_has_produto2` fix |
| 4 | Financeiro › Compras / Compras › Histórico | `view_compras_financeiro` unbounded load | 972ms full scan + `GROUP_CONCAT` every tab open | Same root cause as #3 |
| 5 | Orçamento | Unbounded load + leading-wildcard search, same shape as pre-fix NFe/Financeiro | 2.57s (open) / 2.17s (search) | Sargable rewrite + pagination, same recipe as NFe/Financeiro |
| 6 | Venda | Same shape as #5 | 1.39s (open) / 0.87s (search) | Same recipe |

Plus a **systemic** finding: of ~40 genuine list/browse screens in the app, only 3
(`WidgetNFeEntrada`, `WidgetNFeSaida`, `WidgetFinanceiroContas`) have been migrated to the
sliding-window `SqlPaginatedModel`. Every other list screen still instantiates `SqlTableModel` with
the default constructor (`limit = 0`), which per `SqlTableModel::selectStatement()`
(`src/sqltablemodel.cpp:108`) means **no `LIMIT` clause is ever added** — the full filtered result
set is always requested from MySQL.

---

## Finding 1 — Estoque tab: `HAVING` instead of `WHERE`

`WidgetEstoques::montaFiltro()` (`src/widgetestoques.cpp:111`) calls `Sql::queryEstoque()`
(`src/sql.cpp:441`), which filters the in-stock/out-of-stock toggle via `HAVING restante > 0`
**after** a 5-way `LEFT JOIN` and `GROUP BY e.idEstoque` over the entire `estoque` table (76,000
rows). `restante` is a plain column of `estoque`, not an aggregate, and `GROUP BY e.idEstoque`
groups 1:1 with `estoque`'s primary key — the `GROUP BY` exists only to collapse the join fan-out
from `estoque_has_compra`/`pedido_fornecedor_has_produto2`, not because `restante` needs
aggregating. So the `HAVING` can safely become part of the `WHERE`, letting MySQL filter before
joining instead of after. This is the exact same anti-pattern already diagnosed and fixed for
`conta_a_receber_vencidos`/`vencer` in `db/add_index_conta_receber_status_date_rep.sql`
(`HAVING` → `WHERE`, 179928 → 13474 measured there).

This is the single highest-traffic screen in the app (every user opens Estoque routinely), so this
is the highest-value fix in this report.

```sql
-- current (measured 6373ms):
EXPLAIN ANALYZE
SELECT e.idEstoque, e.status, e.restante FROM estoque e
LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque
LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON pf2.idPedido2 = ehc2.idPedido2
LEFT JOIN nfe n ON e.idNFe = n.idNFe
LEFT JOIN produto p ON e.idProduto = p.idProduto
LEFT JOIN galpao g ON e.idBloco = g.idBloco
WHERE e.status NOT IN ('CANCELADO','IGNORAR')
GROUP BY e.idEstoque
HAVING restante > 0;
-- -> Filter: (e.restante > 0.0000) (actual time=45.9..6373 rows=4202)
--     -> Group (no aggregates) (actual time=28.7..6338 rows=82058)   <- groups ALL 82k rows first

-- proposed (measured 1550ms, 4.1x faster, zero schema change):
EXPLAIN ANALYZE
SELECT e.idEstoque, e.status, e.restante FROM estoque e
LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque
LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON pf2.idPedido2 = ehc2.idPedido2
LEFT JOIN nfe n ON e.idNFe = n.idNFe
LEFT JOIN produto p ON e.idProduto = p.idProduto
LEFT JOIN galpao g ON e.idBloco = g.idBloco
WHERE e.status NOT IN ('CANCELADO','IGNORAR') AND e.restante > 0
GROUP BY e.idEstoque;
-- -> Group (no aggregates) (actual time=1.96..1550 rows=4202)
--     -> Filter: (...status...) and (e.restante > 0.0000)) (actual time=0.55..504 rows=4202)  <- filters FIRST
```

**Fix**: in `Sql::queryEstoque()`, move the `having` parameter's condition into the `WHERE` clause
(both call sites — `montaFiltroContabil()` uses a separate view and is unaffected). Note `e.status
NOT IN (...)` itself still forces a full `PRIMARY` index scan even after this fix (a `NOT IN` won't
use a secondary index over a non-covering column set) — the 1550ms remaining is mostly that scan.
A composite `estoque(status, restante)` index was not tested against the negated `NOT IN`
predicate; worth a follow-up EXPLAIN if 1550ms is still too slow after the WHERE fix ships.

## Finding 2 — `estoque.status` has no index (Logística: Coleta/Recebimento/Agendar Coleta)

`estoque` (76,000 rows) has 9 indexes (`idProduto`, `recebidoPor`, `idNFe`, a fulltext on
`descricao`/`codComercial`, IBS/CBS/IS classification columns, a pallet index) but **none on
`status`**, despite `status` being the primary equality filter for the three views that back the
Logística "coleta"/"recebimento" family:

- `view_agendar_coleta` (`WHERE e.status = 'EM COLETA' AND pf2.dataPrevColeta IS NULL`) — `WidgetLogisticaAgendarColeta`
- `view_coleta` (`WHERE e.status = 'EM COLETA' AND pf2.dataPrevColeta IS NOT NULL`) — `WidgetLogisticaColeta`
- `view_recebimento` (`WHERE e.status = 'EM RECEBIMENTO' AND ...`) — `WidgetLogisticaRecebimento`

Verified by actually adding the index locally and reverting:

```sql
-- before (measured 814ms): "Index scan on e using PRIMARY (actual time=0.548..751 rows=84614)"
--   then Filter: (e.status = 'EM COLETA') -> 153 rows kept out of 84614 scanned
EXPLAIN ANALYZE SELECT * FROM view_agendar_coleta;

-- ALTER TABLE estoque ADD INDEX idx_estoque_status (status);
-- after (measured 15.9ms, 51x faster): "Index lookup on e using idx_estoque_status (status='EM COLETA')"
EXPLAIN ANALYZE SELECT * FROM view_agendar_coleta;
```

**Fix**: `ALTER TABLE estoque ADD INDEX idx_estoque_status (status);` — single-column, low-risk,
same pattern as `db/add_index_pedido_fornecedor_has_produto2_ordemcompra.sql`. Benefits all three
screens above (and their `view_fornecedor_logistica_*` counterparts, which join through the same
filtered `estoque` subset). Does not conflict with Finding 1's fix (different table condition
shape — equality vs. `NOT IN`).

## Finding 3 — `pedido_fornecedor_has_produto.ordemCompra` has no index (sibling of an already-fixed bug)

`db/add_index_pedido_fornecedor_has_produto2_ordemcompra.sql` (23/07/2026) fixed exactly this
problem on `pedido_fornecedor_has_produto2` ("2"). The **non-"2" table**,
`pedido_fornecedor_has_produto` (123,168 rows), has the identical shape and was not covered by that
fix:

```
mysql> EXPLAIN SELECT idPedido1 FROM pedido_fornecedor_has_produto WHERE ordemCompra = 305525;
+----+-------------+--------------------------------+------+---------------+------+---------+------+--------+----------+-------------+
| id | select_type | table                          | type | possible_keys | key  | key_len | ref  | rows   | filtered | Extra       |
+----+-------------+--------------------------------+------+---------------+------+---------+------+--------+----------+-------------+
|  1 | SIMPLE      | pedido_fornecedor_has_produto  | ALL  | NULL          | NULL | NULL    | NULL | 123168 |    10.00 | Using where |
```

This isn't hypothetical — it's live in `WidgetCompraHistorico::on_treeView_activated` (or
equivalent row-click handler), `src/widgetcomprahistorico.cpp:189`:
`modelProdutos.setFilter("ordemCompra = " + ordemCompra)` against
`modelProdutos.setTable("pedido_fornecedor_has_produto")` (`:59`) — a full 123k-row scan on
**every row click** in Compras › Histórico. The view built on the same table,
`view_compras_financeiro` (`GROUP BY pf.ordemCompra WHERE pf.ordemCompra IS NOT NULL`), has the
same gap (see Finding 4).

**Fix**: `ALTER TABLE pedido_fornecedor_has_produto ADD INDEX idx_pf1_ordemcompra (ordemCompra);`
— same low-risk pattern as the existing fix for the "2" table.

## Finding 4 — `view_compras_financeiro` unbounded load (Financeiro › Compras, Compras › Histórico)

`WidgetFinanceiroCompra` (`src/widgetfinanceirocompra.cpp:37`) and `WidgetCompraHistorico`
(`src/widgetcomprahistorico.cpp:49`) both load `view_compras_financeiro` into an unbounded
`SqlTableModel` (`limit = 0`) on every tab open:

```
EXPLAIN ANALYZE SELECT * FROM view_compras_financeiro;
-> Table scan on view_compras_financeiro (actual time=967..972 rows=38191)
    -> Materialize (actual time=967..967 rows=38191)
        -> Group aggregate: group_concat(...) x6, count(...), sum(...) (actual time=485..840 rows=38191)
            -> Sort: pf.ordemCompra (actual time=481..527 rows=121807)
                -> Filter: (pf.ordemCompra is not null) (actual time=1.22..358 rows=121807)
                    -> Table scan on pf (actual time=1.22..345 rows=126327)   <- full scan, same root cause as Finding 3
```

972ms per open, returning all 38,191 grouped purchase orders unbounded — no default filter narrows
this to "open"/recent orders. Finding 3's index directly speeds up the inner scan; the remaining
cost is the `GROUP_CONCAT`-heavy aggregation over the full table, which only pagination (or a
default status/date filter, mirroring what NFe/Financeiro already do) actually bounds.

## Finding 5 — Orçamento: same pre-fix shape as NFe/Financeiro

`WidgetOrcamento::montaFiltro()` (`src/widgetorcamento.cpp:282`) builds a leading-wildcard
`LIKE '%text%'` search across `idOrcamento`/`vendedor`/`cliente`/`profissional` — computed/joined
columns of `view_orcamento` — applied via `QSqlTableModel::setFilter()`, on an unbounded model
(`SqlTableModel modelOrcamento` at `src/widgetorcamento.h:27`, default constructor). This is the
identical shape to the NF-e search bug fixed in `e4515291` (leading-wildcard search applied to an
unbounded/un-materialized query).

```
-- tab opened, no search/status filter (worst case if all status checkboxes are unchecked):
EXPLAIN ANALYZE SELECT * FROM view_orcamento WHERE 1;
-> ... Table scan on o (actual time=1.54..342 rows=161537) ...   total actual time: 2569ms

-- typing a search term:
EXPLAIN ANALYZE SELECT * FROM view_orcamento WHERE (idOrcamento LIKE '%silva%' OR vendedor LIKE '%silva%'
  OR cliente LIKE '%silva%' OR profissional LIKE '%silva%');
-> Filter: (...4x LIKE...) (actual time=1.24..1917 rows=6302)
    -> ...161537 rows joined through 4 point-lookups each...           total actual time: 2166ms
```

`orcamento` has a `status` column but it's the 4th column of a 7-column composite index
(`index7`), not a leading/standalone index — so even adding a default "hide closed" status filter
wouldn't get an efficient index lookup today without also adding a leading-`status` index (unlike
`venda`, which already has `idx_status`, see Finding 6).

**Fix options** (in order of effort): (a) minimal — apply the same "search enters the page-cut
subquery as a non-correlated `IN`, materialized once" rewrite from `e4515291`, keeping the current
`SqlTableModel`; (b) full — migrate to `SqlPaginatedModel` like NFe/Financeiro, which also fixes the
unbounded-load half of this finding, not just the search half.

## Finding 6 — Venda: same shape, currently smaller/less severe

`WidgetVenda::montaFiltro()` (`src/widgetvenda.cpp:71`) has the identical construction — leading
wildcard search across `idVenda`/`vendedor`/`cliente`/`profissional`/`ordemRepresentacao`, unbounded
`SqlTableModel modelVenda` (`src/widgetvenda.h:29`).

```
EXPLAIN ANALYZE SELECT * FROM view_venda WHERE 1;                                    -- 1393ms, table scan 50014 rows
EXPLAIN ANALYZE SELECT * FROM view_venda WHERE (...5x LIKE '%silva%'...);            -- 871ms
```

Less severe than Orçamento locally (venda: 48,645 rows vs. orcamento: 153,419), but same root
cause, and `venda` already has a leading `idx_status` index available — a default "hide
CANCELADO/etc." filter here would actually be cheap to add and should be prioritized ahead of the
full `SqlPaginatedModel` migration.

---

## Systemic finding — unbounded `SqlTableModel` is the default everywhere except 3 screens

`SqlTableModel::selectStatement()` (`src/sqltablemodel.cpp:108`) only appends `LIMIT` when
`limit > 0`, and every `SqlTableModel` member found in the codebase uses the default/no-arg
constructor (`limit = 0`) **except** the three screens already migrated to `SqlPaginatedModel`
(`WidgetNFeEntrada`, `WidgetNFeSaida`, `WidgetFinanceiroContas`). Below is the complete inventory of
every list/browse screen (excluding detail-dialog sub-grids, which are always filtered to one
parent record's ID — see note below), ranked by the largest base table each one's view joins
through, from the local DB:

| Backing table (max rows joined) | Screen(s) | GROUP BY? | Individually measured above? |
|---:|---|:---:|:---:|
| 1,028,362 (`orcamento_has_produto`) | Consistência (tab 7: `view_consistencia_vp_op_quant`) | Y | no |
| 346,271 (`produto`) | Compras › Faturar (`view_fornecedor_compra_faturar`, `view_faturamento`), Logística › Agendar Coleta (`view_agendar_coleta`), Estoque › Produto (`view_produto`) | mixed | Agendar Coleta: yes (Finding 2) |
| 216,254 (`conta_a_pagar_has_pagamento`) | Financeiro › Fluxo de Caixa (`view_fluxo_caixa_realizado`), Compras › Histórico financeiro tab (`view_conta_pagar_idcompra`) | Y | no |
| 211,004 (`venda_has_produto2`) | Compras › Consumos (`view_ordemcompra`), Compras › Pendentes (`view_venda_produto`), Consistência (4 tabs), Devolução, Logística › Entregues (`view_entrega`), Logística › Entregas carga tab | mixed | no |
| 193,722 (`venda_has_produto`) | Consistência tab 2 | Y | no |
| 180,999 (`conta_a_receber_has_pagamento`) | (all consumers are detail dialogs filtered by parent — see note) | — | — |
| 154,249 (`pedido_fornecedor_has_produto2`) | Compras › Confirmar (2 models), Logística › Agendar Coleta fornecedor tab, Coleta (2 models), Logística › Entregas produto tab, Logística › Recebimento (2 models), Logística › Representação | Y (most) | Coleta/Recebimento: yes (Finding 2) |
| 153,419 (`orcamento`) | **Orçamento** (`view_orcamento`) | N | **yes (Finding 5)** |
| 153,036 (`comissao`/`venda`) | Relatório (`view_relatorio`) | Y | no |
| 138,057 (`veiculo_has_produto`) | Galpão, Logística › Agendar Coleta/Entrega veículo tabs, Logística › Caminhão (2 models), Logística › Entregas calendário tab | mixed | no |
| 123,168 (`pedido_fornecedor_has_produto`) | Compras › Consumos pedido tab, Compras › Gerar (2 models), **Compras › Histórico** (2 models), Compras › Pendentes, Compras › Resumo, Consistência tab 1, **Financeiro › Compras**, Logística › Representação fornecedor tab | mixed | **yes (Findings 3 & 4)** |
| 104,737 (`orcamento_has_followup`) | Followup (Orçamento variant) | N | no |
| 68,138 (`nfe`) | NFe Distribuição (`view_nfe_distribuicao`) — Entrada/Saída already fixed | N | no |
| 51,881 (`cliente`) | **Venda** (`view_venda`) | N | **yes (Finding 6)** |

*Note on detail dialogs*: rows for `conta_a_pagar_has_pagamento`/`conta_a_receber_has_pagamento`,
`orcamento_has_produto` (order item grid), `venda_has_produto`/`venda_has_produto2` (sale item
grid), `estoque_has_consumo`, and similar `modelItem`/`modelConsumo`/`modelCompra` instances in
`orcamento.cpp`, `venda.cpp`, `devolucao.cpp`, `estoque.cpp`, `inputdialogconfirmacao.cpp`,
`compraavulsa.cpp`, `importarxml.cpp` etc. are **not** list/browse screens — they're always
`setFilter()`'d to one parent record's ID immediately after `setTable()` (e.g. one open order's line
items), so despite the large table sizes in the inventory, the actual query executed is a
single-key filtered lookup, not a full scan. These were excluded from EXPLAIN measurement as
low-risk *provided* the parent-ID column stays indexed (spot-checked: `idOrcamento`, `idVenda`,
`idCompra` etc. all have FK indexes already).

**What this means**: the row above for Orçamento/Venda/Compras/Estoque/Logística-Coleta are
individually measured and confirmed (Findings 1-6). The remaining ~25 screens in the table share
the exact same code shape (`SqlTableModel`, default constructor, view built on a 100k+ row table,
often with `GROUP_CONCAT`/`GROUP BY`) but were not individually run through `EXPLAIN` in this pass —
treat them as **same-pattern candidates**, not confirmed-safe. The cheapest way to triage the rest:
open each screen against the local DB with no filter and watch `SHOW PROCESSLIST`/query time, same
as done here for the ones already measured.

---

## Views excluded from scope (no C++ caller found)

85 of the 143 views in the schema have no reference anywhere in `src/*.cpp` (checked via literal
string-match against every view name): `aaa`, `bbb`, `despesas por grupo`, `dexco_client`,
`dexco_distributor`, `dexco_invoice_invoiceitem`, `dexco_productstock`, `dexco_soc`,
`edu_view_consumoestoque`, `edu_view_despesas`, `edu_view_estoquecd`, `edu_view_estoqueportinari`,
`edu_view_orcamentos`, `edu_view_receitas`, `edu_view_vendas`, `edu_view_vendedores`,
`v_rpt_comissao_vendedores`, `v_rpt_comissao_vendedores_pedido`, `v_rpt_faturamento`, `v_rpt_rts`,
`vendascmv`, `view--rt`, `view_a_pagar_vencer_base`, `view_a_pagar_vencidos_base`,
`view_a_receber_vencer_base`, `view_a_receber_vencidos_base`, `view_agendar_entrega`,
`view_calendario2`, `view_conta_receber`, `view_custo_estoque`, `view_entrega_pendente`,
`view_entrega_produtos`, `view_estoque_contabil`\*, `view_estoque_disponivel`, `view_estoque_zerado`,
`view_financeiro`, `view_fluxo_caixa_pendente`, `view_fluxo_caixa_realizado_grupo`,
`view_fluxo_resumo_pendente`, `view_fluxo_resumo_realizado`, `view_followup_venda`, `view_galpao`,
`view_gares`, `view_grafico_loja`, `view_grafico_lojas`, `view_mkt_creditos`, `view_mkt_pagamentos`,
`view_nfe_entrada`, `view_nfe_ordemcompra`, `view_nfe_saida`, `view_ordemcompra_nfe`,
`view_pedido_fornecedor_livre`, `view_preparar_entrega`, `view_produto_endereco`,
`view_produtos_pendentes`, `view_relatorio_loja`\*, `view_relatorio_pagar`, `view_relatorio_receber`,
`view_relatorio_reposicao`, `view_relatorio_vendedor`\*, `view_resumo_relatorio`, `view_validacao`,
`view_vinculo_estoque_compra`, and the `viewdespesas*`/`viewestoqueconsumo*`/`viewexcel*`/`viewflx*`/
`viewfrete*`/`viewlucroreal`/`viewpedidofornecedor`/`viewreceitaserviços`/`viewrelatoriosgestao`/
`viewresultadocd`/`viewrt*`/`viewsimuladorexcel`/`viewtaxacartaoexcel`/`viewvendaxcmv`/`xl_*`/`xxxx`
group. These are almost certainly legacy or direct-DB reporting/Excel-export views (BI tooling
querying the DB directly, outside the Qt app) and were excluded from this audit — they don't affect
in-app responsiveness. (\*`view_estoque_contabil`, `view_relatorio_loja`, `view_relatorio_vendedor`
are built as raw SQL strings by `Sql::` C++ functions rather than queried as DB views by name, so
the literal-name grep doesn't find them as "used" even though they are — they're covered under
`Sql::` functions instead, not omitted from the app, just from this particular cross-reference
table.) If any of these turn out to still be live (e.g. wired into a report/Excel export not caught
by the grep), they weren't touched here and would need separate scoping.

## N+1 query patterns

Checked (`for`/`while` loops containing `QSqlQuery`/`.exec()`/`.select()`) across all of `src/*.cpp`.
No problematic read-side N+1 found: the loops that do exist (e.g. `widgetcomprafaturar.cpp:106-221`)
are write-side batch updates bound to the user's current row selection (typically single digits to
low tens of rows), reusing one prepared `QSqlQuery` across iterations — the correct pattern, not a
bug.

## Suggested prioritization

1. ~~**Finding 1** (Estoque `HAVING`→`WHERE`)~~ — **done**, see update at top.
2. ~~**Finding 2** (`estoque.status` index)~~ — **done**, see update at top.
3. ~~**Finding 3** (`pedido_fornecedor_has_produto.ordemCompra` index)~~ — **done**, see update at top.
4. **Finding 5/6** (Orçamento/Venda) — no index-only fix available (see update at top); needs either
   pagination (currently out of scope) or a default status filter (product decision). Orçamento is
   worse today (153k vs 48k base rows) so goes first whenever this is picked back up.
5. **Finding 4** and the rest of the systemic table — same ceiling as #4: indexing alone can't bound
   an unfiltered `SELECT *`/`GROUP BY` over a whole table. Triage the ~25 same-pattern screens to
   confirm which are genuinely unbounded (need pagination/default-filter, out of scope for now) vs.
   which have a narrower, index-fixable query hiding under an unbounded model (like Findings 2/3
   turned out to be) — those narrower fixes stay in scope and are worth finding.

**Remaining production step**: the two new indexes
(`db/add_index_estoque_status.sql`, `db/add_index_pedido_fornecedor_has_produto_ordemcompra.sql`)
are applied on the local dev DB and documented, but — matching how every prior `add_index_*.sql` in
this repo works — were not pushed to production; that's a manual step for whoever has prod access.
