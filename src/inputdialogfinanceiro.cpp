#include "inputdialogfinanceiro.h"
#include "ui_inputdialogfinanceiro.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "editdelegate.h"
#include "noeditdelegate.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"

#include <QDebug>
#include <QLineEdit>
#include <QMessageBox>
#include <QSqlError>

InputDialogFinanceiro::InputDialogFinanceiro(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::InputDialogFinanceiro) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  ui->widgetPgts->setTipo(WidgetPagamentos::Tipo::Compra);

  ui->widgetPgts->hide();

  ui->frameData->hide();
  ui->frameDataPreco->hide();
  ui->groupBoxFinanceiro->hide();

  ui->labelAliquota->hide();
  ui->doubleSpinBoxAliquota->hide();
  ui->labelSt->hide();
  ui->doubleSpinBoxSt->hide();

  ui->dateEditEvento->setDate(qApp->serverDate());
  ui->dateEditProximo->setDate(qApp->serverDate());

  ui->treeView->hide();

  if (tipo == Tipo::ConfirmarCompra) {
    ui->frameData->show();
    ui->frameDataPreco->show();
    ui->groupBoxFrete->show();

    ui->labelEvento->setText("Data confirmação:");
    ui->labelProximoEvento->setText("Data prevista faturamento:");

    ui->widgetPgts->show();

    ui->tableFluxoCaixa->setSelectionMode(QTableView::NoSelection);
  }

  if (tipo == Tipo::Financeiro) {
    ui->frameDataPreco->show();
    ui->groupBoxFinanceiro->show();

    ui->frameAdicionais->hide();

    ui->tableFluxoCaixa->setSelectionMode(QTableView::MultiSelection);
  }

  if (tipo == Tipo::Historico) {
    ui->frameDataPreco->show();

    ui->frameAdicionais->hide();
    ui->framePagamentos->hide();
    ui->checkBoxMarcarTodos->hide();
    ui->pushButtonSalvar->hide();

    ui->tableFluxoCaixa->setSelectionMode(QTableView::NoSelection);
  }

  setConnections();

  showMaximized();
}

InputDialogFinanceiro::~InputDialogFinanceiro() { delete ui; }

void InputDialogFinanceiro::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled, connectionType);
  connect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogFinanceiro::on_comboBoxST_currentTextChanged, connectionType);
  connect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged, connectionType);
  connect(ui->doubleSpinBoxAliquota, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged, connectionType);
  connect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged, connectionType);
  connect(ui->doubleSpinBoxSt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged, connectionType);
  connect(ui->lineEditCodFornecedor, &QLineEdit::textChanged, this, &InputDialogFinanceiro::setCodFornecedor, connectionType);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogFinanceiro::updateTableData, connectionType);
  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::on_table_selectionChanged, connectionType);
  connect(ui->tableFluxoCaixa->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::calcularTotal, connectionType);
  connect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, this, &InputDialogFinanceiro::montarFluxoCaixa, connectionType);
}

void InputDialogFinanceiro::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxMarcarTodos, &QCheckBox::toggled, this, &InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled);
  disconnect(ui->comboBoxST, &QComboBox::currentTextChanged, this, &InputDialogFinanceiro::on_comboBoxST_currentTextChanged);
  disconnect(ui->dateEditEvento, &QDateEdit::dateChanged, this, &InputDialogFinanceiro::on_dateEditEvento_dateChanged);
  disconnect(ui->doubleSpinBoxAliquota, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxSt, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged);
  disconnect(ui->lineEditCodFornecedor, &QLineEdit::textChanged, this, &InputDialogFinanceiro::setCodFornecedor);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &InputDialogFinanceiro::on_pushButtonSalvar_clicked);
  disconnect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &InputDialogFinanceiro::updateTableData);
  disconnect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::on_table_selectionChanged);
  disconnect(ui->tableFluxoCaixa->selectionModel(), &QItemSelectionModel::selectionChanged, this, &InputDialogFinanceiro::calcularTotal);
  disconnect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, this, &InputDialogFinanceiro::montarFluxoCaixa);
}

void InputDialogFinanceiro::on_doubleSpinBoxAliquota_valueChanged(const double aliquota) {
  unsetConnections();

  try {
    double total = 0;

    const auto selection = ui->table->selectionModel()->selectedRows();

    for (const auto &index : selection) {
      const int row = index.row();

      modelPedidoFornecedor2.setData(row, "aliquotaSt", aliquota);

      total += modelPedidoFornecedor2.data(row, "preco").toDouble();
    }

    const double valueSt = total * (aliquota / 100);

    ui->doubleSpinBoxSt->setValue(valueSt);

    calcularTotal();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

QDate InputDialogFinanceiro::getDate() const { return ui->dateEditEvento->date(); }

QDate InputDialogFinanceiro::getNextDate() const { return ui->dateEditProximo->date(); }

void InputDialogFinanceiro::setupTables() {
  // TODO: montar TreeView pf/pf2 (pelo menos no historico)

  modelPedidoFornecedor.setTable("pedido_fornecedor_has_produto");

  //--------------------------------------------------

  modelPedidoFornecedor2.setTable("pedido_fornecedor_has_produto2");

  if (tipo == Tipo::Financeiro or tipo == Tipo::Historico) {
    modelPedidoFornecedor2.setHeaderData("status", "Status");
    modelPedidoFornecedor2.setHeaderData("ordemRepresentacao", "Cód. Rep.");
    modelPedidoFornecedor2.setHeaderData("codFornecedor", "Cód. Forn.");
  }

  modelPedidoFornecedor2.setHeaderData("aliquotaSt", "Alíquota ST");
  modelPedidoFornecedor2.setHeaderData("st", "ST");
  modelPedidoFornecedor2.setHeaderData("idVenda", "Venda");
  modelPedidoFornecedor2.setHeaderData("fornecedor", "Fornecedor");
  modelPedidoFornecedor2.setHeaderData("descricao", "Produto");
  modelPedidoFornecedor2.setHeaderData("obs", "Obs.");
  modelPedidoFornecedor2.setHeaderData("colecao", "Coleção");
  modelPedidoFornecedor2.setHeaderData("codComercial", "Cód. Com.");
  modelPedidoFornecedor2.setHeaderData("quant", "Quant.");
  modelPedidoFornecedor2.setHeaderData("un", "Un.");
  modelPedidoFornecedor2.setHeaderData("caixas", "Caixas");
  modelPedidoFornecedor2.setHeaderData("prcUnitario", "R$ Unit.");
  modelPedidoFornecedor2.setHeaderData("preco", "Total");

  modelPedidoFornecedor2.proxyModel = new SortFilterProxyModel(&modelPedidoFornecedor2, this);

  ui->table->setModel(&modelPedidoFornecedor2);

  if (tipo == Tipo::ConfirmarCompra) {
    ui->table->hideColumn("status");
    ui->table->hideColumn("ordemRepresentacao");
    ui->table->hideColumn("codFornecedor");
  }

  ui->table->hideColumn("idPedido2");
  ui->table->hideColumn("idPedidoFK");
  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("idVendaProduto1");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("un2");
  ui->table->hideColumn("kgcx");
  ui->table->hideColumn("formComercial");
  ui->table->hideColumn("codBarras");
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

  ui->table->setItemDelegateForColumn("obs", new EditDelegate(this));
  ui->table->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(false, this));
  ui->table->setItemDelegateForColumn("st", new ComboBoxDelegate(ComboBoxDelegate::Tipo::ST, this));
  ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  //--------------------------------------------------

  modelFluxoCaixa.setTable("conta_a_pagar_has_pagamento");

  modelFluxoCaixa.setSort("dataPagamento");

  modelFluxoCaixa.setHeaderData("tipo", "Tipo");
  modelFluxoCaixa.setHeaderData("parcela", "Parcela");
  modelFluxoCaixa.setHeaderData("valor", "R$");
  modelFluxoCaixa.setHeaderData("dataPagamento", "Data");
  modelFluxoCaixa.setHeaderData("observacao", "Obs.");
  modelFluxoCaixa.setHeaderData("status", "Status");

  modelFluxoCaixa.proxyModel = new SortFilterProxyModel(&modelFluxoCaixa, this);

  ui->tableFluxoCaixa->setModel(&modelFluxoCaixa);

  ui->tableFluxoCaixa->hideColumn("idPagamento");
  ui->tableFluxoCaixa->hideColumn("dataEmissao");
  ui->tableFluxoCaixa->hideColumn("idCompra");
  ui->tableFluxoCaixa->hideColumn("idVenda");
  ui->tableFluxoCaixa->hideColumn("idLoja");
  ui->tableFluxoCaixa->hideColumn("contraParte");
  ui->tableFluxoCaixa->hideColumn("idNFe");
  ui->tableFluxoCaixa->hideColumn("idCnab");
  ui->tableFluxoCaixa->hideColumn("nfe");
  ui->tableFluxoCaixa->hideColumn("dataRealizado");
  ui->tableFluxoCaixa->hideColumn("valorReal");
  ui->tableFluxoCaixa->hideColumn("tipoReal");
  ui->tableFluxoCaixa->hideColumn("parcelaReal");
  ui->tableFluxoCaixa->hideColumn("idConta");
  ui->tableFluxoCaixa->hideColumn("tipoDet");
  ui->tableFluxoCaixa->hideColumn("centroCusto");
  ui->tableFluxoCaixa->hideColumn("grupo");
  ui->tableFluxoCaixa->hideColumn("subGrupo");
  ui->tableFluxoCaixa->hideColumn("desativado");

  ui->tableFluxoCaixa->setItemDelegate(new NoEditDelegate(this));

  ui->tableFluxoCaixa->setItemDelegateForColumn("valor", new ReaisDelegate(2, true, this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("dataPagamento", new EditDelegate(this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("observacao", new EditDelegate(this));
}

void InputDialogFinanceiro::montarFluxoCaixa() {
  qDebug() << "montarFluxoCaixa";

  unsetConnections();

  try {
    [=, this] {
      if (representacao) { return; }
      if (not ui->widgetPgts->isVisible()) { return; }

      modelFluxoCaixa.revertAll();

      if (tipo == Tipo::Financeiro) {
        const auto selection = ui->tableFluxoCaixa->selectionModel()->selectedRows();

        for (const auto &index : selection) { modelFluxoCaixa.setData(index.row(), "status", "SUBSTITUIDO"); }
      }

      for (auto *pgt : qAsConst(ui->widgetPgts->pagamentos)) { processarPagamento(pgt); }

      if (ui->widgetPgts->pgtFrete) { processarPagamento(ui->widgetPgts->pgtFrete); }

      if (ui->widgetPgts->pgtSt) { processarPagamento(ui->widgetPgts->pgtSt); }
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogFinanceiro::calcularTotal() {
  unsetConnections();

  try {
    auto *table = [&] {
      if (tipo == Tipo::ConfirmarCompra) { return ui->table; }
      if (tipo == Tipo::Financeiro) { return ui->tableFluxoCaixa; }

      throw RuntimeException("Tipo Nulo!");
    }();

    auto *model = [&] {
      if (tipo == Tipo::ConfirmarCompra) { return &modelPedidoFornecedor2; }
      if (tipo == Tipo::Financeiro) { return &modelFluxoCaixa; }

      throw RuntimeException("Tipo Nulo!");
    }();

    const QString coluna = [&] {
      if (tipo == Tipo::ConfirmarCompra) { return "preco"; }
      if (tipo == Tipo::Financeiro) { return "valor"; }

      throw RuntimeException("Tipo Nulo!");
    }();

    ui->widgetPgts->setFrete(ui->doubleSpinBoxFrete->value());

    // -------------------------------

    double total = 0;

    const auto selection = table->selectionModel()->selectedRows();

    for (const auto &index : selection) { total += qApp->roundDouble(model->data(index.row(), coluna).toDouble(), 2); }

    selectedTotal = total;
    ui->widgetPgts->setTotal(selectedTotal);

    // -------------------------------

    double totalSt = 0;

    const auto selection2 = ui->table->selectionModel()->selectedRows();

    for (const auto &index : selection2) {
      const int row = index.row();
      const QString tipoSt = modelPedidoFornecedor2.data(row, "st").toString();
      const double aliquotaSt = modelPedidoFornecedor2.data(row, "aliquotaSt").toDouble();
      const double preco = modelPedidoFornecedor2.data(row, "preco").toDouble();
      if (tipoSt == "ST FORNECEDOR") { totalSt += aliquotaSt / 100 * preco; }
    }

    ui->widgetPgts->setST(totalSt);

    // -------------------------------

    total += ui->doubleSpinBoxFrete->value();
    total += totalSt;

    ui->doubleSpinBoxTotal->setValue(total);

    // -------------------------------

    montarFluxoCaixa();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogFinanceiro::atualizaPrecosPF1(const int rowPF2) {
  const auto match = modelPedidoFornecedor.match("idPedido1", modelPedidoFornecedor2.data(rowPF2, "idPedidoFK"), 1, Qt::MatchExactly);

  if (match.isEmpty()) { throw RuntimeException("Erro atualizando valores na linha mãe!", this); }

  const int rowPF1 = match.first().row();

  const auto match2 = modelPedidoFornecedor2.match("idPedidoFK", modelPedidoFornecedor2.data(rowPF2, "idPedidoFK"), -1, Qt::MatchExactly);

  double newPreco = 0;

  for (auto index2 : match2) { newPreco += modelPedidoFornecedor2.data(index2.row(), "preco").toDouble(); }

  double newPrcUnit = newPreco / modelPedidoFornecedor.data(rowPF1, "quant").toDouble();

  modelPedidoFornecedor.setData(rowPF1, "prcUnitario", newPrcUnit);
  modelPedidoFornecedor.setData(rowPF1, "preco", newPreco);
}

void InputDialogFinanceiro::updateTableData(const QModelIndex &topLeft) {
  unsetConnections();

  try {
    const QString header = modelPedidoFornecedor2.headerData(topLeft.column(), Qt::Horizontal).toString();
    const int rowPF2 = topLeft.row();

    const double quant = modelPedidoFornecedor2.data(rowPF2, "quant").toDouble();

    if (header == "Quant." or header == "R$ Unit.") {
      const double prcUnitario = modelPedidoFornecedor2.data(rowPF2, "prcUnitario").toDouble();
      const double preco = quant * prcUnitario;

      modelPedidoFornecedor2.setData(rowPF2, "preco", preco);
    }

    if (header == "Total") {
      const double preco = modelPedidoFornecedor2.data(rowPF2, "preco").toDouble();
      const double prcUnitario = preco / quant;

      modelPedidoFornecedor2.setData(rowPF2, "prcUnitario", prcUnitario);
    }

    if (header == "Quant." or header == "R$ Unit." or header == "Total") { atualizaPrecosPF1(rowPF2); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  calcularTotal();
  ui->widgetPgts->resetarPagamentos();
}

// void InputDialogFinanceiro::setTreeView() {
//  modelTree.appendModel(&modelPedidoFornecedor);
//  modelTree.appendModel(&modelPedidoFornecedor2);

//  modelTree.updateData();

//  modelTree.setHeaderData("aliquotaSt", "Alíquota ST");
//  modelTree.setHeaderData("st", "ST");
//  modelTree.setHeaderData("status", "Status");
//  modelTree.setHeaderData("ordemRepresentacao", "Cód. Rep.");
//  modelTree.setHeaderData("idVenda", "Código");
//  modelTree.setHeaderData("fornecedor", "Fornecedor");
//  modelTree.setHeaderData("descricao", "Produto");
//  modelTree.setHeaderData("obs", "Obs.");
//  modelTree.setHeaderData("colecao", "Coleção");
//  modelTree.setHeaderData("codComercial", "Cód. Com.");
//  modelTree.setHeaderData("quant", "Quant.");
//  modelTree.setHeaderData("un", "Un.");
//  modelTree.setHeaderData("un2", "Un.2");
//  modelTree.setHeaderData("caixas", "Caixas");
//  modelTree.setHeaderData("prcUnitario", "R$ Unit.");
//  modelTree.setHeaderData("preco", "Total");
//  modelTree.setHeaderData("kgcx", "Kg./Cx.");
//  modelTree.setHeaderData("formComercial", "Formato");

//  ui->treeView->setModel(&modelTree);

//  ui->treeView->hideColumn("idRelacionado");
//  ui->treeView->hideColumn("idPedido2");
//  ui->treeView->hideColumn("idPedidoFK");
//  ui->treeView->hideColumn("selecionado");
//  ui->treeView->hideColumn("statusFinanceiro");
//  ui->treeView->hideColumn("ordemCompra");
//  ui->treeView->hideColumn("idVendaProduto1");
//  ui->treeView->hideColumn("idVendaProduto2");
//  ui->treeView->hideColumn("idCompra");
//  ui->treeView->hideColumn("idProduto");
//  ui->treeView->hideColumn("quantUpd");
//  ui->treeView->hideColumn("codBarras");
//  ui->treeView->hideColumn("dataPrevCompra");
//  ui->treeView->hideColumn("dataRealCompra");
//  ui->treeView->hideColumn("dataPrevConf");
//  ui->treeView->hideColumn("dataRealConf");
//  ui->treeView->hideColumn("dataPrevFat");
//  ui->treeView->hideColumn("dataRealFat");
//  ui->treeView->hideColumn("dataPrevColeta");
//  ui->treeView->hideColumn("dataRealColeta");
//  ui->treeView->hideColumn("dataPrevReceb");
//  ui->treeView->hideColumn("dataRealReceb");
//  ui->treeView->hideColumn("dataPrevEnt");
//  ui->treeView->hideColumn("dataRealEnt");
//  ui->treeView->hideColumn("created");
//  ui->treeView->hideColumn("lastUpdated");

//  ui->treeView->setItemDelegate(new NoEditDelegate(this));

//  ui->treeView->setItemDelegateForColumn("aliquotaSt", new PorcentagemDelegate(false, this));
//  ui->treeView->setItemDelegateForColumn("st", new ComboBoxDelegate(ComboBoxDelegate::Tipo::ST, this));
//  ui->treeView->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
//  ui->treeView->setItemDelegateForColumn("preco", new ReaisDelegate(this));
//  ui->treeView->setItemDelegateForColumn("quant", new EditDelegate(this));
//}

void InputDialogFinanceiro::setFilter(const QString &ordemCompra) {
  if (ordemCompra.isEmpty()) { throw RuntimeException("IdCompra vazio!"); }

  QString filtro = "ordemCompra IN (" + ordemCompra + ")";

  if (tipo == Tipo::ConfirmarCompra) { filtro += " AND status = 'EM COMPRA'"; }

  modelPedidoFornecedor.setFilter(filtro);

  modelPedidoFornecedor.select();

  modelPedidoFornecedor2.setFilter(filtro);

  modelPedidoFornecedor2.select();

  //  setTreeView();

  if (tipo == Tipo::Financeiro) { modelFluxoCaixa.setFilter("idCompra IN (SELECT idCompra FROM pedido_fornecedor_has_produto WHERE ordemCompra = " + ordemCompra + ") AND desativado = FALSE"); }

  modelFluxoCaixa.select();

  if (tipo == Tipo::Financeiro) {
    ui->comboBoxFinanceiro->setCurrentText(modelPedidoFornecedor.data(0, "statusFinanceiro").toString());
    ui->dateEditEvento->setDate(modelFluxoCaixa.data(0, "dataEmissao").toDate());

    if (modelFluxoCaixa.rowCount() == 0) { ui->pushButtonCorrigirFluxo->setDisabled(true); }
  }

  SqlQuery query;
  query.prepare("SELECT v.representacao FROM pedido_fornecedor_has_produto pf LEFT JOIN venda v ON pf.idVenda = v.idVenda WHERE pf.ordemCompra = :ordemCompra");
  query.bindValue(":ordemCompra", ordemCompra);

  if (not query.exec()) { throw RuntimeException("Erro buscando se é representacao: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Erro buscando se é representacao!"); }

  representacao = query.value("representacao").toBool();

  if (representacao) {
    ui->framePagamentos->hide();
    ui->lineEditCodFornecedor->hide();

    if (tipo == Tipo::ConfirmarCompra) { ui->frameAdicionais->hide(); }
    if (tipo == Tipo::Financeiro) { ui->pushButtonSalvar->hide(); }
  }

  ui->widgetPgts->setRepresentacao(representacao);

  setWindowTitle("O.C.: " + modelPedidoFornecedor.data(0, "ordemCompra").toString());

  // -------------------------------------------------------------------------

  if (tipo == Tipo::ConfirmarCompra) { ui->checkBoxMarcarTodos->setChecked(true); }
}

void InputDialogFinanceiro::on_pushButtonSalvar_clicked() {
  unsetConnections();

  try {
    verifyFields();

    qApp->startTransaction("InputDialogFinanceiro::on_pushButtonSalvar");

    cadastrar();

    qApp->endTransaction();

    qApp->enqueueInformation("Dados salvos com sucesso!", this);

    QDialog::accept();
    close();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogFinanceiro::verifyFields() {
  // TODO: implementar outras verificacoes necessarias

  if (ui->widgetPgts->isHidden()) { return; }

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (tipo == Tipo::ConfirmarCompra and selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!"); }

  if (not representacao) {
    if (tipo == Tipo::ConfirmarCompra) {
      for (const auto &index : selection) {
        if (modelPedidoFornecedor2.data(index.row(), "codFornecedor").toString().isEmpty()) { throw RuntimeError("Não preencheu código do fornecedor!"); }
      }

      if (ui->widgetPgts->pagamentos.isEmpty()) {
        QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Sem pagamentos cadastrados, deseja continuar mesmo assim?", QMessageBox::Yes | QMessageBox::No, this);
        msgBox.button(QMessageBox::Yes)->setText("Continuar");
        msgBox.button(QMessageBox::No)->setText("Voltar");

        if (msgBox.exec() == QMessageBox::No) { throw std::exception(); }
      } else {
        const double total = ui->doubleSpinBoxTotal->value();
        const double pagamentos = ui->widgetPgts->getTotalPag();

        if (not qFuzzyCompare(total, pagamentos)) { throw RuntimeError("Soma dos pagamentos difere do total! Favor verificar!"); }

        ui->widgetPgts->verifyFields();
      }
    }

    if (tipo == Tipo::Financeiro) {
      if (not qFuzzyCompare(ui->doubleSpinBoxTotal->value(), ui->widgetPgts->getTotalPag())) { throw RuntimeError("Soma dos pagamentos difere do total! Favor verificar!"); }

      ui->widgetPgts->verifyFields();
    }
  }
}

void InputDialogFinanceiro::cadastrar() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (tipo == Tipo::ConfirmarCompra) {
    for (const auto &index : selection) { modelPedidoFornecedor2.setData(index.row(), "selecionado", true); }
  }

  if (tipo == Tipo::Financeiro) {
    for (const auto &index : selection) { modelPedidoFornecedor2.setData(index.row(), "statusFinanceiro", ui->comboBoxFinanceiro->currentText()); }
  }

  modelPedidoFornecedor.submitAll();

  modelPedidoFornecedor2.submitAll();

  modelFluxoCaixa.submitAll();
}

void InputDialogFinanceiro::on_dateEditEvento_dateChanged(const QDate date) {
  if (ui->dateEditProximo->date() < date) { ui->dateEditProximo->setDate(date); }
}

void InputDialogFinanceiro::on_checkBoxMarcarTodos_toggled(const bool checked) { checked ? ui->table->selectAll() : ui->table->clearSelection(); }

void InputDialogFinanceiro::on_pushButtonCorrigirFluxo_clicked() {
  const auto selection = ui->tableFluxoCaixa->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Selecione os pagamentos que serão substituídos!", this); }

  for (const auto &index : selection) {
    if (modelFluxoCaixa.data(index.row(), "status").toString() == "SUBSTITUIDO") { throw RuntimeError("Selecionado linhas já substituídas!", this); }
  }

  //--------------------------------------------------

  ui->pushButtonCorrigirFluxo->setDisabled(true);
  ui->tableFluxoCaixa->setDisabled(true);

  for (const auto &index : selection) { modelFluxoCaixa.setData(index.row(), "status", "SUBSTITUIDO"); }

  ui->frameAdicionais->show();
  ui->widgetPgts->show();
}

void InputDialogFinanceiro::on_doubleSpinBoxFrete_valueChanged() { calcularTotal(); }

void InputDialogFinanceiro::on_doubleSpinBoxSt_valueChanged(const double valueSt) {
  unsetConnections();

  try {
    double total = 0;

    const auto selection = ui->table->selectionModel()->selectedRows();

    for (const auto &index : selection) { total += modelPedidoFornecedor2.data(index.row(), "preco").toDouble(); }

    const double aliquota = valueSt * 100 / total;

    ui->doubleSpinBoxAliquota->setValue(aliquota);

    for (const auto &index : selection) { modelPedidoFornecedor2.setData(index.row(), "aliquotaSt", aliquota); }

    calcularTotal();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogFinanceiro::on_comboBoxST_currentTextChanged() {
  unsetConnections();

  try {
    const QString text = ui->comboBoxST->currentText();

    if (text == "Sem ST") {
      ui->doubleSpinBoxSt->setValue(0);
      ui->doubleSpinBoxAliquota->setValue(0);

      ui->labelAliquota->hide();
      ui->doubleSpinBoxAliquota->hide();
      ui->labelSt->hide();
      ui->doubleSpinBoxSt->hide();
    }

    if (text == "ST Fornecedor") {
      ui->doubleSpinBoxAliquota->setValue(4.68);

      ui->labelAliquota->show();
      ui->doubleSpinBoxAliquota->show();
      ui->labelSt->show();
      ui->doubleSpinBoxSt->show();
    }

    setMaximumST();

    on_doubleSpinBoxAliquota_valueChanged(ui->doubleSpinBoxAliquota->value());

    const auto selection = ui->table->selectionModel()->selectedRows();

    for (const auto &index : selection) {
      modelPedidoFornecedor2.setData(index.row(), "st", text);
      modelPedidoFornecedor2.setData(index.row(), "aliquotaSt", ui->doubleSpinBoxAliquota->value());
    }

    calcularTotal();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();

  montarFluxoCaixa();
}

void InputDialogFinanceiro::setMaximumST() {
  const double maximoST = selectedTotal * ui->doubleSpinBoxAliquota->maximum() / 100;

  ui->doubleSpinBoxSt->setMaximum(maximoST);
}

void InputDialogFinanceiro::setCodFornecedor() {
  unsetConnections();

  try {
    // clean codFornecedor on unselected rows
    for (int row = 0; row < modelPedidoFornecedor2.rowCount(); ++row) { modelPedidoFornecedor2.setData(row, "codFornecedor", {}); }

    //-----------------------------------------------

    // set again on selected rows
    const auto selection = ui->table->selectionModel()->selectedRows();

    for (const auto &index : selection) { modelPedidoFornecedor2.setData(index.row(), "codFornecedor", ui->lineEditCodFornecedor->text()); }
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void InputDialogFinanceiro::on_table_selectionChanged() {
  if (tipo == Tipo::ConfirmarCompra) {
    setCodFornecedor();
    calcularTotal();
    setMaximumST();
  }
}

void InputDialogFinanceiro::processarPagamento(Pagamento *pgt) {
  if (pgt->comboTipoPgt->currentText() == "ESCOLHA UMA OPÇÃO!") { return; }

  const QString tipoPgt = pgt->comboTipoPgt->currentText();
  const int parcelas = pgt->comboParcela->currentIndex() + 1;

  SqlQuery query;
  query.prepare("SELECT fp.idConta, fp.centavoSobressalente "
                "FROM forma_pagamento fp "
                "LEFT JOIN forma_pagamento_has_taxa fpt ON fp.idPagamento = fpt.idPagamento "
                "WHERE fp.pagamento = :pagamento AND fpt.parcela = :parcela");
  query.bindValue(":pagamento", tipoPgt);
  query.bindValue(":parcela", parcelas);

  if (not query.exec()) { throw RuntimeException("Erro buscando taxa: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para o pagamento: '" + tipoPgt + "'"); }

  const int idConta = query.value("idConta").toInt();
  const bool centavoPrimeiraParcela = query.value("centavoSobressalente").toBool();

  //-----------------------------------------------------------------

  const QString observacaoPgt = pgt->observacao->text();

  const double valor = pgt->valorPgt->value();
  const double valorParcela = qApp->roundDouble(valor / parcelas, 2);
  const double restoParcela = qApp->roundDouble(valor - (valorParcela * parcelas), 2);

  for (int parcela = 0; parcela < parcelas; ++parcela) {
    const int row = modelFluxoCaixa.insertRowAtEnd();

    modelFluxoCaixa.setData(row, "contraParte", modelPedidoFornecedor2.data(0, "fornecedor"));
    modelFluxoCaixa.setData(row, "dataEmissao", ui->dateEditEvento->date());
    modelFluxoCaixa.setData(row, "idCompra", modelPedidoFornecedor2.data(0, "idCompra"));
    // TODO: esse idLoja provavelmente está errado
    modelFluxoCaixa.setData(row, "idLoja", 1); // Geral

    QString tipoData = pgt->comboTipoData->currentText();
    QDate dataPgt = pgt->dataPgt->date();

    if (tipoData == "DATA + 1 MÊS" or tipoData == "DATA MÊS") {
      dataPgt = dataPgt.addMonths(parcela);
    } else {
      dataPgt = dataPgt.addDays(tipoData.toInt() * parcela);
    }

    dataPgt = qApp->ajustarDiaUtil(dataPgt);

    modelFluxoCaixa.setData(row, "dataPagamento", dataPgt);

    const bool primeiraParcela = (parcela == 0);
    const bool ultimaParcela = (parcela == parcelas - 1);
    double val = valorParcela;

    if ((centavoPrimeiraParcela and primeiraParcela) or (not centavoPrimeiraParcela and ultimaParcela)) { val += restoParcela; }

    modelFluxoCaixa.setData(row, "valor", val);
    // TODO: se for ST/FRETE preencher com ST/FRETE no tipo
    modelFluxoCaixa.setData(row, "tipo", QString::number(pgt->posicao) + ". " + tipoPgt);
    modelFluxoCaixa.setData(row, "parcela", parcela + 1);
    modelFluxoCaixa.setData(row, "observacao", observacaoPgt);
    modelFluxoCaixa.setData(row, "idConta", idConta);

    QString grupo;

    if (pgt->tipoPgt == Pagamento::TipoPgt::Normal) { grupo = "PRODUTOS - VENDA"; }
    if (pgt->tipoPgt == Pagamento::TipoPgt::Frete) { grupo = "LOGÍSTICA - FRETES"; }
    if (pgt->tipoPgt == Pagamento::TipoPgt::ST) { grupo = "IMPOSTOS - ICMS;ST;ISS"; }

    modelFluxoCaixa.setData(row, "grupo", grupo);
  }
}

// TODO: [Conrado] copiar de venda as verificacoes/terminar o codigo dos pagamentos

// TODO: 1quando for confirmacao de representacao perguntar qual o id para colocar na observacao das comissoes (codigo que vem do fornecedor)
// TODO: 3quando for representacao mostrar fluxo de comissao
// TODO: 3colocar possibilidade de ajustar valor total para as compras (contabilizar quanto de ajuste foi feito)
// TODO: gerar lancamentos da st por produto
// TODO: colocar um checkbox para dizer se mostra ou não os pagamentos substituidos/cancelados
