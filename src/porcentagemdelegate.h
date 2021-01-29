#pragma once

#include <QStyledItemDelegate>

class PorcentagemDelegate final : public QStyledItemDelegate {

public:
  explicit PorcentagemDelegate(const bool multiplicar, QObject *parent);
  ~PorcentagemDelegate() = default;

  auto displayText(const QVariant &value, const QLocale &locale) const -> QString;

private:
  bool const multiplicar;
};
