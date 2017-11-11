#ifndef COMBOBOXDELEGATE_H
#define COMBOBOXDELEGATE_H

#include <QStyledItemDelegate>

class ComboBoxDelegate final : public QStyledItemDelegate {

public:
  enum class Tipo { Status, StatusReceber, StatusPagar, Conta, Pagamento, Grupo };
  explicit ComboBoxDelegate(const Tipo tipo, QObject *parent = 0);
  ~ComboBoxDelegate() = default;

private:
  const Tipo tipo;
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const final;
  void setEditorData(QWidget *editor, const QModelIndex &index) const final;
  void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const final;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const final;
};

#endif // COMBOBOXDELEGATE_H
