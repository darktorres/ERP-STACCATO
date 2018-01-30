#ifndef LINEEDITDELEGATE_H
#define LINEEDITDELEGATE_H

#include <QStyledItemDelegate>

class LineEditDelegate final : public QStyledItemDelegate {
  Q_OBJECT

public:
  enum class Tipo { ContraPartePagar, ContraParteReceber, Grupo };

  explicit LineEditDelegate(const Tipo tipo, QObject *parent);
  ~LineEditDelegate() = default;
  void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const final;
  QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const final;

private:
  const Tipo tipo;
};

#endif // LINEEDITDELEGATE_H
