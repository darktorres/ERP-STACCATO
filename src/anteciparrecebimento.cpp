#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

#include "anteciparrecebimento.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"
#include "ui_anteciparrecebimento.h"

AnteciparRecebimento::AnteciparRecebimento(QWidget *parent) : Dialog(parent), ui(new Ui::AnteciparRecebimento) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->itemBoxConta->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxConta->setValue(3);

  ui->comboBox->clear();
  ui->comboBox->addItem("");

  ui->comboBoxLoja->clear();
  ui->comboBoxLoja->addItem("");
  ui->comboBoxLoja->addItem("ALPH");
  ui->comboBoxLoja->addItem("GABR");
  ui->comboBoxLoja->addItem("GRAN");

  QSqlQuery query;

  if (not query.exec("SELECT DISTINCT pagamento AS tipo FROM view_pagamento_loja")) emit errorSignal("Erro comunicando com banco de dados: " + query.lastError().text());

  while (query.next()) ui->comboBox->addItem(query.value("tipo").toString());

  ui->dateEditEvento->setDate(QDate::currentDate());

  setConnections();
}

AnteciparRecebimento::~AnteciparRecebimento() { delete ui; }

void AnteciparRecebimento::setConnections() {
  connect(ui->checkBoxIOF, &QCheckBox::clicked, this, &AnteciparRecebimento::calcularTotais);
  connect(ui->comboBox, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged);
  connect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged);
  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &AnteciparRecebimento::calcularTotais);
  connect(ui->doubleSpinBoxDescMes, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::calcularTotais);
  connect(ui->doubleSpinBoxValorPresente, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::on_doubleSpinBoxValorPresente_valueChanged);
  connect(ui->pushButtonGerar, &QPushButton::clicked, this, &AnteciparRecebimento::on_pushButtonGerar_clicked);
  connect(ui->table, &TableView::entered, this, &AnteciparRecebimento::on_table_entered);
  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AnteciparRecebimento::calcularTotais);
}

void AnteciparRecebimento::unsetConnections() {
  disconnect(ui->checkBoxIOF, &QCheckBox::clicked, this, &AnteciparRecebimento::calcularTotais);
  disconnect(ui->comboBox, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged);
  disconnect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged);
  disconnect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &AnteciparRecebimento::calcularTotais);
  disconnect(ui->doubleSpinBoxDescMes, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::calcularTotais);
  disconnect(ui->doubleSpinBoxValorPresente, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::on_doubleSpinBoxValorPresente_valueChanged);
  disconnect(ui->pushButtonGerar, &QPushButton::clicked, this, &AnteciparRecebimento::on_pushButtonGerar_clicked);
  disconnect(ui->table, &TableView::entered, this, &AnteciparRecebimento::on_table_entered);
  disconnect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AnteciparRecebimento::calcularTotais);
}

void AnteciparRecebimento::calcularTotais() {
  // TODO: 1ao selecionar parcelas de cartao somar a taxa tambem
  const auto list = ui->table->selectionModel()->selectedRows();

  double bruto = 0;
  double liquido = 0;
  double prazoMedio = 0;

  for (const auto &item : list) {
    const QString tipo = modelContaReceber.data(item.row(), "tipo").toString();
    const double valor = modelContaReceber.data(item.row(), "valor").toDouble();
    const QDate dataPagamento = modelContaReceber.data(item.row(), "dataPagamento").toDate();

    if (not tipo.contains("Taxa Cartão")) bruto += valor;

    liquido += valor;

    // valor aqui deve considerar a taxa cartao
    const double prazo = ui->dateEditEvento->date().daysTo(dataPagamento) * valor;

    prazoMedio += prazo;
  }

  // be careful about division by 0
  prazoMedio /= liquido;

  ui->doubleSpinBoxDescTotal->setValue(ui->doubleSpinBoxDescMes->value() / 30 * prazoMedio);
  ui->doubleSpinBoxPrazoMedio->setValue(prazoMedio);
  ui->doubleSpinBoxValorBruto->setValue(bruto);
  ui->doubleSpinBoxValorLiquido->setValue(liquido);

  unsetConnections();
  ui->doubleSpinBoxValorPresente->setValue(liquido * (1 - ui->doubleSpinBoxDescTotal->value() / 100));
  setConnections();

  ui->doubleSpinBoxIOF->setValue(0);
  if (ui->checkBoxIOF->isChecked()) ui->doubleSpinBoxIOF->setValue(ui->doubleSpinBoxValorPresente->value() * (0.0038 + 0.0041 * prazoMedio));
  ui->doubleSpinBoxLiqIOF->setValue(ui->doubleSpinBoxValorPresente->value() - ui->doubleSpinBoxIOF->value());
}

void AnteciparRecebimento::setupTables() {
  modelContaReceber.setTable("conta_a_receber_has_pagamento");
  modelContaReceber.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelContaReceber.setHeaderData("dataEmissao", "Data Emissão");
  modelContaReceber.setHeaderData("idVenda", "Código");
  modelContaReceber.setHeaderData("valor", "R$");
  modelContaReceber.setHeaderData("tipo", "Tipo");
  modelContaReceber.setHeaderData("parcela", "Parcela");
  modelContaReceber.setHeaderData("dataPagamento", "Data Pag.");
  modelContaReceber.setHeaderData("observacao", "Obs.");
  modelContaReceber.setHeaderData("status", "Status");
  modelContaReceber.setHeaderData("representacao", "Representação");
  modelContaReceber.setHeaderData("dataRealizado", "Data Realizado");
  modelContaReceber.setHeaderData("valorReal", "Valor Real");
  modelContaReceber.setHeaderData("tipoReal", "Tipo Real");
  modelContaReceber.setHeaderData("parcelaReal", "Parcela Real");
  modelContaReceber.setHeaderData("contaDestino", "Conta Destino");
  modelContaReceber.setHeaderData("tipoDet", "Tipo Det");
  modelContaReceber.setHeaderData("centroCusto", "Centro Custo");
  modelContaReceber.setHeaderData("grupo", "Grupo");
  modelContaReceber.setHeaderData("subGrupo", "SubGrupo");
  modelContaReceber.setHeaderData("contraParte", "Contraparte");
  modelContaReceber.setHeaderData("statusFinanceiro", "Financeiro");

  modelContaReceber.setFilter("(status = 'PENDENTE' OR status = 'CONFERIDO') AND representacao = FALSE AND dataPagamento > NOW()");

  if (not modelContaReceber.select()) {
    emit errorSignal("Erro lendo tabela: " + modelContaReceber.lastError().text());
    return;
  }

  ui->table->setModel(&modelContaReceber);
  ui->table->hideColumn("representacao");
  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("nfe");
  ui->table->hideColumn("dataRealizado");
  ui->table->hideColumn("valorReal");
  ui->table->hideColumn("tipoReal");
  ui->table->hideColumn("parcelaReal");
  ui->table->hideColumn("contaDestino");
  ui->table->hideColumn("tipoDet");
  ui->table->hideColumn("centroCusto");
  ui->table->hideColumn("subGrupo");
  ui->table->hideColumn("comissao");
  ui->table->hideColumn("taxa");
  ui->table->hideColumn("desativado");

  ui->table->setItemDelegate(new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void AnteciparRecebimento::on_table_entered(const QModelIndex) { ui->table->resizeColumnsToContents(); }

void AnteciparRecebimento::on_comboBox_currentTextChanged(const QString &text) {
  modelContaReceber.setFilter("tipo LIKE '%" + ui->comboBox->currentText() + "%' AND idVenda LIKE '" + ui->comboBoxLoja->currentText() +
                              "%' AND (status = 'PENDENTE' or status = 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE and dataPagamento > NOW()");

  if (text == "Cartão de crédito" or ui->comboBox->currentText() == "Cartão de débito") {
    modelContaReceber.setFilter("(tipo LIKE '%Cartão de crédito%' OR tipo LIKE '%Cartão de débito%' OR tipo LIKE '%Taxa Cartão%') AND representacao = FALSE AND idVenda LIKE '" +
                                ui->comboBoxLoja->currentText() + "%' AND (status = 'PENDENTE' OR status = 'CONFERIDO') AND desativado = FALSE and dataPagamento > NOW()");
  }

  if (not modelContaReceber.select()) {
    emit errorSignal("Erro lendo tabela: " + modelContaReceber.lastError().text());
    return;
  }
}

void AnteciparRecebimento::on_doubleSpinBoxValorPresente_valueChanged(double) {
  const double presente = ui->doubleSpinBoxValorPresente->value();
  const double liquido = ui->doubleSpinBoxValorLiquido->value();
  const double descTotal = 1 - (presente / liquido);
  const double prazoMedio = ui->doubleSpinBoxPrazoMedio->value();

  ui->doubleSpinBoxDescTotal->setValue(descTotal * 100);

  const double valor = descTotal / prazoMedio * 30 * 100;
  unsetConnections();
  ui->doubleSpinBoxDescMes->setValue(qIsNaN(valor) or qIsInf(valor) ? 0 : valor);
  setConnections();
  ui->doubleSpinBoxIOF->setValue(0);
  if (ui->checkBoxIOF->isChecked()) ui->doubleSpinBoxIOF->setValue(ui->doubleSpinBoxValorPresente->value() * (0.0038 + 0.0041 * prazoMedio));
  ui->doubleSpinBoxLiqIOF->setValue(ui->doubleSpinBoxValorPresente->value() - ui->doubleSpinBoxIOF->value());
}

bool AnteciparRecebimento::cadastrar(const QModelIndexList &list) {
  for (const auto &item : list) {
    if (not modelContaReceber.setData(item.row(), "status", "RECEBIDO")) return false;
    if (not modelContaReceber.setData(item.row(), "dataRealizado", ui->dateEditEvento->date())) return false;
    if (not modelContaReceber.setData(item.row(), "valorReal", modelContaReceber.data(item.row(), "valor"))) return false;
    if (not modelContaReceber.setData(item.row(), "tipoReal", modelContaReceber.data(item.row(), "tipo"))) return false;
    if (not modelContaReceber.setData(item.row(), "parcelaReal", modelContaReceber.data(item.row(), "parcela"))) return false;
    if (not modelContaReceber.setData(item.row(), "tipoReal", modelContaReceber.data(item.row(), "tipo"))) return false;
    if (not modelContaReceber.setData(item.row(), "contaDestino", ui->itemBoxConta->getValue())) return false;

    if (modelContaReceber.data(item.row(), "centroCusto").isNull()) {
      emit errorSignal("Item sem Centro de Custo identificado: " + modelContaReceber.data(item.row(), "idVenda").toString());
      return false;
    }

    if (modelContaReceber.data(item.row(), "grupo").isNull()) {
      emit errorSignal("Item sem Grupo identificado: " + modelContaReceber.data(item.row(), "idVenda").toString());
      return false;
    }

    if (not modelContaReceber.setData(item.row(), "observacao", modelContaReceber.data(item.row(), "observacao").toString() + "Antecipação")) return false;
  }

  if (not modelContaReceber.submitAll()) return false;

  const int newRow = modelContaReceber.rowCount();
  if (not modelContaReceber.insertRow(newRow)) return false;

  //----------------------------------

  SqlRelationalTableModel modelContaPagar;
  modelContaPagar.setTable("conta_a_pagar_has_pagamento");
  modelContaPagar.setEditStrategy(QSqlTableModel::OnManualSubmit);

  QSqlQuery query;
  query.prepare("SELECT banco FROM loja_has_conta WHERE idConta = :idConta");
  query.bindValue(":idConta", ui->itemBoxConta->getValue());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando 'banco': " + query.lastError().text());
    return false;
  }

  if (ui->doubleSpinBoxValorLiquido->value() - ui->doubleSpinBoxValorPresente->value() > 0) {
    const int rowPagar1 = modelContaPagar.rowCount();
    if (not modelContaPagar.insertRow(rowPagar1)) return false;

    if (not modelContaPagar.setData(rowPagar1, "dataEmissao", ui->dateEditEvento->date())) return false;
    if (not modelContaPagar.setData(rowPagar1, "idLoja", 1)) return false;
    if (not modelContaPagar.setData(rowPagar1, "contraParte", query.value("banco"))) return false;
    if (not modelContaPagar.setData(rowPagar1, "valor", ui->doubleSpinBoxValorLiquido->value() - ui->doubleSpinBoxValorPresente->value())) return false;
    if (not modelContaPagar.setData(rowPagar1, "tipo", "DÉBITO EM CONTA")) return false;
    if (not modelContaPagar.setData(rowPagar1, "dataPagamento", ui->dateEditEvento->date())) return false;
    if (not modelContaPagar.setData(rowPagar1, "observacao", "Juros da antecipação de recebíveis")) return false;
    if (not modelContaPagar.setData(rowPagar1, "status", "PAGO")) return false;
    if (not modelContaPagar.setData(rowPagar1, "dataRealizado", ui->dateEditEvento->date())) return false;
    if (not modelContaPagar.setData(rowPagar1, "valorReal", ui->doubleSpinBoxValorLiquido->value() - ui->doubleSpinBoxValorPresente->value())) return false;
    if (not modelContaPagar.setData(rowPagar1, "tipoReal", "DÉBITO EM CONTA")) return false;
    if (not modelContaPagar.setData(rowPagar1, "contaDestino", ui->itemBoxConta->getValue())) return false;
    if (not modelContaPagar.setData(rowPagar1, "centroCusto", "1")) return false;
    if (not modelContaPagar.setData(rowPagar1, "grupo", "Despesas Financeiras")) return false;
    if (not modelContaPagar.setData(rowPagar1, "subGrupo", "Juros")) return false;
  }

  //

  if (ui->doubleSpinBoxIOF->value() > 0) {
    const int rowPagar2 = modelContaPagar.rowCount();
    if (not modelContaPagar.insertRow(rowPagar2)) return false;

    if (not modelContaPagar.setData(rowPagar2, "dataEmissao", ui->dateEditEvento->date())) return false;
    if (not modelContaPagar.setData(rowPagar2, "idLoja", 1)) return false;
    if (not modelContaPagar.setData(rowPagar2, "contraParte", query.value("banco"))) return false;
    if (not modelContaPagar.setData(rowPagar2, "valor", ui->doubleSpinBoxIOF->value())) return false;
    if (not modelContaPagar.setData(rowPagar2, "tipo", "DÉBITO EM CONTA")) return false;
    if (not modelContaPagar.setData(rowPagar2, "dataPagamento", ui->dateEditEvento->date())) return false;
    if (not modelContaPagar.setData(rowPagar2, "observacao", "IOF da antecipação de recebíveis")) return false;
    if (not modelContaPagar.setData(rowPagar2, "status", "PAGO")) return false;
    if (not modelContaPagar.setData(rowPagar2, "dataRealizado", ui->dateEditEvento->date())) return false;
    if (not modelContaPagar.setData(rowPagar2, "valorReal", ui->doubleSpinBoxIOF->value())) return false;
    if (not modelContaPagar.setData(rowPagar2, "tipoReal", "DÉBITO EM CONTA")) return false;
    if (not modelContaPagar.setData(rowPagar2, "contaDestino", ui->itemBoxConta->getValue())) return false;
    if (not modelContaPagar.setData(rowPagar2, "centroCusto", "1")) return false;
    if (not modelContaPagar.setData(rowPagar2, "grupo", "Despesas Financeiras")) return false;
    if (not modelContaPagar.setData(rowPagar2, "subGrupo", "IOF")) return false;
  }

  if (not modelContaPagar.submitAll()) return false;

  return true;
}

void AnteciparRecebimento::on_pushButtonGerar_clicked() {
  // TODO: 1para gerar a antecipacao: dar baixa nas linhas selecionadas (colocar RECEBIDO e marcar em qual data) e criar
  // uma unica linha dizendo quanto foi pago de juros

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not cadastrar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) return;

  emit transactionEnded();

  emit informationSignal("Operação realizada com sucesso!");
}

// TODO: 0implementar antecipacao

// O Cálculo do prazo médio de vencimento das duplicatas é feito da seguinte forma:

// Com a soma dos valores das parcelas, obtém-se o primeiro valor(A).
// O segundo passo, é a totalização, da multiplicação do número de dias de cada parcela, por seu respectivo valor (B).
// Para obter o prazo médio, é só dividir os valores (B) por (A).

// Ex. 3 parcelas de R$100,00 -> vencimento em 10/20/30 dias

// A = 100,00 + 100,00 + 100,00.
// A = 300,00.

// B = (10 * 100,00) + (20 * 100,00) + (30 * 100,00).
// B = 6.000,00

// Cálculo do prazo médio = 6.000,00 / 300,00
// PRAZO MÉDIO = 20.

// datas / valor liquido

// prazo medio = somatorio de cada linha usando -> (dias parcela * valor) / valor liquido
// ((data do pag - data do evento) * valor[valor lanc. - mdr]) / valor liquido
// valor bruto = soma das linhas que nao sao mdr
// valor liquido = soma inclusive mdr
// taxa desc total = taxa desc mes / 30 * prazo medio
// valor presente = valor liquido * (1 - taxa desc total)

// MDR 2%
// Taxa Antec. 5%

// valor bruto = R$ 100
// valor liq. = R$ 98
// taxa desc = (5% / 30) * (quant. dias antecipados)
//     taxa desc = (5% / 30) * 60d
//     taxa desc = 5% * 2 = 10%
// valor presente = R$ 98 - (R$ 98 * 10%) = R$ 88,2

// TODO: 1para recebiveis diferentes de cartao calcular IOF
