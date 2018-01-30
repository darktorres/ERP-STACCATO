#ifndef CHECKBOXDELEGATE_H
#define CHECKBOXDELEGATE_H

#include <QStyledItemDelegate>

class CheckBoxDelegate final : public QStyledItemDelegate {

public:
  explicit CheckBoxDelegate(QObject *parent, const bool readOnly = false);
  ~CheckBoxDelegate() = default;

private:
  // attributes
  const bool readOnly;
  // methods
  auto displayText(const QVariant &, const QLocale &) const -> QString final;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;
  auto commitAndCloseEditor() -> void;
};

#endif // CHECKBOXDELEGATE_H
