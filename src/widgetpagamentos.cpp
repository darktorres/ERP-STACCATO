#include <QDebug>
#include <QLineEdit>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "logindialog.h"
#include "ui_widgetpagamentos.h"
#include "usersession.h"
#include "widgetpagamentos.h"

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
  labelPagamento->setText("Pgt." + QString::number(ui->scrollArea->widget()->children().size()));
  layout->addWidget(labelPagamento);
}

void WidgetPagamentos::lineEditPgt(QHBoxLayout *layout) {
  auto *lineEditPgt = new QLineEdit(this);
  lineEditPgt->setPlaceholderText("Observação");
  connect(lineEditPgt, &QLineEdit::textChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(lineEditPgt);
  listLinePgt << lineEditPgt;
}

QDateEdit *WidgetPagamentos::dateEditPgt(QHBoxLayout *layout) {
  auto *dateEditPgt = new QDateEdit(this);
  dateEditPgt->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  dateEditPgt->setDisplayFormat("dd/MM/yy");
  dateEditPgt->setCalendarPopup(true);
  dateEditPgt->setDate(qApp->serverDate());
  connect(dateEditPgt, &QDateEdit::dateChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(dateEditPgt);
  listDatePgt << dateEditPgt;

  return dateEditPgt;
}

void WidgetPagamentos::on_doubleSpinBoxPgt_valueChanged(const int index) {
  if (listComboPgt.at(index)->currentText() == "Conta Cliente") { calculaCreditoRestante(); }

  calcularTotal();
}

void WidgetPagamentos::calculaCreditoRestante() {
  double creditoUsado = 0;

  for (int i = 0; i < listComboPgt.size(); ++i) {
    if (listComboPgt.at(i)->currentText() == "Conta Cliente") { creditoUsado += listDoubleSpinPgt.at(i)->value(); }
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
  connect(doubleSpinBoxPgt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [=] { on_doubleSpinBoxPgt_valueChanged(listDoubleSpinPgt.indexOf(doubleSpinBoxPgt)); });
  listDoubleSpinPgt << doubleSpinBoxPgt;
}

void WidgetPagamentos::comboBoxParc(QHBoxLayout *layout) {
  auto *comboboxPgtParc = new QComboBox(this);
  comboboxPgtParc->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  comboboxPgtParc->setMaximumWidth(45);
  connect(comboboxPgtParc, &QComboBox::currentTextChanged, this, &WidgetPagamentos::montarFluxoCaixa);
  layout->addWidget(comboboxPgtParc);
  listComboParc << comboboxPgtParc;
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
  listComboData << comboBoxData;

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
  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, [=] { on_comboBoxPgt_currentTextChanged(listComboPgt.indexOf(comboBoxPgt), comboBoxPgt->currentText()); });
  listComboPgt << comboBoxPgt;

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
  connect(comboBoxPgt, &QComboBox::currentTextChanged, this, [=] { on_comboBoxPgt_currentTextChanged(listComboPgt.indexOf(comboBoxPgt), comboBoxPgt->currentText()); });
  listComboPgt << comboBoxPgt;

  return true;
}

void WidgetPagamentos::on_comboBoxPgt_currentTextChanged(const int index, const QString &text) {
  if (text == "Escolha uma opção!") { return; }

  if (text == "Conta Cliente") {
    if (qFuzzyIsNull(ui->doubleSpinBoxCreditoDisponivel->value())) {
      listComboPgt.at(index)->setCurrentIndex(0);
      return qApp->enqueueError("Não há saldo cliente restante!", this);
    }

    double currentValue = listDoubleSpinPgt.at(index)->value();
    double newValue = qMin(currentValue, creditoRestante);
    calculaCreditoRestante();
    listDoubleSpinPgt.at(index)->setMaximum(newValue);
    listComboParc.at(index)->clear();
    listComboParc.at(index)->addItem("1x");
    emit montarFluxoCaixa();
    return;
  }

  listDoubleSpinPgt.at(index)->setMaximum(total);

  QSqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", listComboPgt.at(index)->currentText());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro lendo formas de pagamentos: " + query.lastError().text(), this); }

  const int parcelas = query.value("parcelas").toInt();

  listComboParc.at(index)->clear();

  // NOTE: this emits montarFluxoCaixa
  for (int i = 0; i < parcelas; ++i) { listComboParc.at(index)->addItem(QString::number(i + 1) + "x"); }

  listComboParc.at(index)->setEnabled(true);

  listDatePgt.at(index)->setEnabled(true);

  calculaCreditoRestante();

  emit montarFluxoCaixa();
}

void WidgetPagamentos::resetarPagamentos() {
  for (auto item : ui->scrollArea->widget()->children()) {
    if (qobject_cast<QFrame *>(item)) { delete item; }
  }

  listCheckBoxRep.clear();
  listComboData.clear();
  listComboParc.clear();
  listComboPgt.clear();
  listDatePgt.clear();
  listDoubleSpinPgt.clear();
  listLinePgt.clear();

  ui->doubleSpinBoxTotalPag->setValue(0);

  calculaCreditoRestante();

  emit montarFluxoCaixa();
}

double WidgetPagamentos::getTotalPag() { return ui->doubleSpinBoxTotalPag->value(); }

void WidgetPagamentos::prepararPagamentosRep() {
  if (qFuzzyIsNull(frete)) { return; }

  on_pushButtonAdicionarPagamento_clicked();

  listCheckBoxRep.at(0)->setChecked(true);
  listLinePgt.at(0)->setText("Frete");
  listLinePgt.at(0)->setReadOnly(true);
  listDoubleSpinPgt.at(0)->setValue(frete);
  listDoubleSpinPgt.at(0)->setReadOnly(true);
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

void WidgetPagamentos::setIdOrcamento(const QString &value) { idOrcamento = value; }

double WidgetPagamentos::getCredito() const { return credito; }

void WidgetPagamentos::calcularTotal() {
  const double sumWithoutLast = std::accumulate(listDoubleSpinPgt.cbegin(), listDoubleSpinPgt.cend() - 1, 0., [=](double accum, const QDoubleSpinBox *spinbox) { return accum += spinbox->value(); });

  auto lastSpinBox = listDoubleSpinPgt.last();

  lastSpinBox->blockSignals(true);
  lastSpinBox->setValue(total - sumWithoutLast);
  lastSpinBox->blockSignals(false);

  // ----------------------------------------

  const double sumAll = std::accumulate(listDoubleSpinPgt.cbegin(), listDoubleSpinPgt.cend(), 0., [=](double accum, const QDoubleSpinBox *spinbox) { return accum += spinbox->value(); });

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

  //---------------------------------------------------

  calcularTotal();

  //---------------------------------------------------

  if (representacao and addFrete and listDatePgt.size() == 1) { prepararPagamentosRep(); }
}

void WidgetPagamentos::on_pushButtonLimparPag_clicked() { resetarPagamentos(); }

void WidgetPagamentos::on_pushButtonPgtLoja_clicked() {
  if (listCheckBoxRep.isEmpty()) { return qApp->enqueueError("Preencha os pagamentos primeiro!", this); }

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
  listLinePgt.at(0)->setText("Frete");
  listLinePgt.at(0)->setReadOnly(true);
  listDoubleSpinPgt.at(0)->setValue(frete);
  listDoubleSpinPgt.at(0)->setReadOnly(true);
}

bool WidgetPagamentos::verifyFields() {
  for (int i = 0; i < listCheckBoxRep.size(); ++i) {
    if (listComboPgt.at(i)->currentText() != "Escolha uma opção!" and listLinePgt.at(i)->text().isEmpty()) {
      qApp->enqueueError("Faltou preencher observação do pagamento " + QString::number(i + 1) + "!", this);
      listLinePgt.at(i)->setFocus();
      return false;
    }

    if (listDoubleSpinPgt.at(i)->value() > 0 and listComboPgt.at(i)->currentText() == "Escolha uma opção!") {
      qApp->enqueueError("Por favor escolha a forma de pagamento " + QString::number(i + 1) + "!", this);
      listComboPgt.at(i)->setFocus();
      return false;
    }

    if (qFuzzyIsNull(listDoubleSpinPgt.at(i)->value()) and listComboPgt.at(i)->currentText() != "Escolha uma opção!") {
      qApp->enqueueError("Pagamento " + QString::number(i + 1) + " está com valor 0!", this);
      listDoubleSpinPgt.at(i)->setFocus();
      return false;
    }
  }

  return true;
}
