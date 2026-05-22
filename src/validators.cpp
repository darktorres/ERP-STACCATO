#include "validators.h"

#include <QVector>

namespace validators {

bool cpfValido(const QString &text) {
  const QString cpf = QString(text).remove(QLatin1Char('.')).remove(QLatin1Char('-'));

  if (cpf.size() != 11) { return false; }

  if (cpf == QLatin1String("00000000000") or cpf == QLatin1String("11111111111") or cpf == QLatin1String("22222222222") or cpf == QLatin1String("33333333333") or
      cpf == QLatin1String("44444444444") or cpf == QLatin1String("55555555555") or cpf == QLatin1String("66666666666") or cpf == QLatin1String("77777777777") or
      cpf == QLatin1String("88888888888") or cpf == QLatin1String("99999999999")) {
    return false;
  }

  QVector<int> sub2;

  for (const auto &c : cpf.left(9)) { sub2.push_back(c.digitValue()); }

  const QVector<int> multiplicadores = {10, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma = 0;

  for (int i = 0; i < 9; ++i) { soma += sub2.at(i) * multiplicadores.at(i); }

  const int resto = soma % 11;
  const int digito1 = resto < 2 ? 0 : 11 - resto;

  sub2.push_back(digito1);

  const QVector<int> multiplicadores2 = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma2 = 0;

  for (int i = 0; i < 10; ++i) { soma2 += sub2.at(i) * multiplicadores2.at(i); }

  const int resto2 = soma2 % 11;
  const int digito2 = resto2 < 2 ? 0 : 11 - resto2;

  return digito1 == cpf.at(9).digitValue() and digito2 == cpf.at(10).digitValue();
}

bool cnpjValido(const QString &text) {
  const QString cnpj = QString(text).remove(QLatin1Char('.')).remove(QLatin1Char('/')).remove(QLatin1Char('-'));

  if (cnpj.size() != 14) { return false; }

  QVector<int> sub2;

  for (const auto &c : cnpj.left(12)) { sub2.push_back(c.digitValue()); }

  const QVector<int> multiplicadores = {5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma = 0;

  for (int i = 0; i < 12; ++i) { soma += sub2.at(i) * multiplicadores.at(i); }

  const int resto = soma % 11;
  const int digito1 = resto < 2 ? 0 : 11 - resto;

  sub2.push_back(digito1);

  const QVector<int> multiplicadores2 = {6, 5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma2 = 0;

  for (int i = 0; i < 13; ++i) { soma2 += sub2.at(i) * multiplicadores2.at(i); }

  const int resto2 = soma2 % 11;
  const int digito2 = resto2 < 2 ? 0 : 11 - resto2;

  return digito1 == cnpj.at(12).digitValue() and digito2 == cnpj.at(13).digitValue();
}

} // namespace validators
