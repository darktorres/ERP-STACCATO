#include "orcamento_calc.h"

#include <QtMath>

#include <algorithm>

namespace orcamento_calc {

double minimoFreteBase(double subTotalBruto, double porcFrete, double minimoFrete) {
  const double fretePorcentagem = subTotalBruto * porcFrete / 100.0;
  return std::max(fretePorcentagem, minimoFrete);
}

double minimoGerente(double freteMaior, double freteQualp) {
  const double freteMenor = std::min(freteQualp, freteMaior);
  return qFuzzyIsNull(freteMenor) ? freteMaior : freteMenor * 0.8;
}

} // namespace orcamento_calc
