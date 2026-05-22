#include "venda_calc.h"

namespace venda_calc {

std::tuple<double, double, double> calcularTotais(const QVector<LineItem> &items) {
  double subTotalBruto = 0.;
  double subTotalLiq = 0.;
  double total = 0.;

  for (const auto &item : items) {
    if (item.pendingDeletion) { continue; }

    subTotalBruto += item.parcial;
    subTotalLiq += item.parcialDesc;
    total += item.total;
  }

  return {subTotalBruto, subTotalLiq, total};
}

} // namespace venda_calc
