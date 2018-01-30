#ifndef PORCENTAGEMDELEGATE_H
#define PORCENTAGEMDELEGATE_H

#include <QStyledItemDelegate>

class PorcentagemDelegate final : public QStyledItemDelegate {

public:
  explicit PorcentagemDelegate(QObject *parent = nullptr);
  ~PorcentagemDelegate() = default;
  QString displayText(const QVariant &value, const QLocale &locale) const;
};

#endif // PORCENTAGEMDELEGATE_H
