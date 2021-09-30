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

  //  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonAdicionarPagamento_clicked, connectionType);
  connect(
      ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, [=] { on_pushButtonAdicionarPagamento_clicked(Pagamento::TipoPgt::Normal); }, connectionType);
  connect(ui->pushButtonLimparPag, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonLimparPag_clicked, connectionType);
  connect(ui->pushButtonPgtLoja, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonPgtLoja_clicked, connectionType);
  connect(ui->pushButtonFreteLoja, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonFreteLoja_clicked, connectionType);
}

void WidgetPagamentos::labelPagamento(Pagamento *pgt) {
  QString label;

  if (pgt->tipoPgt == Pagamento::TipoPgt::Normal) { label = "Pgt. " + QString::number(pagamentos.size() + 1); }
  if (pgt->tipoPgt == Pagamento::TipoPgt::Frete) { label = "FRETE"; }
  if (pgt->tipoPgt == Pagamento::TipoPgt::ST) { label = "ST"; }

  auto *labelPagamento = new QLabel(this);
  labelPagamento->setText(label);

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
  //  doubleSpinBoxPgt->setMinimum(0);
  //  doubleSpinBoxPgt->setMaximum(total);
  doubleSpinBoxPgt->setValue(restante);
  doubleSpinBoxPgt->setGroupSeparatorShown(true);

  if (pgt->tipoPgt == Pagamento::TipoPgt::Frete or pgt->tipoPgt == Pagamento::TipoPgt::ST) { doubleSpinBoxPgt->setButtonSymbols(QDoubleSpinBox::NoButtons); }

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

  QStringList list{"DATA MÊS", "DATA + 1 MÊS", "14", "20", "28", "30", "60", "90", "120", "150", "180", "210"};

  comboBoxTipoData->addItems(list);

  connect(comboBoxTipoData, &QComboBox::currentTextChanged, this, &WidgetPagamentos::on_comboBoxTipoData_currentTextChanged);

  pgt->layout()->addWidget(comboBoxTipoData);
  pgt->comboTipoData = comboBoxTipoData;
}

void WidgetPagamentos::comboBoxPgtCompra(Pagamento *pgt) {
  auto *comboBoxPgt = new QComboBox(pgt);
  comboBoxPgt->setMinimumWidth(140);
  buscarTiposPgtsCompra(comboBoxPgt);

  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, &WidgetPagamentos::on_comboBoxPgt_currentTextChanged);

  pgt->layout()->addWidget(comboBoxPgt);
  pgt->comboTipoPgt = comboBoxPgt;
}

void WidgetPagamentos::checkBoxRep(Pagamento *pgt) {
  auto *checkboxRep = new QCheckBox(pgt);
  //  checkboxRep->setText("Fornecedor");
  checkboxRep->setText("Pago p/ forn.");
  //  checkboxRep->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
  checkboxRep->setVisible(representacao); // why this?
  checkboxRep->setChecked(representacao);
  checkboxRep->setEnabled(false);

  connect(checkboxRep, &QCheckBox::stateChanged, this, &WidgetPagamentos::montarFluxoCaixa);

  pgt->layout()->addWidget(checkboxRep);
  pgt->checkBoxRep = checkboxRep;
}

void WidgetPagamentos::comboBoxPgtVenda(Pagamento *pgt) {
  auto *comboBoxPgt = new QComboBox(pgt);
  comboBoxPgt->setMinimumWidth(140);
  buscarTiposPgtsVenda(comboBoxPgt);

  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, &WidgetPagamentos::on_comboBoxPgt_currentTextChanged);

  pgt->layout()->addWidget(comboBoxPgt);
  pgt->comboTipoPgt = comboBoxPgt;
}

void WidgetPagamentos::on_comboBoxTipoData_currentTextChanged(const QString &text) {
  auto *pgt = qobject_cast<Pagamento *>(QObject::sender());

  if (not pgt) { throw RuntimeException("Pgt Null!"); }

  const QDate currentDate = qApp->serverDate();

  QDate dataPgt;

  if (text == "DATA + 1 MÊS") {
    dataPgt = currentDate.addMonths(1);
  } else if (text == "DATA MÊS") {
    dataPgt = currentDate;
  } else {
    dataPgt = currentDate.addDays(text.toInt());
  }

  pgt->dataPgt->setDate(dataPgt);

  emit montarFluxoCaixa();
}

void WidgetPagamentos::on_comboBoxPgt_currentTextChanged(const QString &text) {
  auto *pgt = qobject_cast<Pagamento *>(QObject::sender());

  if (not pgt) { throw RuntimeException("Pgt Null!"); }

  if (text == "ESCOLHA UMA OPÇÃO!") { return; }

  if (text == "CONTA CLIENTE") {
    if (qFuzzyIsNull(ui->doubleSpinBoxCreditoDisponivel->value())) {
      pgt->comboTipoPgt->setCurrentIndex(0);
      throw RuntimeError("Não há saldo cliente restante!", this);
    }

    //    double currentValue = pgt->valorPgt->value();
    //    double newValue = qMin(currentValue, creditoRestante);
    calculaCreditoRestante();
    //    pgt->valorPgt->setMaximum(newValue);
    pgt->comboParcela->clear();
    pgt->comboParcela->addItem("1x");
    emit montarFluxoCaixa();
    return;
  }

  //  pgt->valorPgt->setMaximum(total); // TODO: it looks like this is not needed anymore since setTotal() update all widgets

  SqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", pgt->comboTipoPgt->currentText());

  if (not query.exec()) { throw RuntimeException("Erro lendo formas de pagamentos: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrou dados do pagamento: " + pgt->comboTipoPgt->currentText()); }

  const int parcelas = query.value("parcelas").toInt();

  { // NOTE: avoid sending signal before widgets are all set
    const QSignalBlocker blocker(pgt->comboParcela);

    pgt->comboParcela->clear();

    for (int i = 0; i < parcelas; ++i) { pgt->comboParcela->addItem(QString::number(i + 1) + "x"); }
  }

  pgt->comboParcela->setEnabled(true);
  pgt->dataPgt->setEnabled(true);

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

  calcularTotal();

  emit montarFluxoCaixa();
}

double WidgetPagamentos::getTotalPag() { return ui->doubleSpinBoxTotalPag->value(); }

// void WidgetPagamentos::prepararPagamentosRep() {
//  if (qFuzzyIsNull(frete)) { return; }

//  on_pushButtonAdicionarPagamento_clicked();

//  pagamentos.at(0)->checkBoxRep->setChecked(not fretePagoLoja);
//  pagamentos.at(0)->observacao->setText("FRETE");
//  pagamentos.at(0)->observacao->setReadOnly(true);
//  pagamentos.at(0)->valorPgt->setValue(frete);
//  pagamentos.at(0)->valorPgt->setReadOnly(true);
//}

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

void WidgetPagamentos::setTotal(const double newTotal) {
  total = newTotal;

  if (tipo == Tipo::Venda and representacao) { total -= frete; }

  calcularTotal();
  //  for (auto *pgt : pagamentos) { pgt->valorPgt->setMaximum(total); }
}

void WidgetPagamentos::setFrete(const double newFrete) {
  frete = newFrete;

  if ((tipo == Tipo::Venda and representacao) or tipo == Tipo::Compra) {
    if (qFuzzyIsNull(frete)) {
      delete pgtFrete;
      pgtFrete = nullptr;
      calcularTotal();
      return;
    }

    if (not pgtFrete) {
      on_pushButtonAdicionarPagamento_clicked(Pagamento::TipoPgt::Frete);

      QSignalBlocker blocker1(pgtFrete->observacao);

      if (fretePagoLoja) { pgtFrete->checkBoxRep->setChecked(false); }

      pgtFrete->observacao->setText("FRETE");
      pgtFrete->observacao->setReadOnly(true);
      pgtFrete->valorPgt->setReadOnly(true);
    }

    QSignalBlocker blocker2(pgtFrete->valorPgt);

    pgtFrete->valorPgt->setMaximum(frete);
    pgtFrete->valorPgt->setMinimum(frete);
    pgtFrete->valorPgt->setValue(frete);

    calcularTotal();
  }
}

void WidgetPagamentos::setST(const double newSt) {
  qDebug() << "";

  st = newSt;

  if (qFuzzyIsNull(st)) {
    delete pgtSt;
    pgtSt = nullptr;
    calcularTotal();
    return;
  }

  if (not pgtSt) {
    on_pushButtonAdicionarPagamento_clicked(Pagamento::TipoPgt::ST);

    QSignalBlocker blocker1(pgtSt->observacao);

    pgtSt->observacao->setText("ST");
    pgtSt->observacao->setReadOnly(true);
    pgtSt->valorPgt->setReadOnly(true);
  }

  QSignalBlocker blocker2(pgtSt->valorPgt);

  pgtSt->valorPgt->setMaximum(st);
  pgtSt->valorPgt->setMinimum(st);
  pgtSt->valorPgt->setValue(st);

  calcularTotal();
}

void WidgetPagamentos::setFretePagoLoja() {
  fretePagoLoja = true;

  if (pgtFrete) { pgtFrete->checkBoxRep->setChecked(not fretePagoLoja); }

  //  ui->pushButtonFreteLoja->setDisabled(true);
}

void WidgetPagamentos::setIdOrcamento(const QString &newIdOrcamento) { idOrcamento = newIdOrcamento; }

double WidgetPagamentos::getCredito() const { return credito; }

void WidgetPagamentos::calcularRestante() {
  if (pagamentos.empty()) { return; }

  double sumWithoutLast = std::accumulate(pagamentos.cbegin(), pagamentos.cend(), 0., [=](double accum, const Pagamento *pgt) { return accum += pgt->valorPgt->value(); });
  sumWithoutLast -= pagamentos.last()->valorPgt->value();

  for (auto *pgt : qAsConst(pagamentos)) {
    pgt->valorPgt->setMaximum(total);
    pgt->valorPgt->setMinimum(0);
  }

  auto *lastSpinBox = pagamentos.constLast()->valorPgt;
  lastSpinBox->blockSignals(true);
  lastSpinBox->setMaximum(total - sumWithoutLast);
  lastSpinBox->setMinimum(total - sumWithoutLast);
  lastSpinBox->setValue(total - sumWithoutLast);
  lastSpinBox->blockSignals(false);
}

void WidgetPagamentos::calcularTotal() {
  calcularRestante();

  // ----------------------------------------

  const double sumAll = std::accumulate(pagamentos.cbegin(), pagamentos.cend(), 0., [=](double accum, const Pagamento *pgt) { return accum += pgt->valorPgt->value(); });

  if (tipo == Tipo::Venda) {
    double total = sumAll;

    if (representacao) { total += frete; }

    ui->doubleSpinBoxTotalPag->setValue(total);
  }

  if (tipo == Tipo::Compra) { ui->doubleSpinBoxTotalPag->setValue(sumAll + frete + st); }

  // ----------------------------------------

  emit montarFluxoCaixa();
}

void WidgetPagamentos::insertPgtInScrollArea(Pagamento *pgt) {
  int index = -1;

  if (pgt->tipoPgt == Pagamento::TipoPgt::Normal) { index = -1; }                                         // insert at end
  if (pgt->tipoPgt == Pagamento::TipoPgt::Frete or pgt->tipoPgt == Pagamento::TipoPgt::ST) { index = 0; } // insert at beginning

  auto *scrollLayout = qobject_cast<QVBoxLayout *>(ui->scrollArea->widget()->layout());
  scrollLayout->insertWidget(index, pgt);

  // TODO: insert spacing at end of layout
}

void WidgetPagamentos::on_pushButtonAdicionarPagamento_clicked(const Pagamento::TipoPgt tipoPgt) {
  if (tipo == Tipo::Nulo) { throw RuntimeException("Erro Tipo::Nulo!", this); }

  auto *pgt = new Pagamento(tipoPgt, this);
  pgt->posicao = pagamentos.size() + 1;
  pgt->setLayout(new QHBoxLayout);
  pgt->layout()->setContentsMargins(0, 0, 0, 0);

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

  if (tipoPgt == Pagamento::TipoPgt::Normal) { pagamentos << pgt; }
  if (tipoPgt == Pagamento::TipoPgt::Frete) { pgtFrete = pgt; }
  if (tipoPgt == Pagamento::TipoPgt::ST) { pgtSt = pgt; }

  insertPgtInScrollArea(pgt);

  //---------------------------------------------------

  calcularTotal();
}

void WidgetPagamentos::on_pushButtonDelete_clicked() {
  //  auto *sender = qobject_cast<QPushButton *>(QObject::sender());

  //  QString name = sender->objectName().remove("pushButtonDelete");
  //  int index = name.toInt() - 1;

  //  auto frames = findChildren<QFrame *>("frame" + name);

  //  qDebug() << "name: " << name;
  //  qDebug() << "index: " << index;
  //  qDebug() << "frames: " << frames;

  //  qDebug() << "1listValor: " << listValorPgt;
  //  qDebug() << "1listValor: " << listValorPgt.at(0)->value();

  //  listCheckBoxRep.removeAt(index);
  //  listTipoPgt.removeAt(index);
  //  listTipoData.removeAt(index);
  //  listParcela.removeAt(index);
  //  listValorPgt.removeAt(index);
  //  listDataPgt.removeAt(index);
  //  listObservacao.removeAt(index);

  //  frames.at(0)->deleteLater();

  //  qtdPagamentos -= 1;
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

  pgtFrete->checkBoxRep->setChecked(false);
}

void WidgetPagamentos::verifyFields() {
  if (pgtFrete) {
    if (pgtFrete->comboTipoPgt->currentText() == "ESCOLHA UMA OPÇÃO!") { throw RuntimeError("Por favor escolha a forma de pagamento do frete!"); }
    if (qFuzzyIsNull(pgtFrete->valorPgt->value())) { throw RuntimeError("Pagamento do frete está com valor 0!"); }
    if (pgtFrete->valorPgt->value() < 0) { throw RuntimeError("Pagamento do frete está com valor negativo!"); }
  }

  if (pgtSt) {
    if (pgtSt->comboTipoPgt->currentText() == "ESCOLHA UMA OPÇÃO!") { throw RuntimeError("Por favor escolha a forma de pagamento da ST!"); }
    if (qFuzzyIsNull(pgtSt->valorPgt->value())) { throw RuntimeError("Pagamento da ST está com valor 0!"); }
    if (pgtSt->valorPgt->value() < 0) { throw RuntimeError("Pagamento da ST´ está com valor negativo!"); }
  }

  // TODO: trocar por um foreach e usar pgt.posicao no lugar de i
  for (int i = 0; i < pagamentos.size(); ++i) {
    if (pagamentos.at(i)->comboTipoPgt->currentText() == "ESCOLHA UMA OPÇÃO!") { throw RuntimeError("Por favor escolha a forma de pagamento " + QString::number(i + 1) + "!"); }
    if (qFuzzyIsNull(pagamentos.at(i)->valorPgt->value())) { throw RuntimeError("Pagamento " + QString::number(i + 1) + " está com valor 0!"); }
    if (pagamentos.at(i)->valorPgt->value() < 0) { throw RuntimeError("Pagamento " + QString::number(i + 1) + " está com valor negativo!"); }
    if (tipo == Tipo::Venda and pagamentos.at(i)->observacao->text().isEmpty()) { throw RuntimeError("Faltou preencher observação do pagamento " + QString::number(i + 1) + "!"); }
  }
}

void WidgetPagamentos::buscarTiposPgtsCompra(QComboBox *comboBoxPgt) {
  SqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE idLoja = :idLoja");
  queryPag.bindValue(":idLoja", User::idLoja);

  if (not queryPag.exec()) { throw RuntimeException("Erro lendo formas de pagamentos: " + queryPag.lastError().text()); }

  QStringList list;
  list << "ESCOLHA UMA OPÇÃO!";

  while (queryPag.next()) { list << queryPag.value("pagamento").toString(); }

  comboBoxPgt->insertItems(0, list);
}

void WidgetPagamentos::buscarTiposPgtsVenda(QComboBox *comboBoxPgt) {
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
}

// TODO: colocar um botão para apagar pagamento em vez de limpar pagamentos e fazer tudo do zero
// TODO: ao alterar total recalcular o valor que sobra e colocar no ultimo pagamento
// TODO: transformar 'frete loja'/'pgt total loja' em checkbox
