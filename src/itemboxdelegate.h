#ifndef ITEMBOXDELEGATE_H
#define ITEMBOXDELEGATE_H

#include <QStyledItemDelegate>

class ItemBoxDelegate final : public QStyledItemDelegate {

public:
  enum class Tipo { Loja, Conta };
  ItemBoxDelegate(const Tipo tipo, const bool isReadOnly, QObject *parent);
  ~ItemBoxDelegate() final = default;

private:
  // attributes
  const bool isReadOnly;
  const Tipo tipo;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;
  auto commitEditor() -> void;
};

#endif // ITEMBOXDELEGATE_H
