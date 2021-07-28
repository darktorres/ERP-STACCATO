#pragma once

#include <QStyledItemDelegate>

class EditDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit EditDelegate(QObject *parent);

private:
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};
