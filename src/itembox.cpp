#include "itembox.h"

#include <QDebug>
#include <QMouseEvent>

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
    if (not id.isNull()) { registerDialog->viewRegisterById(id); }

    registerDialog->show();
  }
}

void ItemBox::resetCursor() { setCursorPosition(0); }

void ItemBox::setRegisterDialog(RegisterDialog *dialog) {
  registerDialog = dialog;
  connect(dialog, &RegisterDialog::registerUpdated, this, &ItemBox::changeItem);
  setIcons();
}

QVariant ItemBox::getId() const { return id; }

void ItemBox::setId(const QVariant &newId) {
  if (newId.isNull()) { return; }

  id = newId;

  if (searchDialog) { setText(searchDialog->getText(newId)); }

  QLineEdit::setToolTip(text());

  emit idChanged(newId);
}

void ItemBox::setReadOnlyItemBox(const bool isReadOnly) { readOnlyItemBox = isReadOnly; }

void ItemBox::clear() {
  id.clear();

  QLineEdit::clear();
}

void ItemBox::setSearchDialog(SearchDialog *dialog) {
  searchDialog = dialog;
  connect(searchDialog, &SearchDialog::itemSelected, this, &ItemBox::changeItem);
  setIcons();
}

void ItemBox::changeItem(const QVariant &newId) {
  setId(newId);

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

  if (searchDialog and not readOnlyItemBox) {
    x -= size.width();
    searchButton->setGeometry(QRect(QPoint(x, y), size));
    searchButton->show();
  } else {
    searchButton->hide();
  }

  if (registerDialog and not readOnlyItemBox) {
    x -= size.width();
    plusButton->setGeometry(QRect(QPoint(x, y), size));
    plusButton->show();
  } else {
    plusButton->hide();
  }

  const auto margins = textMargins();
  setTextMargins(margins.left(), margins.top(), rect().right() - x, margins.bottom());
}

void ItemBox::setRepresentacao(const bool isRepresentacao) { searchDialog->setRepresentacao(isRepresentacao); }

void ItemBox::setFilter(const QString &filter) { searchDialog->setFilter(filter); }

void ItemBox::setFornecedorRep(const QString &fornecedor) { searchDialog->setFornecedorRep(fornecedor); }

// TODO: replace this with eliding? https://wiki.qt.io/Elided_Label
// TODO: ajustar o tamanho do widget de acordo com o tamanho do texto para o autodimensionar funcionar corretamente na tableView (usar sizeHint?)
