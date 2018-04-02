#ifndef PORCENTAGEMDELEGATE_H
#define PORCENTAGEMDELEGATE_H

#include <QStyledItemDelegate>

class PorcentagemDelegate final : public QStyledItemDelegate {

public:
  explicit PorcentagemDelegate(QObject *parent = nullptr);
  ~PorcentagemDelegate() = default;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString;
};

#endif // PORCENTAGEMDELEGATE_H
