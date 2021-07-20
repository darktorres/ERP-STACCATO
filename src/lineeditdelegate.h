#pragma once

#include <QStyledItemDelegate>

class LineEditDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  enum class Tipo { ContraPartePagar, ContraParteReceber, Grupo };
  Q_ENUM(Tipo)

  explicit LineEditDelegate(const Tipo tipo, QObject *parent);
  ~LineEditDelegate() = default;

private:
  // attributes
  Tipo const tipo;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;
};
