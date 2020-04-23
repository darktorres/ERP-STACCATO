#include "widgetpagamentos.h"
#include "ui_widgetpagamentos.h"

#include "application.h"
#include "logindialog.h"
#include "usersession.h"

#include <QDebug>
#include <QLineEdit>
#include <QSqlError>
#include <QSqlQuery>

WidgetPagamentos::WidgetPagamentos(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetPagamentos) {
  ui->setupUi(this);

  //---------------------------------------------------

  ui->scrollArea->setWidgetResizable(true);

  auto *frame = new QFrame(this);
  auto *scrollLayout = new QVBoxLayout(frame);
  scrollLayout->setSizeConstraint(QLayout::SetMinimumSize);
  ui->scrollArea->setWidget(frame);

  //---------------------------------------------------

  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, [=] { on_pushButtonAdicionarPagamento_clicked(); });
  connect(ui->pushButtonLimparPag, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonLimparPag_clicked);
  connect(ui->pushButtonPgtLoja, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonPgtLoja_clicked);
  connect(ui->pushButtonFreteLoja, &QPushButton::clicked, this, &WidgetPagamentos::on_pushButtonFreteLoja_clicked);
}

WidgetPagamentos::~WidgetPagamentos() { delete ui; }

void WidgetPagamentos::labelPagamento(QHBoxLayout *layout) {
  auto *labelPagamento = new QLabel(this);
  labelPagamento->setText("Pgt." + QString::number(pagamentos + 1));
  layout->addWidget(labelPagamento);
}

void WidgetPagamentos::lineEditPgt(QHBoxLayout *layout) {
  auto *lineEditPgt = new QLineEdit(this);
  lineEditPgt->setPlaceholderText("Observação");
  connect(lineEditPgt, &QLineEdit::textChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(lineEditPgt);
  listObservacao << lineEditPgt;
}

QDateEdit *WidgetPagamentos::dateEditPgt(QHBoxLayout *layout) {
  auto *dateEditPgt = new QDateEdit(this);
  dateEditPgt->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  dateEditPgt->setDisplayFormat("dd/MM/yy");
  dateEditPgt->setCalendarPopup(true);
  dateEditPgt->setDate(qApp->serverDate());
  connect(dateEditPgt, &QDateEdit::dateChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(dateEditPgt);
  listDataPgt << dateEditPgt;

  return dateEditPgt;
}

void WidgetPagamentos::on_doubleSpinBoxPgt_valueChanged(const int index) {
  if (listTipoPgt.at(index)->currentText() == "Conta Cliente") { calculaCreditoRestante(); }

  calcularTotal();
}

void WidgetPagamentos::calculaCreditoRestante() {
  double creditoUsado = 0;

  for (int i = 0; i < pagamentos; ++i) {
    if (listTipoPgt.at(i)->currentText() == "Conta Cliente") { creditoUsado += listValorPgt.at(i)->value(); }
  }

  creditoRestante = credito - creditoUsado;
  ui->doubleSpinBoxCreditoDisponivel->setValue(creditoRestante);
}

void WidgetPagamentos::doubleSpinBoxPgt(QHBoxLayout *layout) {
  const double restante = total - ui->doubleSpinBoxTotalPag->value();

  auto *doubleSpinBoxPgt = new QDoubleSpinBox(this);
  doubleSpinBoxPgt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  doubleSpinBoxPgt->setMinimumWidth(80);
  doubleSpinBoxPgt->setPrefix("R$ ");
  doubleSpinBoxPgt->setMaximum(total);
  doubleSpinBoxPgt->setValue(restante);
  doubleSpinBoxPgt->setGroupSeparatorShown(true);
  layout->addWidget(doubleSpinBoxPgt);
  connect(doubleSpinBoxPgt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [=] { on_doubleSpinBoxPgt_valueChanged(listValorPgt.indexOf(doubleSpinBoxPgt)); });
  listValorPgt << doubleSpinBoxPgt;
}

void WidgetPagamentos::comboBoxParc(QHBoxLayout *layout) {
  auto *comboboxPgtParc = new QComboBox(this);
  comboboxPgtParc->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  comboboxPgtParc->setMaximumWidth(45);
  connect(comboboxPgtParc, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(comboboxPgtParc);
  listParcela << comboboxPgtParc;
}

QComboBox *WidgetPagamentos::comboBoxData(QHBoxLayout *layout) {
  auto *comboBoxData = new QComboBox(this);
  comboBoxData->insertItem(0, "Data Mês");
  comboBoxData->insertItem(1, "Data + 1 Mês");
  comboBoxData->insertItem(2, "14");
  comboBoxData->insertItem(3, "20");
  comboBoxData->insertItem(4, "28");
  comboBoxData->insertItem(5, "30");
  layout->addWidget(comboBoxData);
  connect(comboBoxData, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  listTipoData << comboBoxData;

  // connect this comboBoxData to the equivalent dateEditPgt

  // listComboData with 0 elements: size 0
  // listComboData with 1 elements: size 1; first element at [0]

  //  const auto dateEditPair = listDatePgt.at(listComboData.size());

  //  connect(comboBoxData, &QComboBox::currentTextChanged, listDatePgt.at(listComboData.size()), );

  // TODO: quando alterar dataPgt para 14/20/28/30 alterar a data no dateEdit e não apenas no fluxo

  return comboBoxData;
}

bool WidgetPagamentos::comboBoxPgtCompra(QHBoxLayout *layout) {
  auto *comboBoxPgt = new QComboBox(this);
  comboBoxPgt->setMinimumWidth(140);

  QSqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE idLoja = :idLoja");
  queryPag.bindValue(":idLoja", UserSession::idLoja());

  if (not queryPag.exec()) { return qApp->enqueueError(false, "Erro lendo formas de pagamentos: " + queryPag.lastError().text(), this); }

  const QStringList list([&queryPag]() {
    QStringList temp("Escolha uma opção!");
    while (queryPag.next()) { temp << queryPag.value("pagamento").toString(); }
    return temp;
  }());

  comboBoxPgt->insertItems(0, list);
  layout->addWidget(comboBoxPgt);
  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, [=] { on_comboBoxPgt_currentTextChanged(listTipoPgt.indexOf(comboBoxPgt), comboBoxPgt->currentText()); });
  listTipoPgt << comboBoxPgt;

  return true;
}

void WidgetPagamentos::checkBoxRep(QFrame *frame, QHBoxLayout *layout) {
  auto *checkboxRep = new QCheckBox(frame);
  checkboxRep->setText("Fornecedor");
  checkboxRep->setVisible(representacao);
  checkboxRep->setChecked(representacao);
  checkboxRep->setEnabled(false);
  connect(checkboxRep, &QCheckBox::stateChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(checkboxRep);
  listCheckBoxRep << checkboxRep;
}

bool WidgetPagamentos::comboBoxPgtVenda(QFrame *frame, QHBoxLayout *layout) {
  auto *comboBoxPgt = new QComboBox(frame);
  comboBoxPgt->setMinimumWidth(140);

  if (idOrcamento.isEmpty()) { return qApp->enqueueError(false, "Orçamento vazio!", this); }

  QSqlQuery queryOrc;
  queryOrc.prepare("SELECT idUsuario, idOrcamento, idLoja, idUsuarioConsultor, idCliente, idEnderecoEntrega, idEnderecoFaturamento, idProfissional, data, subTotalBru, subTotalLiq, frete, "
                   "freteManual, descontoPorc, descontoReais, total, status, observacao, prazoEntrega, representacao FROM orcamento WHERE idOrcamento = :idOrcamento");
  queryOrc.bindValue(":idOrcamento", idOrcamento);

  if (not queryOrc.exec() or not queryOrc.first()) { return qApp->enqueueError(false, "Erro buscando orçamento: " + queryOrc.lastError().text(), this); }

  QSqlQuery queryPag;
  queryPag.prepare("SELECT pagamento FROM view_pagamento_loja WHERE idLoja = :idLoja");
  queryPag.bindValue(":idLoja", queryOrc.value("idLoja"));

  if (not queryPag.exec()) { return qApp->enqueueError(false, "Erro lendo formas de pagamentos: " + queryPag.lastError().text(), this); }

  const QStringList list([&queryPag]() {
    QStringList temp("Escolha uma opção!");
    while (queryPag.next()) { temp << queryPag.value("pagamento").toString(); }
    return temp;
  }());

  comboBoxPgt->insertItems(0, list);
  if (credito > 0) { comboBoxPgt->addItem("Conta Cliente"); }
  layout->addWidget(comboBoxPgt);
  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, [=] { on_comboBoxPgt_currentTextChanged(listTipoPgt.indexOf(comboBoxPgt), comboBoxPgt->currentText()); });
  listTipoPgt << comboBoxPgt;

  return true;
}

void WidgetPagamentos::on_comboBoxPgt_currentTextChanged(const int index, const QString &text) {
  if (text == "Escolha uma opção!") { return; }

  if (text == "Conta Cliente") {
    if (qFuzzyIsNull(ui->doubleSpinBoxCreditoDisponivel->value())) {
      listTipoPgt.at(index)->setCurrentIndex(0);
      return qApp->enqueueError("Não há saldo cliente restante!", this);
    }

    double currentValue = listValorPgt.at(index)->value();
    double newValue = qMin(currentValue, creditoRestante);
    calculaCreditoRestante();
    listValorPgt.at(index)->setMaximum(newValue);
    listParcela.at(index)->clear();
    listParcela.at(index)->addItem("1x");
    emit montarFluxoCaixa();
    return;
  }

  listValorPgt.at(index)->setMaximum(total);

  QSqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", listTipoPgt.at(index)->currentText());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro lendo formas de pagamentos: " + query.lastError().text(), this); }

  const int parcelas = query.value("parcelas").toInt();

  listParcela.at(index)->clear();

  // NOTE: this emits montarFluxoCaixa
  for (int i = 0; i < parcelas; ++i) { listParcela.at(index)->addItem(QString::number(i + 1) + "x"); }

  listParcela.at(index)->setEnabled(true);

  listDataPgt.at(index)->setEnabled(true);

  calculaCreditoRestante();

  emit montarFluxoCaixa();
}

void WidgetPagamentos::resetarPagamentos() {
  for (auto item : ui->scrollArea->widget()->children()) {
    if (qobject_cast<QFrame *>(item)) { delete item; }
  }

  listCheckBoxRep.clear();
  listTipoData.clear();
  listParcela.clear();
  listTipoPgt.clear();
  listDataPgt.clear();
  listValorPgt.clear();
  listObservacao.clear();

  pagamentos = 0;

  ui->doubleSpinBoxTotalPag->setValue(0);

  calculaCreditoRestante();

  emit montarFluxoCaixa();
}

double WidgetPagamentos::getTotalPag() { return ui->doubleSpinBoxTotalPag->value(); }

void WidgetPagamentos::prepararPagamentosRep() {
  if (qFuzzyIsNull(frete)) { return; }

  on_pushButtonAdicionarPagamento_clicked();

  listCheckBoxRep.at(0)->setChecked(not fretePagoLoja);
  listObservacao.at(0)->setText("Frete");
  listObservacao.at(0)->setReadOnly(true);
  listValorPgt.at(0)->setValue(frete);
  listValorPgt.at(0)->setReadOnly(true);
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

void WidgetPagamentos::setTipo(const Tipo &novoTipo) {
  if (novoTipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  tipo = novoTipo;

  if (tipo == Tipo::Compra) {
    ui->label_4->hide();
    ui->doubleSpinBoxCreditoTotal->hide();
    ui->pushButtonFreteLoja->hide();
    ui->pushButtonPgtLoja->hide();
  }
}

void WidgetPagamentos::setTotal(double value) { total = value; }

void WidgetPagamentos::setFrete(double value) { frete = value; }

void WidgetPagamentos::setFretePagoLoja() {
  fretePagoLoja = true;

  ui->pushButtonFreteLoja->setDisabled(true);
}

void WidgetPagamentos::setIdOrcamento(const QString &value) { idOrcamento = value; }

double WidgetPagamentos::getCredito() const { return credito; }

void WidgetPagamentos::calcularTotal() {
  const double sumWithoutLast = std::accumulate(listValorPgt.cbegin(), listValorPgt.cend() - 1, 0., [=](double accum, const QDoubleSpinBox *spinbox) { return accum += spinbox->value(); });

  auto lastSpinBox = listValorPgt.last();

  lastSpinBox->blockSignals(true);
  lastSpinBox->setValue(total - sumWithoutLast);
  lastSpinBox->blockSignals(false);

  // ----------------------------------------

  const double sumAll = std::accumulate(listValorPgt.cbegin(), listValorPgt.cend(), 0., [=](double accum, const QDoubleSpinBox *spinbox) { return accum += spinbox->value(); });

  ui->doubleSpinBoxTotalPag->setValue(sumAll);

  // ----------------------------------------

  emit montarFluxoCaixa();
}

void WidgetPagamentos::on_pushButtonAdicionarPagamento_clicked(const bool addFrete) {
  if (tipo == Tipo::Nulo) { return qApp->enqueueError("Erro Tipo::Nulo!", this); }

  auto *frame = new QFrame(this);
  auto *layout = new QHBoxLayout(frame);
  frame->setLayout(layout);

  labelPagamento(layout);

  QComboBox *comboBox = nullptr;

  if (tipo == Tipo::Venda) {
    checkBoxRep(frame, layout);
    if (not comboBoxPgtVenda(frame, layout)) { return; }
  }

  if (tipo == Tipo::Compra) {
    if (not comboBoxPgtCompra(layout)) { return; }
    comboBox = comboBoxData(layout);
  }

  comboBoxParc(layout);
  doubleSpinBoxPgt(layout);
  auto dateEdit = dateEditPgt(layout);
  lineEditPgt(layout);

  if (tipo == Tipo::Compra) {
    connect(comboBox, &QComboBox::currentTextChanged, [=] {
      const QString currentText = comboBox->currentText();
      const QDate currentDate = qApp->serverDate();
      QDate dataPgt;

      if (currentText == "Data + 1 Mês") {
        dataPgt = currentDate.addMonths(1);
      } else if (currentText == "Data Mês") {
        dataPgt = currentDate;
      } else {
        dataPgt = currentDate.addDays(currentText.toInt());
      }

      dateEdit->setDate(dataPgt);
    });
  }

  ui->scrollArea->widget()->layout()->addWidget(frame);

  pagamentos += 1;

  //---------------------------------------------------

  calcularTotal();

  //---------------------------------------------------

  if (representacao and addFrete and pagamentos == 1) { prepararPagamentosRep(); }
}

void WidgetPagamentos::on_pushButtonLimparPag_clicked() { resetarPagamentos(); }

void WidgetPagamentos::on_pushButtonPgtLoja_clicked() {
  if (pagamentos == 0) { return qApp->enqueueError("Preencha os pagamentos primeiro!", this); }

  LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

  if (dialog.exec() == QDialog::Rejected) { return; }

  for (auto item : std::as_const(listCheckBoxRep)) { item->setChecked(false); }
}

void WidgetPagamentos::on_pushButtonFreteLoja_clicked() {
  if (qFuzzyIsNull(frete)) { return qApp->enqueueError("Não há frete!", this); }

  resetarPagamentos();

  on_pushButtonAdicionarPagamento_clicked(false);
  on_pushButtonAdicionarPagamento_clicked(false);

  listCheckBoxRep.at(0)->setChecked(false);
  listObservacao.at(0)->setText("Frete");
  listObservacao.at(0)->setReadOnly(true);
  listValorPgt.at(0)->setValue(frete);
  listValorPgt.at(0)->setReadOnly(true);
}

bool WidgetPagamentos::verifyFields() {
  for (int i = 0; i < pagamentos; ++i) {
    if (listTipoPgt.at(i)->currentText() == "Escolha uma opção!") {
      qApp->enqueueError("Por favor escolha a forma de pagamento " + QString::number(i + 1) + "!", this);
      listTipoPgt.at(i)->setFocus();
      return false;
    }

    if (qFuzzyIsNull(listValorPgt.at(i)->value())) {
      qApp->enqueueError("Pagamento " + QString::number(i + 1) + " está com valor 0!", this);
      listValorPgt.at(i)->setFocus();
      return false;
    }

    if (tipo == Tipo::Venda and listObservacao.at(i)->text().isEmpty()) {
      qApp->enqueueError("Faltou preencher observação do pagamento " + QString::number(i + 1) + "!", this);
      listObservacao.at(i)->setFocus();
      return false;
    }
  }

  return true;
}
