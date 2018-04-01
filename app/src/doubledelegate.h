#ifndef DOUBLEDELEGATE_H
#define DOUBLEDELEGATE_H

#include <QStyledItemDelegate>

class DoubleDelegate final : public QStyledItemDelegate {

public:
  explicit DoubleDelegate(QObject *parent = nullptr, const int decimais = 2);
  ~DoubleDelegate() = default;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;

private:
  const int decimais;
};

#endif // DOUBLEDELEGATE_H
