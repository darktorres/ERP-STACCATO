#ifndef REAISDELEGATE_H
#define REAISDELEGATE_H

#include <QStyledItemDelegate>

class ReaisDelegate final : public QStyledItemDelegate {

public:
  explicit ReaisDelegate(QObject *parent = nullptr, const int decimais = 2);
  ~ReaisDelegate() = default;
  QString displayText(const QVariant &value, const QLocale &locale) const;

private:
  const int decimais;
};

#endif // REAISDELEGATE_H
