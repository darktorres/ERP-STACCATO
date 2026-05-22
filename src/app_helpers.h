#pragma once

#include <QDate>
#include <QString>

#include <optional>

// Pure helpers historically defined as instance methods on Application but
// touching no instance state. Extracted so Tier 1 tests can exercise them
// without instantiating QApplication/Application or pulling in the DB
// connection setup. The Application methods now delegate here; production
// callers keep their existing syntax (`qApp->roundDouble(...)`).

namespace app {

// Round `value` to `decimals` fractional digits (default 4). Uses
// std::round(value * 10^d) / 10^d — half-away-from-zero behaviour.
double roundDouble(double value, int decimals = 4);

// Strip characters that historically broke SQL string building in this app
// (`+ @ > < ~ * ' \`). Not a substitute for parameterized queries.
QString sanitizeSQL(const QString &string);

// Unicode normalize (NFKD) and drop non-Latin1 marks; optionally also drop
// non-alphanumeric symbols. Used for normalizing user input before search /
// fuzzy comparison.
QString removerDiacriticos(const QString &s, bool removerSimbolos = false);

// Advance `date` forward to the next weekday (Mon–Fri). Bank holidays are
// not yet considered — see `// TODO: adicionar feriados bancarios` in the
// original Application::ajustarDiaUtil.
QDate ajustarDiaUtil(QDate date);

// Search `texto` for a line whose first characters match `tag` (case-
// insensitive), preceded by CRLF, and return the rest of that line. Returns
// `std::nullopt` if the tag isn't present. Used to parse NFe SEFAZ events.
// `Application::findTag` wraps this and throws RuntimeException on miss.
std::optional<QString> findTag(const QString &texto, const QString &tag);

} // namespace app
