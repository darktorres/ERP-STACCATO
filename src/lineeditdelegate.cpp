#include "lineeditdelegate.h"

#include "sqlquerymodel.h"

#include <QCompleter>
#include <QLineEdit>

LineEditDelegate::LineEditDelegate(const Tipo tipo, QObject *parent) : QStyledItemDelegate(parent), tipo(tipo) {}

QWidget *LineEditDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(option)
  Q_UNUSED(index)

  auto *editor = new QLineEdit(parent);

  auto *model = new SqlQueryModel(parent);

  // TODO: query caches

  if (tipo == Tipo::ContraPartePagar) { model->setQuery("SELECT DISTINCT(contraParte) FROM conta_a_pagar_has_pagamento"); }
  if (tipo == Tipo::ContraParteReceber) { model->setQuery("SELECT DISTINCT(contraParte) FROM conta_a_receber_has_pagamento"); }
  if (tipo == Tipo::Grupo) { model->setQuery("SELECT tipo FROM despesa"); }

  model->select();

  auto *completer = new QCompleter(model, parent);
  completer->setCaseSensitivity(Qt::CaseInsensitive);
  editor->setCompleter(completer);

  return editor;
}

void LineEditDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(index)

  editor->setGeometry(option.rect.adjusted(0, 0, 200, 0));
}
