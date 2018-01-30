#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "doubledelegate.h"
#include "inputdialogfinanceiro.h"
#include "noeditdelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "singleeditdelegate.h"
#include "ui_inputdialogfinanceiro.h"
#include "usersession.h"

InputDialogFinanceiro::InputDialogFinanceiro(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogFinanceiro) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->frameData->hide();
  ui->frameDataPreco->hide();
  ui->frameFrete->hide();
  ui->checkBoxMarcarTodos->hide();
  ui->groupBoxFinanceiro->hide();
  ui->framePgtTotal->hide();

  ui->dateEditEvento->setDate(QDate::currentDate());
  ui->dateEditProximo->setDate(QDate::currentDate());
  ui->dateEditPgtSt->setDate(QDate::currentDate());

  if (tipo == Tipo::ConfirmarCompra) {
    ui->frameData->show();
    ui->frameDataPreco->show();
    ui->frameFrete->show();
    ui->checkBoxMarcarTodos->show();
    ui->framePgtTotal->show();

    ui->labelEvento->setText("Data confirmação:");
    ui->labelProximoEvento->setText("Data prevista faturamento:");
  }

  if (tipo == Tipo::Financeiro) {
    ui->frameDataPreco->show();
    ui->groupBoxFinanceiro->show();
  }

  setConnections();

  connect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, [=]() { this->montarFluxoCaixa(); });
  connect(ui->widgetPgts, &WidgetPagamentos::valueChanged, this, &InputDialogFinanceiro::on_doubleSpinBoxPgt_valueChanged);

  show();
}

void InputDialogFinanceiro::setConnections() {
  connect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled);
  connect(ui->dateEditEvento, &QDateTimeEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged);
  connect(ui->dateEditPgtSt, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditPgtSt_dateChanged);
  connect(ui->doubleSpinBoxAdicionais, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAdicionais_valueChanged);
  connect(ui->doubleSpinBoxFrete, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged);
  connect(ui->doubleSpinBoxSt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged);
  connect(ui->doubleSpinBoxTotalPag, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxTotalPag_valueChanged);
  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonAdicionarPagamento_clicked);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked);
  connect(ui->pushButtonLimparPag, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonLimparPag_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked);
}

void InputDialogFinanceiro::unsetConnections() {
  disconnect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled);
  disconnect(ui->dateEditEvento, &QDateTimeEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged);
  disconnect(ui->dateEditPgtSt, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditPgtSt_dateChanged);
  disconnect(ui->doubleSpinBoxAdicionais, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAdicionais_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxSt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged);
  disconnect(ui->doubleSpinBoxTotalPag, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxTotalPag_valueChanged);
  disconnect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonAdicionarPagamento_clicked);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonLimparPag, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonLimparPag_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked);
}

InputDialogFinanceiro::~InputDialogFinanceiro() { delete ui; }

QDateTime InputDialogFinanceiro::getDate() const { return ui->dateEditEvento->dateTime(); }

QDateTime InputDialogFinanceiro::getNextDate() const { return ui->dateEditProximo->dateTime(); }

void InputDialogFinanceiro::setupTables() {
  model.setTable("pedido_fornecedor_has_produto");
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);
  model.setHeaderData("idVenda", "Código");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("colecao", "Coleção");
  model.setHeaderData("caixas", "Caixas");
  model.setHeaderData("prcUnitario", "$ Unit.");
  model.setHeaderData("quant", "Quant.");
  model.setHeaderData("preco", "Total");
  model.setHeaderData("un", "Un.");
  model.setHeaderData("un2", "Un.2");
  model.setHeaderData("kgcx", "Kg./Cx.");
  model.setHeaderData("formComercial", "Formato");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("obs", "Obs.");
  model.setHeaderData("aliquotaSt", "Alíquota ST");
  model.setHeaderData("st", "ST");

  ui->table->setModel(&model);
  ui->table->hideColumn("idVendaProduto");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("quantConsumida");
  ui->table->hideColumn("idNfe");
  ui->table->hideColumn("idEstoque");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("idPedido");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("codBarras");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("status");
  ui->table->hideColumn("dataPrevCompra");
  ui->table->hideColumn("dataRealCompra");
  ui->table->hideColumn("dataPrevConf");
  ui->table->hideColumn("dataRealConf");
  ui->table->hideColumn("dataPrevFat");
  ui->table->hideColumn("dataRealFat");
  ui->table->hideColumn("dataPrevColeta");
  ui->table->hideColumn("dataRealColeta");
  ui->table->hideColumn("dataPrevReceb");
  ui->table->hideColumn("dataRealReceb");
  ui->table->hideColumn("dataPrevEnt");
  ui->table->hideColumn("dataRealEnt");
  ui->table->setItemDelegate(new NoEditDelegate(this));
  ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(this));

  if (tipo == Tipo::ConfirmarCompra) ui->table->setItemDelegateForColumn("quant", new SingleEditDelegate(this));

  modelFluxoCaixa.setTable("conta_a_pagar_has_pagamento");
  modelFluxoCaixa.setEditStrategy(QSqlTableModel::OnManualSubmit);
  modelFluxoCaixa.setHeaderData("tipo", "Tipo");
  modelFluxoCaixa.setHeaderData("parcela", "Parcela");
  modelFluxoCaixa.setHeaderData("valor", "R$");
  modelFluxoCaixa.setHeaderData("dataPagamento", "Data");
  modelFluxoCaixa.setHeaderData("observacao", "Obs.");
  modelFluxoCaixa.setHeaderData("status", "Status");

  ui->tableFluxoCaixa->setModel(&modelFluxoCaixa);
  ui->tableFluxoCaixa->setItemDelegate(new DoubleDelegate(this));
  ui->tableFluxoCaixa->hideColumn("nfe");
  ui->tableFluxoCaixa->hideColumn("contraParte");
  ui->tableFluxoCaixa->hideColumn("idCompra");
  ui->tableFluxoCaixa->hideColumn("idLoja");
  ui->tableFluxoCaixa->hideColumn("idPagamento");
  ui->tableFluxoCaixa->hideColumn("dataEmissao");
  ui->tableFluxoCaixa->hideColumn("dataRealizado");
  ui->tableFluxoCaixa->hideColumn("valorReal");
  ui->tableFluxoCaixa->hideColumn("tipoReal");
  ui->tableFluxoCaixa->hideColumn("parcelaReal");
  ui->tableFluxoCaixa->hideColumn("contaDestino");
  ui->tableFluxoCaixa->hideColumn("tipoDet");
  ui->tableFluxoCaixa->hideColumn("centroCusto");
  ui->tableFluxoCaixa->hideColumn("grupo");
  ui->tableFluxoCaixa->hideColumn("subGrupo");
  ui->tableFluxoCaixa->hideColumn("taxa");
  ui->tableFluxoCaixa->hideColumn("desativado");
}

void InputDialogFinanceiro::montarFluxoCaixa(const bool updateDate) {
  // TODO: 'ST Loja' tem a data do faturamento, 'ST Fornecedor' segue as datas dos pagamentos

  unsetConnections();

  [=]() {
    if (representacao) return;

    if (not modelFluxoCaixa.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + modelFluxoCaixa.lastError().text());

    if (tipo == Tipo::Financeiro) {
      for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
        if (not modelFluxoCaixa.setData(row, "status", "SUBSTITUIDO")) {
          QMessageBox::critical(this, "Erro!", "Erro mudando status para 'SUBSTITUIDO'!");
          return;
        }
      }
    }

    for (int i = 0; i < ui->widgetPgts->listComboData.size(); ++i) {
      if (ui->widgetPgts->listComboPgt.at(i)->currentText() != "Escolha uma opção!") {
        // REFAC: simplify/redo this block
        const int parcelas = ui->widgetPgts->listComboParc.at(i)->currentIndex() + 1;
        const double valor = ui->widgetPgts->listDoubleSpinPgt.at(i)->value();
        const float temp = ui->widgetPgts->listComboPgt.at(i)->currentText() == "Cartão de crédito" ? static_cast<float>(static_cast<int>(static_cast<float>(valor / parcelas) * 100)) / 100
                                                                                                    : qRound(valor / parcelas * 100) / 100.;
        const double resto = static_cast<double>(valor - (temp * parcelas));
        const double parcela = static_cast<double>(temp);

        for (int x = 0, y = parcelas - 1; x < parcelas; ++x, --y) {
          const int row = modelFluxoCaixa.rowCount();
          modelFluxoCaixa.insertRow(row);
          if (not modelFluxoCaixa.setData(row, "contraParte", model.data(0, "fornecedor"))) return;
          if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) return;
          if (not modelFluxoCaixa.setData(row, "idCompra", model.data(0, "idCompra"))) return;
          if (not modelFluxoCaixa.setData(row, "idLoja", 1)) return; // Geral
          const QDate dataPgt = (ui->widgetPgts->listComboData.at(i)->currentText() == "Data + 1 Mês"
                                     ? ui->widgetPgts->listDatePgt.at(i)->date().addMonths(x + 1)
                                     : ui->widgetPgts->listComboData.at(i)->currentText() == "Data Mês"
                                           ? ui->widgetPgts->listDatePgt.at(i)->date().addMonths(x)
                                           : ui->widgetPgts->listDatePgt.at(i)->date().addDays(ui->widgetPgts->listComboData.at(i)->currentText().toInt() * (x + 1)));
          if (not modelFluxoCaixa.setData(row, "dataPagamento", dataPgt)) return;
          if (not modelFluxoCaixa.setData(row, "valor", parcela + (x == 0 ? resto : 0))) return;
          if (not modelFluxoCaixa.setData(row, "tipo", QString::number(i + 1) + ". " + ui->widgetPgts->listComboPgt.at(i)->currentText())) return;
          if (not modelFluxoCaixa.setData(row, "parcela", parcelas - y)) return;
          if (not modelFluxoCaixa.setData(row, "observacao", ui->widgetPgts->listLinePgt.at(i)->text())) return;
        }
      }
    }

    if (ui->doubleSpinBoxFrete->value() > 0) {
      const int row = modelFluxoCaixa.rowCount();
      modelFluxoCaixa.insertRow(row);
      if (not modelFluxoCaixa.setData(row, "contraParte", model.data(0, "fornecedor"))) return;
      if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) return;
      if (not modelFluxoCaixa.setData(row, "idCompra", model.data(0, "idCompra"))) return;
      if (not modelFluxoCaixa.setData(row, "idLoja", 1)) return;                           // Geral
      if (not modelFluxoCaixa.setData(row, "dataPagamento", QDate::currentDate())) return; // TODO: 5redo this with a editable date
      if (not modelFluxoCaixa.setData(row, "valor", ui->doubleSpinBoxFrete->value())) return;
      if (not modelFluxoCaixa.setData(row, "tipo", "Frete")) return;
      if (not modelFluxoCaixa.setData(row, "parcela", 1)) return;
      if (not modelFluxoCaixa.setData(row, "observacao", "")) return;
    }

    // set st date
    if (updateDate) {
      QDate date = modelFluxoCaixa.data(0, "dataPagamento").toDate();

      for (int row = 1; row < modelFluxoCaixa.rowCount(); ++row) {
        const QDate current = modelFluxoCaixa.data(row, "dataPagamento").toDate();

        if (current < date) date = current;
      }

      ui->dateEditPgtSt->setDate(date);
    }

    if (ui->doubleSpinBoxSt->value() > 0) {
      const int row = modelFluxoCaixa.rowCount();
      modelFluxoCaixa.insertRow(row);

      if (not modelFluxoCaixa.setData(row, "contraParte", model.data(0, "fornecedor"))) return;
      if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) return;
      if (not modelFluxoCaixa.setData(row, "idCompra", model.data(0, "idCompra"))) return;
      if (not modelFluxoCaixa.setData(row, "idLoja", 1)) return; // Geral
      if (not modelFluxoCaixa.setData(row, "dataPagamento", ui->dateEditPgtSt->date())) return;
      if (not modelFluxoCaixa.setData(row, "valor", ui->doubleSpinBoxSt->value())) return;
      if (not modelFluxoCaixa.setData(row, "tipo", "ST Fornecedor")) return;
      if (not modelFluxoCaixa.setData(row, "parcela", 1)) return;
      if (not modelFluxoCaixa.setData(row, "observacao", "")) return;
    }

    ui->tableFluxoCaixa->resizeColumnsToContents();
  }();

  setConnections();
}

void InputDialogFinanceiro::calcularTotal() {
  double total = 0;
  double st = 0;

  const auto list = ui->table->selectionModel()->selectedRows();

  for (const auto &item : list) {
    const double preco = model.data(item.row(), "preco").toDouble();
    const QString tipoSt = model.data(item.row(), "st").toString();
    const double aliquota = model.data(item.row(), "aliquotaSt").toDouble();

    const bool isSt = (tipoSt == "ST Fornecedor" or tipoSt == "ST Loja");

    st += isSt ? preco * aliquota / 100 : 0;
    total += preco;
  }

  total -= ui->doubleSpinBoxAdicionais->value();

  ui->doubleSpinBoxSt->setValue(st);
  ui->doubleSpinBoxTotal->setValue(total);
}

void InputDialogFinanceiro::updateTableData(const QModelIndex &topLeft) {
  const QString header = model.headerData(topLeft.column(), Qt::Horizontal).toString();
  const int row = topLeft.row();

  if (header == "Quant." or header == "$ Unit.") {
    const double preco = model.data(row, "quant").toDouble() * model.data(row, "prcUnitario").toDouble();
    if (not model.setData(row, "preco", preco)) return;
  }

  if (header == "Total") {
    const double preco = model.data(row, "preco").toDouble() / model.data(row, "quant").toDouble();
    if (not model.setData(row, "prcUnitario", preco)) return;
  }

  calcularTotal();
}

void InputDialogFinanceiro::resetarPagamentos() {
  // REFAC: redo this (clear lists)

  ui->doubleSpinBoxTotalPag->setValue(0);
  ui->doubleSpinBoxTotalPag->setMaximum(ui->doubleSpinBoxTotal->value());

  ui->widgetPgts->resetarPagamentos();

  montarFluxoCaixa();
}

bool InputDialogFinanceiro::setFilter(const QString &idCompra) {
  if (idCompra.isEmpty()) {
    model.setFilter("idPedido = 0");
    QMessageBox::critical(this, "Erro!", "IdCompra vazio!");
    return false;
  }

  if (tipo == Tipo::ConfirmarCompra) model.setFilter("idCompra = " + idCompra + " AND status = 'EM COMPRA'");
  if (tipo == Tipo::Financeiro) model.setFilter("idCompra = " + idCompra);

  if (not model.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela pedido_fornecedor_has_produto: " + model.lastError().text());
    return false;
  }

  ui->table->resizeColumnsToContents();

  if (tipo == Tipo::ConfirmarCompra or tipo == Tipo::Financeiro) {
    modelFluxoCaixa.setFilter(tipo == Tipo::ConfirmarCompra ? "0" : "idCompra = " + idCompra);

    if (not modelFluxoCaixa.select()) {
      QMessageBox::critical(this, "Erro!", "Erro lendo tabela conta_a_pagar_has_pagamento: " + modelFluxoCaixa.lastError().text());
      return false;
    }

    calcularTotal();

    ui->checkBoxMarcarTodos->setChecked(true);

    ui->tableFluxoCaixa->resizeColumnsToContents();
  }

  if (tipo == Tipo::Financeiro) ui->comboBoxFinanceiro->setCurrentText(model.data(0, "statusFinanceiro").toString());

  QSqlQuery query;
  query.prepare("SELECT v.representacao FROM pedido_fornecedor_has_produto pf LEFT JOIN venda v ON pf.idVenda = v.idVenda WHERE idCompra = :idCompra");
  query.bindValue(":idCompra", idCompra);

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando se é representacao: " + query.lastError().text());
    return false;
  }

  representacao = query.value("representacao").toBool();

  if (representacao) {
    ui->framePagamentos->hide();
    ui->frameFrete->hide();
  }

  setWindowTitle("OC: " + model.data(0, "ordemCompra").toString());

  if (tipo == Tipo::ConfirmarCompra) {
    connect(&model, &SqlRelationalTableModel::dataChanged, this, &InputDialogFinanceiro::updateTableData);
    connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::calcularTotal);
    //    connect(ui->doubleSpinBoxTotalPag, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::resetarPagamentos);
  }

  //

  calcularTotal();

  return true;
}

void InputDialogFinanceiro::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) return;

  // REFAC: refactor this function (why ConfirmarCompra doesnt use a transaction?)

  if (tipo == Tipo::ConfirmarCompra) {
    if (not cadastrar()) {
      return;
    }

    QDialog::accept();
    close();
  }

  //  if (type == ConfirmarCompra and not verifyFields()) return;

  if (tipo == Tipo::Financeiro) {
    emit transactionStarted();

    QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
    QSqlQuery("START TRANSACTION").exec();

    if (not cadastrar()) {
      QSqlQuery("ROLLBACK").exec();
      emit transactionEnded();
      return;
    }

    QSqlQuery("COMMIT").exec();

    emit transactionEnded();

    QMessageBox::information(this, "Aviso!", "Dados salvos com sucesso!");
    QDialog::accept();
    close();
  }
}

bool InputDialogFinanceiro::verifyFields() {
  if (ui->table->selectionModel()->selectedRows().isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return false;
  }

  if (not representacao) {
    if (not qFuzzyCompare(ui->doubleSpinBoxTotalPag->value(), ui->doubleSpinBoxTotal->value())) {
      QMessageBox::critical(this, "Erro!", "Soma dos pagamentos difere do total! Favor verificar!");
      return false;
    }
  }

  return true;
}

bool InputDialogFinanceiro::cadastrar() {
  if (tipo == Tipo::ConfirmarCompra) {
    const auto list = ui->table->selectionModel()->selectedRows();

    for (const auto &item : list) {
      if (not model.setData(item.row(), "selecionado", true)) return false;
    }
  }

  if (tipo == Tipo::Financeiro) {
    if (not model.setData(0, "statusFinanceiro", ui->comboBoxFinanceiro->currentText())) {
      emit errorSignal("Erro salvando status na tabela: " + model.lastError().text());
      return false;
    }
  }

  if (not model.submitAll()) {
    emit errorSignal("Erro salvando dados na tabela: " + model.lastError().text());
    return false;
  }

  if (tipo == Tipo::ConfirmarCompra or tipo == Tipo::Financeiro) {
    if (not modelFluxoCaixa.submitAll()) {
      emit errorSignal("Erro salvando dados do pagamento: " + modelFluxoCaixa.lastError().text());
      return false;
    }
  }

  return true;
}

void InputDialogFinanceiro::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) ui->dateEditProximo->setDate(date);
}

void InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled(bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void InputDialogFinanceiro::on_doubleSpinBoxPgt_valueChanged() {
  double sum = 0;

  Q_FOREACH (const auto &spinbox, ui->widgetPgts->listDoubleSpinPgt) { sum += spinbox->value(); }

  ui->doubleSpinBoxTotalPag->setValue(sum);

  montarFluxoCaixa();
}

void InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked() {
  // REFAC: redo this

  //  ui->framePgt1->show();
  //  ui->framePgt2->show();
  //  ui->framePgt3->show();
  //  ui->framePgt4->show();
  //  ui->framePgt5->show();
  ui->framePgtTotal->show();

  //

  if (modelFluxoCaixa.rowCount() > 0) {
    const QDate date = modelFluxoCaixa.data(0, "dataPagamento").toDate();
    Q_UNUSED(date);

    //    ui->dateEditPgt1->setDate(date);
    //    ui->dateEditPgt2->setDate(date);
    //    ui->dateEditPgt3->setDate(date);
    //    ui->dateEditPgt4->setDate(date);
    //    ui->dateEditPgt5->setDate(date);
  }

  //

  // TODO: 5alterar para que apenas na tela do financeiro compra a opcao de corrigir fluxo percorra todas as linhas
  // (enquanto na confirmacao de pagamento percorre apenas as linhas selecionadas)

  calcularTotal();
  resetarPagamentos();
}

void InputDialogFinanceiro::on_pushButtonLimparPag_clicked() { resetarPagamentos(); }

void InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged(double) {
  calcularTotal();
  montarFluxoCaixa();
}

void InputDialogFinanceiro::on_doubleSpinBoxAdicionais_valueChanged(const double value) {
  Q_UNUSED(value);
  // TODO: 0fracionar desconto nos itens
  // TODO: 0mudanca de selecao deve rodar esse codigo novamente para redistribuir o desconto

  /*

  QSqlQuery query;

  QString queryString;

  for (auto index : ui->table->selectionModel()->selectedRows()) {
    queryString += queryString.isEmpty() ? "idPedido = " + model.data(index.row(), "idPedido").toString()
                                         : " OR idPedido = " + model.data(index.row(), "idPedido").toString();
  }

  queryString.prepend("SELECT SUM(preco) AS total FROM pedido_fornecedor_has_produto WHERE ");

  if (not query.exec(queryString) or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando total: " + query.lastError().text());
    return;
  }

  const double total = query.value("total").toDouble();
  //  const double total = ui->doubleSpinBoxTotalPag->value();

  QVector<int> indexs;

  for (const auto index : ui->table->selectionModel()->selectedRows()) {
    indexs << index.row();
  }

  model.select();

  for (auto row : indexs) {
    ui->table->selectionModel()->select(model.index(row, 0),
                                        QItemSelectionModel::Rows | QItemSelectionModel::SelectCurrent);
  }

  // TODO: 0percorrer apenas as linhas selecionadas?

  for (const auto index : ui->table->selectionModel()->selectedRows()) {
    //  for (int row = 0; row < model.rowCount(); ++row) {
    const double totalProd = QString::number(model.data(index.row(), "preco").toDouble(), 'f', 2).toDouble();
    const double valorProporcional = totalProd - (totalProd / total * value);

    model.setData(index.row(), "preco", valorProporcional);

    model.setData(index.row(), "prcUnitario", valorProporcional / model.data(index.row(), "quant").toDouble());
  }
  // TODO: 0lidar com divisao por zero/nan

  */

  calcularTotal();
}

void InputDialogFinanceiro::on_comboBoxPgt_currentTextChanged(const int index, const QString &text) {
  if (text == "Escolha uma opção!") return;

  QSqlQuery query;
  query.prepare("SELECT parcelas FROM forma_pagamento WHERE pagamento = :pagamento");
  query.bindValue(":pagamento", ui->widgetPgts->listComboPgt.at(index)->currentText());

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo formas de pagamentos: " + query.lastError().text());
    return;
  }

  const int parcelas = query.value("parcelas").toInt();

  ui->widgetPgts->listComboParc.at(index)->clear();

  for (int i = 0; i < parcelas; ++i) ui->widgetPgts->listComboParc.at(index)->addItem(QString::number(i + 1) + "x");

  ui->widgetPgts->listComboParc.at(index)->setEnabled(true);
  ui->widgetPgts->listDatePgt.at(index)->setEnabled(true);

  montarFluxoCaixa();
}

void InputDialogFinanceiro::on_dateEditPgtSt_dateChanged(const QDate &) { montarFluxoCaixa(false); }

void InputDialogFinanceiro::on_pushButtonAdicionarPagamento_clicked() {
  ui->widgetPgts->adicionarPagamentoCompra(ui->doubleSpinBoxTotal->value() - ui->doubleSpinBoxTotalPag->value());

  on_doubleSpinBoxPgt_valueChanged();
}

void InputDialogFinanceiro::on_doubleSpinBoxTotalPag_valueChanged(double) {
  if (ui->widgetPgts->listDoubleSpinPgt.size() <= 1) return;

  double sumWithoutLast = 0;

  Q_FOREACH (const auto &item, ui->widgetPgts->listDoubleSpinPgt) {
    item->setMaximum(ui->doubleSpinBoxTotal->value());
    sumWithoutLast += item->value();
  }

  const auto lastSpinBox = ui->widgetPgts->listDoubleSpinPgt.at(ui->widgetPgts->listDoubleSpinPgt.size() - 1);

  sumWithoutLast -= lastSpinBox->value();

  lastSpinBox->setMaximum(ui->doubleSpinBoxTotal->value() - sumWithoutLast);
  lastSpinBox->setValue(ui->doubleSpinBoxTotal->value() - sumWithoutLast);

  qDebug() << "min: " << lastSpinBox->minimum();
  qDebug() << "max: " << lastSpinBox->maximum();
}

// TODO: [Conrado] copiar de venda as verificacoes/terminar o codigo dos pagamentos
// REFAC: refatorar o frame pagamentos para um widget para nao duplicar codigo

// TODO: 1quando for confirmacao de representacao perguntar qual o id para colocar na observacao das comissoes (codigo
// que vem do fornecedor)
// TODO: 3quando for representacao mostrar fluxo de comissao
// TODO: ?colocar formas de pagamento 4 e 5
// TODO: 3colocar possibilidade de ajustar valor total para as compras (contabilizar quanto de ajuste foi feito)

void InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged(double) { montarFluxoCaixa(); }
