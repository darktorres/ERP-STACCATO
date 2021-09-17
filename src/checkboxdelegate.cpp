#include "checkboxdelegate.h"

#include <QCheckBox>
#include <QDebug>

CheckBoxDelegate::CheckBoxDelegate(const bool readOnly, QObject *parent) : QStyledItemDelegate(parent), readOnly(readOnly) {}

CheckBoxDelegate::CheckBoxDelegate(QObject *parent) : CheckBoxDelegate(false, parent) {}

QWidget *CheckBoxDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(option)
  Q_UNUSED(index)

  auto *editor = new QCheckBox(parent);
  if (readOnly) { editor->setDisabled(true); }

  connect(editor, &QCheckBox::toggled, this, &CheckBoxDelegate::commitEditor);

  return editor;
}

void CheckBoxDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const {
  if (auto *cb = qobject_cast<QCheckBox *>(editor)) { cb->setChecked(index.data(Qt::EditRole).toBool()); }
}

void CheckBoxDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const {
  if (auto *cb = qobject_cast<QCheckBox *>(editor)) { model->setData(index, cb->isChecked(), Qt::EditRole); }
}

void CheckBoxDelegate::updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const {
  Q_UNUSED(index)

  editor->setGeometry(option.rect);
}

QString CheckBoxDelegate::displayText(const QVariant &value, const QLocale &locale) const {
  Q_UNUSED(value)
  Q_UNUSED(locale)

  return QString();
}

void CheckBoxDelegate::commitEditor() {
  QWidget *editor = qobject_cast<QWidget *>(sender());
  emit commitData(editor);
}

// TODO: trocar checkbox por ✓✔☑✅√ ✖✘✗✕
