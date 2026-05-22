#pragma once

// Pure arithmetic helpers extracted from Orcamento::calcularFrete. Lets
// Tier 1 tests pin the freight invariants without needing the QUALP API,
// the DB, or the dialog. Matches the venda_calc layout.
//
// `Orcamento::calcularFrete` itself remains in orcamento.cpp — it still
// has to query the DB for per-product weight and call the QUALP service;
// the extracted helpers cover the pure math at the boundaries.

namespace orcamento_calc {

// Initial freight floor from the percentage rule and the loja's absolute
// minimum. Returns max(subTotalBruto * porcFrete / 100, minimoFrete).
//
// Negative inputs are not sensible (subtotal and minimum are positive in
// the schema; porcFrete is a percentage). No clamping — the caller is
// responsible for sourcing sane values.
double minimoFreteBase(double subTotalBruto, double porcFrete, double minimoFrete);

// Lower bound a gerente user is allowed to set, given the auto-computed
// `freteMaior` and the QUALP-reported `freteQualp` (which is 0 when the
// API call failed). Production rule (Orcamento::calcularFrete:1339-1342):
//
//   freteMenor = min(freteQualp, freteMaior)
//   if (freteMenor ≈ 0)  return freteMaior         // no QUALP, no discount
//   else                 return freteMenor * 0.8   // gerente may go 20% under
double minimoGerente(double freteMaior, double freteQualp);

} // namespace orcamento_calc
