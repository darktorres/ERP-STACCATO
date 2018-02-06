#ifndef LINEEDITDELEGATE_H
#define LINEEDITDELEGATE_H

#include <QStyledItemDelegate>

class LineEditDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  enum class Tipo { ContraPartePagar, ContraParteReceber, Grupo };

  explicit LineEditDelegate(const Tipo tipo, QObject *parent);
  ~LineEditDelegate() = default;
  auto createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const -> QWidget * final;
  auto updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const -> void final;

private:
  const Tipo tipo;
};

#endif // LINEEDITDELEGATE_H
