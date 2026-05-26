# Deep review — `src/orcamento.cpp`

Scope: `src/orcamento.cpp` (1944 lines), `src/orcamento.h`, `src/orcamento_calc.{h,cpp}`.
Ordered by severity.

---

## Real bugs

### 1. "Pending deletion" rows leak into business logic
Most loops correctly skip rows where `modelItem.headerData(row, Qt::Vertical) == "!"` (rows that `removeRow` marked for deletion but not yet submitted). Four places don't, and each is a real bug:

- **`verificaServicosEspeciais()` @ src/orcamento.cpp:1273** — builds `fornecedores` from every row. If all remaining (non-`!`) rows are SSE but a `!`-row from a different fornecedor lingers, `fornecedores.size() == 1` is false → SSE freight-zero rule fails to trigger.
- **`calcularFrete()` @ src/orcamento.cpp:1297** — sums `pesoSul`/`pesoTotal` over deleted rows too, inflating QUALP weights and the resulting freight floor.
- **`buscarConsultor()` @ src/orcamento.cpp:731** — collects fornecedores from `!`-rows, then may throw "Mais de um consultor disponível" or set the wrong consultor.
- **`on_pushButtonReplicar_clicked()` @ src/orcamento.cpp:1392** — replicates rows the user just deleted (the unsaved deletion is invisible to the new dialog).

Fix: add the same `if (modelItem.headerData(row, Qt::Vertical) == "!") continue;` guard, or factor a small helper `forEachActiveRow(...)` to make this uniform.

### 2. `on_doubleSpinBoxCaixas_valueChanged` rounds with the wrong step
At src/orcamento.cpp:1053-1054:
```cpp
const double resto = fmod(caixas, stepCx);
const double caixas2 = not qFuzzyIsNull(resto) ? ceil(caixas) : caixas;
```
The peer `on_doubleSpinBoxQuant_valueChanged` (src/orcamento.cpp:819-820) correctly does `ceil(quant / stepQt) * stepQt`. Here, when a product has a non-unit `multiplo`, `setarParametrosProduto` sets `doubleSpinBoxCaixas->setSingleStep(multiplo / quantCaixa)` (src/orcamento.cpp:1173) — a fraction. `ceil(caixas)` then snaps to integer caixas, ignoring `stepCx`. Should be `ceil(caixas / stepCx) * stepCx`.

### 3. `atualizaReplica` only patches `EXPIRADO`
src/orcamento.cpp:756: `WHERE idOrcamento = :idOrcamento AND status = 'EXPIRADO'`. But the replicar button is shown for any non-ATIVO/expired status (src/orcamento.cpp:256-259), so replicating a `FECHADO`/`PERDIDO`/`CANCELADO` orçamento silently leaves `replicadoEm` unset on the source. Either drop the status predicate or branch explicitly on status.

### 4. `buscarConsultor` bypasses `RegisterDialog::setData` on the empty branch
src/orcamento.cpp:747-749:
```cpp
if (query.size() == 1 and query.first()) { setData("idUsuarioConsultor", query.value("idUsuario")); }
if (query.size() == 0) { model.setData(currentRow, "idUsuarioConsultor", {}); }
```
The success branch goes through the wrapper (dirty tracking, `adjustValue`), the clear branch goes through the raw model. They should be symmetric (`setData("idUsuarioConsultor", {})`). Also: `query.size()` is driver-dependent (returns -1 in some setups) — prefer `query.first()` followed by `query.next()` to detect "exactly one", or `COUNT(*)` in SQL.

### 5. `removeItem` can throw from unrelated business rules
`removeItem` → `save(true)` (src/orcamento.cpp:522) → `cadastrar` → `savingProcedures` → `buscarConsultor`. If a stale "more than one consultor" state exists (e.g., from a row the user added before fixing it), deleting an item now fails for an unrelated reason. Reorderings (`subir`/`descer` also call `save(true)`) carry the same risk. `buscarConsultor` arguably belongs only in `Tipo::Cadastrar` (it's already on the wrong side of `if (tipo == Tipo::Cadastrar)` at src/orcamento.cpp:699).

### 6. Replica seeded without disconnecting signals
`on_pushButtonReplicar_clicked` (src/orcamento.cpp:1465-1473) bulk-assigns to `itemBoxProduto`, `doubleSpinBoxQuant`, etc. on the new dialog without any `unsetConnections()`. Each `setValue` fires its handler. `replicando = true` short-circuits `calcularFrete`, but `on_doubleSpinBoxQuant_valueChanged` *does* fire and silently re-rounds the quantity per the bug in #2. For correctness and perf, wrap the loop in unset/set or call a "seed quietly" variant.

### 7. `proxyModel->sort()` after two `setData` calls is racy
src/orcamento.cpp:780-784 (also 800-804) sets `ordem` on `rowA`, then on `rowB`, then calls `proxyModel->sort(...)`. With `QSortFilterProxyModel::dynamicSortFilter` on (default), the first `setData` can reorder the proxy before the second `setData` runs — so `rowB` no longer points to the row you intended. In practice the two values are adjacent so the swap usually "works", but the invariant is fragile. Either disable dynamic sorting around the swap or update both rows atomically via a single transaction on the model.

### 8. URL not percent-encoded in `on_pushButtonModelo3d_clicked`
src/orcamento.cpp:1771 interpolates `fornecedor` and `codComercial` directly into a `https://…` URL. Fornecedor names contain spaces and accents (and parentheses — "STACCATO SERVIÇOS ESPECIAIS (SSE)"). `QUrl(QString)` is lenient but downstream WebDAV servers vary. Use `QUrl::toPercentEncoding` for both segments.

---

## Suspect / inconsistent

- **`removeRow` failure leaves connections off.** `removeItem` (src/orcamento.cpp:517) throws inside the try, the catch reconnects — OK. But the caller path from `save(true)` failing inside `cadastrar` does not run through `removeItem`'s catch. Audit that the global try/catch in callers re-syncs.
- **`spinBoxPesoTotal->setValue(total)`** at src/orcamento.cpp:1845 with `total` a `double` — silent narrowing (the existing `// TODO: implicit conversion double -> int` flags this).
- **Validade UI uses local date math** (`addDays(data("validade").toInt())`, src/orcamento.cpp:254) while saving uses `qApp->serverDateTime()`. Cross-timezone clients could see expiry inconsistently.
- **N+1 SQL in `calcularFrete`** (two queries × `rowCount`, src/orcamento.cpp:1300-1316) and **`calcularPesoTotal`** (one query × rowCount, src/orcamento.cpp:1833-1838). For an orçamento with 50 items, that's 100–150 round-trips just to refresh totals — and `calcPrecoGlobalTotal` calls `calcularFrete` on every quant/desconto change. Pre-fetch into a `QHash<idProduto, kgcx>` (and `vemDoSul`) once per save / per orçamento load.
- **`generateId` ID size invariant** (`id.size() != 12 and id.size() != 13`, src/orcamento.cpp:566) silently wedges any loja that exceeds 9999 orcamentos in a year.
- **`buscarConsultor` empty-fornecedores edge case.** It returns early only on `rowCount() == 0`, never clearing `idUsuarioConsultor` when all fornecedor strings are blanks — but the `IN ('','','')` clause finds nothing, falls to the `size() == 0` branch, which does clear. OK in practice but only by luck.
- **`Sql::updateFornecedoresOrcamento(primaryId)`** runs inside the transaction (src/orcamento.cpp:1499) — make sure it actually participates in the same connection/transaction, otherwise a rollback leaves it half-applied.

---

## Code-quality / structural

- **1944-line "god dialog".** Strong candidate for further extraction. Pure math is already in `orcamento_calc.cpp`; freight querying, item CRUD, and id generation are all extractable similarly. The existing pattern (`venda_calc`, `orcamento_calc`) is the right direction.
- **`unset/setConnections` pattern is everywhere and brittle.** A scoped `BlockSignalsGuard` (RAII) eliminates the manual try/catch in 8+ handlers and removes the "remember to call setConnections before early `return`" footgun (src/orcamento.cpp:1583-1586 is a near-miss).
- **`setConnections`/`unsetConnections` are two parallel ~35-line lists.** A `QVector<QMetaObject::Connection>` populated once and toggled with `blockSignals` on each widget would halve maintenance cost.
- **Loose duplication of `if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); }`** in `removeItem`, `subir`, `descer`, `adicionarItem`. Wrap in a `persistIfSaved()` helper.
- **`montarLog` and the totals re-check** (src/orcamento.cpp:642-664) implement defensive double-bookkeeping. Worth a comment explaining the historical drift this was added to catch, otherwise future-you will be tempted to delete it.
- **Magic strings** for status (`"ATIVO"`, `"EXPIRADO"`, `"REPLICADO"`, `"FECHADO"`, `"PERDIDO"`, `"CANCELADO"`) and headerData (`"!"`, `"*"`) — there's already an enum-based status refactor in the project notes ("Process Improvements" in CLAUDE.md); orcamento.cpp is a heavy user and a good migration target.

---

## Suggested next steps (in priority order)

1. Add the `headerData == "!"` skip to the four loops in §1 — this is mechanical and has direct user-visible effects.
2. Fix the caixas rounding (§2) and add a tier-1 test in `orcamento_calc` for "non-unit caixas step" mirroring the existing freight tests.
3. Move `buscarConsultor()` inside the `tipo == Tipo::Cadastrar` block in `savingProcedures` (§5) and symmetrize its `setData` calls (§4).
4. Decide on `atualizaReplica`'s status filter (§3) — likely drop the `status = 'EXPIRADO'` predicate.
5. Wrap the replica-seeding loop in `unsetConnections` (§6).
