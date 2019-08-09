#pragma once

#include <QStyledItemDelegate>

class EditDelegate final : public QStyledItemDelegate {

public:
  explicit EditDelegate(QObject *parent = nullptr);
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};
