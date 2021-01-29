#pragma once

#include <QStyledItemDelegate>

class ItemBoxDelegate final : public QStyledItemDelegate {

public:
  enum class Tipo { Loja, Conta };

  ItemBoxDelegate(const Tipo tipo, const bool readOnly, QObject *parent);
  ~ItemBoxDelegate() final = default;

private:
  // attributes
  bool const readOnly;
  Tipo const tipo;
  // methods
  auto commitEditor() -> void;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;
};
