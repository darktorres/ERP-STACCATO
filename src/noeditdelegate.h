#pragma once

#include <QStyledItemDelegate>

class NoEditDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit NoEditDelegate(QObject *parent);

private:
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
};
