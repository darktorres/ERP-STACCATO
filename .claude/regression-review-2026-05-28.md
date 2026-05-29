# Deep regression review — 2026-05-28 commit batch

## Context
Goal: review all recent commits to ensure we are not introducing regressions.
Scope reviewed: the 2026-05-28 work cluster — 10 code commits plus uncommitted WIP.
Docs/build-only commits (`5a7d6296`, `fad5a94a`, `25ee6acf`, `833a815a`, `5386c886`)
were noted but not deeply analyzed (no runtime-logic risk; `5386c886` is a build-config
revert to a single `Loja.pro`).

**Bottom line: no must-fix regressions found.** The one real regression the big refactor
introduced (dead signals in `venda`) was already caught and fixed in `5831dc66`. Remaining
items are one deployment step, one consistency nit, and a few intended behavior changes to
be aware of.

---

## Findings (ranked by importance)

### A. Deployment dependency — run the SEPARADO procedure migration on the LIVE DB — MEDIUM
Commit `0f4b8b2e` adds the `SEPARADO` logística status. The status columns are
`VARCHAR(45)` (verified in `initdb.sql`; only `orcamento.status` is an ENUM and it is
unaffected), so **no column migration is needed** — good. The four status-cascade stored
procedures were updated in both `initdb.sql` (fresh installs) and `db/separado_status_procedures.sql`
(live DB). MySQL cannot ALTER a procedure body, so the migration drops+recreates them.

- **Risk if the migration is NOT run on the live `staccato` DB:** clicking "Separar" writes
  `status='SEPARADO'` to the child rows (VARCHAR accepts it), but the live procedures lack the
  SEPARADO rollup line, so the parent `venda`/`pedido_fornecedor` status will not roll up to
  SEPARADO (it stays at `ENTREGA AGEND.`). This is a **cosmetic status-lag, not data loss**.
  NF-e issuance and delivery confirmation still work because those queries use
  `status IN ('ENTREGA AGEND.', 'SEPARADO')` (cadastrarnfe.cpp, widgetlogisticaentregas.cpp).
- **Action:** confirm `db/separado_status_procedures.sql` has been executed on the live DB.

### B. `orcamento.cpp:1638` `on_doubleSpinBoxTotalItem_valueChanged` — half-applied guard — LOW
Has `ScopedUpdate guard(updating)` but is missing the matching `if (updating) { return; }`
early-return (the CLAUDE.md-documented pattern is *both*). Pre-refactor it used
`unsetConnections()`/`setConnections()` to fully block.
- Recursion is **bounded** — `on_doubleSpinBoxDesconto_valueChanged` (line 1517) has the
  early-return that stops the loop, and the Desconto↔TotalItem values converge algebraically,
  so no confirmed wrong value. No concrete failure case was found.
- Theoretical edge: if a guarded populate sets `TotalItem` before `quant`/`prcUn`, this slot
  could recompute Desconto from partial fields. Adding `if (updating) { return; }` aligns it
  with the documented pattern and removes the ambiguity at zero risk.

### C. `Sql::pesosProdutos` — leniency change (intended) — LOW / informational
`5831dc66` batches the per-row kgcx/vemDoSul lookups into one `LEFT JOIN fornecedor` query
(`src/sql.cpp`). Behavior change: a produto whose `fornecedor` row is missing now defaults
`vemDoSul=false` instead of throwing "Fornecedor não encontrado" as the old per-row code did.
This is more robust and matches the batching intent — just noting it.

### D. WebDAV uploads now block the UI (intended) — LOW / informational
`d212abb0`'s new `uploadWebDav()` (`src/webdav.cpp`) is synchronous (nested `QEventLoop`)
vs. the old fire-and-forget upload. Large files will briefly freeze the UI — the deliberate
tradeoff to verify the written size via HEAD and catch silent 0-byte saves. Throwing now
happens synchronously from the slot, which is **safer** than the old throw-from-`finished`-lambda
(which threw across Qt's event loop). Empty source files are rejected up front.

### E. Uncommitted WIP — `src/inputdialogconfirmacao.{cpp,h}` — info
The `moverValorParaReposicao` logic (conditioned on `idNFeSaida IS NULL`) is correct and
matches `db/fix_quebrado_reposicao_valor.sql` and the project memory on QUEBRADO/reposição.
It is **not yet committed** — make sure it lands.

### F. Explore-agent false positives — recorded so they are not re-flagged
A subagent flagged "missing re-entrancy guards" on several `_toggled`/`_clicked` handlers
(venda `MostrarCancelados`/`PontuacaoPadrao`/`PontuacaoIsento`/`RT`/`FreteManual`,
orcamento `Representacao`/`dataEmissao`, contas/inputdialogfinanceiro checkboxes). **Verified
these are NOT regressions:** they either only set model filters / child visibility (no
re-trigger), set sibling checkboxes in a bounded non-recursive way, or are `clicked` signals
(user-only, never fire on programmatic `setChecked`). The old `setConnections/unsetConnections`
was applied defensively; removing it for these is safe.

---

## Confirmed clean
- **SEPARADO code paths** — `cadastrarnfe` (EM ENTREGA from AGEND.|SEPARADO), `confirmarEntrega`,
  `processarConsultaNFe`, `widgetnfeentrada` inutilizar guard, `widgetcompraconsumos` desfazer
  guard, button enablement in `on_tableCarga_selectionChanged`, and cascade ordering
  (SEPARADO ranked between EM ENTREGA and ENTREGA AGEND. = correct least-progressed rollup).
- **`ScopedUpdate` refactor (`e3aa1655`)** — all 16 changed `.cpp` use `Qt::UniqueConnection`
  and connect either in the constructor (orcamento, cadastroproduto, cadastrostaccatooff) or
  once via the `if (not isSet)` lazy-init in `updateTables()` (the Widget* tabs). No
  double-connect (Mode C) and no dead-signal path (Mode A) beyond the already-fixed venda case.
- **faturar multi-supplier (`b974da6a`)** — mesmo-fornecedor gate removed safely; `pularNota`
  computed over all selected suppliers; mixed representação/normal throws a clear error;
  `aliquotaSt`/`comboBoxST` are not read downstream in `widgetcomprafaturar` (verified), so
  skipping the prefill for `Tipo::Faturamento` is safe.
- **contas a pagar filters (`604cafa9`, `147dab50`)** — structured filters carried into the day
  view; `filtrarPorResumo` maps status from header text and **safe-degrades to "Todos"** if no
  match, so worst case is "filter not narrowed" (no worse than before).
- **cadastroproduto minimo/multiplo (`fdc08270`)** — additive; NULL-on-zero matches the
  Excel/XML import convention; mapper round-trips cleanly.
- **orcamento fixes (`f18724d5`)** — duplicate-estoque guard, desconto handler reads the
  grid-snapped `doubleSpinBoxQuant` directly, three calculofrete queries parameterized.
- **venda remediation (`5831dc66`)** — `connectSignals()` now also called from `viewRegister`
  (idempotent via `UniqueConnection`) fixing the refactor's dead-signal regression;
  div-by-zero guards on `subTotalLiq`; cancelamento reactivates orçamento as ATIVO/EXPIRADO by
  validade; 3D button hidden + `QPointer`-guarded async lambda; remaining queries parameterized.

---

## Optional follow-up actions
1. **(B)** Add `if (updating) { return; }` to `Orcamento::on_doubleSpinBoxTotalItem_valueChanged`
   (`src/orcamento.cpp:1638`) for consistency with the documented guard pattern.
2. Confirm/ensure `db/separado_status_procedures.sql` is run on the live DB (deployment step, not code).
3. Commit the `inputdialogconfirmacao` reposição WIP (item E).

## Verification
- Build with the documented MSVC 14.44 / Qt 5.15.2 32-bit toolchain.
- If B is applied: open an orçamento, edit an item's "Total do item" and its "Desconto"
  field; confirm Desconto↔Total still cross-update once and converge with no flicker/loop.
- SEPARADO end-to-end: from an `ENTREGA AGEND.` carga, click "Separar" → status becomes
  SEPARADO; confirm GerarNFe + ConfirmarEntrega stay enabled and the parent venda status rolls
  up (requires the live-DB procedure migration from item A).
