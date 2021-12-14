#pragma once

#include <QStyledItemDelegate>

class CheckBoxDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  explicit CheckBoxDelegate(const bool readOnly, QObject *parent);
  explicit CheckBoxDelegate(QObject *parent);
  ~CheckBoxDelegate() final = default;

private:
  // attributes
  bool const readOnly = false;
  // methods
  auto commitEditor() -> void;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const -> void final;
};
