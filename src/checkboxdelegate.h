#pragma once

#include <QStyledItemDelegate>

class CheckBoxDelegate final : public QStyledItemDelegate {

public:
  explicit CheckBoxDelegate(const bool readOnly, QObject *parent);
  explicit CheckBoxDelegate(QObject *parent);
  ~CheckBoxDelegate() = default;

private:
  // attributes
  const bool readOnly = false;
  // methods
  auto displayText(const QVariant &, const QLocale &) const -> QString final;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;
  auto commitEditor() -> void;
};
