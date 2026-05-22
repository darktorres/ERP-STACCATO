#pragma once

#include <QVector>

#include <tuple>

// Pure arithmetic helpers extracted from Venda. Lets Tier 1 tests check
// invariants (subTotalBruto ≥ subTotalLiq, deletion-pending rows ignored,
// etc.) without instantiating the dialog or its SqlTableModel.

namespace venda_calc {

// One row of the venda items grid, reduced to just the fields totals depend
// on. Constructed by Venda::calcularTotais from `modelItem` and passed
// here as a flat array.
struct LineItem {
  double parcial;
  double parcialDesc;
  double total;
  bool pendingDeletion; // true when modelItem.headerData(row) == "!"
};

// Returns (subTotalBruto, subTotalLiq, total). Rows with `pendingDeletion`
// are skipped — they exist in the model only until the dialog is saved.
auto calcularTotais(const QVector<LineItem> &items) -> std::tuple<double, double, double>;

} // namespace venda_calc
