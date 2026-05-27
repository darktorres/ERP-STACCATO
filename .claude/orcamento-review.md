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

---

## Suspect / worth knowing

- **`on_pushButtonModelo3d_clicked` URL not percent-encoded** (src/orcamento.cpp:1771). Investigated: `QUrl(QString)` defaults to `TolerantMode`, which auto-encodes spaces and most non-ASCII characters, so "MODELOS 3D" and accented fornecedor names like "SÃO" survive. The remaining hazards are admin-controlled but theoretically real: fornecedor / codComercial containing `/`, `?`, or `#` would be interpreted as path / query / fragment delimiters (parentheses are sub-delims and pass through). Severity downgraded from "bug" to "harden if fornecedor naming is ever opened up."
- **`removeRow` failure leaves connections off in the parent caller path.** `removeItem`'s own try/catch reconnects (src/orcamento.cpp:533-536). But if `save(true)` later throws from `cadastrar`, the outer `cadastrar` catch in src/orcamento.cpp:1508 rolls the transaction back but doesn't re-fire `setConnections`. Audit relies on the caller (`save()` in `RegisterDialog`) to leave the dialog in a sensible state.
- **N+1 SQL in `calcularFrete`** (two queries × `rowCount`, src/orcamento.cpp:1300-1316) and **`calcularPesoTotal`** (one query × rowCount, src/orcamento.cpp:1833-1838). For a 50-item orcamento with QUALP enabled, that's 100-150 round trips per recalculation, and `calcPrecoGlobalTotal` calls `calcularFrete` on every quant/desconto edit. A single JOIN against `produto`/`fornecedor` once per save would be a major win.
- **`Sql::updateFornecedoresOrcamento(primaryId)`** (called at src/orcamento.cpp:1499). Source is `CALL update_fornecedores_orcamento('<id>')` with the id interpolated into SQL, not parameterized (src/sql.cpp:22). The id comes from `generateId` (deterministic, not user input), so not a live SQL-injection issue, but worth parameterizing. The CALL participates in the same connection / transaction since `SqlQuery` uses the default connection — confirmed safe under rollback assuming the stored procedure body itself doesn't `COMMIT`.
- **`calculofrete.cpp` SQL concatenation** (src/calculofrete.cpp:224, 245, 441). Three queries use string concatenation for `idEndereco` and `idProduto` values instead of parameterized queries: the QUALP cache lookup (line 224), the destination address lookup (line 245), and the fornecedor-vemDoSul lookup in the per-row loop (line 441). Values originate from ItemBox IDs (integer-derived from DB lookups), so not directly exploitable. But the same file uses `prepare`/`bindValue` in other queries, making the inconsistency a maintenance hazard.
- **`generateId` ID size invariant** (`id.size() != 12 and id.size() != 13`, src/orcamento.cpp:566) silently throws on any loja that ever exceeds 9999 orcamentos in a year.
- **`spinBoxPesoTotal->setValue(double total)`** at src/orcamento.cpp:1845 — silent narrowing (the file's own `// TODO: implicit conversion double -> int` flags this).
- **Validade UI uses local date math** (`addDays(data("validade").toInt())`, src/orcamento.cpp:254) while save uses `qApp->serverDateTime()` (src/orcamento.cpp:701). `serverDate()` is cached (src/application.cpp:462-463), so cross-day drift is bounded but cross-timezone clients could see different expiry states. Low severity.

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

1. **§1**: add the `headerData == "!"` skip in `verificaServicosEspeciais`, `calcularFrete`, `buscarConsultor`. Mechanical change with direct user-visible effects.
2. **§2**: fix caixas rounding to `ceil(caixas / stepCx) * stepCx`, and add a tier-1 test in `orcamento_calc` covering "non-unit caixas step" alongside the existing freight tests.
3. **§4**: relax `buscarConsultor`'s throw on the `Atualizar` path (warn + keep existing value) or scope it to fornecedor-set changes only.
4. **§3**: split `atualizaReplica` into two updates — always set `replicadoEm`, only flip status when source was EXPIRADO.
5. **§5**: wrap the replica-seeding loop in `unsetConnections` / `setConnections`.
6. **Cross-file: audit `venda.cpp` discount handlers for the same `"!"` skip.** `src/venda.cpp` lines 896, 978, 1008 have the identical pattern — looping over `modelItem.rowCount()` without `headerData == "!"` checks. Same severity assessment as the orcamento §1 addendum (gated by `unsetConnections` during delete flow). Fix alongside the orcamento sites.
7. **Parameterize SQL in `calculofrete.cpp`** (lines 224, 245, 441). Mechanical change to match the file's own established `prepare`/`bindValue` style.
