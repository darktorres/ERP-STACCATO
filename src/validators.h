#pragma once

#include <QString>

// Pure validation helpers — no Qt dialog/exception side effects, returning
// `bool` so they can be exercised by Tier 1 tests without dragging the rest
// of the application graph in. Use these directly in new code; the wrappers
// `RegisterDialog::validaCPF/CNPJ` still exist for legacy callers that rely
// on the throw-RuntimeError-on-bad-checksum behavior.

namespace validators {

// Returns true iff `text` is a valid Brazilian CPF (11 digits after stripping
// dots and hyphens, not an all-same-digit sequence, and both check digits
// match). Returns false for any malformed or invalid input.
bool cpfValido(const QString &text);

// Returns true iff `text` is a valid Brazilian CNPJ (14 digits after stripping
// dots, slashes and hyphens, with both check digits matching).
bool cnpjValido(const QString &text);

} // namespace validators
