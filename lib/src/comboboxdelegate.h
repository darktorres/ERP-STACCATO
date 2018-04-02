#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>

class ComboBoxDelegate final : public QStyledItemDelegate {

public:
  enum class Tipo { Status, StatusReceber, StatusPagar, Conta, Pagamento, Grupo, ST };
  explicit ComboBoxDelegate(const Tipo tipo, QObject *parent = nullptr);
  ~ComboBoxDelegate() = default;

private:
  // attributes
  const Tipo tipo;
  // methods
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void final;
  auto setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const -> void final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;
};

#endif // COMBOBOXDELEGATE_H
