# Performance audit — tracking

Living checklist for the performance audit started 2026-08-04 (see
`.claude/performance-audit-2026-08-04.md` for the original findings write-up and evidence). This
file exists so we don't lose track of what's actually been measured vs. only statically flagged —
update it as each item gets `EXPLAIN`'d and/or fixed, don't let it go stale.

**Status legend**

| Symbol | Meaning |
|---|---|
| ✅ | Fixed — index/query change applied, remeasured, confirmed better |
| 🔍 | Measured — `EXPLAIN`/`EXPLAIN ANALYZE` run; either OK as-is or the only fix is out of current scope (pagination) |
| ⏳ | Not measured — flagged by the static pass (row count / `GROUP BY` / pattern) but no `EXPLAIN` run yet |
| 🚫 | Excluded — detail dialog or lookup filtered by a parent ID, presumed low-risk (see caveat below each table) |

Scope note (2026-08-04): pagination (`SqlPaginatedModel` migration) is explicitly **out of scope**
for now — only query/index-level fixes count as "fixed" here. Items whose only available fix is
pagination are marked 🔍 (measured, not fixable in current scope), not ⏳.

---

## A. Applied fixes (log)

| Date | Item | Fix | Files | Before → After |
|---|---|---|---|---|
| 2026-08-04 | Estoque tab full load | `HAVING restante > 0` → `WHERE` | `src/sql.cpp` (`Sql::queryEstoque`), `src/widgetestoques.cpp` | 6373ms → 1448ms |
| 2026-08-04 | `estoque.status` unindexed | new index `idx_estoque_status` | `db/add_index_estoque_status.sql` (applied local only; prod = manual) | `view_agendar_coleta` 814ms→11.6ms; `view_coleta`→2.1ms; `view_recebimento`→0.05ms |
| 2026-08-04 | `pedido_fornecedor_has_produto.ordemCompra` unindexed | new index `idx_pf1_ordemcompra` | `db/add_index_pedido_fornecedor_has_produto_ordemcompra.sql` (applied local only; prod = manual) | OC lookup: `type=ALL,rows=123168` → `type=ref,rows=5` |
| 2026-08-04 | `orcamento_has_produto(idOrcamento,codComercial)` unindexed | new index `idx_op_orcamento_codcomercial` | `db/add_index_orcamento_has_produto_orcamento_codcomercial.sql` (applied local only; prod = manual) | `view_consistencia_vp_op_quant`: 19.9s → 11.6s |
| 2026-08-04 | `view_consistencia_vp_op_quant` — **correctness bug**, not just slow: join-then-`SUM` fan-out inflated both sides whenever `orcamento_has_produto`/`venda_has_produto` had a different number of rows per product (room-by-room quote lines vs. post-copy edits) | Rewrote view to aggregate each side independently (correlated subquery for the orçamento side) before comparing — see `db/fix_view_consistencia_vp_op_quant.sql` for the full investigation | `db/fix_view_consistencia_vp_op_quant.sql` (view redefinition, applied local only; prod = manual, also needs the index above) | Flagged rows: 1549 → 682 (was over-reporting by ~2.3×); time: 11.6s → 11.4s (same ballpark, now correct) |
| 2026-08-04 | `view_estoque_contabil` ("Estoque Contábil" toggle) — same `HAVING`-not-`WHERE` shape as Finding 1: `contabil` (`e.quant + e.ajuste + ehc.contabil`) isn't a true aggregate, `GROUP BY` only exists to undo join fan-out | Moved `contabil > 0` into `WHERE` | `src/sql.cpp` (`Sql::view_estoque_contabil`) | 8.59s → 4.26s (2×) |

**Still pending**: push all new index/view scripts to production (manual step, same workflow as every
prior `db/add_index_*.sql` in this repo).

---

## B. Browse/list screens (the real risk surface)

One row per `setTable`/`setQuery` call site in a `Widget*` tab screen, sorted by the largest base
table its query touches (local DB row count, see original report for methodology/caveats on
absolute numbers). `GB` = view has a `GROUP BY`.

| Rows | GB | File:line | Model | Table/view | Status | Note |
|---:|:-:|---|---|---|:-:|---|
| 1,028,362 | Y | widgetconsistencia.cpp:108 | model7 | view_consistencia_vp_op_quant | ✅ | Correctness bug + perf, see `db/fix_view_consistencia_vp_op_quant.sql` — was over-reporting inconsistencies ~2.3× |
| 346,271 | Y | widgetcomprafaturar.cpp:28 | modelResumo | view_fornecedor_compra_faturar | 🔍 | 69.6ms, fine — `pf2_status` index covers the `status='EM FATURAMENTO'` filter well |
| 346,271 | Y | widgetcomprafaturar.cpp:38 | modelFaturamento | view_faturamento | 🔍 | 38.1ms, fine, same index |
| 346,271 | N | widgetestoqueproduto.cpp:22 | modelProdutos | view_produto | 🔍 | always filtered (`estoque=TRUE AND descontinuado=FALSE AND desativado=FALSE`), measured 621ms, fine |
| 346,271 | Y | widgetlogisticaagendarcoleta.cpp:42 | modelEstoque | view_agendar_coleta | ✅ | Finding 2, `idx_estoque_status` |
| 216,254 | Y | pagamentosdia.cpp:24 | modelFluxoCaixa | view_fluxo_caixa_realizado | 🔍 | 547ms, fine |
| 216,254 | Y | widgetcomprahistorico.cpp:78 | modelFinanceiro | view_conta_pagar_idcompra | 🔍 | 634ms, fine |
| 216,254 | N | widgetfinanceirocontas.cpp:840 | modelImportar | conta_a_pagar_has_pagamento | 🚫 | import dialog, not a browse grid |
| 216,254 | N | widgetrh.cpp:191 | modelPagar | conta_a_pagar_has_pagamento | 🚫 | false positive — local, write-only model (Excel import), only ever inserts rows, never browsed (`setTable()` defaults filter to `"0"`) |
| 211,004 | N | widgetcompraconsumos.cpp:42 | modelProduto | view_ordemcompra | 🔍 | 494ms, acceptable (slowest of the Compras cluster, no fix needed) |
| 211,004 | N | widgetcompradevolucao.cpp:45 | modelVendaProduto | venda_has_produto2 | 🔍 | always filtered (`status = / != 'PENDENTE DEV.' AND quant < 0`), worst case measured 838ms, fine |
| 211,004 | N | widgetcomprapendentes.cpp:139 | modelProduto | view_venda_produto | 🔍 | 136ms, fine |
| 211,004 | Y | widgetconsistencia.cpp:68 | model3 | view_consistencia_vp1_vp2_quant | 🔍 | 366ms, fine — 1:1 `idVendaProduto1` join, no fan-out risk (grouped by vp1's own PK) |
| 211,004 | Y | widgetconsistencia.cpp:78 | model4 | view_consistencia_vp1_vp2_total | 🔍 | 531ms, fine, same shape as above |
| 211,004 | N | widgetconsistencia.cpp:88 | model5 | view_consistencia_vp2_ehc_quant | 🔍 | 313ms, fine |
| 211,004 | N | widgetconsistencia.cpp:98 | model6 | view_consistencia_vp2_pf2_quant | 🔍 | 366ms, fine |
| 211,004 | N | widgetconsistencia.cpp:118 | model8 | view_consistencia_vinculos | 🔍 | 303ms, fine |
| 211,004 | N | widgetdevolucao.cpp:68 | model | view_devolucao | 🔍 | 104ms, fine |
| 211,004 | Y | widgetlogisticaentregas.cpp:105 | modelCarga | view_calendario_carga | 🔍 | 143ms, fine (measured earlier in Logística batch) |
| 211,004 | Y | widgetlogisticaentregues.cpp:77 | modelVendas | view_entrega | 🔍 | 542ms, fine |
| 193,722 | Y | widgetconsistencia.cpp:58 | model2 | view_consistencia_vp1_v_total | 🔍 | 500ms, fine |
| 154,249 | Y | widgetcompraconfirmar.cpp:26 | modelResumo | view_fornecedor_compra_confirmar | 🔍 | 1.55ms, fine |
| 154,249 | Y | widgetcompraconfirmar.cpp:34 | modelCompras | view_compras | 🔍 | 9.12ms, fine |
| 154,249 | Y | widgetlogisticaagendarcoleta.cpp:146 | modelFornecedor | view_fornecedor_logistica_agendar_coleta | ✅ | confirmed free win from `idx_estoque_status`, 814ms-class → 1.71ms |
| 154,249 | Y | widgetlogisticacoleta.cpp:52 | modelColeta | view_coleta | ✅ | Finding 2, `idx_estoque_status` |
| 154,249 | Y | widgetlogisticacoleta.cpp:83 | modelFornecedor | view_fornecedor_logistica_coleta | ✅ | confirmed free win from `idx_estoque_status`, → 0.98ms |
| 154,249 | Y | widgetlogisticaentregas.cpp:129 | modelProdutos | view_calendario_produto | 🔍 | 123ms, fine |
| 154,249 | Y | widgetlogisticarecebimento.cpp:53 | modelRecebimento | view_recebimento | ✅ | Finding 2, `idx_estoque_status` |
| 154,249 | Y | widgetlogisticarecebimento.cpp:83 | modelFornecedor | view_fornecedor_logistica_recebimento | ✅ | confirmed free win from `idx_estoque_status`, → 0.03ms |
| 154,249 | Y | widgetlogisticarepresentacao.cpp:44 | modelRepresentacao | view_logistica_representacao | 🔍 | 289ms, fine |
| 153,419 | N | widgetorcamento.cpp:60 | modelOrcamento | view_orcamento | 🔍 | Finding 5 — unbounded load 2.57s, search 2.17s; no index fix exists (no `WHERE` at all on open); needs pagination or default filter, both out of scope |
| 153,036 | Y | widgetrelatorio.cpp:82 | modelRelatorio | view_relatorio | 🔍 | 212ms, fine |
| 138,057 | N | widgetgalpao.cpp:149 | modelTranspAgend | veiculo_has_produto | 🔍 | direct table, no status column to index; base-table scans on this table measured <200ms via the views below |
| 138,057 | N | widgetlogisticaagendarcoleta.cpp:80 | modelTranspAtual | veiculo_has_produto | 🔍 | same as above |
| 138,057 | N | widgetlogisticaagendarcoleta.cpp:113 | modelTranspAgend | veiculo_has_produto | 🔍 | same as above |
| 138,057 | N | widgetlogisticaagendarentrega.cpp:90 | modelTranspAtual | veiculo_has_produto | 🔍 | same as above |
| 138,057 | N | widgetlogisticaagendarentrega.cpp:123 | modelTranspAgend | veiculo_has_produto | 🔍 | same as above |
| 138,057 | Y | widgetlogisticacaminhao.cpp:21 | modelCaminhao | view_caminhao | 🔍 | 99.6ms, fine |
| 138,057 | Y | widgetlogisticacaminhao.cpp:32 | modelCarga | view_caminhao_resumo | 🔍 | 172ms, fine |
| 138,057 | Y | widgetlogisticaentregas.cpp:84 | modelCalendario | view_calendario_entrega | 🔍 | 98.4ms, fine |
| 123,168 | Y | widgetcompraconsumos.cpp:36 | modelPedido | view_ordemcompra_resumo | 🔍 | 101ms, fine |
| 123,168 | Y | widgetcompragerar.cpp:43 | modelResumo | view_fornecedor_compra_gerar | 🔍 | 1.36ms, fine |
| 123,168 | N | widgetcompragerar.cpp:51 | modelProdutos | view_compras_gerar | 🔍 | 12.8ms, fine |
| 123,168 | Y | widgetcomprahistorico.cpp:49 | modelCompras | view_compras_financeiro | 🔍 | Finding 4 — `idx_pf1_ordemcompra` removed the Sort but query is unbounded (972ms→931ms); needs pagination/default filter to actually fix, out of scope |
| 123,168 | N | widgetcomprahistorico.cpp:59 | modelProdutos | pedido_fornecedor_has_produto | ✅ | Finding 3 — filtered by ordemCompra on row click, now uses `idx_pf1_ordemcompra` |
| 123,168 | N | widgetcomprapendentes.cpp:277 | model | pedido_fornecedor_has_produto | 🚫 | false positive — local, write-only model, inserts one row per "enviar produto para compras" action, never browsed |
| 123,168 | Y | widgetcompraresumo.cpp:11 | modelResumo | view_fornecedor_compra | 🔍 | 3.68ms, fine |
| 123,168 | N | widgetconsistencia.cpp:48 | model1 | view_consistencia_compra | 🔍 | 228ms, fine |
| 123,168 | Y | widgetfinanceirocompra.cpp:37 | model | view_compras_financeiro | 🔍 | same as widgetcomprahistorico.cpp:49 above |
| 123,168 | Y | widgetlogisticarepresentacao.cpp:72 | modelFornecedor | view_fornecedor_logistica_representacao | 🔍 | 159ms, fine |
| 68,138 | N | widgetnfedistribuicao.cpp:172 | model | view_nfe_distribuicao | 🔍 | 2.28s unfiltered (`nsu IS NOT NULL`, ~35.9k rows, no covering index possible for the ~19 needed columns off a wide table). Same shape as Findings 5/6: status-checkbox filter is optional (`WidgetNFeDistribuicao::montaFiltro`), empty-filter case hits this full cost. Fix = pagination/default-filter, out of scope |
| 68,138 | N | widgetnfesaida.cpp:410 | view | view_relatorio_nfe | 🔍 | False alarm on first pass (28s unfiltered) — not representative, the widget always applies a date range; with a realistic ~3-week filter it's 9.75ms via `idx_nfe_tipo_status_created` (already fixed pre-audit) |
| 51,881 | N | widgetvenda.cpp:22 | modelVenda | view_venda | 🔍 | Finding 6 — same shape as Orçamento, smaller today (1.39s/0.87s); no index fix exists; `venda.status` already has a leading index (`idx_status`) so a default-filter fix would be cheap *if* that's ever put in scope |
| 9,908 | N | widgetcompraavulsa.cpp:13 | modelCompra | compra_avulsa | 🔍 | 75.4ms unfiltered, fine |
| 4,668 | N | widgetrh.cpp:71 | modelFolhaPag | folha_pagamento | 🔍 | 38.1ms unfiltered, fine |
| 4,668 | N | widgetrh.cpp:139 | modelImportar | folha_pagamento | 🚫 | import dialog |
| 168 | N | cadastropagamento.cpp:144 | modelAssocia2 | view_pagamento_loja | 🚫 | trivially small |
| 60 | N | widgetgalpaopeso.cpp:112 | model | estoque_peso | 🚫 | trivially small |

**`setQuery()`-based screens** (built as raw SQL strings, not `QSqlTableModel::setTable`) — these
weren't in the row-count ranking since the query text has to be read per case to know what's
scanned:

| File:line | Model | Status | Note |
|---|---|:-:|---|
| widgetestoques.cpp:114 | model | ✅ | Finding 1 — `Sql::queryEstoque`, `HAVING`→`WHERE` |
| widgetestoques.cpp:122 | model | ✅ | `Sql::view_estoque_contabil` — same `HAVING`→`WHERE` fix as Finding 1, 8.59s→4.26s |
| widgetestoques.cpp:193 | modelContabil | ✅ | same fix, second call site |
| widgetestoques.cpp:233 | tempModel | 🔍 | `Sql::queryExportarNCM` — 8.18s, but cost is `EXTRACTVALUE(n.xml,...)` parsing ~51k rows' worth of `nfe.xml`, not a missing index (`e.contabil > 0` filter itself is 655ms); export-button action, not a paint-path grid; no index fix available, same wide-`nfe`-table cost class as elsewhere |
| widgetfinanceirocontas.cpp:32/33/43/44 | modelVencidos/modelVencer | 🔍 | `Sql::view_a_pagar/receber_vencidos/vencer` — already fixed pre-audit (`db/add_index_conta_pagar_status_date_valor.sql`, `..._conta_receber_status_date_rep.sql`) |
| widgetfinanceirofluxocaixa.cpp:85/123/159 | modelCaixa/modelCaixa2/modelFuturo | 🔍 | window-function queries over `view_fluxo_resumo_realizado`/`pendente`, worst case measured 523ms / 246ms, fine |
| widgetgalpao.cpp:186/455/493/562 | modelPallet | 🚫 | confirmed always filtered by a specific `idBloco` (`Sql::view_galpao(currentPallet->getIdBloco())`), matches Sql:: table D entry |
| widgetgare.cpp:137/147 | modelVencidos/modelVencer | 🔍 | `Sql::view_gare_vencidos/vencer` — checked: optimizer already picks `idx_conta_pagar_status_date_valor` correctly without `FORCE INDEX` (rare status values), both <1ms, no fix needed |
| widgetlogisticaagendarentrega.cpp:31/53/395/439/1310 | modelVendas/modelProdutos/modelRomaneio | 🔍 | `Sql::view_entrega_pendente`/`view_agendar_entrega`, no-filter worst case measured: 12.1ms / 613ms, both fine |
| widgetlogisticaentregas.cpp:528 | modelProdutosAgrupado | 🚫 | filtered by specific `idVenda` + `idEvento` (row-click detail query), not a browse grid |
| widgetnfeentrada.cpp:161 / widgetnfesaida.cpp:133 | modelResumo | 🔍 | small `GROUP BY status` summary query on the already-paginated screens, low risk |
| widgetrelatorio.cpp:39/65 | modelVendedor/modelLoja | 🔍 | `Sql::view_relatorio_vendedor/loja`, no-filter worst case measured: 183ms / 458ms, both fine |

**Caveat on this table**: import/summary sub-dialogs marked 🚫 above are a judgment call, not a
guarantee — revisit if a specific screen turns out to be slow in practice.

---

## C. Detail dialogs / lookup sub-grids (presumed low risk)

Every other `setTable()` call site — item grids inside a single open record (order lines, payment
lines, consumption lines, etc.), always `setFilter()`'d to one parent ID right after `setTable()`.
Not individually `EXPLAIN`'d; spot-checked that the parent-ID columns they filter on have FK
indexes. Full list (44 call sites) intentionally not reproduced row-by-row here — see
`src/orcamento.cpp`, `venda.cpp`, `devolucao.cpp`, `estoque.cpp`, `inputdialogconfirmacao.cpp`,
`compraavulsa.cpp`, `importarxml.cpp`, `anteciparrecebimento.cpp`, `contas.cpp`, `followup.cpp`,
`calculofrete.cpp`, `cancelaproduto.cpp`, `pdf.cpp`, `precoestoque.cpp`, `inserirlancamento.cpp`,
`inserirtransferencia.cpp`, `baixaorcamento.cpp`, `inputdialogfinanceiro.cpp`,
`inputdialogproduto.cpp`, and the small `cadastro*.cpp` dialogs.

**`cadastrarnfe.cpp`** (`modelVenda`, `modelProduto` — `venda`, `view_produto_estoque`) — checked:
both are filtered by indexed keys right after `setTable()` (`modelVenda.setFilter("idVenda = ...")`
on the PK; `modelProduto.setFilter("idVendaProduto2 IN (...)")`, `idVendaProduto2` is
`venda_has_produto2`'s PK). Not a search combo as first suspected — 🚫 confirmed, same as the rest
of this table.

Also dynamic-table dialogs (`registerdialog.cpp`, `registeraddressdialog.cpp`, `searchdialog.cpp`,
`pdf.cpp`) build their `setTable()` argument from a variable, so the actual table depends on the
caller — not individually tracked here.

---

## D. `Sql::` C++-built query functions (`src/sql.cpp`)

| Function | Params | Status | Note |
|---|---|:-:|---|
| `queryEstoque` | match, where | ✅ | Finding 1 |
| `queryExportarNCM` | — | 🔍 | 8.18s, dominated by `EXTRACTVALUE` XML-parsing per row, not indexing; export action, no fix available in this scope |
| `view_a_pagar_vencer` / `_vencidos` | — | 🔍 | fixed pre-audit (jan/2026 analysis, `db/add_index_conta_pagar_status_date_valor.sql`) |
| `view_a_receber_vencer` / `_vencidos` | — | 🔍 | fixed pre-audit (`db/add_index_conta_receber_status_date_rep.sql`) |
| `view_gare_vencer` / `_vencidos` | — | 🔍 | checked — optimizer already picks the composite index correctly without `FORCE INDEX`, both <1ms |
| `view_relatorio_loja` / `_vendedor` | mes, idUsuario, idUsuarioConsultor, loja (all optional) | 🔍 | no-filter worst case: 458ms / 183ms, both fine |
| `view_estoque_contabil` | match, data (defaults to today) | ✅ | same `HAVING`→`WHERE` bug as Finding 1 — fixed, 8.59s→4.26s |
| `view_estoque` | idEstoque | 🚫 | single-record lookup |
| `view_galpao` | idBloco, filtroText | 🚫 | filtered by idBloco |
| `view_followup_venda_misto` | idVenda | 🚫 | filtered by idVenda |
| `view_agendar_entrega` | idVenda, status (optional) | 🔍 | no-filter worst case: 613ms, fine |
| `view_entrega_pendente` | filtroBusca, filtroCheck, filtroStatus, filtroAtelier, filtroServico (all optional) | 🔍 | no-filter worst case: 12.1ms, fine |
| `updateFornecedoresOrcamento` / `updateFornecedoresVenda` / `updateOrdemRepresentacaoVenda` (×2) / `updateVendaStatus` (×2) | — | — | write-side (UPDATE), out of this audit's read-performance scope |
| `pesosProdutos` | idProdutos | 🚫 | always bounded by an explicit product-ID list (`IN (...)`), no unbounded path |

---

## E. Views with no C++ caller (excluded from this audit entirely)

85 views — legacy/BI-export, not queried by the app. Full list in the original report
(`.claude/performance-audit-2026-08-04.md`, "Views excluded from scope" section). Not repeated here
to keep this file focused on trackable work.

---

## How to keep this current

- When you `EXPLAIN` something in table B/C/D that's ⏳, flip it to 🔍 (with a one-line note on what
  you found) or ✅ (with the fix + before/after) and add a row to table A if it's a fix.
- If a screen turns out to have a default filter that already bounds it (discovered while checking),
  note that instead of leaving it ⏳ forever — "checked, bounded by X, no action needed" is a valid
  terminal state too, distinct from "haven't looked yet."
- Don't add new `db/add_index_*.sql` files without updating table A here to match.
