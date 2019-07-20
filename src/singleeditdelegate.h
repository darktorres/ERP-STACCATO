#pragma once

#include <QStyledItemDelegate>

// TODO: rename this to EditDelegate?
class SingleEditDelegate final : public QStyledItemDelegate {

public:
  explicit SingleEditDelegate(QObject *parent = nullptr);
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
};
