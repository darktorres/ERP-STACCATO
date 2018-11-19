#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSqlError>
#include <QSqlQuery>
#include <QVBoxLayout>

#include "application.h"
#include "ui_widgetpagamentos.h"
#include "usersession.h"
#include "widgetpagamentos.h"

WidgetPagamentos::WidgetPagamentos(QWidget *parent) : QScrollArea(parent), ui(new Ui::WidgetPagamentos) {
  ui->setupUi(this);
  setWidgetResizable(true);

  auto *frame = new QFrame(this);
  auto *scrollLayout = new QVBoxLayout(frame);
  scrollLayout->setSizeConstraint(QLayout::SetMinimumSize);
  setWidget(frame);
}

WidgetPagamentos::~WidgetPagamentos() { delete ui; }

bool WidgetPagamentos::adicionarPagamentoCompra(const double restante) {
  // REFAC: refactor this with the other adicionarPagamento

  auto *frame = new QFrame(this);
  auto *layout = new QHBoxLayout(frame);
  frame->setLayout(layout);
  // label
  auto *labelPagamento = new QLabel(this);
  labelPagamento->setText("Pgt." + QString::number(widget()->children().size()));
  layout->addWidget(labelPagamento);
  // combobox pgt
  auto *comboBoxPgt = new QComboBox(this);
  comboBoxPgt->setMinimumWidth(140);

  QSqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE idLoja = :idLoja");
  queryPag.bindValue(":idLoja", UserSession::idLoja());

  if (not queryPag.exec()) { return qApp->enqueueError(false, "Erro lendo formas de pagamentos: " + queryPag.lastError().text()); }

  const QStringList list([&queryPag]() {
    QStringList temp("Escolha uma opção!");
    while (queryPag.next()) { temp << queryPag.value("pagamento").toString(); }
    return temp;
  }());

  comboBoxPgt->insertItems(0, list);
  layout->addWidget(comboBoxPgt);
  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, [=] { on_comboBoxPgt_currentTextChanged(listComboPgt.indexOf(comboBoxPgt), comboBoxPgt->currentText()); });
  listComboPgt << comboBoxPgt;
  // combobox data
  auto *comboBoxData = new QComboBox(this);
  comboBoxData->insertItem(0, "Data Mês");
  comboBoxData->insertItem(1, "Data + 1 Mês");
  comboBoxData->insertItem(2, "14");
  comboBoxData->insertItem(3, "20");
  comboBoxData->insertItem(4, "28");
  comboBoxData->insertItem(5, "30");
  layout->addWidget(comboBoxData);
  connect(comboBoxData, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  listComboData << comboBoxData;
  // combobox parc
  auto *comboboxPgtParc = new QComboBox(this);
  comboboxPgtParc->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  comboboxPgtParc->setMaximumWidth(45);
  connect(comboboxPgtParc, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(comboboxPgtParc);
  listComboParc << comboboxPgtParc;
  // doublespinbox
  auto *doubleSpinBoxPgt = new QDoubleSpinBox(this);
  doubleSpinBoxPgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  doubleSpinBoxPgt->setMinimumWidth(80);
  doubleSpinBoxPgt->setPrefix("R$ ");
  doubleSpinBoxPgt->setMaximum(restante);
  doubleSpinBoxPgt->setValue(restante);
  doubleSpinBoxPgt->setGroupSeparatorShown(true);
  layout->addWidget(doubleSpinBoxPgt);
  connect(doubleSpinBoxPgt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetPagamentos::valueChanged);
  listDoubleSpinPgt << doubleSpinBoxPgt;
  // dateedit
  auto *dateEditPgt = new QDateEdit(this);
  dateEditPgt->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  dateEditPgt->setDisplayFormat("dd/MM/yy");
  dateEditPgt->setCalendarPopup(true);
  dateEditPgt->setDate(QDate::currentDate());
  connect(dateEditPgt, &QDateEdit::dateChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(dateEditPgt);
  listDatePgt << dateEditPgt;
  // lineedit
  auto *lineEditPgt = new QLineEdit(this);
  lineEditPgt->setPlaceholderText("Observação");
  connect(lineEditPgt, &QLineEdit::textChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(lineEditPgt);
  listLinePgt << lineEditPgt;
  //
  widget()->layout()->addWidget(frame);

  return true;
}

bool WidgetPagamentos::adicionarPagamentoVenda(const bool representacao, const QString &idOrcamento, const double creditoTotal, const double restante) {
  auto *frame = new QFrame(this);
  frame->setLayout(new QHBoxLayout(frame));
  // label
  auto *labelPagamento = new QLabel(frame);
  labelPagamento->setText("Pgt." + QString::number(widget()->children().size()));
  frame->layout()->addWidget(labelPagamento);
  // checkbox
  auto *checkboxRep = new QCheckBox(frame);
  checkboxRep->setText("Fornecedor");
  checkboxRep->setVisible(representacao);
  checkboxRep->setChecked(representacao);
  checkboxRep->setEnabled(false);
  connect(checkboxRep, &QCheckBox::stateChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  frame->layout()->addWidget(checkboxRep);
  listCheckBoxRep << checkboxRep;
  // combo
  auto *comboBoxPgt = new QComboBox(frame);
  comboBoxPgt->setMinimumWidth(140);

  QSqlQuery queryOrc;
  queryOrc.prepare("SELECT idUsuario, idOrcamento, idLoja, idUsuarioConsultor, idCliente, idEnderecoEntrega, idEnderecoFaturamento, idProfissional, data, subTotalBru, subTotalLiq, frete, "
                   "freteManual, descontoPorc, descontoReais, total, status, observacao, prazoEntrega, representacao FROM orcamento WHERE idOrcamento = :idOrcamento");
  queryOrc.bindValue(":idOrcamento", idOrcamento);

  if (not queryOrc.exec() or not queryOrc.first()) { return qApp->enqueueError(false, "Erro buscando orçamento: " + queryOrc.lastError().text()); }

  QSqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE idLoja = :idLoja");
  queryPag.bindValue(":idLoja", queryOrc.value("idLoja"));

  if (not queryPag.exec()) { return qApp->enqueueError(false, "Erro lendo formas de pagamentos: " + queryPag.lastError().text()); }

  const QStringList list([&queryPag]() {
    QStringList temp("Escolha uma opção!");
    while (queryPag.next()) { temp << queryPag.value("pagamento").toString(); }
    return temp;
  }());

  comboBoxPgt->insertItems(0, list);
  if (creditoTotal > 0) { comboBoxPgt->addItem("Conta Cliente"); }
  frame->layout()->addWidget(comboBoxPgt);
  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, [=] { on_comboBoxPgt_currentTextChanged(listComboPgt.indexOf(comboBoxPgt), comboBoxPgt->currentText(), creditoTotal); });
  listComboPgt << comboBoxPgt;
  // combo
  auto *comboboxPgtParc = new QComboBox(frame);
  comboboxPgtParc->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  comboboxPgtParc->setMaximumWidth(45);
  connect(comboboxPgtParc, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  frame->layout()->addWidget(comboboxPgtParc);
  listComboParc << comboboxPgtParc;
  // doublespinbox
  auto *doubleSpinBoxPgt = new QDoubleSpinBox(frame);
  doubleSpinBoxPgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  doubleSpinBoxPgt->setMinimumWidth(80);
  doubleSpinBoxPgt->setPrefix("R$ ");
  doubleSpinBoxPgt->setMaximum(restante);
  doubleSpinBoxPgt->setValue(restante);
  doubleSpinBoxPgt->setGroupSeparatorShown(true);
  frame->layout()->addWidget(doubleSpinBoxPgt);
  connect(doubleSpinBoxPgt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetPagamentos::valueChanged);
  listDoubleSpinPgt << doubleSpinBoxPgt;
  // dateedit
  auto *dateEditPgt = new QDateEdit(frame);
  dateEditPgt->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  dateEditPgt->setDisplayFormat("dd/MM/yy");
  dateEditPgt->setCalendarPopup(true);
  dateEditPgt->setDate(QDate::currentDate());
  connect(dateEditPgt, &QDateEdit::dateChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  frame->layout()->addWidget(dateEditPgt);
  listDatePgt << dateEditPgt;
  // lineedit
  auto *lineEditPgt = new QLineEdit(frame);
  lineEditPgt->setPlaceholderText("Observação");
  connect(lineEditPgt, &QLineEdit::textChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  frame->layout()->addWidget(lineEditPgt);
  listLinePgt << lineEditPgt;
  //

  widget()->layout()->addWidget(frame);

  return true;
}

void WidgetPagamentos::on_comboBoxPgt_currentTextChanged(const int index, const QString &text, const double creditoTotal) {
  if (text == "Escolha uma opção!") { return; }

  if (text == "Conta Cliente") {
    listDoubleSpinPgt.at(index)->setMaximum(creditoTotal);
    listComboParc.at(index)->clear();
    listComboParc.at(index)->addItem("1x");
    emit montarFluxoCaixa();
    return;
  }

  QSqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", listComboPgt.at(index)->currentText());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro lendo formas de pagamentos: " + query.lastError().text()); }

  const int parcelas = query.value("parcelas").toInt();

  listComboParc.at(index)->clear();

  for (int i = 0; i < parcelas; ++i) { listComboParc.at(index)->addItem(QString::number(i + 1) + "x"); }

  listComboParc.at(index)->setEnabled(true);

  listDatePgt.at(index)->setEnabled(true);

  emit montarFluxoCaixa();
}

void WidgetPagamentos::on_comboBoxPgt_currentTextChanged(const int index, const QString &text) {
  if (text == "Escolha uma opção!") { return; }

  QSqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", listComboPgt.at(index)->currentText());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro lendo formas de pagamentos: " + query.lastError().text()); }

  const int parcelas = query.value("parcelas").toInt();

  listComboParc.at(index)->clear();

  // NOTE: this emits montarFluxoCaixa
  for (int i = 0; i < parcelas; ++i) { listComboParc.at(index)->addItem(QString::number(i + 1) + "x"); }

  listComboParc.at(index)->setEnabled(true);

  listDatePgt.at(index)->setEnabled(true);

  emit montarFluxoCaixa();
}

void WidgetPagamentos::resetarPagamentos() {
  for (auto item : widget()->children()) {
    if (qobject_cast<QFrame *>(item)) { delete item; }
  }

  listCheckBoxRep.clear();
  listComboData.clear();
  listComboParc.clear();
  listComboPgt.clear();
  listDatePgt.clear();
  listDoubleSpinPgt.clear();
  listLinePgt.clear();
}
