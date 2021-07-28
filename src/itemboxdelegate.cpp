#include "itemboxdelegate.h"

#include "itembox.h"

#include <QDebug>

ItemBoxDelegate::ItemBoxDelegate(const Tipo tipo, const bool isReadOnly, QObject *parent) : QStyledItemDelegate(parent), readOnly(isReadOnly), tipo(tipo) {}

QWidget *ItemBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &, const QModelIndex &) const {
  if (readOnly) { return nullptr; }

  auto *editor = new ItemBox(parent);

  if (tipo == Tipo::Loja) { editor->setSearchDialog(SearchDialog::loja(parent)); }
  if (tipo == Tipo::Conta) { editor->setSearchDialog(SearchDialog::conta(parent)); }

  connect(editor, &ItemBox::textChanged, this, &ItemBoxDelegate::commitEditor);

  return editor;
}

void ItemBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  if (auto *box = qobject_cast<ItemBox *>(editor)) { box->setId(index.data(Qt::EditRole).toInt()); }
}

void ItemBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  if (auto *box = qobject_cast<ItemBox *>(editor)) { model->setData(index, box->getId(), Qt::EditRole); }
}

void ItemBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &) const { editor->setGeometry(option.rect); }

void ItemBoxDelegate::commitEditor() {
  QWidget *editor = qobject_cast<QWidget *>(sender());

  emit commitData(editor);
  emit closeEditor(editor);
}

QString ItemBoxDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  Q_UNUSED(value)
  Q_UNUSED(locale)

  if (value.isNull() or value.toInt() == 0 or value.toString() == "0") { return QString(); }

  QString dispText;

  if (tipo == Tipo::Loja) { dispText = SearchDialog::getCacheLoja()->getText(value); }
  if (tipo == Tipo::Conta) { dispText = SearchDialog::getCacheConta()->getText(value); }

  return dispText;
}
