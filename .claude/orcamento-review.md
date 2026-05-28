# Deep review — `src/orcamento.cpp`

Scope: `src/orcamento.cpp` (1944 lines), `src/orcamento.h`, `src/orcamento_calc.{h,cpp}`. Cross-checked against `src/sqltablemodel.{h,cpp}`, `src/registerdialog.{h,cpp}`, `src/sortfilterproxymodel.{h,cpp}`, `src/produtoproxymodel.{h,cpp}`, `src/sql.cpp`, `src/application.cpp`.

Ordered by severity. Each finding annotated with whether deeper investigation confirmed, narrowed, or invalidated it.

---

## Confirmed bugs

### 1. "Pending deletion" rows leak into business logic — **confirmed for 3 of 4 sites**
`SqlTableModel::setTable` forces `OnManualSubmit` (src/sqltablemodel.cpp:165), so `removeRow` only marks rows for deletion — `headerData(row, Qt::Vertical)` returns `"!"` until `submitAll()` runs. Most loops handle this; these three don't:

- **`verificaServicosEspeciais()` @ src/orcamento.cpp:1273** — collects `fornecedores` from every row. With a `!`-marked row of a different fornecedor still present, the SSE-only check (`fornecedores.size() == 1 and fornecedores.first() == "SSE"`) fails to trigger the freight-zero rule.
- **`calcularFrete()` @ src/orcamento.cpp:1297** — sums `pesoSul` / `pesoTotal` over deleted rows, inflating the QUALP-derived freight floor.
- **`buscarConsultor()` @ src/orcamento.cpp:731** — collects fornecedores from `!`-rows, may then throw "Mais de um consultor disponível" or set the wrong consultor.

Reachability confirmed via `removeItem` (src/orcamento.cpp:513): after `modelItem.removeRow(currentRowItem)` at line 517, both `calcPrecoGlobalTotal()` (which calls `calcularFrete` → `verificaServicosEspeciais`) and `save(true)` (which calls `buscarConsultor` via `savingProcedures`) run while the `!`-row is still present. `on_itemBoxEndereco_idChanged` (src/orcamento.cpp:1255) also calls `calcularFrete` and is reachable after a delete but before save.

**Originally cited but actually safe:** `on_pushButtonReplicar_clicked` (src/orcamento.cpp:1392). The replicar button is only shown in the read-only branch of `viewRegister` (src/orcamento.cpp:256-259), and `on_tableProdutos_selectionChanged` early-returns under `isReadOnly` (src/orcamento.cpp:102) so the remove button is never exposed. No `!`-rows can exist on this code path.

Fix: add `if (modelItem.headerData(row, Qt::Vertical) == "!") continue;` to the three sites, or factor a `forEachActiveRow(...)` helper.

**Same class, lower severity — three more sites in discount/total handlers:**

- **`on_doubleSpinBoxDescontoGlobalReais_valueChanged`** loop @ src/orcamento.cpp:1592
- **`on_doubleSpinBoxDescontoGlobal_valueChanged`** loop @ src/orcamento.cpp:1658
- **`on_doubleSpinBoxTotal_valueChanged`** loop @ src/orcamento.cpp:1693

All three iterate `modelItem.rowCount()` and call `setData` on every row (setting `descGlobal` and `total`) without checking for `"!"` pending-deletion rows. Meanwhile, `calcPrecoGlobalTotal` at lines 855 and 887 DOES correctly skip them, as do `corrigirValores` (line 588), `calcularTotais` (line 607), and `montarLog` (line 630).

**Severity is lower than the three confirmed sites above** because all three handlers are guarded by `unsetConnections()` during the `removeItem` flow: `removeItem` calls `unsetConnections()` at line 516 before `calcPrecoGlobalTotal()` or `save()`, and `calcPrecoGlobalTotal` directly sets the discount/total spinbox values (lines 880-884) without reconnecting. The handlers can only fire from direct user interaction with the spinboxes, which cannot happen during the synchronous `removeItem` execution.

**Residual risk:** if `unset/setConnections` is ever migrated to `ScopedUpdate`, the `setData` calls on `"!"` rows would still dirty those rows in the model. Worth fixing for consistency with the established skip pattern regardless. Same fix as above.

### 2. `on_doubleSpinBoxCaixas_valueChanged` rounds with the wrong step — **confirmed**
src/orcamento.cpp:1053-1054:
```cpp
const double resto = fmod(caixas, stepCx);
const double caixas2 = not qFuzzyIsNull(resto) ? ceil(caixas) : caixas;
```
The peer `on_doubleSpinBoxQuant_valueChanged` does `ceil(quant / stepQt) * stepQt`. Here `stepCx` is `1` by default but becomes `multiplo / quantCaixa` when the product has a `multiplo` set (src/orcamento.cpp:1173) — a fraction. Worked example: produto with `quantCaixa = 10`, `multiplo = 3` → `stepCx = 0.3`. User types `caixas = 2.0`: `resto = fmod(2, 0.3) ≈ 0.2`, `ceil(2.0) = 2.0` → snaps to 2.0 caixas (not a multiple of 0.3). The correct value is `ceil(2 / 0.3) * 0.3 = 2.1` caixas (= 21 units = 7×3). The bug also degrades the quant downstream because `quant2 = caixas2 * stepQt` propagates the off-grid value.

Fix: `ceil(caixas / stepCx) * stepCx`, mirroring the quant handler.

### 3. `atualizaReplica` strands `replicadoEm` for non-EXPIRADO sources — **confirmed, with nuance**
src/orcamento.cpp:756: `UPDATE orcamento SET status = 'REPLICADO', replicadoEm = :idReplica WHERE idOrcamento = :idOrcamento AND status = 'EXPIRADO'`.

The replicar button is shown for any non-ATIVO/non-expired status (src/orcamento.cpp:256-259), i.e. `{EXPIRADO, FECHADO, PERDIDO, CANCELADO, REPLICADO}`. The status transition `EXPIRADO → REPLICADO` is presumably intentional (you don't want to "downgrade" FECHADO/PERDIDO/CANCELADO to REPLICADO since those carry more business weight). But `replicadoEm` is purely informational — it tracks "what was this replicated into" — and the current WHERE clause silently drops the linkage whenever the source isn't EXPIRADO. The "Abrir Réplica" button on the source dialog (src/orcamento.cpp:326-330, src/orcamento.cpp:1892) never appears for those orcamentos.

Also: replicating from an already-REPLICADO source overwrites nothing because the WHERE filter excludes it, so the first replica's id stays sticky in `replicadoEm` regardless of how many further replicas are made. May be intentional, but is not documented.

Fix: split the two updates — always set `replicadoEm`; only transition status when the source is EXPIRADO.

### 4. `removeItem` / `subir` / `descer` can throw from `buscarConsultor` — **confirmed, narrower than originally stated**
Trace: `removeItem` → `save(true)` (src/orcamento.cpp:522) → `RegisterDialog::save` → `cadastrar` (src/orcamento.cpp:1481) → `savingProcedures` → `buscarConsultor` (src/orcamento.cpp:699). The `buscarConsultor` throw "Mais de um consultor disponível" (src/orcamento.cpp:745) cannot fire if it didn't fire at original-create time, *unless* the consultor/fornecedor-especialidade mapping changed in the DB between save and now (admin added a new consultor). Rare but observable.

Original review claimed `buscarConsultor` is "on the wrong side of `if (tipo == Tipo::Cadastrar)`". Re-reading, the placement is *intentional*: removing the last item from a fornecedor needs to clear `idUsuarioConsultor`, so it must run on `Atualizar` too. The real defect is that on an `Atualizar` path the throw is too eager — at update time, prefer downgrading to a non-fatal warning or fall back to the existing `idUsuarioConsultor` when it remains valid.

Fix options: (a) suppress the throw when `tipo == Atualizar` and just leave `idUsuarioConsultor` unchanged; (b) only re-run `buscarConsultor` when the fornecedor set actually changed since load.

### 5. Replica seeded without disconnecting signals — **confirmed, smaller blast radius**
src/orcamento.cpp:1453-1473 bulk-assigns to the new dialog's widgets without `unsetConnections()`. Signal handlers fire per-`setX`. Re-traced what each one actually does:

- The pre-loop setters (cliente / profissional / vendedor / endereco / dataEmissao / representacao) fire *before* `replicando = true`, so `on_itemBoxEndereco_idChanged` → `calcularFrete(true)` runs with empty `modelItem` and writes `minimoFrete` into the freight spinbox. Recovered after the loop by the explicit `replica->calcPrecoGlobalTotal()` at src/orcamento.cpp:1476.
- Inside the loop, `replicando = true` short-circuits `calcularFrete`, so totals don't churn against the API. But `on_doubleSpinBoxQuant_valueChanged` *does* fire and applies the bug from §2 if the product's `multiplo` changed since the original save. `setarParametrosProduto` also sets `setMinimum(minimo)` and `setMaximum(estoqueRestante)` (src/orcamento.cpp:1158, 1183-1184) before the setValue lands, so a `setValue(oldQuant)` can be silently *clamped up* to a newer `minimo` — not caught by the pre-loop estoque skip-list.
- Each `on_itemBoxProduto_idChanged` runs a full `setarParametrosProduto` DB lookup — a few queries per replicated row. Perf nuisance, not a bug.

Worth a wrap in unset/set for correctness around §2 and the minimo-clamp scenario, plus reduced query churn.

### 6. `cadastrar()` catch block doesn't restore connections — **already fixed by ScopedUpdate migration**
`cadastrar` now uses `ScopedUpdate guard(updating)` (RAII), which automatically restores the `updating` counter when the guard's scope exits — including on exception. No manual `setConnections()` needed.

### 7. `setarParametrosProduto` — division by zero if `quantCaixa == 0` — **confirmed**
src/orcamento.cpp:1155-1185. Three divisions by `quantCaixa` with no guard:
- Line 1161: `ui->doubleSpinBoxCaixas->setMinimum(minimo / quantCaixa)`
- Line 1175: `ui->doubleSpinBoxCaixas->setSingleStep(multiplo / quantCaixa)`
- Line 1185: `ui->doubleSpinBoxCaixas->setMaximum(estoqueRestante / quantCaixa)`

`quantCaixa` comes from `query.value("quantCaixa").toDouble()` (line 1155). If a product has `quantCaixa = 0` or `NULL` in the DB (data migration error, admin misconfiguration), the division produces `inf` or `nan`. Qt's QDoubleSpinBox doesn't crash on these values but becomes unusable — step/min/max are meaningless. Subsequent operations using caixas (freight weight, item total) produce `nan` that propagates through all calculations.

Fix: add a guard after line 1155: `if (qFuzzyIsNull(quantCaixa)) { throw RuntimeException("Produto com quantCaixa inválida: " + idProduto); }`

### 8. `on_itemBoxEndereco_idChanged` — no exception handling around `calcularFrete` — **confirmed**
src/orcamento.cpp:1250-1270. This signal handler modifies state before calling `calcularFrete(true)` at line 1257:
- Line 1251: `minimoGerente = 0.`
- Line 1252: `canChangeFrete = false`
- Line 1253-1254: `checkBoxFreteManual` unchecked and enabled
- Line 1256: frete minimum reset to 0

`calcularFrete` has an internal try/catch for the QUALP API (lines 1332-1337), but the per-row SQL queries at lines 1300-1318 can throw unguarded `RuntimeException`. If they do, the exception propagates through this handler (which has no try/catch) into Qt's signal dispatch. The pre-1257 state changes (`canChangeFrete = false`, `checkBoxFreteManual` unchecked) are not rolled back. On the next address selection, `canChangeFrete` is stuck at false and the frete spinbox minimum may be wrong.

Fix: wrap `calcularFrete(true)` in try/catch, log the error, and restore `canChangeFrete` / `checkBoxFreteManual` state.

### 9. `calcPrecoGlobalTotal` doesn't update total spinbox minimum when frete changes — **confirmed**
src/orcamento.cpp:874-884. `calcPrecoGlobalTotal` updates the frete spinbox (via `calcularFrete` at line 874), then sets `doubleSpinBoxTotal`'s maximum (line 883) and value (line 884), but does NOT update its minimum. The minimum is only set in `on_doubleSpinBoxFrete_valueChanged` at line 1618 (`setMinimum(frete)`), which doesn't fire during `calcPrecoGlobalTotal` because connections are off.

Scenario: user selects address (frete = 100, total min = 100). User adds a heavy item → `calcPrecoGlobalTotal` recalculates frete to 200 (connections off → frete handler doesn't fire → total min stays 100). User edits total directly → can set total to 100 → `descontoReais = subTotalLiq + 200 - 100`, `descontoPorc = 1 + 100/subTotalLiq > 1` → writes `descGlobal > 100%` to all rows → negative line-item totals.

The reverse case also applies: if frete decreases but total min stays at the old higher value, the user can't lower total below the stale minimum.

Fix: in `calcPrecoGlobalTotal`, add `ui->doubleSpinBoxTotal->setMinimum(frete)` at line 883 (before setMaximum), mirroring the logic in `on_doubleSpinBoxFrete_valueChanged`.

### 10. `on_dataEmissao_dateChanged` — validade max clamped to days remaining in month — **confirmed**
src/orcamento.cpp:1659: `ui->spinBoxValidade->setMaximum(date.daysInMonth() - date.day())`.

On the last day of any month the max becomes 0. `newRegister()` at line 457-458 calls `on_dataEmissao_dateChanged(serverDate())` followed by `spinBoxValidade->setValue(7)`, which gets clamped to 0. Any orcamento created on the 30th or 31st has 0-day validity and expires immediately.

Fix: use a fixed maximum (e.g., 30 or a `loja` parameter) instead of days-remaining-in-month.

### 11. `on_doubleSpinBoxDesconto_valueChanged` — same rounding bug as original §2 — **confirmed**
src/orcamento.cpp:1517: `const double caixas2 = not qFuzzyIsNull(fmod(caixas, step)) ? ceil(caixas) : caixas;`.

Uses `ceil(caixas)` instead of `ceil(caixas / step) * step` — the same formula that was fixed in `on_doubleSpinBoxCaixas_valueChanged` (§2). The handler also doesn't update the spinbox values (unlike the quant handler), so `caixas2` diverges from the displayed value.

Fix: use `ui->doubleSpinBoxQuant->value()` directly instead of re-deriving from caixas. The spinbox is already grid-snapped.

### 12. Duplicate stock items allowed in orcamento — **confirmed (existing FIXME)**
src/orcamento.cpp:1871: `// FIXME: orçamento permite adicionar o mesmo estoque duas vezes`. No guard in `adicionarItem()` prevents adding the same estoque product twice. When converted to a venda, stock deduction may happen twice for the same product.

Fix: in `adicionarItem`, check if `idProduto` with `estoque = true` already exists in the model.

---

## Suspect / worth knowing

- **`on_pushButtonModelo3d_clicked` URL not percent-encoded** (src/orcamento.cpp:1771). Investigated: `QUrl(QString)` defaults to `TolerantMode`, which auto-encodes spaces and most non-ASCII characters, so "MODELOS 3D" and accented fornecedor names like "SÃO" survive. The remaining hazards are admin-controlled but theoretically real: fornecedor / codComercial containing `/`, `?`, or `#` would be interpreted as path / query / fragment delimiters (parentheses are sub-delims and pass through). Severity downgraded from "bug" to "harden if fornecedor naming is ever opened up."
- **`removeRow` failure leaves connections off in the parent caller path** — resolved by ScopedUpdate migration in `cadastrar` (see §6).
- **N+1 SQL in `calcularFrete`** (two queries × `rowCount`, src/orcamento.cpp:1300-1316) and **`calcularPesoTotal`** (one query × rowCount, src/orcamento.cpp:1833-1838). For a 50-item orcamento with QUALP enabled, that's 100-150 round trips per recalculation, and `calcPrecoGlobalTotal` calls `calcularFrete` on every quant/desconto edit. A single JOIN against `produto`/`fornecedor` once per save would be a major win.
- **`Sql::updateFornecedoresOrcamento(primaryId)`** (called at src/orcamento.cpp:1499). Source is `CALL update_fornecedores_orcamento('<id>')` with the id interpolated into SQL, not parameterized (src/sql.cpp:22). The id comes from `generateId` (deterministic, not user input), so not a live SQL-injection issue, but worth parameterizing. The CALL participates in the same connection / transaction since `SqlQuery` uses the default connection — confirmed safe under rollback assuming the stored procedure body itself doesn't `COMMIT`.
- **`calculofrete.cpp` SQL concatenation** (src/calculofrete.cpp:224, 245, 441). Three queries use string concatenation for `idEndereco` and `idProduto` values instead of parameterized queries: the QUALP cache lookup (line 224), the destination address lookup (line 245), and the fornecedor-vemDoSul lookup in the per-row loop (line 441). Values originate from ItemBox IDs (integer-derived from DB lookups), so not directly exploitable. But the same file uses `prepare`/`bindValue` in other queries, making the inconsistency a maintenance hazard.
- **`generateId` ID size invariant** (`id.size() != 12 and id.size() != 13`, src/orcamento.cpp:566) silently throws on any loja that ever exceeds 9999 orcamentos in a year.
- **`spinBoxPesoTotal->setValue(double total)`** at src/orcamento.cpp:1845 — silent narrowing (the file's own `// TODO: implicit conversion double -> int` flags this).
- **Validade UI uses local date math** (`addDays(data("validade").toInt())`, src/orcamento.cpp:254) while save uses `qApp->serverDateTime()` (src/orcamento.cpp:701). `serverDate()` is cached (src/application.cpp:462-463), so cross-day drift is bounded but cross-timezone clients could see different expiry states. Low severity.
- **`calcularFrete` called on empty model** (src/orcamento.cpp:1257). If the user sets an address before adding any items, `on_itemBoxEndereco_idChanged` → `calcularFrete(true)` runs with `modelItem.rowCount() == 0`. The per-row loops (lines 1299-1323) are skipped, `pesoTotal = 0`, and the QUALP API is called with zero weight. Not a crash, but a wasted API call that may return an unexpected freight floor. Adding `if (modelItem.rowCount() == 0) return;` at the top of `calcularFrete` would prevent this.
- **`on_pushButtonAbrirReplicaDe/Em/Venda_clicked` don't check `viewRegisterById` return** (src/orcamento.cpp:1812-1834). If the ID is not found in the DB, `viewRegisterById` returns false but the dialog is shown anyway in a broken state.
- **`on_checkBoxRepresentacao_toggled` hides `checkBoxFreteManual` without unchecking** (src/orcamento.cpp:1503-1504). When representação is toggled ON, the frete-manual checkbox is hidden but stays checked. On toggle OFF it reappears still checked, leading to inconsistent freight state.

---

## Code-quality / structural (unchanged)

- **1944-line "god dialog."** Pure math already extracted into `orcamento_calc.cpp`; freight querying, item CRUD, and id generation are the next layers to peel off, following the existing `venda_calc` / `orcamento_calc` pattern.
- **`unset/setConnections` pattern is brittle.** Migrating to the `ScopedUpdate` guard from `src/scopedupdate.h` (connect signals once, gate slot bodies with `if (updating) return; ScopedUpdate guard(updating);`) would eliminate the manual try/catch in 8+ handlers and the early-return footgun in src/orcamento.cpp:1583-1586. Per CLAUDE.md, do NOT use `QSignalBlocker`/`blockSignals` — they break signals Qt internals depend on.
- **Duplicated `if (ui->lineEditOrcamento->text() != "Auto gerado") { save(true); }`** in `removeItem` / `subir` / `descer` / `adicionarItem`. Wrap in a `persistIfSaved()` helper.
- **`montarLog` + double-bookkeeping `verificarTotais`** (src/orcamento.cpp:642-664) defensively re-runs the math against the spinbox values to catch drift. Worth a comment explaining the historical incident this guards against; otherwise it reads as dead defensive code.
- **Dead code behind `pushButtonModelo3d->hide()`** (src/orcamento.cpp:54). The `on_pushButtonModelo3d_clicked` handler (src/orcamento.cpp:1762) and its `authenticationRequired` lambda (line 1778, which uses `[&]` capture — a dangling-reference hazard if the signal fired asynchronously) are unreachable. Consider removing the handler if 3D model support is not planned for re-enablement.
- **Magic strings** for statuses (`"ATIVO"`, `"EXPIRADO"`, `"REPLICADO"`, `"FECHADO"`, `"PERDIDO"`, `"CANCELADO"`) and `headerData` (`"!"`, `"*"`). CLAUDE.md's "Process Improvements" already targets an enum-based status refactor; this file is a heavy user.

---

## Suggested next steps (in priority order)

1. ~~**§6**: add `setConnections()` to `cadastrar`'s catch block.~~ Already fixed by ScopedUpdate migration.
2. ~~**§1**: add the `headerData == "!"` skip in `verificaServicosEspeciais`, `calcularFrete`, `buscarConsultor` + discount/total handlers.~~ Fixed.
3. ~~**§9**: add `ui->doubleSpinBoxTotal->setMinimum(frete)` in `calcPrecoGlobalTotal`.~~ Fixed.
4. ~~**§7**: add `quantCaixa == 0` guard in `setarParametrosProduto`.~~ Fixed.
5. ~~**§8**: wrap `calcularFrete(true)` in try/catch inside `on_itemBoxEndereco_idChanged`.~~ Fixed.
6. ~~**§2**: fix caixas rounding to `ceil(caixas / stepCx) * stepCx`.~~ Fixed.
7. ~~**§4**: relax `buscarConsultor`'s throw on the `Atualizar` path.~~ Fixed (returns early instead of throwing).
8. ~~**§3**: split `atualizaReplica` into two updates — always set `replicadoEm`, only flip status when source was EXPIRADO.~~ Fixed.
9. ~~**§5**: wrap the replica-seeding loop.~~ Fixed (ScopedUpdate guard on replica->updating).
10. **§10**: fix `on_dataEmissao_dateChanged` validade max — replace `daysInMonth() - day()` with a fixed or configurable maximum. HIGH — orcamentos created on month-end have 0-day validity.
11. **§11**: fix `on_doubleSpinBoxDesconto_valueChanged` — use `ui->doubleSpinBoxQuant->value()` instead of re-deriving caixas2 with the wrong rounding formula.
12. **§12**: add duplicate estoque guard in `adicionarItem()`.
13. **Cross-file: audit `venda.cpp` discount handlers for the same `"!"` skip.** `src/venda.cpp` lines 896, 978, 1008 have the identical pattern — looping over `modelItem.rowCount()` without `headerData == "!"` checks. Fix alongside the orcamento sites.
14. **Parameterize SQL in `calculofrete.cpp`** (lines 224, 245, 441). Mechanical change to match the file's own established `prepare`/`bindValue` style.
