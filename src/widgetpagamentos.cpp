#include "widgetpagamentos.h"
#include "ui_widgetpagamentos.h"

#include "application.h"
#include "logindialog.h"
#include "user.h"

#include <QDebug>
#include <QLineEdit>
#include <QSqlError>

WidgetPagamentos::WidgetPagamentos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetPagamentos) {
  ui->setupUi(this);

  //---------------------------------------------------

  ui->scrollArea->setWidgetResizable(true);

  auto *frame = new QFrame(this);
  auto *scrollLayout = new QVBoxLayout(frame);
  scrollLayout->setSizeConstraint(QLayout::SetMinimumSize);
  ui->scrollArea->setWidget(frame);

  //---------------------------------------------------

  setConnections();
}

WidgetPagamentos::~WidgetPagamentos() { delete ui; }

void WidgetPagamentos::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonAdicionarPagamento_clicked, connectionType);
  connect(ui->pushButtonLimparPag, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonLimparPag_clicked, connectionType);
  connect(ui->pushButtonPgtLoja, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonPgtLoja_clicked, connectionType);
  connect(ui->pushButtonFreteLoja, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonFreteLoja_clicked, connectionType);
}

void WidgetPagamentos::labelPagamento(Pagamento *pgt) {
  auto *labelPagamento = new QLabel(pgt);
  labelPagamento->setText("Pgt." + QString::number(pagamentos.size()));

  pgt->layout()->addWidget(labelPagamento);
  pgt->label = labelPagamento;
}

void WidgetPagamentos::lineEditPgt(Pagamento *pgt) {
  auto *lineEditPgt = new QLineEdit(pgt);
  lineEditPgt->setPlaceholderText("Observação");

  connect(lineEditPgt, &QLineEdit::textChanged, this, &WidgetPagamentos::montarFluxoCaixa);

  pgt->layout()->addWidget(lineEditPgt);
  pgt->observacao = lineEditPgt;
}

void WidgetPagamentos::dateEditPgt(Pagamento *pgt) {
  auto *dateEditPgt = new QDateEdit(pgt);
  dateEditPgt->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  dateEditPgt->setDisplayFormat("dd/MM/yy");
  dateEditPgt->setCalendarPopup(true);
  dateEditPgt->setDate(qApp->serverDate());

  connect(dateEditPgt, &QDateEdit::dateChanged, this, &WidgetPagamentos::montarFluxoCaixa);

  pgt->layout()->addWidget(dateEditPgt);
  pgt->dataPgt = dateEditPgt;
}

void WidgetPagamentos::on_doubleSpinBoxPgt_valueChanged() {
  auto *pgt = qobject_cast<Pagamento *>(sender()->parent());

  if (not pgt) { throw RuntimeException("Pgt Null!"); }

  if (pgt->comboTipoPgt->currentText() == "CONTA CLIENTE") { calculaCreditoRestante(); }

  calcularTotal();
}

void WidgetPagamentos::calculaCreditoRestante() {
  double creditoUsado = 0;

  for (auto *pgt : qAsConst(pagamentos)) {
    if (pgt->comboTipoPgt->currentText() == "CONTA CLIENTE") { creditoUsado += pgt->valorPgt->value(); }
  }

  creditoRestante = credito - creditoUsado;
  ui->doubleSpinBoxCreditoDisponivel->setValue(creditoRestante);
}

void WidgetPagamentos::doubleSpinBoxPgt(Pagamento *pgt) {
  const double restante = total - ui->doubleSpinBoxTotalPag->value();

  auto *doubleSpinBoxPgt = new QDoubleSpinBox(pgt);
  doubleSpinBoxPgt->setPrefix("R$ ");
  doubleSpinBoxPgt->setMaximum(total);
  doubleSpinBoxPgt->setValue(restante);
  doubleSpinBoxPgt->setGroupSeparatorShown(true);

  connect(doubleSpinBoxPgt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetPagamentos::on_doubleSpinBoxPgt_valueChanged);

  pgt->layout()->addWidget(doubleSpinBoxPgt);
  pgt->valorPgt = doubleSpinBoxPgt;
}

void WidgetPagamentos::comboBoxParc(Pagamento *pgt) {
  auto *comboboxPgtParc = new QComboBox(pgt);
  comboboxPgtParc->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  comboboxPgtParc->setMaximumWidth(45);

  connect(comboboxPgtParc, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);

  pgt->layout()->addWidget(comboboxPgtParc);
  pgt->comboParcela = comboboxPgtParc;
}

void WidgetPagamentos::comboBoxTipoData(Pagamento *pgt) {
  auto *comboBoxTipoData = new QComboBox(pgt);
  comboBoxTipoData->addItem("DATA MÊS");
  comboBoxTipoData->addItem("DATA + 1 MÊS");
  comboBoxTipoData->addItem("14");
  comboBoxTipoData->addItem("20");
  comboBoxTipoData->addItem("28");
  comboBoxTipoData->addItem("30");
  comboBoxTipoData->addItem("60");
  comboBoxTipoData->addItem("90");
  comboBoxTipoData->addItem("120");
  comboBoxTipoData->addItem("150");
  comboBoxTipoData->addItem("180");
  comboBoxTipoData->addItem("210");

  connect(comboBoxTipoData, &QComboBox::currentTextChanged, this, &WidgetPagamentos::on_comboBoxTipoData_currentTextChanged);

  pgt->layout()->addWidget(comboBoxTipoData);
  pgt->comboTipoData = comboBoxTipoData;

  // connect this comboBoxData to the equivalent dateEditPgt

  // listComboData with 0 elements: size 0
  // listComboData with 1 elements: size 1; first element at [0]

  //  const auto dateEditPair = listDatePgt.at(listComboData.size());

  //  connect(comboBoxData, &QComboBox::currentTextChanged, listDatePgt.at(listComboData.size()), );

  // TODO: quando alterar dataPgt para 14/20/28/30 alterar a data no dateEdit e não apenas no fluxo
}

void WidgetPagamentos::comboBoxPgtCompra(Pagamento *pgt) {
  auto *comboBoxPgt = new QComboBox(pgt);
  comboBoxPgt->setMinimumWidth(140);

  SqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE idLoja = :idLoja");
  queryPag.bindValue(":idLoja", User::idLoja);

  if (not queryPag.exec()) { throw RuntimeException("Erro lendo formas de pagamentos: " + queryPag.lastError().text()); }

  QStringList list;
  list << "ESCOLHA UMA OPÇÃO!";

  while (queryPag.next()) { list << queryPag.value("pagamento").toString(); }

  comboBoxPgt->insertItems(0, list);

  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, &WidgetPagamentos::on_comboBoxPgt_currentTextChanged);

  pgt->layout()->addWidget(comboBoxPgt);
  pgt->comboTipoPgt = comboBoxPgt;
}

void WidgetPagamentos::checkBoxRep(Pagamento *pgt) {
  auto *checkboxRep = new QCheckBox(pgt);
  checkboxRep->setText("Fornecedor");
  checkboxRep->setVisible(representacao);
  checkboxRep->setChecked(representacao);
  checkboxRep->setEnabled(false);

  connect(checkboxRep, &QCheckBox::stateChanged, this, &WidgetPagamentos::montarFluxoCaixa);

  pgt->layout()->addWidget(checkboxRep);
  pgt->checkBoxRep = checkboxRep;
}

void WidgetPagamentos::comboBoxPgtVenda(Pagamento *pgt) {
  auto *comboBoxPgt = new QComboBox(pgt);
  comboBoxPgt->setMinimumWidth(140);

  if (idOrcamento.isEmpty()) { throw RuntimeError("Orçamento vazio!"); }

  SqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE apenasRepresentacao = :apenasRepresentacao AND idLoja = (SELECT idLoja FROM orcamento WHERE idOrcamento = :idOrcamento)");
  queryPag.bindValue(":idOrcamento", idOrcamento);
  queryPag.bindValue(":apenasRepresentacao", representacao);

  if (not queryPag.exec()) { throw RuntimeException("Erro lendo formas de pagamentos: " + queryPag.lastError().text()); }

  QStringList list;
  list << "ESCOLHA UMA OPÇÃO!";

  while (queryPag.next()) { list << queryPag.value("pagamento").toString(); }

  if (credito > 0) { list << "CONTA CLIENTE"; }

  comboBoxPgt->insertItems(0, list);

  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, &WidgetPagamentos::on_comboBoxPgt_currentTextChanged);

  pgt->layout()->addWidget(comboBoxPgt);
  pgt->comboTipoPgt = comboBoxPgt;
}

void WidgetPagamentos::on_comboBoxTipoData_currentTextChanged(const QString &text) {
  auto *sender = qobject_cast<Pagamento *>(QObject::sender());

  const QDate currentDate = qApp->serverDate();

  QDate dataPgt;

  if (text == "DATA + 1 MÊS") {
    dataPgt = currentDate.addMonths(1);
  } else if (text == "DATA MÊS") {
    dataPgt = currentDate;
  } else {
    dataPgt = currentDate.addDays(text.toInt());
  }

  sender->dataPgt->setDate(dataPgt);

  emit montarFluxoCaixa();
}

void WidgetPagamentos::on_comboBoxPgt_currentTextChanged(const QString &text) {
  auto *sender = qobject_cast<Pagamento *>(QObject::sender());

  if (text == "ESCOLHA UMA OPÇÃO!") { return; }

  if (text == "CONTA CLIENTE") {
    if (qFuzzyIsNull(ui->doubleSpinBoxCreditoDisponivel->value())) {
      sender->comboTipoPgt->setCurrentIndex(0);
      throw RuntimeError("Não há saldo cliente restante!", this);
    }

    double currentValue = sender->valorPgt->value();
    double newValue = qMin(currentValue, creditoRestante);
    calculaCreditoRestante();
    sender->valorPgt->setMaximum(newValue);
    sender->comboParcela->clear();
    sender->comboParcela->addItem("1x");
    emit montarFluxoCaixa();
    return;
  }

  sender->valorPgt->setMaximum(total); // TODO: it looks like this is not needed anymore since setTotal() update all widgets

  SqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", sender->comboTipoPgt->currentText());

  if (not query.exec()) { throw RuntimeException("Erro lendo formas de pagamentos: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrou dados do pagamento: " + sender->comboTipoPgt->currentText()); }

  const int parcelas = query.value("parcelas").toInt();

  { // NOTE: avoid sending signal before widgets are all set
    const QSignalBlocker blocker(sender->comboParcela);

    sender->comboParcela->clear();

    for (int i = 0; i < parcelas; ++i) { sender->comboParcela->addItem(QString::number(i + 1) + "x"); }
  }

  sender->comboParcela->setEnabled(true);
  sender->dataPgt->setEnabled(true);

  calculaCreditoRestante();

  emit montarFluxoCaixa();
}

void WidgetPagamentos::resetarPagamentos() {
  for (auto *item : ui->scrollArea->widget()->children()) {
    if (qobject_cast<QFrame *>(item)) { delete item; }
  }

  qDeleteAll(pagamentos);
  pagamentos.clear();

  ui->doubleSpinBoxTotalPag->setValue(0);

  calculaCreditoRestante();

  emit montarFluxoCaixa();
}

double WidgetPagamentos::getTotalPag() { return ui->doubleSpinBoxTotalPag->value(); }

void WidgetPagamentos::prepararPagamentosRep() {
  if (qFuzzyIsNull(frete)) { return; }

  on_pushButtonAdicionarPagamento_clicked();

  pagamentos.at(0)->checkBoxRep->setChecked(not fretePagoLoja);
  pagamentos.at(0)->observacao->setText("FRETE");
  pagamentos.at(0)->observacao->setReadOnly(true);
  pagamentos.at(0)->valorPgt->setValue(frete);
  pagamentos.at(0)->valorPgt->setReadOnly(true);
}

void WidgetPagamentos::setCredito(const double creditoCliente) {
  credito = creditoCliente;
  creditoRestante = creditoCliente;
  ui->doubleSpinBoxCreditoTotal->setValue(creditoCliente);
  ui->doubleSpinBoxCreditoDisponivel->setValue(creditoRestante);
}

void WidgetPagamentos::setRepresentacao(const bool isRepresentacao) {
  representacao = isRepresentacao;

  if (not isRepresentacao) {
    ui->pushButtonFreteLoja->hide();
    ui->pushButtonPgtLoja->hide();
  }
}

void WidgetPagamentos::setTipo(const Tipo novoTipo) {
  if (novoTipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  tipo = novoTipo;

  if (tipo == Tipo::Compra) {
    ui->label_4->hide();
    ui->doubleSpinBoxCreditoTotal->hide();
    ui->pushButtonFreteLoja->hide();
    ui->pushButtonPgtLoja->hide();
  }
}

void WidgetPagamentos::setTotal(const double value) {
  total = value;

  for (auto *pgt : pagamentos) { pgt->valorPgt->setMaximum(total); }
}

void WidgetPagamentos::setFrete(const double value) { frete = value; }

void WidgetPagamentos::setFretePagoLoja() {
  fretePagoLoja = true;

  ui->pushButtonFreteLoja->setDisabled(true);
}

void WidgetPagamentos::setIdOrcamento(const QString &value) { idOrcamento = value; }

double WidgetPagamentos::getCredito() const { return credito; }

void WidgetPagamentos::calcularTotal() {
  //  if (not pagamentos.isEmpty()) {
  const double sumWithoutLast = std::accumulate(pagamentos.cbegin(), pagamentos.cend() - 1, 0., [=](double accum, const Pagamento *pgt) { return accum += pgt->valorPgt->value(); });

  auto *lastSpinBox = pagamentos.constLast()->valorPgt;

  lastSpinBox->blockSignals(true);
  lastSpinBox->setValue(total - sumWithoutLast);
  lastSpinBox->blockSignals(false);

  // ----------------------------------------

  const double sumAll = std::accumulate(pagamentos.cbegin(), pagamentos.cend(), 0., [=](double accum, const Pagamento *pgt) { return accum += pgt->valorPgt->value(); });

  ui->doubleSpinBoxTotalPag->setValue(sumAll);
  //  }

  // ----------------------------------------

  emit montarFluxoCaixa();
}

void WidgetPagamentos::on_pushButtonAdicionarPagamento_clicked(const bool addFrete) {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  auto *pgt = new Pagamento(Pagamento::TipoPgt::Normal, this);
  pgt->posicao = pagamentos.size() + 1;
  pgt->setLayout(new QHBoxLayout);
  pgt->layout()->setContentsMargins(0, 0, 0, 0);

  pagamentos << pgt;

  //---------------------------------------------------

  labelPagamento(pgt);

  if (tipo == Tipo::Venda) {
    checkBoxRep(pgt);
    comboBoxPgtVenda(pgt);
  }

  if (tipo == Tipo::Compra) {
    comboBoxPgtCompra(pgt);
    comboBoxTipoData(pgt);
  }

  comboBoxParc(pgt);
  doubleSpinBoxPgt(pgt);
  dateEditPgt(pgt);
  lineEditPgt(pgt);

  ui->scrollArea->widget()->layout()->addWidget(pgt);

  //---------------------------------------------------

  calcularTotal();

  //---------------------------------------------------

  if (representacao and addFrete and pagamentos.size() == 1) { prepararPagamentosRep(); }
}

void WidgetPagamentos::on_pushButtonLimparPag_clicked() { resetarPagamentos(); }

void WidgetPagamentos::on_pushButtonPgtLoja_clicked() {
  if (pagamentos.isEmpty()) { throw RuntimeError("Preencha os pagamentos primeiro!", this); }

  LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

  if (dialog.exec() == QDialog::Rejected) { return; }

  for (auto *pgt : pagamentos) { pgt->checkBoxRep->setChecked(false); }
}

void WidgetPagamentos::on_pushButtonFreteLoja_clicked() {
  if (qFuzzyIsNull(frete)) { throw RuntimeError("Não há frete!", this); }

  resetarPagamentos();

  on_pushButtonAdicionarPagamento_clicked(false);
  on_pushButtonAdicionarPagamento_clicked(false);

  pagamentos.at(0)->checkBoxRep->setChecked(false);
  pagamentos.at(0)->observacao->setText("FRETE");
  pagamentos.at(0)->observacao->setReadOnly(true);
  pagamentos.at(0)->valorPgt->setValue(frete);
  pagamentos.at(0)->valorPgt->setReadOnly(true);
}

void WidgetPagamentos::verifyFields() {
  for (auto *pgt : pagamentos) {
    if (pgt->comboTipoPgt->currentText() == "ESCOLHA UMA OPÇÃO!") { throw RuntimeError("Por favor escolha a forma de pagamento " + QString::number(pgt->posicao) + "!"); }

    if (qFuzzyIsNull(pgt->valorPgt->value())) { throw RuntimeError("Pagamento " + QString::number(pgt->posicao) + " está com valor 0!"); }

    if (tipo == Tipo::Venda and pgt->observacao->text().isEmpty()) { throw RuntimeError("Faltou preencher observação do pagamento " + QString::number(pgt->posicao) + "!"); }
  }
}
