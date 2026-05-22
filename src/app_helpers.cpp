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

} // namespace app
