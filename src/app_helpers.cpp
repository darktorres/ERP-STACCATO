#include "app_helpers.h"

#include <QRegularExpression>

#include <algorithm>
#include <cmath>

namespace app {

double roundDouble(const double value, const int decimals) {
  const double multiploDez = std::pow(10, decimals);
  return std::round(value * multiploDez) / multiploDez;
}

QString sanitizeSQL(const QString &string) {
  QString sanitized = string;
  sanitized.remove("+").remove("@").remove(">").remove("<").remove("~").remove("*").remove("'").remove(R"(\)");
  return sanitized;
}

QString removerDiacriticos(const QString &s, const bool removerSimbolos) {
  const QString normalized = s.normalized(QString::NormalizationForm_KD);
  QString result;

  std::copy_if(normalized.begin(), normalized.end(), std::back_inserter(result), [](const QChar &c) { return c.toLatin1() != 0; });

  if (removerSimbolos) { result.remove(QRegularExpression("[^a-zA-Z\\d\\s]")); }

  return result;
}

QDate ajustarDiaUtil(QDate date) {
  // TODO: adicionar feriados bancarios
  while (date.dayOfWeek() > 5) { date = date.addDays(1); }
  return date;
}

std::optional<QString> findTag(const QString &texto, const QString &tag) {
  // NOTE: `Qt::CaseInsensitive` is the historical literal here but it lands
  // in QString::indexOf's `from` argument (=1), not the case-sensitivity
  // argument. See header for details.
  const int index = texto.indexOf(QStringLiteral("\r\n") + tag, Qt::CaseInsensitive);

  if (index == -1) { return std::nullopt; }

  return texto.mid(index + tag.length() + 2).split(QStringLiteral("\r\n")).first();
}

} // namespace app
