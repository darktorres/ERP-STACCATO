#include <QDebug>
#include <QMouseEvent>

#include "itembox.h"

ItemBox::ItemBox(QWidget *parent) : QLineEdit(parent) {
  setReadOnly(true);

  searchButton = new QPushButton(this);
  searchButton->setIcon(QIcon(":/search.png"));
  searchButton->setAutoDefault(false);
  searchButton->setFlat(true);
  searchButton->setIconSize(QSize(14, 14));

  plusButton = new QPushButton(this);
  plusButton->setIcon(QIcon(":/plus.png"));
  plusButton->setAutoDefault(false);
  plusButton->setFlat(true);
  plusButton->setIconSize(QSize(14, 14));

  ensurePolished();

  connect(searchButton, &QAbstractButton::clicked, this, &ItemBox::search);
  connect(plusButton, &QAbstractButton::clicked, this, &ItemBox::edit);
  connect(this, &QLineEdit::cursorPositionChanged, this, &ItemBox::resetCursor);
}

void ItemBox::resizeEvent(QResizeEvent *event) {
  setIcons();
  QLineEdit::resizeEvent(event);
}

void ItemBox::search() {
  if (searchDialog) { searchDialog->show(); }
}

void ItemBox::edit() {
  if (registerDialog) {
    if (not value.isNull()) { registerDialog->viewRegisterById(value); }

    registerDialog->show();
  }
}

// REFAC: replace this with eliding? https://wiki.qt.io/Elided_Label
void ItemBox::resetCursor() { setCursorPosition(0); }

void ItemBox::setRegisterDialog(RegisterDialog *dialog) {
  registerDialog = dialog;
  connect(dialog, &RegisterDialog::registerUpdated, this, &ItemBox::changeItem);
  setIcons();
}

QVariant ItemBox::getValue() const { return value; }

void ItemBox::setValue(const QVariant &newValue) {
  if (newValue.isNull()) { return; }
  if (value == newValue) { return; }

  value = newValue;

  if (searchDialog) { setText(searchDialog->getText(newValue)); }

  QLineEdit::setToolTip(text());

  emit valueChanged(newValue);
}

void ItemBox::setReadOnlyItemBox(const bool isReadOnly) {
  readOnlyItemBox = isReadOnly;

  plusButton->setHidden(isReadOnly);
  plusButton->setDisabled(isReadOnly);
  searchButton->setHidden(isReadOnly);
  searchButton->setDisabled(isReadOnly);
}

void ItemBox::clear() {
  value.clear();

  QLineEdit::clear();
}

void ItemBox::setSearchDialog(SearchDialog *dialog) {
  searchDialog = dialog;
  connect(searchDialog, &SearchDialog::itemSelected, this, &ItemBox::changeItem);
  setIcons();
}

void ItemBox::changeItem(const QVariant &newValue) {
  setValue(newValue);

  if (registerDialog) { registerDialog->close(); }
  if (searchDialog) { searchDialog->close(); }
}

void ItemBox::mouseDoubleClickEvent(QMouseEvent *event) {
  if (readOnlyItemBox) { return; }

  search();
  event->accept();
}

void ItemBox::setIcons() {
  const QSize size = searchButton->minimumSizeHint();
  int x = rect().right();
  int y = (rect().height() - size.height()) / 2;

  if (searchDialog) {
    x -= size.width();
    searchButton->setGeometry(QRect(QPoint(x, y), size));
    searchButton->show();
  } else {
    searchButton->hide();
  }

  if (registerDialog) {
    x -= size.width();
    plusButton->setGeometry(QRect(QPoint(x, y), size));
    plusButton->show();
  } else {
    plusButton->hide();
  }

  int left, top, bottom;
  getTextMargins(&left, &top, nullptr, &bottom);
  setTextMargins(left, top, rect().right() - x + 4, bottom);
}

void ItemBox::setRepresentacao(const bool isRepresentacao) { searchDialog->setRepresentacao(isRepresentacao); }

void ItemBox::setFilter(const QString &filter) { searchDialog->setFilter(filter); }

void ItemBox::setFornecedorRep(const QString &fornecedor) { searchDialog->setFornecedorRep(fornecedor); }
