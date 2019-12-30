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
#include "sql.h"
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

  for (auto &widget : ui->frameRT->findChildren<QWidget *>()) { widget->setHidden(true); }

  ui->splitter->setStretchFactor(0, 255);
  ui->splitter->setStretchFactor(1, 1);

  show();
}

Venda::~Venda() { delete ui; }

void Venda::setTreeView() {
  modelTree.appendModel(&modelItem);
  modelTree.appendModel(&modelItem2);

  modelTree.updateData();

  modelTree.setHeaderData("status", "Status");
  modelTree.setHeaderData("fornecedor", "Fornecedor");
  modelTree.setHeaderData("produto", "Produto");
  modelTree.setHeaderData("obs", "Obs.");
  modelTree.setHeaderData("lote", "Lote");
  modelTree.setHeaderData("prcUnitario", "Preço/Un");
  modelTree.setHeaderData("caixas", "Caixas");
  modelTree.setHeaderData("quant", "Quant.");
  modelTree.setHeaderData("un", "Un.");
  modelTree.setHeaderData("unCaixa", "Un./Cx.");
  modelTree.setHeaderData("codComercial", "Código");
  modelTree.setHeaderData("formComercial", "Formato");
  modelTree.setHeaderData("parcial", "Subtotal");
  modelTree.setHeaderData("desconto", "Desc. %");
  modelTree.setHeaderData("parcialDesc", "Desc. Parc.");
  modelTree.setHeaderData("descGlobal", "Desc. Glob. %");
  modelTree.setHeaderData("total", "Total");
  modelTree.setHeaderData("dataPrevCompra", "Prev. Compra");
  modelTree.setHeaderData("dataRealCompra", "Data Compra");
  modelTree.setHeaderData("dataPrevConf", "Prev. Confirm.");
  modelTree.setHeaderData("dataRealConf", "Data Confirm.");
  modelTree.setHeaderData("dataPrevFat", "Prev. Fat.");
  modelTree.setHeaderData("dataRealFat", "Data Fat.");
  modelTree.setHeaderData("dataPrevColeta", "Prev. Coleta");
  modelTree.setHeaderData("dataRealColeta", "Data Coleta");
  modelTree.setHeaderData("dataPrevReceb", "Prev. Receb.");
  modelTree.setHeaderData("dataRealReceb", "Data Receb.");
  modelTree.setHeaderData("dataPrevEnt", "Prev. Ent.");
  modelTree.setHeaderData("dataRealEnt", "Data Ent.");

  modelTree.proxyModel = new SearchDialogProxyModel(&modelTree, this);

  ui->treeView->setModel(&modelTree);

  ui->treeView->hideColumn("selecionado");
  ui->treeView->hideColumn("idRelacionado");
  ui->treeView->hideColumn("statusOriginal");
  ui->treeView->hideColumn("recebeu");
  ui->treeView->hideColumn("entregou");
  ui->treeView->hideColumn("descUnitario");
  ui->treeView->hideColumn("estoque");
  ui->treeView->hideColumn("promocao");
  ui->treeView->hideColumn("idCompra");
  ui->treeView->hideColumn("idNFeSaida");
  ui->treeView->hideColumn("idNFeFutura");
  ui->treeView->hideColumn("idVenda");
  ui->treeView->hideColumn("idLoja");
  ui->treeView->hideColumn("idProduto");
  ui->treeView->hideColumn("reposicaoEntrega");
  ui->treeView->hideColumn("reposicaoReceb");
  ui->treeView->hideColumn("mostrarDesconto");
  ui->treeView->hideColumn("created");
  ui->treeView->hideColumn("lastUpdated");

  ui->treeView->setItemDelegate(new QTreeViewGridDelegate(this));

  ui->treeView->setItemDelegateForColumn("quant", new DoubleDelegate(this, 4, true));
  ui->treeView->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this, 2, true));
  ui->treeView->setItemDelegateForColumn("parcial", new ReaisDelegate(this, 2, true));
  ui->treeView->setItemDelegateForColumn("parcialDesc", new ReaisDelegate(this, 2, true));
  ui->treeView->setItemDelegateForColumn("desconto", new PorcentagemDelegate(this, true));
  ui->treeView->setItemDelegateForColumn("descGlobal", new PorcentagemDelegate(this, true));
  ui->treeView->setItemDelegateForColumn("total", new ReaisDelegate(this, 2, true));
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
  ui->lineEditIdOrcamento->setText(idOrcamento);
  ui->lineEditVenda->setText("Auto gerado");
  ui->dateTimeEdit->setDateTime(qApp->serverDateTime());

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

  const bool freteManual = queryOrc.value("freteManual").toBool();
  const double frete = queryOrc.value("frete").toDouble();

  ui->doubleSpinBoxFrete->setMinimum(freteManual ? 0 : frete);
  ui->doubleSpinBoxFrete->setValue(frete);
  ui->checkBoxFreteManual->setChecked(freteManual);
  canChangeFrete = freteManual;

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

  setConnections();
}

bool Venda::verifyFields() {
  // TODO: pintar campos errados de vermelho
  // TODO: pintar campos necessarios de amarelo
  // TODO: pintar campos certos de verde
  // TODO: pintar totalPag de vermelho enquanto o total for diferente

  // TODO: copiar do orçamento a verificação de disponibilidade de estoque?

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

  if (not financeiro and ui->itemBoxProfissional->getId() != 1 and not ui->checkBoxPontuacaoIsento->isChecked() and not ui->checkBoxPontuacaoPadrao->isChecked()) {
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
  ui->tableFluxoCaixa2->hide();
  ui->widgetPgts->hide();
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

  const auto ok = [&] {
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

    ui->doubleSpinBoxFrete->setReadOnly(true);
    ui->doubleSpinBoxFrete->setButtonSymbols(QDoubleSpinBox::NoButtons);

    if (data("status") == "CANCELADO" or data("status") == "DEVOLUÇÃO") {
      ui->pushButtonCancelamento->hide();
      ui->pushButtonDevolucao->hide();
    }

    const QString tipoUsuario = UserSession::tipoUsuario();

    if (not tipoUsuario.contains("GERENTE") and tipoUsuario != "DIRETOR" and tipoUsuario != "ADMINISTRADOR" and tipoUsuario != "ADMINISTRATIVO") {
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

    ui->tableFluxoCaixa2->hide();
    ui->widgetPgts->hide();

    return true;
  }();

  setConnections();

  return ok;
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

      const QDate dataEmissao = correcao ? modelFluxoCaixa.data(0, "dataEmissao").toDate() : qApp->serverDate();

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

      // calculo comissao loja
      for (int z = 0, total = modelFluxoCaixa.rowCount(); z < total; ++z) {
        if (modelFluxoCaixa.data(z, "representacao").toBool() and modelFluxoCaixa.data(z, "observacao").toString() == "Frete") { continue; }
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

        const bool isFreteLoja = ui->widgetPgts->listLinePgt.at(0)->text() == "Frete";

        const double valorAjustado = isFreteLoja ? valorComissao * taxaComissao : taxaComissao * (valorComissao - (valorComissao / totalVenda * frete));

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

  [&] {
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
  }();

  setConnections();
}

void Venda::on_checkBoxFreteManual_clicked(const bool checked) {
  if (not canChangeFrete) {
    qApp->enqueueInformation("Necessário autorização de um gerente ou administrador!", this);

    LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

    if (dialog.exec() != QDialog::Accepted) {
      ui->checkBoxFreteManual->setChecked(not checked);
      return;
    }

    canChangeFrete = true;
  }

  const double frete = qMax(ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100., minimoFrete);

  ui->doubleSpinBoxFrete->setMinimum(checked ? 0 : frete);

  if (not checked) { ui->doubleSpinBoxFrete->setValue(frete); }
}

void Venda::on_doubleSpinBoxFrete_valueChanged(const double frete) {
  const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
  const double desconto = ui->doubleSpinBoxDescontoGlobalReais->value();

  unsetConnections();

  [&] {
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - desconto + frete);
    ui->widgetPgts->setFrete(frete);
    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    ui->widgetPgts->resetarPagamentos();
  }();

  setConnections();
}

void Venda::on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) {
  unsetConnections();

  [&] {
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

  [&] {
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

    if (ui->checkBoxPontuacaoPadrao->isChecked() and ui->itemBoxProfissional->getId() != 1) {
      const QDate date = ui->dateTimeEdit->date();
      const double valor = (ui->doubleSpinBoxSubTotalLiq->value() - ui->doubleSpinBoxDescontoGlobalReais->value()) * ui->doubleSpinBoxPontuacao->value() / 100;

      QSqlQuery query1;
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

    if (not criarConsumos()) { return false; }

    if (not Sql::updateVendaStatus(ui->lineEditVenda->text())) { return false; }

    return true;
  }();

  if (success) {
    if (not qApp->endTransaction()) { return false; }

    backupItem.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelItem.setFilter(primaryKey + " = '" + primaryId + "'");

    modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND comissao = FALSE AND taxa = FALSE");
  } else {
    qApp->rollbackTransaction();
    void(model.select());
    void(modelItem.select());

    for (auto &record : backupItem) { modelItem.insertRecord(-1, record); }
  }

  return success;
}

bool Venda::criarConsumos() {
  QSqlQuery query;
  query.prepare("SELECT p.idEstoque, vp2.idVendaProduto2, vp2.quant FROM venda_has_produto2 vp2 LEFT JOIN produto p ON vp2.idProduto = p.idProduto WHERE vp2.idVenda = :idVenda AND vp2.estoque > 0");
  query.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query.exec()) { return qApp->enqueueError(false, "Erro buscando produtos estoque: " + query.lastError().text(), this); }

  while (query.next()) {
    auto *estoque = new Estoque(query.value("idEstoque").toString(), false, this);

    if (not estoque->criarConsumo(query.value("idVendaProduto2").toInt(), query.value("quant").toDouble())) { return false; }
  }

  return true;
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

  QSqlQuery queryDelete;
  queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto2 IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda)");
  queryDelete.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not queryDelete.exec()) { return qApp->enqueueError(false, "Erro removendo consumo estoque: " + queryDelete.lastError().text(), this); }

  // -------------------------------------------------------------------------

  QSqlQuery query3;
  query3.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, `idVendaProduto2` = NULL WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda)");
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
      // TODO: é gerado crédito mesmo se a conta nao chegou a ser paga?
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
  const QDate dataAtual = qApp->serverDate();

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
    if (modelItem.data(row, "estoque").toInt() > 0) { continue; }

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

  QString id = siglaLoja->toString() + "-" + ui->dateTimeEdit->date().toString("yy");

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

void Venda::on_dateTimeEdit_dateTimeChanged(const QDateTime &) { ui->widgetPgts->resetarPagamentos(); }

void Venda::setFinanceiro() {
  ui->groupBoxFinanceiro->show();
  ui->tableFluxoCaixa2->show();

  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario != "ADMINISTRADOR" and tipoUsuario != "ADMINISTRATIVO" and tipoUsuario != "GERENTE DEPARTAMENTO") { ui->pushButtonCorrigirFluxo->hide(); }

  ui->frameButtons->hide();
  financeiro = true;
}

bool Venda::financeiroSalvar() {
  if (not atualizarCredito()) { return false; }

  if (not setData("statusFinanceiro", ui->comboBoxFinanceiro->currentText())) { return false; }

  if (not setData("dataFinanceiro", qApp->serverDateTime())) { return false; }

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

  ui->tableFluxoCaixa2->show();
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
  for (auto &widget : ui->frameRT->findChildren<QWidget *>()) { widget->setVisible(checked); }

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

    if (modelItem.data(rowItem, "estoque").toInt() > 0) {
      QSqlQuery queryStatus;

      if (not queryStatus.exec("SELECT e.status FROM estoque e LEFT JOIN produto p ON e.idEstoque = p.idEstoque WHERE p.idProduto = " + modelItem.data(rowItem, "idProduto").toString()) or
          not queryStatus.first()) {
        return qApp->enqueueError(false, "Erro buscando status do estoque: " + queryStatus.lastError().text(), this);
      }

      if (not modelItem.setData(rowItem, "status", queryStatus.value("status"))) { return false; }
    } else {
      if (not modelItem.setData(rowItem, "status", "PENDENTE")) { return false; }
    }
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
