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
  QString displayText(const QVariant &, const QLocale &) const final;
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const final;
  void setEditorData(QWidget *editor, const QModelIndex &index) const final;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const final;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const final;
  void commitAndCloseEditor();
};

#endif // CHECKBOXDELEGATE_H
