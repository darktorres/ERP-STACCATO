#pragma once

#include <QStyledItemDelegate>

class ComboBoxDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  enum class Tipo { CompraAvulsa, PagarAvulso, Receber, Pagar, Conta, Pagamento, Grupo, ST };
  Q_ENUM(Tipo)

  explicit ComboBoxDelegate(const Tipo tipo, QObject *parent);
  ~ComboBoxDelegate() = default;

private:
  // attributes
  Tipo const tipo;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const -> void final;
};
