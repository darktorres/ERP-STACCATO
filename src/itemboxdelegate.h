#pragma once

#include <QStyledItemDelegate>

class ItemBoxDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  enum class Tipo { Loja, Conta };
  Q_ENUM(Tipo)

  ItemBoxDelegate(const Tipo tipo, const bool isReadOnly, QObject *parent);
  ~ItemBoxDelegate() final = default;

private:
  // attributes
  bool const readOnly;
  Tipo const tipo;
  // methods
  auto commitEditor() -> void;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const -> QWidget * final;
  auto displayText(const QVariant &value, const QLocale &locale) const -> QString final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const -> void final;
};
