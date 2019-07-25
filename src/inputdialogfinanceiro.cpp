#include <QDebug>
#include <QLineEdit>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "comboboxdelegate.h"
#include "doubledelegate.h"
#include "inputdialogfinanceiro.h"
#include "noeditdelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "editdelegate.h"
#include "sortfilterproxymodel.h"
#include "ui_inputdialogfinanceiro.h"

InputDialogFinanceiro::InputDialogFinanceiro(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogFinanceiro) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->widgetPgts->setTipo(WidgetPagamentos::Tipo::Compra);

  ui->widgetPgts->hide();

  ui->frameData->hide();
  ui->frameDataPreco->hide();
  ui->checkBoxMarcarTodos->hide();
  ui->groupBoxFinanceiro->hide();

  ui->labelAliquota->hide();
  ui->doubleSpinBoxAliquota->hide();
  ui->labelSt->hide();
  ui->doubleSpinBoxSt->hide();

  ui->dateEditEvento->setDate(QDate::currentDate());
  ui->dateEditProximo->setDate(QDate::currentDate());
  ui->dateEditPgtSt->setDate(QDate::currentDate());

  if (tipo == Tipo::ConfirmarCompra) {
    ui->frameData->show();
    ui->frameDataPreco->show();
    ui->frameFrete->show();
    ui->checkBoxMarcarTodos->show();

    ui->labelEvento->setText("Data confirmação:");
    ui->labelProximoEvento->setText("Data prevista faturamento:");

    ui->widgetPgts->show();
  }

  if (tipo == Tipo::Financeiro) {
    ui->frameDataPreco->show();
    ui->groupBoxFinanceiro->show();

    ui->frameAdicionais->hide();

    //    ui->table->setSelectionMode(QTableView::NoSelection);
  }

  setConnections();

  connect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, [=]() { montarFluxoCaixa(true); });

  show();
}

InputDialogFinanceiro::~InputDialogFinanceiro() { delete ui; }

void InputDialogFinanceiro::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled, connectionType);
  connect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogFinanceiro::on_comboBoxST_currentTextChanged, connectionType);
  connect(ui->dateEditEvento, &QDateTimeEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged, connectionType);
  connect(ui->dateEditPgtSt, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditPgtSt_dateChanged, connectionType);
  connect(ui->doubleSpinBoxAliquota, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged, connectionType);
  connect(ui->doubleSpinBoxFrete, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged, connectionType);
  connect(ui->doubleSpinBoxSt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged, connectionType);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked, connectionType);
  connect(modelPedidoFornecedor.proxyModel, &QAbstractProxyModel::dataChanged, this, &InputDialogFinanceiro::updateTableData, connectionType);

  if (tipo == Tipo::ConfirmarCompra) { connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::calcularTotal, connectionType); }
}

void InputDialogFinanceiro::unsetConnections() {
  disconnect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled);
  disconnect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogFinanceiro::on_comboBoxST_currentTextChanged);
  disconnect(ui->dateEditEvento, &QDateTimeEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged);
  disconnect(ui->dateEditPgtSt, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditPgtSt_dateChanged);
  disconnect(ui->doubleSpinBoxAliquota, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxSt, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked);
  disconnect(modelPedidoFornecedor.proxyModel, &QAbstractProxyModel::dataChanged, this, &InputDialogFinanceiro::updateTableData);

  if (tipo == Tipo::ConfirmarCompra) { disconnect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::calcularTotal); }
}

void InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged(const double aliquota) {
  // FIXME: porcentagem esta limitado a 20% mas valor nao

  unsetConnections();

  [&] {
    double total = 0;

    const auto list = ui->table->selectionModel()->selectedRows();

    for (const auto &index : list) {
      if (not modelPedidoFornecedor.setData(index.row(), "aliquotaSt", aliquota)) { return; }

      total += modelPedidoFornecedor.data(index.row(), "preco").toDouble();
    }

    const double valueSt = total * (aliquota / 100);

    ui->doubleSpinBoxSt->setValue(valueSt);

    // TODO: adicionar frete/adicionais
    ui->doubleSpinBoxTotal->setValue(total);
  }();

  setConnections();

  montarFluxoCaixa();
}

QDateTime InputDialogFinanceiro::getDate() const { return ui->dateEditEvento->dateTime(); }

QDateTime InputDialogFinanceiro::getNextDate() const { return ui->dateEditProximo->dateTime(); }

void InputDialogFinanceiro::setupTables() {
  modelPedidoFornecedor.setTable("pedido_fornecedor_has_produto");

  modelPedidoFornecedor.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  modelPedidoFornecedor.setHeaderData("idVenda", "Código");
  modelPedidoFornecedor.setHeaderData("fornecedor", "Fornecedor");
  modelPedidoFornecedor.setHeaderData("descricao", "Produto");
  modelPedidoFornecedor.setHeaderData("colecao", "Coleção");
  modelPedidoFornecedor.setHeaderData("caixas", "Caixas");
  modelPedidoFornecedor.setHeaderData("prcUnitario", "$ Unit.");
  modelPedidoFornecedor.setHeaderData("quant", "Quant.");
  modelPedidoFornecedor.setHeaderData("preco", "Total");
  modelPedidoFornecedor.setHeaderData("un", "Un.");
  modelPedidoFornecedor.setHeaderData("un2", "Un.2");
  modelPedidoFornecedor.setHeaderData("kgcx", "Kg./Cx.");
  modelPedidoFornecedor.setHeaderData("formComercial", "Formato");
  modelPedidoFornecedor.setHeaderData("codComercial", "Cód. Com.");
  modelPedidoFornecedor.setHeaderData("obs", "Obs.");
  modelPedidoFornecedor.setHeaderData("aliquotaSt", "Alíquota ST");
  modelPedidoFornecedor.setHeaderData("st", "ST");
  modelPedidoFornecedor.setHeaderData("status", "Status");

  modelPedidoFornecedor.proxyModel = new SortFilterProxyModel(&modelPedidoFornecedor, this);

  ui->table->setModel(&modelPedidoFornecedor);

  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("idVendaProduto");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("quantConsumida");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("idPedido");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("codBarras");
  ui->table->hideColumn("idCompra");
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

  ui->table->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(this));
  ui->table->setItemDelegateForColumn("st", new ComboBoxDelegate(ComboBoxDelegate::Tipo::ST, this));
  ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("quant", new EditDelegate(this));

  //--------------------------------------------------

  modelFluxoCaixa.setTable("conta_a_pagar_has_pagamento");

  modelFluxoCaixa.setHeaderData("tipo", "Tipo");
  modelFluxoCaixa.setHeaderData("parcela", "Parcela");
  modelFluxoCaixa.setHeaderData("valor", "R$");
  modelFluxoCaixa.setHeaderData("dataPagamento", "Data");
  modelFluxoCaixa.setHeaderData("observacao", "Obs.");
  modelFluxoCaixa.setHeaderData("status", "Status");

  ui->tableFluxoCaixa->setModel(&modelFluxoCaixa);

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
  ui->tableFluxoCaixa->hideColumn("desativado");

  ui->tableFluxoCaixa->setItemDelegate(new DoubleDelegate(this));

  ui->tableFluxoCaixa->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void InputDialogFinanceiro::montarFluxoCaixa(const bool updateDate) {
  unsetConnections();

  [=]() {
    if (representacao) { return; }

    modelFluxoCaixa.revertAll();

    if (tipo == Tipo::Financeiro) {
      for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
        if (not modelFluxoCaixa.setData(row, "status", "SUBSTITUIDO")) { return; }
      }
    }

    for (int i = 0; i < ui->widgetPgts->listComboData.size(); ++i) {
      if (ui->widgetPgts->listComboPgt.at(i)->currentText() != "Escolha uma opção!") {
        const int parcelas = ui->widgetPgts->listComboParc.at(i)->currentIndex() + 1;
        const double valor = ui->widgetPgts->listDoubleSpinPgt.at(i)->value();

        const double part1 = valor / parcelas;
        const int part2 = static_cast<int>(part1 * 100);
        const double part3 = static_cast<double>(part2) / 100;

        const double parcela = static_cast<double>(ui->widgetPgts->listComboPgt.at(i)->currentText() == "Cartão de crédito" ? part3 : qRound(valor / parcelas * 100) / 100.);
        const double resto = static_cast<double>(valor - (parcela * parcelas));

        for (int x = 0, y = parcelas - 1; x < parcelas; ++x, --y) {
          const int row = modelFluxoCaixa.insertRowAtEnd();

          if (not modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor.data(0, "fornecedor"))) { return; }
          if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) { return; }
          if (not modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor.data(0, "idCompra"))) { return; }
          if (not modelFluxoCaixa.setData(row, "idLoja", 1)) { return; } // Geral

          const QString currentText = ui->widgetPgts->listComboData.at(i)->currentText();
          const QDate currentDate = ui->widgetPgts->listDatePgt.at(i)->date();
          const QDate dataPgt = (currentText == "Data + 1 Mês" ? currentDate.addMonths(x) : currentText == "Data Mês" ? currentDate.addMonths(x) : currentDate.addDays(currentText.toInt() * (x)));

          if (not modelFluxoCaixa.setData(row, "dataPagamento", dataPgt)) { return; }
          if (not modelFluxoCaixa.setData(row, "valor", parcela + (x == 0 ? resto : 0))) { return; }
          if (not modelFluxoCaixa.setData(row, "tipo", QString::number(i + 1) + ". " + ui->widgetPgts->listComboPgt.at(i)->currentText())) { return; }
          if (not modelFluxoCaixa.setData(row, "parcela", parcelas - y)) { return; }
          if (not modelFluxoCaixa.setData(row, "observacao", ui->widgetPgts->listLinePgt.at(i)->text())) { return; }
        }
      }
    }

    if (ui->doubleSpinBoxFrete->value() > 0) {
      const int row = modelFluxoCaixa.insertRowAtEnd();

      if (not modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor.data(0, "fornecedor"))) { return; }
      if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) { return; }
      if (not modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor.data(0, "idCompra"))) { return; }
      if (not modelFluxoCaixa.setData(row, "idLoja", 1)) { return; }                           // Geral
      if (not modelFluxoCaixa.setData(row, "dataPagamento", QDate::currentDate())) { return; } // TODO: 5redo this with a editable date
      if (not modelFluxoCaixa.setData(row, "valor", ui->doubleSpinBoxFrete->value())) { return; }
      if (not modelFluxoCaixa.setData(row, "tipo", "Frete")) { return; }
      if (not modelFluxoCaixa.setData(row, "parcela", 1)) { return; }
      if (not modelFluxoCaixa.setData(row, "observacao", "")) { return; }
    }

    // set st date
    if (updateDate) {
      if (not ui->widgetPgts->listDatePgt.isEmpty()) { ui->dateEditPgtSt->setDate(ui->widgetPgts->listDatePgt.at(0)->date()); }
    }

    //----------------------------------------------

    double stForn = 0;
    double stLoja = 0;

    const auto list = ui->table->selectionModel()->selectedRows();

    for (const auto &item : list) {
      const QString tipoSt = modelPedidoFornecedor.data(item.row(), "st").toString();

      if (tipoSt == "Sem ST") { continue; }

      const double aliquotaSt = modelPedidoFornecedor.data(item.row(), "aliquotaSt").toDouble();
      const double preco = modelPedidoFornecedor.data(item.row(), "preco").toDouble();
      const double valorSt = preco * (aliquotaSt / 100);

      if (tipoSt == "ST Fornecedor") { stForn += valorSt; }
      if (tipoSt == "ST Loja") { stLoja += valorSt; }
    }

    // TODO: 'ST Loja' tem a data do faturamento, 'ST Fornecedor' segue as datas dos pagamentos

    if (stForn > 0) {
      const int row = modelFluxoCaixa.insertRowAtEnd();

      if (not modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor.data(0, "fornecedor"))) { return; }
      if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) { return; }
      if (not modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor.data(0, "idCompra"))) { return; }
      if (not modelFluxoCaixa.setData(row, "idLoja", 1)) { return; } // Geral
      if (not modelFluxoCaixa.setData(row, "dataPagamento", ui->dateEditPgtSt->date())) { return; }
      if (not modelFluxoCaixa.setData(row, "valor", stForn)) { return; }
      if (not modelFluxoCaixa.setData(row, "tipo", "ST Fornecedor")) { return; }
      if (not modelFluxoCaixa.setData(row, "parcela", 1)) { return; }
      if (not modelFluxoCaixa.setData(row, "observacao", "")) { return; }
    }

    if (stLoja > 0) {
      const int row = modelFluxoCaixa.insertRowAtEnd();

      if (not modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor.data(0, "fornecedor"))) { return; }
      if (not modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->dateTime())) { return; }
      if (not modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor.data(0, "idCompra"))) { return; }
      if (not modelFluxoCaixa.setData(row, "idLoja", 1)) { return; } // Geral
      if (not modelFluxoCaixa.setData(row, "dataPagamento", ui->dateEditPgtSt->date())) { return; }
      if (not modelFluxoCaixa.setData(row, "valor", stLoja)) { return; }
      if (not modelFluxoCaixa.setData(row, "tipo", "ST Loja")) { return; }
      if (not modelFluxoCaixa.setData(row, "parcela", 1)) { return; }
      if (not modelFluxoCaixa.setData(row, "observacao", "")) { return; }
    }
  }();

  setConnections();
}

void InputDialogFinanceiro::calcularTotal() {
  unsetConnections();

  [=] {
    double total = 0;

    const auto list = ui->table->selectionModel()->selectedRows();

    if (tipo == Tipo::ConfirmarCompra) {
      for (const auto &index : list) { total += modelPedidoFornecedor.data(index.row(), "preco").toDouble(); }
    }

    if (tipo == Tipo::Financeiro) {
      for (int row = 0; row < modelPedidoFornecedor.rowCount(); ++row) { total += modelPedidoFornecedor.data(row, "preco").toDouble(); }
    }

    ui->doubleSpinBoxTotal->setValue(total);
    ui->widgetPgts->setTotal(total);
  }();

  setConnections();
}

void InputDialogFinanceiro::updateTableData(const QModelIndex &topLeft) {
  unsetConnections();

  [&] {
    const QString header = modelPedidoFornecedor.headerData(topLeft.column(), Qt::Horizontal).toString();
    const int row = topLeft.row();

    if (header == "Quant." or header == "$ Unit.") {
      const double preco = modelPedidoFornecedor.data(row, "quant").toDouble() * modelPedidoFornecedor.data(row, "prcUnitario").toDouble();
      if (not modelPedidoFornecedor.setData(row, "preco", preco)) { return; }
    }

    if (header == "Total") {
      const double preco = modelPedidoFornecedor.data(row, "preco").toDouble() / modelPedidoFornecedor.data(row, "quant").toDouble();
      if (not modelPedidoFornecedor.setData(row, "prcUnitario", preco)) { return; }
    }
  }();

  setConnections();

  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

bool InputDialogFinanceiro::setFilter(const QString &idCompra) {
  if (idCompra.isEmpty()) { return qApp->enqueueError(false, "IdCompra vazio!", this); }

  if (tipo == Tipo::ConfirmarCompra) { modelPedidoFornecedor.setFilter("idCompra = " + idCompra + " AND status = 'EM COMPRA'"); }
  if (tipo == Tipo::Financeiro) { modelPedidoFornecedor.setFilter("idCompra = " + idCompra); }

  if (not modelPedidoFornecedor.select()) { return false; }

  if (tipo == Tipo::ConfirmarCompra or tipo == Tipo::Financeiro) {
    modelFluxoCaixa.setFilter(tipo == Tipo::ConfirmarCompra ? "0" : "idCompra = " + idCompra + " AND status NOT IN ('CANCELADO', 'SUBSTITUIDO')");

    if (not modelFluxoCaixa.select()) { return false; }

    ui->checkBoxMarcarTodos->setChecked(true);
  }

  if (tipo == Tipo::Financeiro) {
    ui->comboBoxFinanceiro->setCurrentText(modelPedidoFornecedor.data(0, "statusFinanceiro").toString());
    ui->dateEditEvento->setDate(modelFluxoCaixa.data(0, "dataEmissao").toDate());
  }

  QSqlQuery query;
  query.prepare("SELECT v.representacao FROM pedido_fornecedor_has_produto pf LEFT JOIN venda v ON pf.idVenda = v.idVenda WHERE idCompra = :idCompra");
  query.bindValue(":idCompra", idCompra);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando se é representacao: " + query.lastError().text(), this); }

  representacao = query.value("representacao").toBool();

  if (representacao) { ui->framePagamentos->hide(); }

  ui->widgetPgts->setRepresentacao(representacao);

  setWindowTitle("OC: " + modelPedidoFornecedor.data(0, "ordemCompra").toString());

  // -------------------------------------------------------------------------

  calcularTotal();

  return true;
}

void InputDialogFinanceiro::on_pushButtonSalvar_clicked() {
  unsetConnections();

  [=] {
    if (not verifyFields()) { return; }

    if (not qApp->startTransaction()) { return; }

    if (not cadastrar()) { return qApp->rollbackTransaction(); }

    if (not qApp->endTransaction()) { return; }

    qApp->enqueueInformation("Dados salvos com sucesso!", this);

    QDialog::accept();
    close();
  }();

  setConnections();
}

bool InputDialogFinanceiro::verifyFields() {
  // TODO: implementar outras verificacoes necessarias

  if (ui->widgetPgts->isHidden()) { return true; }

  if (ui->table->selectionModel()->selectedRows().isEmpty()) { return qApp->enqueueError(false, "Nenhum item selecionado!", this); }

  if (not representacao) {
    if (not qFuzzyCompare(ui->doubleSpinBoxTotal->value(), ui->widgetPgts->getTotalPag())) { return qApp->enqueueError(false, "Soma dos pagamentos difere do total! Favor verificar!", this); }

    if (not ui->widgetPgts->verifyFields()) { return false; }
  }

  return true;
}

bool InputDialogFinanceiro::cadastrar() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (tipo == Tipo::ConfirmarCompra) {
    for (const auto &item : list) {
      // REFAC: setData already qApp->enqueueError internally, just set a return false everywhere
      if (not modelPedidoFornecedor.setData(item.row(), "selecionado", true)) { return false; }
    }
  }

  if (tipo == Tipo::Financeiro) {
    for (const auto &item : list) {
      if (not modelPedidoFornecedor.setData(item.row(), "statusFinanceiro", ui->comboBoxFinanceiro->currentText())) { return false; }
    }
  }

  if (not modelPedidoFornecedor.submitAll()) { return false; }

  if (not modelFluxoCaixa.submitAll()) { return false; }

  return true;
}

void InputDialogFinanceiro::on_dateEditEvento_dateChanged(const QDate &date) {
  if (ui->dateEditProximo->date() < date) { ui->dateEditProximo->setDate(date); }
}

void InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked() {
  ui->frameAdicionais->show();
  //  ui->framePgtTotal->show();
  //  ui->pushButtonAdicionarPagamento->show();
  ui->widgetPgts->show();

  // TODO: 5alterar para que apenas na tela do financeiro compra a opcao de corrigir fluxo percorra todas as linhas
  // (enquanto na confirmacao de pagamento percorre apenas as linhas selecionadas)

  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

void InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged(double) {
  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

void InputDialogFinanceiro::on_dateEditPgtSt_dateChanged(const QDate &) { montarFluxoCaixa(false); }

void InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged(const double valueSt) {
  unsetConnections();

  [=] {
    double total = 0;

    const auto list = ui->table->selectionModel()->selectedRows();

    for (const auto &item : list) { total += modelPedidoFornecedor.data(item.row(), "preco").toDouble(); }

    const double aliquota = valueSt * 100 / total;

    ui->doubleSpinBoxAliquota->setValue(aliquota);

    for (const auto &item : list) {
      if (not modelPedidoFornecedor.setData(item.row(), "aliquotaSt", aliquota)) { return; }
    }

    ui->doubleSpinBoxTotal->setValue(total);
  }();

  setConnections();

  montarFluxoCaixa();
}

void InputDialogFinanceiro::on_comboBoxST_currentTextChanged(const QString &text) {
  unsetConnections();

  [=]() {
    if (text == "Sem ST") {
      ui->doubleSpinBoxSt->setValue(0);
      ui->doubleSpinBoxAliquota->setValue(0);

      ui->labelAliquota->hide();
      ui->doubleSpinBoxAliquota->hide();
      ui->labelSt->hide();
      ui->doubleSpinBoxSt->hide();
    }

    if (text == "ST Fornecedor" or text == "ST Loja") {
      ui->doubleSpinBoxAliquota->setValue(4.68);

      ui->labelAliquota->show();
      ui->doubleSpinBoxAliquota->show();
      ui->labelSt->show();
      ui->doubleSpinBoxSt->show();
      ui->dateEditPgtSt->show();
    }

    const auto list = ui->table->selectionModel()->selectedRows();

    for (const auto &index : list) {
      if (not modelPedidoFornecedor.setData(index.row(), "st", text)) { return; }
      if (not modelPedidoFornecedor.setData(index.row(), "aliquotaSt", ui->doubleSpinBoxAliquota->value())) { return; }
    }
  }();

  setConnections();

  montarFluxoCaixa();
}

// TODO: [Conrado] copiar de venda as verificacoes/terminar o codigo dos pagamentos
// REFAC: refatorar o frame pagamentos para um widget para nao duplicar codigo

// TODO: 1quando for confirmacao de representacao perguntar qual o id para colocar na observacao das comissoes (codigo
// que vem do fornecedor)
// TODO: 3quando for representacao mostrar fluxo de comissao
// TODO: 3colocar possibilidade de ajustar valor total para as compras (contabilizar quanto de ajuste foi feito)
// TODO: gerar lancamentos da st por produto
// TODO: colocar checkbox para dizer se a ST vai ser na data do primeiro pagamento ou vai ser dividida junto com as parcelas
