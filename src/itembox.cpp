#include "itembox.h"

#include "application.h"
#include "cadastrocliente.h"
#include "cadastrofornecedor.h"
#include "cadastroloja.h"
#include "cadastroprofissional.h"
#include "cadastrotransportadora.h"

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

  setConnections();
}

void ItemBox::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(searchButton, &QAbstractButton::clicked, this, &ItemBox::search, connectionType);
  connect(plusButton, &QAbstractButton::clicked, this, &ItemBox::edit, connectionType);
  connect(this, &QLineEdit::cursorPositionChanged, this, &ItemBox::resetCursor, connectionType);
}

void ItemBox::resizeEvent(QResizeEvent *event) {
  setIcons();
  QLineEdit::resizeEvent(event);
}

void ItemBox::search() {
  if (searchDialog) { searchDialog->show(); }
}

void ItemBox::edit() {
  if (not registerDialog and not registerDialogType.isEmpty()) {
    if (registerDialogType == "CadastroCliente") { registerDialog = new CadastroCliente(this); }
    if (registerDialogType == "CadastroFornecedor") { registerDialog = new CadastroFornecedor(this); }
    if (registerDialogType == "CadastroLoja") { registerDialog = new CadastroLoja(this); }
    if (registerDialogType == "CadastroProfissional") { registerDialog = new CadastroProfissional(this); }
    if (registerDialogType == "CadastroTransportadora") { registerDialog = new CadastroTransportadora(this); }

    if (not registerDialog) { throw RuntimeException("RegisterDialogType inválido!"); }

    connect(registerDialog, &RegisterDialog::registerUpdated, this, &ItemBox::changeItem);
  }

  if (registerDialog) {
    registerDialog->setReadOnly(true);

    if (not id.isNull()) { registerDialog->viewRegisterById(id); }

    registerDialog->show();
  }
}

void ItemBox::resetCursor() { setCursorPosition(0); }

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

  QLineEdit::setToolTip(QString());
}

void ItemBox::setRegisterDialog(const QString &type) {
  registerDialogType = type;
  setIcons();
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
  if (readOnlyItemBox) { return edit(); }

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

  if (not registerDialogType.isEmpty()) {
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

QSize ItemBox::sizeHint() const {
  int iconsMargin = 0;

  if (searchButton->isVisible()) { iconsMargin += 30; }
  if (plusButton->isVisible()) { iconsMargin += 30; }

  QSize customSize = QLineEdit::sizeHint();
  customSize.setWidth(fontMetrics().horizontalAdvance(text()) + iconsMargin);
  return customSize;
}

// TODO: replace this with eliding? https://wiki.qt.io/Elided_Label
// TODO: ajustar o tamanho do widget de acordo com o tamanho do texto para o autodimensionar funcionar corretamente na tableView (usar sizeHint?)
// TODO: add clearButtonEnabled?
// TODO: adicionar placeholder no lugar de usar um label junto do itemBox?
