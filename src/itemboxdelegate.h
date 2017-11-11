#ifndef ITEMBOXDELEGATE_H
#define ITEMBOXDELEGATE_H

#include <QStyledItemDelegate>

class ItemBoxDelegate final : public QStyledItemDelegate {

public:
  enum class Tipo { Loja, Conta };
  ItemBoxDelegate(const Tipo tipo, const bool isReadOnly, QObject *parent);
  ~ItemBoxDelegate() = default;

private:
  // attributes
  const bool isReadOnly;
  const Tipo tipo;
  // methods
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const final;
  void setEditorData(QWidget *editor, const QModelIndex &index) const final;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const final;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const final;
  void commitAndCloseEditor();
};

#endif // ITEMBOXDELEGATE_H
