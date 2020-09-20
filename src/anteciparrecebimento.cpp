#include "anteciparrecebimento.h"
#include "ui_anteciparrecebimento.h"

#include "application.h"
#include "doubledelegate.h"
#include "reaisdelegate.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>

AnteciparRecebimento::AnteciparRecebimento(QWidget *parent) : QDialog(parent), ui(new Ui::AnteciparRecebimento) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->itemBoxConta->setSearchDialog(SearchDialog::conta(this));
  ui->itemBoxConta->setId(3);

  ui->comboBox->clear();
  ui->comboBox->addItem("");

  ui->comboBoxLoja->clear();
  ui->comboBoxLoja->addItem("");
  ui->comboBoxLoja->addItem("ALPH");
  ui->comboBoxLoja->addItem("GABR");
  ui->comboBoxLoja->addItem("GRAN");

  QSqlQuery query;

  if (not query.exec("SELECT DISTINCT pagamento AS tipo FROM view_pagamento_loja")) { throw RuntimeException("Erro comunicando com banco de dados: " + query.lastError().text(), this); }

  while (query.next()) { ui->comboBox->addItem(query.value("tipo").toString()); }

  ui->dateEditEvento->setDate(qApp->serverDate());

  setConnections();
}

AnteciparRecebimento::~AnteciparRecebimento() { delete ui; }

void AnteciparRecebimento::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxIOF, &QCheckBox::clicked, this, &AnteciparRecebimento::calcularTotais, connectionType);
  connect(ui->comboBox, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged, connectionType);
  connect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged, connectionType);
  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &AnteciparRecebimento::calcularTotais, connectionType);
  connect(ui->doubleSpinBoxDescMes, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::calcularTotais, connectionType);
  connect(ui->doubleSpinBoxValorPresente, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::on_doubleSpinBoxValorPresente_valueChanged, connectionType);
  connect(ui->pushButtonGerar, &QPushButton::clicked, this, &AnteciparRecebimento::on_pushButtonGerar_clicked, connectionType);
  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AnteciparRecebimento::calcularTotais, connectionType);
}

void AnteciparRecebimento::unsetConnections() {
  disconnect(ui->checkBoxIOF, &QCheckBox::clicked, this, &AnteciparRecebimento::calcularTotais);
  disconnect(ui->comboBox, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged);
  disconnect(ui->comboBoxLoja, &QComboBox::currentTextChanged, this, &AnteciparRecebimento::on_comboBox_currentTextChanged);
  disconnect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &AnteciparRecebimento::calcularTotais);
  disconnect(ui->doubleSpinBoxDescMes, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::calcularTotais);
  disconnect(ui->doubleSpinBoxValorPresente, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &AnteciparRecebimento::on_doubleSpinBoxValorPresente_valueChanged);
  disconnect(ui->pushButtonGerar, &QPushButton::clicked, this, &AnteciparRecebimento::on_pushButtonGerar_clicked);
  disconnect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &AnteciparRecebimento::calcularTotais);
}

void AnteciparRecebimento::calcularTotais() {
  // TODO: 1ao selecionar parcelas de cartao somar a taxa tambem
  const auto list = ui->table->selectionModel()->selectedRows();

  double bruto = 0;
  double liquido = 0;
  double prazoMedio = 0;

  for (const auto &index : list) {
    const int row = index.row();

    const QString tipo = modelContaReceber.data(row, "tipo").toString();
    const double valor = modelContaReceber.data(row, "valor").toDouble();
    const QDate dataPagamento = modelContaReceber.data(row, "dataPagamento").toDate();

    if (not tipo.contains("Taxa Cartão")) { bruto += valor; }

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
  [&] { ui->doubleSpinBoxValorPresente->setValue(liquido * (1 - ui->doubleSpinBoxDescTotal->value() / 100)); }();
  setConnections();

  ui->doubleSpinBoxIOF->setValue(0);
  if (ui->checkBoxIOF->isChecked()) { ui->doubleSpinBoxIOF->setValue(ui->doubleSpinBoxValorPresente->value() * (0.0038 + 0.0041 * prazoMedio)); }
  ui->doubleSpinBoxLiqIOF->setValue(ui->doubleSpinBoxValorPresente->value() - ui->doubleSpinBoxIOF->value());
}

void AnteciparRecebimento::setupTables() {
  modelContaReceber.setTable("conta_a_receber_has_pagamento");

  modelContaReceber.setHeaderData("dataEmissao", "Data Emissão");
  modelContaReceber.setHeaderData("idVenda", "Código");
  modelContaReceber.setHeaderData("contraParte", "Contraparte");
  modelContaReceber.setHeaderData("valor", "R$");
  modelContaReceber.setHeaderData("tipo", "Tipo");
  modelContaReceber.setHeaderData("parcela", "Parcela");
  modelContaReceber.setHeaderData("dataPagamento", "Data Pag.");
  modelContaReceber.setHeaderData("observacao", "Obs.");
  modelContaReceber.setHeaderData("status", "Status");
  modelContaReceber.setHeaderData("grupo", "Grupo");

  modelContaReceber.setFilter("status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE ORDER BY dataPagamento");

  modelContaReceber.select();

  ui->table->setModel(&modelContaReceber);

  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("nfe");
  ui->table->hideColumn("representacao");
  ui->table->hideColumn("dataRealizado");
  ui->table->hideColumn("valorReal");
  ui->table->hideColumn("tipoReal");
  ui->table->hideColumn("parcelaReal");
  ui->table->hideColumn("idConta");
  ui->table->hideColumn("tipoDet");
  ui->table->hideColumn("centroCusto");
  ui->table->hideColumn("subGrupo");
  ui->table->hideColumn("comissao");
  ui->table->hideColumn("taxa");
  ui->table->hideColumn("desativado");

  ui->table->setItemDelegate(new DoubleDelegate(this));

  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void AnteciparRecebimento::montaFiltro() {
  const QString textTipo = ui->comboBox->currentText();
  const QString textLoja = ui->comboBoxLoja->currentText();

  if (textTipo == "Cartão de crédito" or textTipo == "Cartão de débito") {
    modelContaReceber.setFilter("(tipo LIKE '%Cartão de crédito%' OR tipo LIKE '%Cartão de débito%' OR tipo LIKE '%Taxa Cartão%') AND representacao = FALSE AND idVenda LIKE '" + textLoja +
                                "%' AND status IN ('PENDENTE', 'CONFERIDO') AND desativado = FALSE ORDER BY dataPagamento");
  } else {
    modelContaReceber.setFilter("tipo LIKE '%" + textTipo + "%' AND idVenda LIKE '" + textLoja +
                                "%' AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE ORDER BY dataPagamento");
  }
}

void AnteciparRecebimento::on_comboBoxLoja_currentTextChanged(const QString &) { montaFiltro(); }

void AnteciparRecebimento::on_comboBox_currentTextChanged(const QString &) { montaFiltro(); }

void AnteciparRecebimento::on_doubleSpinBoxValorPresente_valueChanged(double) {
  const double presente = ui->doubleSpinBoxValorPresente->value();
  const double liquido = ui->doubleSpinBoxValorLiquido->value();
  const double descTotal = 1 - (presente / liquido);
  const double prazoMedio = ui->doubleSpinBoxPrazoMedio->value();

  ui->doubleSpinBoxDescTotal->setValue(descTotal * 100);

  const double valor = descTotal / prazoMedio * 30 * 100;
  unsetConnections();
  [&] { ui->doubleSpinBoxDescMes->setValue(qIsNaN(valor) or qIsInf(valor) ? 0 : valor); }();
  setConnections();
  ui->doubleSpinBoxIOF->setValue(0);
  if (ui->checkBoxIOF->isChecked()) { ui->doubleSpinBoxIOF->setValue(ui->doubleSpinBoxValorPresente->value() * (0.0038 + 0.0041 * prazoMedio)); }
  ui->doubleSpinBoxLiqIOF->setValue(ui->doubleSpinBoxValorPresente->value() - ui->doubleSpinBoxIOF->value());
}

void AnteciparRecebimento::cadastrar(const QModelIndexList &list) {
  for (const auto &index : list) {
    const int row = index.row();

    modelContaReceber.setData(row, "status", "RECEBIDO");
    modelContaReceber.setData(row, "dataRealizado", ui->dateEditEvento->date());
    modelContaReceber.setData(row, "valorReal", modelContaReceber.data(row, "valor"));
    modelContaReceber.setData(row, "tipoReal", modelContaReceber.data(row, "tipo"));
    modelContaReceber.setData(row, "parcelaReal", modelContaReceber.data(row, "parcela"));
    modelContaReceber.setData(row, "tipoReal", modelContaReceber.data(row, "tipo"));
    modelContaReceber.setData(row, "idConta", ui->itemBoxConta->getId());
    modelContaReceber.setData(row, "observacao", modelContaReceber.data(row, "observacao").toString() + "Antecipação");
  }

  modelContaReceber.submitAll();

  //----------------------------------

  SqlTableModel modelContaPagar;
  modelContaPagar.setTable("conta_a_pagar_has_pagamento");

  QSqlQuery query;
  query.prepare("SELECT banco FROM loja_has_conta WHERE idConta = :idConta");
  query.bindValue(":idConta", ui->itemBoxConta->getId());

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando 'banco': " + query.lastError().text()); }

  if (not qFuzzyIsNull(ui->doubleSpinBoxValorLiquido->value() - ui->doubleSpinBoxValorPresente->value())) {
    const int rowPagar1 = modelContaPagar.insertRowAtEnd();

    modelContaPagar.setData(rowPagar1, "dataEmissao", ui->dateEditEvento->date());
    modelContaPagar.setData(rowPagar1, "idLoja", 1);
    modelContaPagar.setData(rowPagar1, "contraParte", query.value("banco"));
    modelContaPagar.setData(rowPagar1, "valor", ui->doubleSpinBoxValorLiquido->value() - ui->doubleSpinBoxValorPresente->value());
    modelContaPagar.setData(rowPagar1, "tipo", "DÉBITO EM CONTA");
    modelContaPagar.setData(rowPagar1, "dataPagamento", ui->dateEditEvento->date());
    modelContaPagar.setData(rowPagar1, "observacao", "Juros da antecipação de recebíveis");
    modelContaPagar.setData(rowPagar1, "status", "PAGO");
    modelContaPagar.setData(rowPagar1, "dataRealizado", ui->dateEditEvento->date());
    modelContaPagar.setData(rowPagar1, "valorReal", ui->doubleSpinBoxValorLiquido->value() - ui->doubleSpinBoxValorPresente->value());
    modelContaPagar.setData(rowPagar1, "tipoReal", "DÉBITO EM CONTA");
    modelContaPagar.setData(rowPagar1, "idConta", ui->itemBoxConta->getId());
    modelContaPagar.setData(rowPagar1, "centroCusto", "1");
    modelContaPagar.setData(rowPagar1, "grupo", "Despesas Financeiras");
    modelContaPagar.setData(rowPagar1, "subGrupo", "Juros");
  }

  //

  if (not qFuzzyIsNull(ui->doubleSpinBoxIOF->value())) {
    const int rowPagar2 = modelContaPagar.insertRowAtEnd();

    modelContaPagar.setData(rowPagar2, "dataEmissao", ui->dateEditEvento->date());
    modelContaPagar.setData(rowPagar2, "idLoja", 1);
    modelContaPagar.setData(rowPagar2, "contraParte", query.value("banco"));
    modelContaPagar.setData(rowPagar2, "valor", ui->doubleSpinBoxIOF->value());
    modelContaPagar.setData(rowPagar2, "tipo", "DÉBITO EM CONTA");
    modelContaPagar.setData(rowPagar2, "dataPagamento", ui->dateEditEvento->date());
    modelContaPagar.setData(rowPagar2, "observacao", "IOF da antecipação de recebíveis");
    modelContaPagar.setData(rowPagar2, "status", "PAGO");
    modelContaPagar.setData(rowPagar2, "dataRealizado", ui->dateEditEvento->date());
    modelContaPagar.setData(rowPagar2, "valorReal", ui->doubleSpinBoxIOF->value());
    modelContaPagar.setData(rowPagar2, "tipoReal", "DÉBITO EM CONTA");
    modelContaPagar.setData(rowPagar2, "idConta", ui->itemBoxConta->getId());
    modelContaPagar.setData(rowPagar2, "centroCusto", "1");
    modelContaPagar.setData(rowPagar2, "grupo", "Despesas Financeiras");
    modelContaPagar.setData(rowPagar2, "subGrupo", "IOF");
  }

  modelContaPagar.submitAll();
}

void AnteciparRecebimento::on_pushButtonGerar_clicked() {
  // TODO: 1para gerar a antecipacao: dar baixa nas linhas selecionadas (colocar RECEBIDO e marcar em qual data) e criar
  // uma unica linha dizendo quanto foi pago de juros

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  verifyFields(list);

  qApp->startTransaction("AnteciparRecebimento::on_pushButtonGerar");

  cadastrar(list);

  qApp->endTransaction();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void AnteciparRecebimento::verifyFields(const QModelIndexList &list) {
  for (const auto &index : list) {
    const int row = index.row();

    if (modelContaReceber.data(row, "centroCusto").isNull()) { throw RuntimeError("Item sem Centro de Custo identificado: " + modelContaReceber.data(row, "idVenda").toString(), this); }

    if (modelContaReceber.data(row, "grupo").isNull()) { throw RuntimeError("Item sem Grupo identificado: " + modelContaReceber.data(row, "idVenda").toString(), this); }
  }
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

// TODO: fazer uma tela igual para dar baixa em lote nos recebimentos
