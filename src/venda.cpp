#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "cadastrocliente.h"
#include "checkboxdelegate.h"
#include "devolucao.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "estoque.h"
#include "excel.h"
#include "impressao.h"
#include "logindialog.h"
#include "noeditdelegate.h"
#include "orcamento.h"
#include "porcentagemdelegate.h"
#include "qtreeviewgriddelegate.h"
#include "reaisdelegate.h"
#include "ui_venda.h"
#include "usersession.h"
#include "venda.h"

Venda::Venda(QWidget *parent) : RegisterDialog("venda", "idVenda", parent), ui(new Ui::Venda) {
  ui->setupUi(this);

  const auto children = findChildren<QLineEdit *>();

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  setupTables();

  ui->widgetPgts->setTipo(WidgetPagamentos::Tipo::Venda);

  connect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, this, &Venda::montarFluxoCaixa);

  ui->itemBoxCliente->setRegisterDialog(new CadastroCliente(this));
  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxConsultor->setSearchDialog(SearchDialog::vendedor(this));
  ui->itemBoxEndereco->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxEnderecoFat->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(true, this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));

  setupMapper();
  newRegister();

  ui->labelConsultor->hide();
  ui->itemBoxConsultor->hide();
  ui->itemBoxConsultor->setReadOnlyItemBox(true);

  ui->groupBoxFinanceiro->hide();
  ui->tableFluxoCaixa2->hide();

  for (auto &item : ui->frameRT->findChildren<QWidget *>()) { item->setHidden(true); }

  ui->splitter->setStretchFactor(0, 1);
  ui->splitter->setStretchFactor(1, 0);

  show();
}

Venda::~Venda() { delete ui; }

void Venda::setTreeView() {
  modelTree.appendModel(&modelItem);
  modelTree.appendModel(&modelItem2);

  modelTree.updateData();

  modelTree.setHeaderData(modelTree.fieldIndex("status"), Qt::Horizontal, "Status");
  modelTree.setHeaderData(modelTree.fieldIndex("fornecedor"), Qt::Horizontal, "Fornecedor");
  modelTree.setHeaderData(modelTree.fieldIndex("produto"), Qt::Horizontal, "Produto");
  modelTree.setHeaderData(modelTree.fieldIndex("obs"), Qt::Horizontal, "Obs.");
  modelTree.setHeaderData(modelTree.fieldIndex("lote"), Qt::Horizontal, "Lote");
  modelTree.setHeaderData(modelTree.fieldIndex("prcUnitario"), Qt::Horizontal, "Preço/Un");
  modelTree.setHeaderData(modelTree.fieldIndex("caixas"), Qt::Horizontal, "Caixas");
  modelTree.setHeaderData(modelTree.fieldIndex("quant"), Qt::Horizontal, "Quant.");
  modelTree.setHeaderData(modelTree.fieldIndex("un"), Qt::Horizontal, "Un.");
  modelTree.setHeaderData(modelTree.fieldIndex("unCaixa"), Qt::Horizontal, "Un./Cx.");
  modelTree.setHeaderData(modelTree.fieldIndex("codComercial"), Qt::Horizontal, "Código");
  modelTree.setHeaderData(modelTree.fieldIndex("formComercial"), Qt::Horizontal, "Formato");
  modelTree.setHeaderData(modelTree.fieldIndex("parcial"), Qt::Horizontal, "Subtotal");
  modelTree.setHeaderData(modelTree.fieldIndex("desconto"), Qt::Horizontal, "Desc. %");
  modelTree.setHeaderData(modelTree.fieldIndex("parcialDesc"), Qt::Horizontal, "Desc. Parc.");
  modelTree.setHeaderData(modelTree.fieldIndex("descGlobal"), Qt::Horizontal, "Desc. Glob. %");
  modelTree.setHeaderData(modelTree.fieldIndex("total"), Qt::Horizontal, "Total");
  modelTree.setHeaderData(modelTree.fieldIndex("dataPrevCompra"), Qt::Horizontal, "Prev. Compra");
  modelTree.setHeaderData(modelTree.fieldIndex("dataRealCompra"), Qt::Horizontal, "Data Compra");
  modelTree.setHeaderData(modelTree.fieldIndex("dataPrevConf"), Qt::Horizontal, "Prev. Confirm.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataRealConf"), Qt::Horizontal, "Data Confirm.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataPrevFat"), Qt::Horizontal, "Prev. Fat.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataRealFat"), Qt::Horizontal, "Data Fat.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataPrevColeta"), Qt::Horizontal, "Prev. Coleta");
  modelTree.setHeaderData(modelTree.fieldIndex("dataRealColeta"), Qt::Horizontal, "Data Coleta");
  modelTree.setHeaderData(modelTree.fieldIndex("dataPrevReceb"), Qt::Horizontal, "Prev. Receb.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataRealReceb"), Qt::Horizontal, "Data Receb.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataPrevEnt"), Qt::Horizontal, "Prev. Ent.");
  modelTree.setHeaderData(modelTree.fieldIndex("dataRealEnt"), Qt::Horizontal, "Data Ent.");

  ui->treeView->setModel(new SearchDialogProxyModel(&modelTree, this));

  connect(ui->treeView, &QTreeView::expanded, this, [&] {
    for (int col = 0; col < modelTree.columnCount(); ++col) { ui->treeView->resizeColumnToContents(col); }
  });

  connect(ui->treeView, &QTreeView::collapsed, this, [&] {
    for (int col = 0; col < modelTree.columnCount(); ++col) { ui->treeView->resizeColumnToContents(col); }
  });

  ui->treeView->hideColumn(modelTree.fieldIndex("selecionado"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idRelacionado"));
  ui->treeView->hideColumn(modelTree.fieldIndex("statusOriginal"));
  ui->treeView->hideColumn(modelTree.fieldIndex("recebeu"));
  ui->treeView->hideColumn(modelTree.fieldIndex("entregou"));
  ui->treeView->hideColumn(modelTree.fieldIndex("descUnitario"));
  ui->treeView->hideColumn(modelTree.fieldIndex("estoque"));
  ui->treeView->hideColumn(modelTree.fieldIndex("promocao"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idCompra"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idNFeSaida"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idNFeFutura"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idVenda"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idLoja"));
  ui->treeView->hideColumn(modelTree.fieldIndex("idProduto"));
  ui->treeView->hideColumn(modelTree.fieldIndex("reposicaoEntrega"));
  ui->treeView->hideColumn(modelTree.fieldIndex("reposicaoReceb"));
  ui->treeView->hideColumn(modelTree.fieldIndex("mostrarDesconto"));
  ui->treeView->hideColumn(modelTree.fieldIndex("created"));
  ui->treeView->hideColumn(modelTree.fieldIndex("lastUpdated"));

  ui->treeView->setItemDelegate(new QTreeViewGridDelegate(this));

  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("quant"), new DoubleDelegate(this, 4, true));
  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("prcUnitario"), new ReaisDelegate(this, 2, true));
  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("parcial"), new ReaisDelegate(this, 2, true));
  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("parcialDesc"), new ReaisDelegate(this, 2, true));
  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("desconto"), new PorcentagemDelegate(this, true));
  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("descGlobal"), new PorcentagemDelegate(this, true));
  ui->treeView->setItemDelegateForColumn(modelTree.fieldIndex("total"), new ReaisDelegate(this, 2, true));
}

void Venda::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxFreteManual, &QCheckBox::clicked, this, &Venda::on_checkBoxFreteManual_clicked, connectionType);
  connect(ui->checkBoxPontuacaoIsento, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoIsento_toggled, connectionType);
  connect(ui->checkBoxPontuacaoPadrao, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoPadrao_toggled, connectionType);
  connect(ui->checkBoxRT, &QCheckBox::toggled, this, &Venda::on_checkBoxRT_toggled, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &Venda::on_dateTimeEdit_dateTimeChanged, connectionType);
  connect(ui->doubleSpinBoxDescontoGlobal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobal_valueChanged, connectionType);
  connect(ui->doubleSpinBoxDescontoGlobalReais, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobalReais_valueChanged, connectionType);
  connect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxFrete_valueChanged, connectionType);
  connect(ui->doubleSpinBoxTotal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxTotal_valueChanged, connectionType);
  connect(ui->itemBoxProfissional, &ItemBox::textChanged, this, &Venda::on_itemBoxProfissional_textChanged, connectionType);
  connect(ui->pushButtonCadastrarPedido, &QPushButton::clicked, this, &Venda::on_pushButtonCadastrarPedido_clicked, connectionType);
  connect(ui->pushButtonCancelamento, &QPushButton::clicked, this, &Venda::on_pushButtonCancelamento_clicked, connectionType);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &Venda::on_pushButtonCorrigirFluxo_clicked, connectionType);
  connect(ui->pushButtonDevolucao, &QPushButton::clicked, this, &Venda::on_pushButtonDevolucao_clicked, connectionType);
  connect(ui->pushButtonFinanceiroSalvar, &QPushButton::clicked, this, &Venda::on_pushButtonFinanceiroSalvar_clicked, connectionType);
  connect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Venda::on_pushButtonGerarExcel_clicked, connectionType);
  connect(ui->pushButtonImprimir, &QPushButton::clicked, this, &Venda::on_pushButtonImprimir_clicked, connectionType);
  connect(ui->pushButtonVoltar, &QPushButton::clicked, this, &Venda::on_pushButtonVoltar_clicked, connectionType);
  connect(ui->treeView, &QTreeView::entered, this, &Venda::on_treeView_entered, connectionType);
}

void Venda::unsetConnections() {
  disconnect(ui->checkBoxFreteManual, &QCheckBox::clicked, this, &Venda::on_checkBoxFreteManual_clicked);
  disconnect(ui->checkBoxPontuacaoIsento, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoIsento_toggled);
  disconnect(ui->checkBoxPontuacaoPadrao, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoPadrao_toggled);
  disconnect(ui->checkBoxRT, &QCheckBox::toggled, this, &Venda::on_checkBoxRT_toggled);
  disconnect(ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &Venda::on_dateTimeEdit_dateTimeChanged);
  disconnect(ui->doubleSpinBoxDescontoGlobal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobal_valueChanged);
  disconnect(ui->doubleSpinBoxDescontoGlobalReais, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobalReais_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxTotal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxTotal_valueChanged);
  disconnect(ui->itemBoxProfissional, &ItemBox::textChanged, this, &Venda::on_itemBoxProfissional_textChanged);
  disconnect(ui->pushButtonCadastrarPedido, &QPushButton::clicked, this, &Venda::on_pushButtonCadastrarPedido_clicked);
  disconnect(ui->pushButtonCancelamento, &QPushButton::clicked, this, &Venda::on_pushButtonCancelamento_clicked);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &Venda::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonDevolucao, &QPushButton::clicked, this, &Venda::on_pushButtonDevolucao_clicked);
  disconnect(ui->pushButtonFinanceiroSalvar, &QPushButton::clicked, this, &Venda::on_pushButtonFinanceiroSalvar_clicked);
  disconnect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Venda::on_pushButtonGerarExcel_clicked);
  disconnect(ui->pushButtonImprimir, &QPushButton::clicked, this, &Venda::on_pushButtonImprimir_clicked);
  disconnect(ui->pushButtonVoltar, &QPushButton::clicked, this, &Venda::on_pushButtonVoltar_clicked);
  disconnect(ui->treeView, &QTreeView::entered, this, &Venda::on_treeView_entered);
}

void Venda::setupTables() {
  modelItem.setTable("venda_has_produto");

  //-----------------------------------------------------------------

  modelItem2.setTable("venda_has_produto2");

  //-----------------------------------------------------------------

  modelFluxoCaixa.setTable("conta_a_receber_has_pagamento");

  modelFluxoCaixa.setHeaderData("tipo", "Tipo");
  modelFluxoCaixa.setHeaderData("parcela", "Parcela");
  modelFluxoCaixa.setHeaderData("valor", "R$");
  modelFluxoCaixa.setHeaderData("dataPagamento", "Data");
  modelFluxoCaixa.setHeaderData("observacao", "Obs.");
  modelFluxoCaixa.setHeaderData("status", "Status");
  modelFluxoCaixa.setHeaderData("representacao", "Representação");

  modelFluxoCaixa.proxyModel = new SortFilterProxyModel(&modelFluxoCaixa, this);

  ui->tableFluxoCaixa->setModel(&modelFluxoCaixa);

  ui->tableFluxoCaixa->hideColumn("nfe");
  ui->tableFluxoCaixa->hideColumn("contraParte");
  ui->tableFluxoCaixa->hideColumn("idVenda");
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
  ui->tableFluxoCaixa->hideColumn("comissao");
  ui->tableFluxoCaixa->hideColumn("taxa");
  ui->tableFluxoCaixa->hideColumn("desativado");

  ui->tableFluxoCaixa->setItemDelegate(new NoEditDelegate(this));

  ui->tableFluxoCaixa->setItemDelegateForColumn("representacao", new CheckBoxDelegate(this, true));
  ui->tableFluxoCaixa->setItemDelegateForColumn("observacao", new EditDelegate(this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  ui->tableFluxoCaixa->setPersistentColumns({"representacao"});

  //-----------------------------------------------------------------

  modelFluxoCaixa2.setTable("conta_a_receber_has_pagamento");

  modelFluxoCaixa2.setHeaderData("contraParte", "ContraParte");
  modelFluxoCaixa2.setHeaderData("tipo", "Tipo");
  modelFluxoCaixa2.setHeaderData("parcela", "Parcela");
  modelFluxoCaixa2.setHeaderData("valor", "R$");
  modelFluxoCaixa2.setHeaderData("dataPagamento", "Data");
  modelFluxoCaixa2.setHeaderData("observacao", "Obs.");
  modelFluxoCaixa2.setHeaderData("status", "Status");

  modelFluxoCaixa2.proxyModel = new SortFilterProxyModel(&modelFluxoCaixa2, this);

  ui->tableFluxoCaixa2->setModel(&modelFluxoCaixa2);

  ui->tableFluxoCaixa2->hideColumn("idPagamento");
  ui->tableFluxoCaixa2->hideColumn("dataEmissao");
  ui->tableFluxoCaixa2->hideColumn("idVenda");
  ui->tableFluxoCaixa2->hideColumn("idLoja");
  ui->tableFluxoCaixa2->hideColumn("nfe");
  ui->tableFluxoCaixa2->hideColumn("representacao");
  ui->tableFluxoCaixa2->hideColumn("dataRealizado");
  ui->tableFluxoCaixa2->hideColumn("valorReal");
  ui->tableFluxoCaixa2->hideColumn("tipoReal");
  ui->tableFluxoCaixa2->hideColumn("parcelaReal");
  ui->tableFluxoCaixa2->hideColumn("contaDestino");
  ui->tableFluxoCaixa2->hideColumn("tipoDet");
  ui->tableFluxoCaixa2->hideColumn("centroCusto");
  ui->tableFluxoCaixa2->hideColumn("grupo");
  ui->tableFluxoCaixa2->hideColumn("subGrupo");
  ui->tableFluxoCaixa2->hideColumn("comissao");
  ui->tableFluxoCaixa2->hideColumn("taxa");
  ui->tableFluxoCaixa2->hideColumn("desativado");

  ui->tableFluxoCaixa2->setItemDelegateForColumn("valor", new ReaisDelegate(this));
}

void Venda::prepararVenda(const QString &idOrcamento) {
  // TODO: verificar se as quantidades de produto_estoque ainda estão disponiveis

  ui->lineEditIdOrcamento->setText(idOrcamento);
  ui->lineEditVenda->setText("Auto gerado");
  ui->dateTimeEdit->setDateTime(QDateTime::currentDateTime());

  if (not copiaProdutosOrcamento()) { return; }

  setTreeView();

  // -------------------------------------------------------------------------

  QSqlQuery queryOrc;
  queryOrc.prepare("SELECT idUsuario, idLoja, idUsuarioConsultor, idCliente, idEnderecoEntrega, idProfissional, data, subTotalBru, subTotalLiq, frete, "
                   "freteManual, descontoPorc, descontoReais, total, status, observacao, prazoEntrega, representacao FROM orcamento WHERE idOrcamento = :idOrcamento");
  queryOrc.bindValue(":idOrcamento", idOrcamento);

  if (not queryOrc.exec() or not queryOrc.first()) { return qApp->enqueueError("Erro buscando orçamento: " + queryOrc.lastError().text(), this); }

  ui->itemBoxVendedor->setId(queryOrc.value("idUsuario"));
  ui->itemBoxConsultor->setId(queryOrc.value("idUsuarioConsultor"));
  ui->itemBoxEndereco->setFilter("idCliente = " + queryOrc.value("idCliente").toString() + " AND desativado = FALSE");
  ui->itemBoxEnderecoFat->setFilter("idCliente = " + queryOrc.value("idCliente").toString() + " AND desativado = FALSE");
  ui->itemBoxCliente->setId(queryOrc.value("idCliente"));
  ui->itemBoxProfissional->setId(queryOrc.value("idProfissional"));
  ui->itemBoxEndereco->setId(queryOrc.value("idEnderecoEntrega"));
  ui->dateTimeEditOrc->setDateTime(queryOrc.value("data").toDateTime());
  ui->plainTextEdit->setPlainText(queryOrc.value("observacao").toString());
  ui->doubleSpinBoxSubTotalBruto->setValue(queryOrc.value("subTotalBru").toDouble());
  ui->doubleSpinBoxSubTotalLiq->setValue(queryOrc.value("subTotalLiq").toDouble());
  ui->doubleSpinBoxFrete->setValue(queryOrc.value("frete").toDouble());
  silentFrete = true;
  ui->checkBoxFreteManual->setChecked(queryOrc.value("freteManual").toBool());
  ui->doubleSpinBoxDescontoGlobal->setValue(queryOrc.value("descontoPorc").toDouble());
  ui->doubleSpinBoxDescontoGlobalReais->setValue(queryOrc.value("descontoReais").toDouble());
  ui->doubleSpinBoxTotal->setValue(queryOrc.value("total").toDouble());
  ui->spinBoxPrazoEntrega->setValue(queryOrc.value("prazoEntrega").toInt());

  idLoja = queryOrc.value("idLoja").toInt();
  representacao = queryOrc.value("representacao").toBool();

  // -------------------------------------------------------------------------

  QSqlQuery queryFrete;
  queryFrete.prepare("SELECT valorMinimoFrete, porcentagemFrete FROM loja WHERE idLoja = :idLoja");
  queryFrete.bindValue(":idLoja", idLoja);

  if (not queryFrete.exec() or not queryFrete.first()) { return qApp->enqueueError("Erro buscando parâmetros do frete: " + queryFrete.lastError().text(), this); }

  minimoFrete = queryFrete.value("valorMinimoFrete").toDouble();
  porcFrete = queryFrete.value("porcentagemFrete").toDouble();

  if (not representacao) { ui->tableFluxoCaixa->hideColumn("representacao"); }

  // -------------------------------------------------------------------------

  QSqlQuery queryCredito;
  queryCredito.prepare("SELECT credito FROM cliente WHERE idCliente = :idCliente");
  queryCredito.bindValue(":idCliente", queryOrc.value("idCliente"));

  if (not queryCredito.exec() or not queryCredito.first()) { return qApp->enqueueError("Erro buscando crédito cliente: " + queryCredito.lastError().text(), this); }

  ui->widgetPgts->setCredito(queryCredito.value("credito").toDouble());

  // -------------------------------------------------------------------------

  ui->widgetPgts->setIdOrcamento(ui->lineEditIdOrcamento->text());
  ui->widgetPgts->setRepresentacao(representacao);
  ui->widgetPgts->setFrete(ui->doubleSpinBoxFrete->value());
  ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());

  ui->widgetPgts->setMinimumWidth(550);

  if (ui->itemBoxConsultor->getId().isValid()) {
    ui->labelConsultor->show();
    ui->itemBoxConsultor->show();
  }

  // -------------------------------------------------------------------------

  on_checkBoxFreteManual_clicked(ui->checkBoxFreteManual->isChecked());
  silentFrete = false;

  // -------------------------------------------------------------------------

  setConnections();
}

bool Venda::verifyFields() {
  // TODO: pintar campos errados de vermelho
  // TODO: pintar campos necessarios de amarelo
  // TODO: pintar campos certos de verde
  // TODO: pintar totalPag de vermelho enquanto o total for diferente

  if (ui->widgetPgts->isHidden()) { return true; }

  if (not qFuzzyCompare(ui->widgetPgts->getTotalPag(), ui->doubleSpinBoxTotal->value())) { return qApp->enqueueError(false, "Total dos pagamentos difere do total do pedido!", this); }

  if (not ui->widgetPgts->verifyFields()) { return false; }

  if (ui->spinBoxPrazoEntrega->value() == 0) {
    qApp->enqueueError("Por favor preencha o prazo de entrega.", this);
    ui->spinBoxPrazoEntrega->setFocus();
    return false;
  }

  if (ui->itemBoxEnderecoFat->text().isEmpty()) {
    qApp->enqueueError("Deve selecionar um endereço de faturamento!", this);
    ui->itemBoxEnderecoFat->setFocus();
    return false;
  }

  if (ui->itemBoxProfissional->getId() != 1 and not ui->checkBoxPontuacaoIsento->isChecked() and not ui->checkBoxPontuacaoPadrao->isChecked()) {
    qApp->enqueueError("Por favor preencha a pontuação!", this);
    ui->checkBoxRT->setChecked(true);
    ui->doubleSpinBoxPontuacao->setFocus();
    return false;
  }

  return true;
}

void Venda::calcPrecoGlobalTotal() {
  double subTotalBruto = 0.;
  double subTotalItens = 0.;

  for (int row = 0, rowCount = modelItem.rowCount(); row < rowCount; ++row) {
    const double itemBruto = modelItem.data(row, "quant").toDouble() * modelItem.data(row, "prcUnitario").toDouble();
    const double descItem = modelItem.data(row, "desconto").toDouble() / 100.;
    const double stItem = itemBruto * (1. - descItem);
    subTotalBruto += itemBruto;
    subTotalItens += stItem;
  }

  ui->doubleSpinBoxSubTotalBruto->setValue(subTotalBruto);
  ui->doubleSpinBoxSubTotalLiq->setValue(subTotalItens);
}

void Venda::clearFields() {}

void Venda::setupMapper() {
  addMapping(ui->checkBoxFreteManual, "freteManual");
  addMapping(ui->dateTimeEdit, "data");
  addMapping(ui->dateTimeEditOrc, "dataOrc");
  addMapping(ui->doubleSpinBoxDescontoGlobal, "descontoPorc");
  addMapping(ui->doubleSpinBoxDescontoGlobalReais, "descontoReais");
  addMapping(ui->doubleSpinBoxFrete, "frete");
  addMapping(ui->doubleSpinBoxSubTotalBruto, "subTotalBru");
  addMapping(ui->doubleSpinBoxSubTotalLiq, "subTotalLiq");
  addMapping(ui->doubleSpinBoxTotal, "total");
  addMapping(ui->itemBoxCliente, "idCliente", "id");
  addMapping(ui->itemBoxConsultor, "idUsuarioConsultor", "id");
  addMapping(ui->itemBoxEndereco, "idEnderecoEntrega", "id");
  addMapping(ui->itemBoxEnderecoFat, "idEnderecoFaturamento", "id");
  addMapping(ui->itemBoxProfissional, "idProfissional", "id");
  addMapping(ui->itemBoxVendedor, "idUsuario", "id");
  addMapping(ui->lineEditIdOrcamento, "idOrcamento");
  addMapping(ui->lineEditVenda, "idVenda");
  addMapping(ui->plainTextEdit, "observacao");
  addMapping(ui->spinBoxPrazoEntrega, "prazoEntrega");
}

void Venda::on_pushButtonCadastrarPedido_clicked() { save(); }

bool Venda::savingProcedures() {
  // TODO: remove novoPrazoEntrega from DB?

  if (not setData("status", todosProdutosSaoEstoque() ? "ESTOQUE" : "PENDENTE")) { return false; }
  if (not setData("data", ui->dateTimeEdit->dateTime())) { return false; }
  if (not setData("dataOrc", ui->dateTimeEditOrc->dateTime())) { return false; }
  if (not setData("descontoPorc", ui->doubleSpinBoxDescontoGlobal->value())) { return false; }
  if (not setData("descontoReais", ui->doubleSpinBoxDescontoGlobalReais->value())) { return false; }
  if (not setData("frete", ui->doubleSpinBoxFrete->value())) { return false; }
  if (not setData("freteManual", ui->checkBoxFreteManual->isChecked())) { return false; }
  if (not setData("idCliente", ui->itemBoxCliente->getId())) { return false; }
  if (not setData("idEnderecoEntrega", ui->itemBoxEndereco->getId())) { return false; }
  if (not setData("idEnderecoFaturamento", ui->itemBoxEnderecoFat->getId())) { return false; }
  if (not setData("idLoja", idLoja)) { return false; }
  if (not setData("idOrcamento", ui->lineEditIdOrcamento->text())) { return false; }
  if (not setData("idProfissional", ui->itemBoxProfissional->getId())) { return false; }
  if (not setData("idUsuario", ui->itemBoxVendedor->getId())) { return false; }
  if (not setData("idUsuarioConsultor", ui->itemBoxConsultor->getId())) { return false; }
  if (not setData("idVenda", ui->lineEditVenda->text())) { return false; }
  if (not setData("idVendaBase", ui->lineEditVenda->text().left(11))) { return false; }
  if (not setData("novoPrazoEntrega", ui->spinBoxPrazoEntrega->value())) { return false; }
  if (not setData("observacao", ui->plainTextEdit->toPlainText())) { return false; }
  if (not setData("prazoEntrega", ui->spinBoxPrazoEntrega->value())) { return false; }
  if (not setData("representacao", ui->lineEditVenda->text().endsWith("R") ? 1 : 0)) { return false; }
  if (not setData("rt", ui->doubleSpinBoxPontuacao->value())) { return false; }
  if (not setData("subTotalBru", ui->doubleSpinBoxSubTotalBruto->value())) { return false; }
  if (not setData("subTotalLiq", ui->doubleSpinBoxSubTotalLiq->value())) { return false; }
  if (not setData("total", ui->doubleSpinBoxTotal->value())) { return false; }

  return true;
}

bool Venda::todosProdutosSaoEstoque() {
  bool temEstoque = true;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (not modelItem.data(row, "estoque").toBool()) {
      temEstoque = false;
      break;
    }
  }

  return temEstoque;
}

void Venda::registerMode() {
  ui->framePagamentos->show();
  ui->pushButtonGerarExcel->hide();
  ui->pushButtonImprimir->hide();
  ui->pushButtonCancelamento->hide();
  ui->pushButtonDevolucao->hide();
  ui->pushButtonCadastrarPedido->show();
  ui->pushButtonVoltar->show();
  ui->doubleSpinBoxDescontoGlobal->setReadOnly(false);
  ui->doubleSpinBoxDescontoGlobal->setFrame(true);
  ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::UpDownArrows);
  ui->doubleSpinBoxTotal->setReadOnly(false);
  ui->doubleSpinBoxTotal->setFrame(true);
  ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::UpDownArrows);
  ui->checkBoxFreteManual->show();
}

void Venda::updateMode() {
  ui->splitter_2->hide();
  ui->pushButtonGerarExcel->show();
  ui->pushButtonImprimir->show();
  ui->pushButtonCadastrarPedido->hide();
  ui->pushButtonVoltar->hide();
  ui->doubleSpinBoxDescontoGlobal->setReadOnly(true);
  ui->doubleSpinBoxDescontoGlobal->setFrame(false);
  ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxDescontoGlobalReais->setReadOnly(true);
  ui->doubleSpinBoxDescontoGlobalReais->setFrame(false);
  ui->doubleSpinBoxDescontoGlobalReais->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxTotal->setReadOnly(true);
  ui->doubleSpinBoxTotal->setFrame(false);
  ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->checkBoxFreteManual->setDisabled(true);
}

bool Venda::viewRegister() {
  unsetConnections();

  const auto load = [&] {
    if (not RegisterDialog::viewRegister()) { return false; }

    ui->doubleSpinBoxDescontoGlobalReais->setMinimum(-9999999);

    modelItem.setFilter("idVenda = '" + model.data(0, "idVenda").toString() + "'");

    if (not modelItem.select()) { return false; }

    modelItem2.setFilter("idVenda = '" + model.data(0, "idVenda").toString() + "'");

    if (not modelItem2.select()) { return false; }

    setTreeView();

    calcPrecoGlobalTotal();

    const QString idCliente = ui->itemBoxCliente->getId().toString();

    ui->itemBoxEndereco->setFilter("idCliente = " + idCliente + " AND desativado = FALSE");
    ui->itemBoxEnderecoFat->setFilter("idCliente = " + idCliente + " AND desativado = FALSE");

    modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND comissao = FALSE AND taxa = FALSE");

    if (not modelFluxoCaixa.select()) { return false; }

    if (financeiro) {
      // TODO: 1quando estiver tudo pago bloquear correcao de fluxo
      modelFluxoCaixa2.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND (comissao = TRUE OR taxa = TRUE)");

      if (not modelFluxoCaixa2.select()) { return false; }

      ui->comboBoxFinanceiro->setCurrentText(model.data(0, "statusFinanceiro").toString());
    }

    ui->tableFluxoCaixa->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->spinBoxPrazoEntrega->setReadOnly(true);

    ui->itemBoxCliente->setReadOnlyItemBox(true);
    ui->itemBoxEndereco->setReadOnlyItemBox(true);
    ui->itemBoxEnderecoFat->setReadOnlyItemBox(true);
    ui->itemBoxProfissional->setReadOnlyItemBox(true);
    ui->itemBoxVendedor->setReadOnlyItemBox(true);

    ui->dateTimeEdit->setReadOnly(true);

    ui->plainTextEdit->setReadOnly(true);
    ui->plainTextEdit->setPlainText(data("observacao").toString());

    ui->pushButtonCancelamento->show();
    ui->pushButtonDevolucao->show();

    const bool freteManual = ui->checkBoxFreteManual->isChecked();

    ui->doubleSpinBoxFrete->setReadOnly(not freteManual);
    ui->doubleSpinBoxFrete->setButtonSymbols(freteManual ? QDoubleSpinBox::UpDownArrows : QDoubleSpinBox::NoButtons);

    if (data("status") == "CANCELADO" or data("status") == "DEVOLUÇÃO") {
      ui->pushButtonCancelamento->hide();
      ui->pushButtonDevolucao->hide();
    }

    const QString tipoUsuario = UserSession::tipoUsuario();

    if (not tipoUsuario.contains("GERENTE") and tipoUsuario != "DIRETOR" and tipoUsuario != "ADMINISTRADOR") {
      ui->pushButtonDevolucao->hide();
      ui->pushButtonCancelamento->hide();
    }

    ui->framePagamentos->adjustSize();

    ui->checkBoxRT->setChecked(false);
    ui->checkBoxRT->setHidden(true);

    if (data("devolucao").toBool()) { ui->pushButtonDevolucao->hide(); }

    if (data("idUsuarioConsultor").toInt() != 0) {
      ui->labelConsultor->show();
      ui->itemBoxConsultor->show();
    }

    idLoja = data("idLoja").toInt();
    representacao = data("representacao").toBool();

    ui->widgetPgts->setIdOrcamento(ui->lineEditIdOrcamento->text());
    ui->widgetPgts->setRepresentacao(representacao);
    ui->widgetPgts->setFrete(ui->doubleSpinBoxFrete->value());
    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());

    for (int col = 0; col < ui->treeView->model()->columnCount(); ++col) { ui->treeView->resizeColumnToContents(col); }

    ui->splitter_2->hide();

    return true;
  }();

  setConnections();

  return load;
}

void Venda::on_pushButtonVoltar_clicked() {
  auto *orcamento = new Orcamento(parentWidget());
  orcamento->viewRegisterById(ui->lineEditIdOrcamento->text());

  orcamento->show();

  close();
}

void Venda::montarFluxoCaixa() {
  // TODO: 0nao calcular comissao para profissional 'NAO HA'
  // TODO: verificar se esta funcao nao está rodando mais de uma vez por operacao
  // TODO: lancamento de credito deve ser marcado direto como 'recebido' e statusFinanceiro == liberado

  if (ui->widgetPgts->isHidden()) { return; }

  modelFluxoCaixa.revertAll();
  modelFluxoCaixa2.revertAll();

  if (financeiro) {
    for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
      if (not modelFluxoCaixa.setData(row, "status", "SUBSTITUIDO")) { return; }
    }

    for (int row = 0; row < modelFluxoCaixa2.rowCount(); ++row) {
      if (not modelFluxoCaixa2.setData(row, "status", "SUBSTITUIDO")) { return; }
    }
  }

  for (int i = 0; i < ui->widgetPgts->listCheckBoxRep.size(); ++i) {
    if (ui->widgetPgts->listComboPgt.at(i)->currentText() != "Escolha uma opção!") {
      const int parcelas = ui->widgetPgts->listComboParc.at(i)->currentIndex() + 1;
      const double valor = ui->widgetPgts->listDoubleSpinPgt.at(i)->value();
      const int cartao = ui->widgetPgts->listComboPgt.at(i)->currentText() == "Cartão de crédito" ? 1 : 0;

      const double part1 = valor / parcelas;
      const int part2 = static_cast<int>(part1 * 100); // truncate with 2 decimals
      const double parcela = static_cast<double>(part2) / 100;
      const double resto = valor - (parcela * parcelas);

      const QDate dataEmissao = correcao ? modelFluxoCaixa.data(0, "dataEmissao").toDate() : QDate::currentDate();

      for (int x = 0, y = parcelas - 1; x < parcelas; ++x, --y) {
        const int row = modelFluxoCaixa.insertRowAtEnd();

        if (not modelFluxoCaixa.setData(row, "contraParte", ui->itemBoxCliente->text())) { return; }
        if (not modelFluxoCaixa.setData(row, "dataEmissao", dataEmissao)) { return; }
        if (not modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text())) { return; }
        if (not modelFluxoCaixa.setData(row, "idLoja", idLoja)) { return; }
        if (not modelFluxoCaixa.setData(row, "dataPagamento", ui->widgetPgts->listDatePgt.at(i)->date().addMonths(x + cartao))) { return; }
        if (not modelFluxoCaixa.setData(row, "valor", parcela + (x == 0 ? resto : 0))) { return; }
        if (not modelFluxoCaixa.setData(row, "tipo", QString::number(i + 1) + ". " + ui->widgetPgts->listComboPgt.at(i)->currentText())) { return; }
        if (not modelFluxoCaixa.setData(row, "parcela", parcelas - y)) { return; }
        if (not modelFluxoCaixa.setData(row, "observacao", ui->widgetPgts->listLinePgt.at(i)->text())) { return; }
        if (not modelFluxoCaixa.setData(row, "representacao", ui->widgetPgts->listCheckBoxRep.at(i)->isChecked())) { return; }
        if (not modelFluxoCaixa.setData(row, "grupo", "Produtos - Venda")) { return; }
        if (not modelFluxoCaixa.setData(row, "subGrupo", "")) { return; }
        if (not modelFluxoCaixa.setData(row, "centroCusto", idLoja)) { return; }
      }

      // calculo comissao
      for (int z = 0, total = modelFluxoCaixa.rowCount(); z < total; ++z) {
        if (not modelFluxoCaixa.data(z, "representacao").toBool()) { continue; }
        if (not modelFluxoCaixa.data(z, "tipo").toString().contains(QString::number(i + 1))) { continue; }
        if (modelFluxoCaixa.data(z, "status").toString() == "SUBSTITUIDO") { continue; }

        const QString fornecedor = modelItem.data(0, "fornecedor").toString();
        const double totalVenda = ui->doubleSpinBoxTotal->value();
        const double frete = ui->doubleSpinBoxFrete->value();

        QSqlQuery query;
        query.prepare("SELECT comissaoLoja FROM fornecedor WHERE razaoSocial = :razaoSocial");
        query.bindValue(":razaoSocial", fornecedor);

        if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando comissão: " + query.lastError().text(), this); }

        const double taxaComissao = query.value("comissaoLoja").toDouble() / 100;
        const double valorComissao = modelFluxoCaixa.data(z, "valor").toDouble();
        double valorAjustado = taxaComissao * (valorComissao - (valorComissao / totalVenda * frete));

        if (modelFluxoCaixa.data(0, "observacao").toString() == "FRETE") { valorAjustado = valorComissao * taxaComissao; }

        const int row = modelFluxoCaixa2.insertRowAtEnd();

        if (not modelFluxoCaixa2.setData(row, "contraParte", modelItem.data(0, "fornecedor"))) { return; }
        if (not modelFluxoCaixa2.setData(row, "dataEmissao", dataEmissao)) { return; }
        if (not modelFluxoCaixa2.setData(row, "idVenda", ui->lineEditVenda->text())) { return; }
        if (not modelFluxoCaixa2.setData(row, "idLoja", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(row, "dataPagamento", modelFluxoCaixa.data(z, "dataPagamento").toDate().addMonths(1))) { return; }
        if (not modelFluxoCaixa2.setData(row, "valor", valorAjustado)) { return; }
        if (not modelFluxoCaixa2.setData(row, "tipo", QString::number(i + 1) + ". Comissão")) { return; }
        if (not modelFluxoCaixa2.setData(row, "parcela", modelFluxoCaixa.data(z, "parcela").toString())) { return; }
        if (not modelFluxoCaixa2.setData(row, "comissao", 1)) { return; }
        if (not modelFluxoCaixa2.setData(row, "centroCusto", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(row, "grupo", "Comissão Representação")) { return; }
      }

      // calculo taxas cartao
      for (int z = 0, total = modelFluxoCaixa.rowCount(); z < total; ++z) {
        if (not modelFluxoCaixa.data(z, "tipo").toString().contains(QString::number(i + 1))) { continue; }
        if (modelFluxoCaixa.data(z, "status").toString() == "SUBSTITUIDO") { continue; }
        if (modelFluxoCaixa.data(z, "representacao").toBool()) { continue; }
        if (modelFluxoCaixa.data(z, "tipo").toString().contains("Conta Cliente")) { continue; }

        QSqlQuery query;
        query.prepare("SELECT taxa FROM forma_pagamento fp LEFT JOIN forma_pagamento_has_taxa fpt ON fp.idPagamento = fpt.idPagamento WHERE pagamento = :pagamento AND parcela = :parcela");
        query.bindValue(":pagamento", ui->widgetPgts->listComboPgt.at(i)->currentText());
        query.bindValue(":parcela", ui->widgetPgts->listComboParc.at(i)->currentText().remove("x").toInt());

        if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando taxa: " + query.lastError().text(), this); }

        if (qFuzzyIsNull(query.value("taxa").toDouble())) { continue; }

        const double taxa = query.value("taxa").toDouble() / 100;

        const int row = modelFluxoCaixa2.insertRowAtEnd();

        if (not modelFluxoCaixa2.setData(row, "contraParte", "Administradora Cartão")) { return; }
        if (not modelFluxoCaixa2.setData(row, "dataEmissao", dataEmissao)) { return; }
        if (not modelFluxoCaixa2.setData(row, "idVenda", ui->lineEditVenda->text())) { return; }
        if (not modelFluxoCaixa2.setData(row, "idLoja", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(row, "dataPagamento", modelFluxoCaixa.data(z, "dataPagamento"))) { return; }
        if (not modelFluxoCaixa2.setData(row, "valor", taxa * modelFluxoCaixa.data(z, "valor").toDouble() * -1)) { return; }
        if (not modelFluxoCaixa2.setData(row, "tipo", QString::number(i + 1) + ". Taxa Cartão")) { return; }
        if (not modelFluxoCaixa2.setData(row, "parcela", modelFluxoCaixa.data(z, "parcela").toString())) { return; }
        if (not modelFluxoCaixa2.setData(row, "taxa", 1)) { return; }
        if (not modelFluxoCaixa2.setData(row, "centroCusto", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(row, "grupo", "Tarifas Cartão")) { return; }
      }
    }
  }
}

void Venda::on_doubleSpinBoxTotal_valueChanged(const double total) {
  unsetConnections();

  {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();
    const double descontoReais = subTotalLiq + frete - total;
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "descGlobal", descontoPorc * 100)) { return; }

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      if (not modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc))) { return; }
    }

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxDescontoGlobalReais->setValue(descontoReais);

    ui->widgetPgts->setTotal(total);
    ui->widgetPgts->resetarPagamentos();
  }

  setConnections();
}

void Venda::on_checkBoxFreteManual_clicked(const bool checked) {
  if (not silentFrete and not canChangeFrete) {
    qApp->enqueueInformation("Necessário autorização de um gerente ou administrador!", this);

    LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

    if (dialog.exec() == QDialog::Rejected) {
      ui->checkBoxFreteManual->setChecked(not checked);
      return;
    }

    canChangeFrete = true;
  }

  ui->doubleSpinBoxFrete->setReadOnly(not checked);
  ui->doubleSpinBoxFrete->setButtonSymbols(checked ? QDoubleSpinBox::UpDownArrows : QDoubleSpinBox::NoButtons);

  if (checked) { return; }

  ui->doubleSpinBoxFrete->setValue(qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete));
}

void Venda::on_doubleSpinBoxFrete_valueChanged(const double frete) {
  const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
  const double desconto = ui->doubleSpinBoxDescontoGlobalReais->value();

  unsetConnections();

  {
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - desconto + frete);
    ui->widgetPgts->setFrete(frete);
    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    ui->widgetPgts->resetarPagamentos();
  }

  setConnections();
}

void Venda::on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) {
  unsetConnections();

  [=]() {
    const double descontoPorc2 = descontoPorc / 100;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "descGlobal", descontoPorc)) { return; }

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      if (not modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc2))) { return; }
    }

    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobalReais->setValue(subTotalLiq * descontoPorc2);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq * (1 - descontoPorc2) + frete);

    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    ui->widgetPgts->resetarPagamentos();
  }();

  setConnections();
}

void Venda::on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) {
  unsetConnections();

  [=]() {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "descGlobal", descontoPorc * 100)) { return; }

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      if (not modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc))) { return; }
    }

    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - descontoReais + frete);

    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    ui->widgetPgts->resetarPagamentos();
  }();

  setConnections();
}

void Venda::on_pushButtonImprimir_clicked() {
  Impressao impressao(data("idVenda").toString(), Impressao::Tipo::Venda, this);
  impressao.print();
}

void Venda::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Venda cadastrada com sucesso!", this); }

void Venda::on_pushButtonGerarExcel_clicked() {
  Excel excel(ui->lineEditVenda->text(), Excel::Tipo::Venda);
  excel.gerarExcel();
}

bool Venda::atualizarCredito() {
  double creditoRestante = ui->widgetPgts->getCredito();
  bool update = false;

  for (int i = 0; i < ui->widgetPgts->listCheckBoxRep.size(); ++i) {
    if (ui->widgetPgts->listComboPgt.at(i)->currentText() == "Conta Cliente") {
      creditoRestante -= ui->widgetPgts->listDoubleSpinPgt.at(i)->value();
      update = true;
    }
  }

  if (update) {
    QSqlQuery query;
    query.prepare("UPDATE cliente SET credito = :credito WHERE idCliente = :idCliente");
    query.bindValue(":credito", creditoRestante);
    query.bindValue(":idCliente", ui->itemBoxCliente->getId());

    if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando crédito do cliente: " + query.lastError().text(), this); }
  }

  return true;
}

bool Venda::cadastrar() {
  if (not qApp->startTransaction()) { return false; }

  const bool success = [&] {
    if (tipo == Tipo::Cadastrar) {
      if (not generateId()) { return false; }

      currentRow = model.insertRowAtEnd();
    }

    if (not savingProcedures()) { return false; }

    if (not atualizarCredito()) { return false; }

    // inserir rt em contas_pagar

    QSqlQuery query1;

    if (ui->checkBoxPontuacaoPadrao->isChecked()) {
      const QDate date = ui->dateTimeEdit->date();
      const double valor = (ui->doubleSpinBoxSubTotalLiq->value() - ui->doubleSpinBoxDescontoGlobalReais->value()) * ui->doubleSpinBoxPontuacao->value() / 100;

      query1.prepare("INSERT INTO conta_a_pagar_has_pagamento (dataEmissao, contraParte, idLoja, centroCusto, valor, tipo, dataPagamento, grupo) VALUES (:dataEmissao, :contraParte, :idLoja, "
                     ":centroCusto, :valor, :tipo, :dataPagamento, :grupo)");
      query1.bindValue(":dataEmissao", date);
      query1.bindValue(":contraParte", ui->itemBoxProfissional->text());
      query1.bindValue(":idLoja", idLoja);
      query1.bindValue(":centroCusto", idLoja);
      query1.bindValue(":valor", valor);
      query1.bindValue(":tipo", "1. Dinheiro");
      // 01-15 paga dia 30, 16-30 paga prox dia 15
      query1.bindValue(":dataPagamento", date.day() <= 15 ? QDate(date.year(), date.month(), 30 > date.daysInMonth() ? date.daysInMonth() : 30) : QDate(date.year(), date.month() + 1, 15));
      query1.bindValue(":grupo", "RT's");

      if (not query1.exec()) { return qApp->enqueueError(false, "Erro cadastrando pontuação: " + query1.lastError().text(), this); }
    }

    // -------------------------------------------------------------------------

    if (not model.submitAll()) { return false; }

    // -------------------------------------------------------------------------

    primaryId = ui->lineEditVenda->text();

    if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

    // -------------------------------------------------------------------------

    for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
      if (not modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text())) { return false; }
    }

    if (not modelFluxoCaixa.submitAll()) { return false; }

    // -------------------------------------------------------------------------

    for (int row = 0; row < modelFluxoCaixa2.rowCount(); ++row) {
      if (not modelFluxoCaixa2.setData(row, "idVenda", ui->lineEditVenda->text())) { return false; }
    }

    if (not modelFluxoCaixa2.submitAll()) { return false; }

    // -------------------------------------------------------------------------

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      if (not modelItem.setData(row, "idVenda", ui->lineEditVenda->text())) { return false; }
    }

    if (not modelItem.submitAll()) { return false; }

    // -------------------------------------------------------------------------

    QSqlQuery query3;
    query3.prepare("UPDATE orcamento SET status = 'FECHADO' WHERE idOrcamento = :idOrcamento");
    query3.bindValue(":idOrcamento", ui->lineEditIdOrcamento->text());

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro marcando orçamento como 'FECHADO': " + query3.lastError().text(), this); }

    return true;
  }();

  if (success) {
    if (not qApp->endTransaction()) { return false; }

    backupItem.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelItem.setFilter(primaryKey + " = '" + primaryId + "'");

    modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND comissao = FALSE AND taxa = FALSE");

    criarConsumos();
  } else {
    qApp->rollbackTransaction();
    void(model.select());
    void(modelItem.select());

    for (auto &record : backupItem) { modelItem.insertRecord(-1, record); }
  }

  return success;
}

void Venda::criarConsumos() {
  QSqlQuery query2;
  query2.prepare(
      "SELECT p.idEstoque, vp2.idVendaProduto2, pf2.idPedido2, vp2.quant FROM venda_has_produto2 vp2 LEFT JOIN produto p ON vp2.idProduto = p.idProduto LEFT JOIN estoque e ON p.idEstoque = "
      "e.idEstoque LEFT JOIN estoque_has_compra ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN pedido_fornecedor_has_produto2 pf2 ON pf2.idPedido2 = ehc.idPedido2 WHERE vp2.idVenda = :idVenda AND "
      "vp2.estoque > 0");
  query2.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query2.exec()) { return qApp->enqueueError("Erro buscando produtos estoque: " + query2.lastError().text(), this); }

  while (query2.next()) {
    auto *estoque = new Estoque(query2.value("idEstoque").toString(), false, this);

    //    if (not estoque->criarConsumo(query2.value("idVendaProduto2").toInt(), query2.value("idPedido2").toInt(), query2.value("quant").toDouble())) { return; }
    if (not estoque->criarConsumo(query2.value("idVendaProduto2").toInt(), query2.value("quant").toDouble())) { return; }
  }
}

bool Venda::cancelamento() {
  const QString idOrcamento = ui->lineEditIdOrcamento->text();

  QSqlQuery query1;

  if (not idOrcamento.isEmpty()) {
    query1.prepare("UPDATE orcamento SET status = 'ATIVO' WHERE idOrcamento = :idOrcamento");
    query1.bindValue(":idOrcamento", idOrcamento);

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro reativando orçamento: " + query1.lastError().text(), this); }
  }

  // -------------------------------------------------------------------------

  QSqlQuery query2;
  query2.prepare("UPDATE venda SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query2.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query2.exec()) { return qApp->enqueueError(false, "Erro marcando venda como cancelada: " + query2.lastError().text(), this); }

  // -------------------------------------------------------------------------

  QSqlQuery query3;
  query3.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, `idVendaProduto2` = NULL WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda) AND "
      "status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");
  query3.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query3.exec()) { return qApp->enqueueError(false, "Erro removendo vínculo da compra: " + query3.lastError().text(), this); }

  // -------------------------------------------------------------------------

  QSqlQuery query4;
  query4.prepare("UPDATE venda_has_produto2 SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query4.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query4.exec()) { return qApp->enqueueError(false, "Erro marcando produtos da venda como cancelados: " + query4.lastError().text(), this); }

  QSqlQuery query5;
  query5.prepare("UPDATE venda_has_produto SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query5.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query5.exec()) { return qApp->enqueueError(false, "Erro marcando produtos da venda como cancelados: " + query5.lastError().text(), this); }

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    if (modelFluxoCaixa.data(row, "tipo").toString().contains("Conta Cliente")) {
      if (modelFluxoCaixa.data(row, "status").toString() == "CANCELADO") { continue; }
      const double credito = modelFluxoCaixa.data(row, "valor").toDouble();

      QSqlQuery query6;
      query6.prepare("UPDATE cliente SET credito = credito + :valor WHERE idCliente = :idCliente");
      query6.bindValue(":valor", credito);
      query6.bindValue(":idCliente", model.data(0, "idCliente"));

      if (not query6.exec()) { return qApp->enqueueError(false, "Erro voltando credito do cliente: " + query6.lastError().text(), this); }
    }
  }

  // TODO: 0nao deixar cancelar se tiver ocorrido algum evento de conta
  QSqlQuery query7;
  query7.prepare("UPDATE conta_a_receber_has_pagamento SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query7.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query7.exec()) { return qApp->enqueueError(false, "Erro marcando contas como canceladas: " + query7.lastError().text(), this); }

  return true;
}

void Venda::on_pushButtonCancelamento_clicked() {
  // TODO: perguntar e salvar motivo do cancelamento

  // caso pedido nao seja do mes atual, bloquear se nao estiver no primeiro dia util
  const QDate dataVenda = data("data").toDate();
  const QDate dataAtual = QDate::currentDate();

  if (dataVenda.month() != dataAtual.month()) {
    const int diaSemana = dataAtual.dayOfWeek();
    const int dia = dataAtual.day();

    bool allowed = false;

    if (dia == 1) { allowed = true; }
    if (diaSemana == 1 and dia < 4) { allowed = true; }

    if (not allowed) { return qApp->enqueueError("Não está no primeiro dia útil do mês!", this); }
  }

  // -------------------------------------------------------------------------

  bool ok = true;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.data(row, "status").toString() != "PENDENTE") {
      ok = false;
      break;
    }
  }

  if (not ok) { return qApp->enqueueError("Um ou mais produtos não estão pendentes!", this); }

  // -------------------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar venda");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // -------------------------------------------------------------------------

  if (not qApp->startTransaction()) { return; }

  if (not cancelamento()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  const QString idOrcamento = ui->lineEditIdOrcamento->text();

  qApp->enqueueInformation(idOrcamento.isEmpty() ? "Não havia Orçamento associado para ativar!" : "Orçamento " + idOrcamento + " reativado!", this);

  qApp->enqueueInformation("Venda cancelada!", this);
  close();
}

bool Venda::generateId() {
  const auto siglaLoja = UserSession::fromLoja("sigla", ui->itemBoxVendedor->text());

  if (not siglaLoja) { return qApp->enqueueError(false, "Erro buscando sigla da loja!", this); }

  const auto idLoja = UserSession::fromLoja("loja.idLoja", ui->itemBoxVendedor->text());
  // TODO: V688 http://www.viva64.com/en/V688 The 'idLoja' local variable possesses the same name as one of the class members,
  // which can result in a confusion.  const auto idLoja = UserSession::fromLoja("loja.idLoja", ui->itemBoxVendedor->text());

  if (not idLoja) { return qApp->enqueueError(false, "Erro buscando idLoja!", this); }

  QString id = siglaLoja.value().toString() + "-" + ui->dateTimeEdit->date().toString("yy");

  QSqlQuery query;
  query.prepare("SELECT MAX(idVenda) AS idVenda FROM venda WHERE idVenda LIKE :id");
  query.bindValue(":id", id + "%");

  if (not query.exec()) { return qApp->enqueueError(false, "Erro na query: " + query.lastError().text(), this); }

  const int last = query.first() ? query.value("idVenda").toString().remove(id).leftRef(4).toInt() : 0;

  id += QString("%1").arg(last + 1, 4, 10, QChar('0'));

  if (id.size() != 11) { return qApp->enqueueError(false, "Ocorreu algum erro ao gerar id: " + id, this); }

  id += representacao ? "R" : "";

  ui->lineEditVenda->setText(id);

  return true;
}

void Venda::on_pushButtonDevolucao_clicked() {
  auto *devolucao = new Devolucao(data("idVenda").toString(), this);
  connect(devolucao, &Devolucao::finished, [&] { viewRegisterById(ui->lineEditVenda->text()); });
  devolucao->show();
}

void Venda::on_treeView_entered() {
  for (int col = 0; col < ui->treeView->model()->columnCount(); ++col) { ui->treeView->resizeColumnToContents(col); }
}

void Venda::on_dateTimeEdit_dateTimeChanged(const QDateTime &) { ui->widgetPgts->resetarPagamentos(); }

void Venda::setFinanceiro() {
  ui->groupBoxFinanceiro->show();
  ui->tableFluxoCaixa2->show();

  if (UserSession::tipoUsuario() != "ADMINISTRADOR" and UserSession::tipoUsuario() != "GERENTE DEPARTAMENTO") { ui->pushButtonCorrigirFluxo->hide(); }

  ui->frameButtons->hide();
  financeiro = true;
}

bool Venda::financeiroSalvar() {
  if (not atualizarCredito()) { return false; }

  if (not setData("statusFinanceiro", ui->comboBoxFinanceiro->currentText())) { return false; }

  if (not setData("dataFinanceiro", QDateTime::currentDateTime())) { return false; }

  if (not model.submitAll()) { return false; }

  if (not modelFluxoCaixa.submitAll()) { return false; }

  if (not modelFluxoCaixa2.submitAll()) { return false; }

  return true;
}

void Venda::on_pushButtonFinanceiroSalvar_clicked() {
  if (not verifyFields()) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not financeiroSalvar()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Dados salvos com sucesso!", this);
  close();
}

void Venda::on_pushButtonCorrigirFluxo_clicked() {
  QSqlQuery queryPag;
  queryPag.prepare("SELECT credito FROM cliente WHERE idCliente = :idCliente");
  queryPag.bindValue(":idCliente", data("idCliente"));

  if (not queryPag.exec() or not queryPag.first()) { return qApp->enqueueError("Erro lendo credito cliente: " + queryPag.lastError().text(), this); }

  double credito = queryPag.value("credito").toDouble();

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    if (modelFluxoCaixa.data(row, "tipo").toString().contains("Conta Cliente")) { credito += modelFluxoCaixa.data(row, "valor").toDouble(); }
  }

  ui->widgetPgts->setCredito(credito);

  // -------------------------------------------------------------------------

  if (not data("representacao").toBool()) { ui->tableFluxoCaixa->hideColumn("representacao"); }

  ui->splitter_2->show();
  ui->widgetPgts->show();
  ui->widgetPgts->resetarPagamentos();

  correcao = true;
}

void Venda::on_checkBoxPontuacaoPadrao_toggled(bool checked) {
  if (checked) {
    QSqlQuery query;
    query.prepare("SELECT comissao FROM profissional WHERE idProfissional = :idProfissional");
    query.bindValue(":idProfissional", ui->itemBoxProfissional->getId());

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando pontuação: " + query.lastError().text(), this); }

    double comissao = query.value("comissao").toDouble();

    ui->checkBoxPontuacaoIsento->setChecked(false);
    ui->doubleSpinBoxPontuacao->setEnabled(true);

    if (qFuzzyIsNull(comissao)) { comissao = 5; }

    ui->doubleSpinBoxPontuacao->setMaximum(comissao);
    ui->doubleSpinBoxPontuacao->setValue(comissao);
  }

  // TODO: 5criar linha em contas_pagar de comissao/rt
}

void Venda::on_checkBoxPontuacaoIsento_toggled(bool checked) {
  if (checked) {
    ui->checkBoxPontuacaoPadrao->setChecked(false);
    ui->doubleSpinBoxPontuacao->setValue(0);
    ui->doubleSpinBoxPontuacao->setDisabled(true);
  } else {
    ui->doubleSpinBoxPontuacao->setEnabled(true);
  }
}

void Venda::on_checkBoxRT_toggled(bool checked) {
  for (auto &item : ui->frameRT->findChildren<QWidget *>()) { item->setVisible(checked); }

  ui->checkBoxRT->setText(checked ? "Pontuação" : "");
}

void Venda::on_itemBoxProfissional_textChanged(const QString &) { on_checkBoxPontuacaoPadrao_toggled(ui->checkBoxPontuacaoPadrao->isChecked()); }

bool Venda::copiaProdutosOrcamento() {
  QSqlQuery queryProdutos;
  queryProdutos.prepare("SELECT * FROM orcamento_has_produto WHERE idOrcamento = :idOrcamento");
  queryProdutos.bindValue(":idOrcamento", ui->lineEditIdOrcamento->text());

  if (not queryProdutos.exec()) { return qApp->enqueueError(false, "Erro buscando produtos: " + queryProdutos.lastError().text(), this); }

  while (queryProdutos.next()) {
    const int rowItem = modelItem.insertRowAtEnd();

    for (int column = 0, columnCount = queryProdutos.record().count(); column < columnCount; ++column) {
      const QString field = queryProdutos.record().fieldName(column);

      if (field == "created") { continue; }
      if (field == "lastUpdated") { continue; }
      if (modelItem.fieldIndex(field, true) == -1) { continue; }

      if (not modelItem.setData(rowItem, field, queryProdutos.value(field))) { return false; }
    }

    if (not modelItem.setData(rowItem, "status", modelItem.data(rowItem, "estoque").toBool() ? "ESTOQUE" : "PENDENTE")) { return false; }
  }

  for (int row = 0; row < modelItem.rowCount(); ++row) { backupItem.append(modelItem.record(row)); }

  return true;
}

// TODO: 0hide 'nfe' field from tables that use conta_a_receber
// TODO: 0nao gerar RT quando o total for zero (e apagar os zerados quando nao houver profissional)
// TODO: 0se o pedido estiver cancelado ou devolvido bloquear os botoes correspondentes
// TODO: 0no corrigir fluxo esta mostrando os botoes de 'frete pago a loja' e 'pagamento total a loja' em pedidos que nao
// sao de representacao
// TODO: 5verificar se um pedido nao deveria ter seu 'statusFinanceiro' alterado para 'liberado'
// ao ter todos os pagamentos recebidos ('status' e 'statusFinanceiro' deveriam ser vinculados?)
// TODO: 0quando for 'MATR' nao criar fluxo caixa
// NOTE: prazoEntrega por produto
// NOTE: bloquear desconto maximo por classe de funcionario
// TODO: 2no caso de reposicao colocar formas de pagamento diferenciado ou nao usar pagamento?
// REFAC: em vez de ter uma caixinha 'un', concatenar em 'quant', 'minimo' e 'un/cx'
// TODO: usar coluna 'idRelacionado' para vincular a linha quebrada/reposicao com a correspondente
// TODO: depois de cadastrar venda esconder os elementos graficos da pontuacao

// NOTE: for coloring childs:
//      QTreeView::item:!has-children
//      {
//      	background-color: rgb(255, 85, 0);
//      }

// TODO: implement proxyModel for treeView to color lines if estoque/promocao
