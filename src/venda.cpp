#include "venda.h"
#include "ui_venda.h"

#include "acbrlib.h"
#include "application.h"
#include "calculofrete.h"
#include "checkboxdelegate.h"
#include "comprovantes.h"
#include "devolucao.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "estoque.h"
#include "excel.h"
#include "file.h"
#include "log.h"
#include "logindialog.h"
#include "noeditdelegate.h"
#include "orcamento.h"
#include "pdf.h"
#include "porcentagemdelegate.h"
#include "produtoproxymodel.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "user.h"

#include <QAuthenticator>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QInputDialog>
#include <QMessageBox>
#include <QNetworkReply>
#include <QSqlError>
#include <QSqlRecord>

Venda::Venda(QWidget *parent) : RegisterDialog("venda", "idVenda", parent), ui(new Ui::Venda) {
  ui->setupUi(this);

  connectLineEditsToDirty();
  setItemBoxes();
  setupTables();
  setupMapper();
  newRegister();

  ui->widgetPgts->setTipo(WidgetPagamentos::Tipo::Venda);

  if (User::isAdministrativo()) {
    ui->dateTimeEdit->setReadOnly(false);
    ui->dateTimeEdit->setCalendarPopup(true);
  }

  ui->splitter->setStretchFactor(0, 255);
  ui->splitter->setStretchFactor(1, 1);

  ui->splitter_3->setStretchFactor(0, 1);
  ui->splitter_3->setStretchFactor(1, 255);
  ui->splitter_3->setStretchFactor(2, 1);

  ui->lineEditVenda->setResizeToText();
}

Venda::~Venda() { delete ui; }

void Venda::setItemBoxes() {
  ui->itemBoxCliente->setRegisterDialog("CadastroCliente");
  ui->itemBoxCliente->setSearchDialog(SearchDialog::cliente(this));
  ui->itemBoxConsultor->setSearchDialog(SearchDialog::vendedor(this));
  ui->itemBoxEndereco->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxEnderecoFat->setSearchDialog(SearchDialog::enderecoCliente(this));
  ui->itemBoxProfissional->setRegisterDialog("CadastroProfissional");
  ui->itemBoxProfissional->setSearchDialog(SearchDialog::profissional(true, this));
  ui->itemBoxVendedor->setSearchDialog(SearchDialog::vendedor(this));
}

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
  modelTree.setHeaderData("kg", "Kg.");
  modelTree.setHeaderData("caixas", "Caixas");
  modelTree.setHeaderData("quant", "Quant.");
  modelTree.setHeaderData("un", "Un.");
  modelTree.setHeaderData("quantCaixa", "Quant./Cx.");
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

  modelTree.proxyModel = new ProdutoProxyModel(&modelTree, this);

  ui->treeView->setModel(&modelTree);

  ui->treeView->hideColumn("idNFeEntrada");
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

  ui->treeView->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));
  ui->treeView->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("parcial", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("parcialDesc", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("desconto", new PorcentagemDelegate(false, this));
  ui->treeView->setItemDelegateForColumn("descGlobal", new PorcentagemDelegate(false, this));
  ui->treeView->setItemDelegateForColumn("total", new ReaisDelegate(this));
}

void Venda::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxFreteManual, &QCheckBox::clicked, this, &Venda::on_checkBoxFreteManual_clicked, connectionType);
  connect(ui->checkBoxMostrarCancelados, &QCheckBox::toggled, this, &Venda::on_checkBoxMostrarCancelados_toggled, connectionType);
  connect(ui->checkBoxPontuacaoIsento, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoIsento_toggled, connectionType);
  connect(ui->checkBoxPontuacaoPadrao, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoPadrao_toggled, connectionType);
  connect(ui->checkBoxRT, &QCheckBox::toggled, this, &Venda::on_checkBoxRT_toggled, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &Venda::on_dateTimeEdit_dateTimeChanged, connectionType);
  connect(ui->doubleSpinBoxDescontoGlobal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobal_valueChanged, connectionType);
  connect(ui->doubleSpinBoxDescontoGlobalReais, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobalReais_valueChanged, connectionType);
  connect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxFrete_valueChanged, connectionType);
  connect(ui->doubleSpinBoxTotal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxTotal_valueChanged, connectionType);
  connect(ui->itemBoxEndereco, &ItemBox::idChanged, this, &Venda::on_itemBoxEndereco_idChanged, connectionType);
  connect(ui->itemBoxProfissional, &ItemBox::textChanged, this, &Venda::on_itemBoxProfissional_textChanged, connectionType);
  connect(ui->pushButtonAbrirOrcamento, &QPushButton::clicked, this, &Venda::on_pushButtonAbrirOrcamento_clicked, connectionType);
  connect(ui->pushButtonAdicionarObservacao, &QPushButton::clicked, this, &Venda::on_pushButtonAdicionarObservacao_clicked, connectionType);
  connect(ui->pushButtonAtualizarEnderecoEnt, &QPushButton::clicked, this, &Venda::on_pushButtonAtualizarEnderecoEnt_clicked, connectionType);
  connect(ui->pushButtonCadastrarPedido, &QPushButton::clicked, this, &Venda::on_pushButtonCadastrarVenda_clicked, connectionType);
  connect(ui->pushButtonCancelamento, &QPushButton::clicked, this, &Venda::on_pushButtonCancelamento_clicked, connectionType);
  connect(ui->pushButtonComprovantes, &QPushButton::clicked, this, &Venda::on_pushButtonComprovantes_clicked, connectionType);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &Venda::on_pushButtonCorrigirFluxo_clicked, connectionType);
  connect(ui->pushButtonDevolucao, &QPushButton::clicked, this, &Venda::on_pushButtonDevolucao_clicked, connectionType);
  connect(ui->pushButtonFinanceiroSalvar, &QPushButton::clicked, this, &Venda::on_pushButtonFinanceiroSalvar_clicked, connectionType);
  connect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Venda::on_pushButtonGerarExcel_clicked, connectionType);
  connect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Venda::on_pushButtonGerarPdf_clicked, connectionType);
  connect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &Venda::on_pushButtonModelo3d_clicked, connectionType);
  connect(ui->pushButtonVoltar, &QPushButton::clicked, this, &Venda::on_pushButtonVoltar_clicked, connectionType);
  connect(ui->treeView, &QTreeView::doubleClicked, this, &Venda::on_treeView_doubleClicked, connectionType);
  connect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, this, &Venda::montarFluxoCaixa, connectionType);
}

void Venda::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxFreteManual, &QCheckBox::clicked, this, &Venda::on_checkBoxFreteManual_clicked);
  disconnect(ui->checkBoxMostrarCancelados, &QCheckBox::toggled, this, &Venda::on_checkBoxMostrarCancelados_toggled);
  disconnect(ui->checkBoxPontuacaoIsento, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoIsento_toggled);
  disconnect(ui->checkBoxPontuacaoPadrao, &QCheckBox::toggled, this, &Venda::on_checkBoxPontuacaoPadrao_toggled);
  disconnect(ui->checkBoxRT, &QCheckBox::toggled, this, &Venda::on_checkBoxRT_toggled);
  disconnect(ui->dateTimeEdit, &QDateTimeEdit::dateTimeChanged, this, &Venda::on_dateTimeEdit_dateTimeChanged);
  disconnect(ui->doubleSpinBoxDescontoGlobal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobal_valueChanged);
  disconnect(ui->doubleSpinBoxDescontoGlobalReais, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxDescontoGlobalReais_valueChanged);
  disconnect(ui->doubleSpinBoxFrete, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxFrete_valueChanged);
  disconnect(ui->doubleSpinBoxTotal, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &Venda::on_doubleSpinBoxTotal_valueChanged);
  disconnect(ui->itemBoxEndereco, &ItemBox::idChanged, this, &Venda::on_itemBoxEndereco_idChanged);
  disconnect(ui->itemBoxProfissional, &ItemBox::textChanged, this, &Venda::on_itemBoxProfissional_textChanged);
  disconnect(ui->pushButtonAbrirOrcamento, &QPushButton::clicked, this, &Venda::on_pushButtonAbrirOrcamento_clicked);
  disconnect(ui->pushButtonAdicionarObservacao, &QPushButton::clicked, this, &Venda::on_pushButtonAdicionarObservacao_clicked);
  disconnect(ui->pushButtonAtualizarEnderecoEnt, &QPushButton::clicked, this, &Venda::on_pushButtonAtualizarEnderecoEnt_clicked);
  disconnect(ui->pushButtonCadastrarPedido, &QPushButton::clicked, this, &Venda::on_pushButtonCadastrarVenda_clicked);
  disconnect(ui->pushButtonCancelamento, &QPushButton::clicked, this, &Venda::on_pushButtonCancelamento_clicked);
  disconnect(ui->pushButtonComprovantes, &QPushButton::clicked, this, &Venda::on_pushButtonComprovantes_clicked);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &Venda::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonDevolucao, &QPushButton::clicked, this, &Venda::on_pushButtonDevolucao_clicked);
  disconnect(ui->pushButtonFinanceiroSalvar, &QPushButton::clicked, this, &Venda::on_pushButtonFinanceiroSalvar_clicked);
  disconnect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Venda::on_pushButtonGerarExcel_clicked);
  disconnect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Venda::on_pushButtonGerarPdf_clicked);
  disconnect(ui->pushButtonModelo3d, &QPushButton::clicked, this, &Venda::on_pushButtonModelo3d_clicked);
  disconnect(ui->pushButtonVoltar, &QPushButton::clicked, this, &Venda::on_pushButtonVoltar_clicked);
  disconnect(ui->treeView, &QTreeView::doubleClicked, this, &Venda::on_treeView_doubleClicked);
  disconnect(ui->widgetPgts, &WidgetPagamentos::montarFluxoCaixa, this, &Venda::montarFluxoCaixa);
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

  ui->tableFluxoCaixa->hideColumn("idNFe");
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
  ui->tableFluxoCaixa->hideColumn("idConta");
  ui->tableFluxoCaixa->hideColumn("tipoDet");
  ui->tableFluxoCaixa->hideColumn("centroCusto");
  ui->tableFluxoCaixa->hideColumn("grupo");
  ui->tableFluxoCaixa->hideColumn("subGrupo");
  ui->tableFluxoCaixa->hideColumn("comissao");
  ui->tableFluxoCaixa->hideColumn("taxa");
  ui->tableFluxoCaixa->hideColumn("desativado");

  ui->tableFluxoCaixa->setItemDelegate(new NoEditDelegate(this));

  ui->tableFluxoCaixa->setItemDelegateForColumn("representacao", new CheckBoxDelegate(true, this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("dataPagamento", new EditDelegate(this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("observacao", new EditDelegate(this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("valor", new ReaisDelegate(2, true, this));

  ui->tableFluxoCaixa->setPersistentColumns({"representacao"});

  //-----------------------------------------------------------------

  modelFluxoCaixa2.setTable("conta_a_receber_has_pagamento");

  modelFluxoCaixa2.setHeaderData("contraParte", "Contraparte");
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
  ui->tableFluxoCaixa2->hideColumn("idNFe");
  ui->tableFluxoCaixa2->hideColumn("nfe");
  ui->tableFluxoCaixa2->hideColumn("representacao");
  ui->tableFluxoCaixa2->hideColumn("dataRealizado");
  ui->tableFluxoCaixa2->hideColumn("valorReal");
  ui->tableFluxoCaixa2->hideColumn("tipoReal");
  ui->tableFluxoCaixa2->hideColumn("parcelaReal");
  ui->tableFluxoCaixa2->hideColumn("idConta");
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
  ui->dateTimeEdit->setDate(qApp->serverDate());

  copiaProdutosOrcamento();

  setTreeView();

  // -------------------------------------------------------------------------

  SqlQuery queryOrc;
  queryOrc.prepare("SELECT idUsuario, idLoja, idUsuarioConsultor, idCliente, idEnderecoEntrega, idProfissional, data, subTotalBru, subTotalLiq, frete, "
                   "freteManual, descontoPorc, descontoReais, total, status, observacao, prazoEntrega, representacao FROM orcamento WHERE idOrcamento = :idOrcamento");
  queryOrc.bindValue(":idOrcamento", idOrcamento);

  if (not queryOrc.exec()) { throw RuntimeException("Erro buscando orçamento: " + queryOrc.lastError().text()); }

  if (not queryOrc.first()) { throw RuntimeException("Orçamento não encontrado: '" + idOrcamento + "'"); }

  ui->itemBoxVendedor->setId(queryOrc.value("idUsuario"));
  ui->itemBoxConsultor->setId(queryOrc.value("idUsuarioConsultor"));
  ui->itemBoxEndereco->setFilter("idCliente = " + queryOrc.value("idCliente").toString() + " AND desativado = FALSE");
  ui->itemBoxEnderecoFat->setFilter("idCliente = " + queryOrc.value("idCliente").toString() + " AND desativado = FALSE");
  ui->itemBoxCliente->setId(queryOrc.value("idCliente"));
  ui->itemBoxProfissional->setId(queryOrc.value("idProfissional"));
  ui->itemBoxEndereco->setId(queryOrc.value("idEnderecoEntrega"));
  ui->dateTimeEditOrc->setDateTime(queryOrc.value("data").toDateTime());
  ui->plainTextEditObs->setPlainText(queryOrc.value("observacao").toString());
  ui->doubleSpinBoxSubTotalBruto->setValue(queryOrc.value("subTotalBru").toDouble());
  ui->doubleSpinBoxSubTotalLiq->setValue(queryOrc.value("subTotalLiq").toDouble());

  const double frete = queryOrc.value("frete").toDouble();

  if (User::isGerente()) { calcularFrete(false); }
  ui->doubleSpinBoxFrete->setValue(frete);
  canChangeFrete = queryOrc.value("freteManual").toBool();

  if (canChangeFrete) {
    ui->checkBoxFreteManual->setChecked(true);
    ui->checkBoxFreteManual->setDisabled(true);
  }

  ui->doubleSpinBoxDescontoGlobal->setValue(queryOrc.value("descontoPorc").toDouble());
  ui->doubleSpinBoxDescontoGlobalReais->setValue(queryOrc.value("descontoReais").toDouble());
  ui->doubleSpinBoxTotal->setValue(queryOrc.value("total").toDouble());
  ui->spinBoxPrazoEntrega->setValue(queryOrc.value("prazoEntrega").toInt());

  idLoja = queryOrc.value("idLoja").toInt();
  representacao = queryOrc.value("representacao").toBool();

  if (representacao) {
    ui->checkBoxFreteManual->setDisabled(true);
    ui->checkBoxFreteManual->setChecked(true);
  }

  // -------------------------------------------------------------------------

  SqlQuery queryFrete;
  queryFrete.prepare("SELECT valorMinimoFrete, porcentagemFrete FROM loja WHERE idLoja = :idLoja");
  queryFrete.bindValue(":idLoja", idLoja);

  if (not queryFrete.exec()) { throw RuntimeException("Erro buscando parâmetros do frete: " + queryFrete.lastError().text()); }

  if (not queryFrete.first()) { throw RuntimeException("Dados do frete não encontrado!"); }

  minimoFrete = queryFrete.value("valorMinimoFrete").toDouble();
  porcFrete = queryFrete.value("porcentagemFrete").toDouble();

  if (not representacao) { ui->tableFluxoCaixa->hideColumn("representacao"); }

  // -------------------------------------------------------------------------

  SqlQuery queryCredito;
  queryCredito.prepare("SELECT credito FROM cliente WHERE idCliente = :idCliente");
  queryCredito.bindValue(":idCliente", queryOrc.value("idCliente"));

  if (not queryCredito.exec()) { throw RuntimeException("Erro buscando crédito cliente: " + queryCredito.lastError().text()); }

  if (not queryCredito.first()) { throw RuntimeException("Crédito do cliente não encontrado!"); }

  ui->widgetPgts->setCredito(queryCredito.value("credito").toDouble());

  // -------------------------------------------------------------------------

  ui->widgetPgts->setIdLoja(idLoja);
  ui->widgetPgts->setRepresentacao(representacao);
  ui->widgetPgts->setFrete(ui->doubleSpinBoxFrete->value());
  ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());

  ui->widgetPgts->setMinimumWidth(550);

  if (ui->itemBoxConsultor->getId().isValid()) {
    ui->labelConsultor->show();
    ui->itemBoxConsultor->show();
  }

  // -------------------------------------------------------------------------

  if (representacao) { verificaFreteLoja(); }

  // -------------------------------------------------------------------------

  const bool naoHa = (ui->itemBoxProfissional->text() == "NÃO HÁ");

  ui->frameRT->setDisabled(naoHa);

  // -------------------------------------------------------------------------

  calcularPesoTotal();

  // -------------------------------------------------------------------------

  setConnections();
}

void Venda::verifyFields() {
  // TODO: pintar campos errados de vermelho
  // TODO: pintar campos necessarios de amarelo
  // TODO: pintar campos certos de verde
  // TODO: pintar totalPag de vermelho enquanto o total for diferente

  if (not financeiro) { verificaDisponibilidadeEstoque(); }

  if (ui->widgetPgts->isVisible()) {
    if (abs(ui->widgetPgts->getTotalPag() - ui->doubleSpinBoxTotal->value()) > 0.1) { throw RuntimeError("Total dos pagamentos difere do total do pedido!"); }

    ui->widgetPgts->verifyFields();

    if (modelFluxoCaixa.rowCount() == 0) { throw RuntimeError("Sem linhas de pagamento!"); }
  }

  if (ui->spinBoxPrazoEntrega->value() == 0) { throw RuntimeError("Por favor preencha o prazo de entrega!"); }

  if (ui->itemBoxEnderecoFat->text().isEmpty()) { throw RuntimeError("Deve selecionar um endereço de faturamento!"); }

  if (not financeiro and ui->itemBoxProfissional->text() != "NÃO HÁ" and not ui->checkBoxPontuacaoIsento->isChecked() and not ui->checkBoxPontuacaoPadrao->isChecked()) {
    ui->checkBoxRT->setChecked(true);
    throw RuntimeError("Por favor preencha a pontuação!");
  }
}

void Venda::clearFields() {}

void Venda::setupMapper() {
  addMapping(ui->checkBoxFreteManual, "freteManual");
  addMapping(ui->comboBoxFinanceiro, "statusFinanceiro");
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
  addMapping(ui->plainTextEditObs, "observacao");
  addMapping(ui->spinBoxPrazoEntrega, "prazoEntrega");
}

void Venda::on_pushButtonCadastrarVenda_clicked() { save(); }

void Venda::savingProcedures() {
  generateId();

  setData("data", ui->dateTimeEdit->isReadOnly() ? qApp->serverDateTime() : ui->dateTimeEdit->dateTime());
  setData("data2", data("data").toDate().toString("yyyy-MM"));
  setData("data3", data("data").toDate().toString("yyyy-MM-dd"));
  setData("dataOrc", ui->dateTimeEditOrc->dateTime());
  setData("descontoPorc", ui->doubleSpinBoxDescontoGlobal->value());
  setData("descontoReais", ui->doubleSpinBoxDescontoGlobalReais->value());
  setData("frete", ui->doubleSpinBoxFrete->value());
  setData("freteManual", ui->checkBoxFreteManual->isChecked());
  setData("idCliente", ui->itemBoxCliente->getId());
  setData("idEnderecoEntrega", ui->itemBoxEndereco->getId());
  setData("idEnderecoFaturamento", ui->itemBoxEnderecoFat->getId());
  setData("idLoja", idLoja);
  setData("idOrcamento", ui->lineEditIdOrcamento->text());
  setData("idProfissional", ui->itemBoxProfissional->getId());
  setData("idUsuario", ui->itemBoxVendedor->getId());
  setData("idUsuarioConsultor", ui->itemBoxConsultor->getId());
  setData("idVenda", ui->lineEditVenda->text());
  setData("idVendaBase", ui->lineEditVenda->text().left(11));
  setData("novoPrazoEntrega", ui->spinBoxPrazoEntrega->value());
  setData("observacao", ui->plainTextEditObs->toPlainText());
  setData("prazoEntrega", ui->spinBoxPrazoEntrega->value());
  setData("representacao", ui->lineEditVenda->text().endsWith("R") ? 1 : 0);
  setData("rt", ui->doubleSpinBoxPontuacao->value());
  setData("subTotalBru", ui->doubleSpinBoxSubTotalBruto->value());
  setData("subTotalLiq", ui->doubleSpinBoxSubTotalLiq->value());
  setData("total", ui->doubleSpinBoxTotal->value());
}

void Venda::registerMode() {
  ui->checkBoxFreteManual->show();
  ui->framePagamentos->show();
  ui->pushButtonCadastrarPedido->show();
  ui->pushButtonVoltar->show();

  ui->groupBoxFinanceiro->hide();
  ui->itemBoxConsultor->hide();
  ui->labelConsultor->hide();
  ui->labelRT->hide();
  ui->pushButtonAdicionarObservacao->hide();
  ui->pushButtonAtualizarEnderecoEnt->hide();
  ui->pushButtonCancelamento->hide();
  ui->pushButtonComprovantes->hide();
  ui->pushButtonDevolucao->hide();
  ui->pushButtonGerarExcel->hide();
  ui->pushButtonGerarPdf->hide();
  ui->tableFluxoCaixa2->hide();

  ui->itemBoxConsultor->setReadOnlyItemBox(true);

  ui->doubleSpinBoxDescontoGlobal->setReadOnly(false);
  ui->doubleSpinBoxTotal->setReadOnly(false);

  ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::UpDownArrows);
  ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::UpDownArrows);

  for (auto &widget : ui->frameRT->findChildren<QWidget *>()) { widget->setHidden(true); }
}

void Venda::updateMode() {
  ui->pushButtonAdicionarObservacao->show();
  ui->pushButtonCancelamento->show();
  ui->pushButtonComprovantes->show();
  ui->pushButtonDevolucao->show();
  ui->pushButtonGerarExcel->show();
  ui->pushButtonGerarPdf->show();

  ui->checkBoxRT->hide();
  ui->frameRT->hide();
  ui->pushButtonAtualizarEnderecoEnt->hide();
  ui->pushButtonCadastrarPedido->hide();
  ui->pushButtonVoltar->hide();
  ui->widgetPgts->hide();

  ui->checkBoxFreteManual->setDisabled(true);

  ui->checkBoxRT->setChecked(false);

  ui->itemBoxCliente->setReadOnlyItemBox(true);
  ui->itemBoxEndereco->setReadOnlyItemBox(true);
  ui->itemBoxEnderecoFat->setReadOnlyItemBox(true);
  ui->itemBoxProfissional->setReadOnlyItemBox(true);
  ui->itemBoxVendedor->setReadOnlyItemBox(true);

  ui->dateTimeEdit->setReadOnly(true);
  ui->doubleSpinBoxDescontoGlobal->setReadOnly(true);
  ui->doubleSpinBoxDescontoGlobalReais->setReadOnly(true);
  ui->doubleSpinBoxFrete->setReadOnly(true);
  ui->doubleSpinBoxTotal->setReadOnly(true);
  ui->plainTextEditObs->setReadOnly(true);
  ui->spinBoxPrazoEntrega->setReadOnly(true);

  ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxDescontoGlobalReais->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxFrete->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->spinBoxPrazoEntrega->setButtonSymbols(QSpinBox::NoButtons);

  ui->tableFluxoCaixa->setEditTriggers(QAbstractItemView::NoEditTriggers);

  ui->dateTimeEdit->setCalendarPopup(false);
  ui->dateTimeEdit->setButtonSymbols(QDateTimeEdit::NoButtons);
}

bool Venda::viewRegister() {
  unsetConnections();

  const auto ok = [&] {
    if (not RegisterDialog::viewRegister()) { return false; }

    //-----------------------------------------------------------------

    modelItem.setFilter("idVenda = '" + model.data(0, "idVenda").toString() + "'");

    modelItem.select();

    modelItem2.setFilter("idVenda = '" + model.data(0, "idVenda").toString() + "'");

    modelItem2.select();

    setTreeView();

    modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND comissao = FALSE AND taxa = FALSE");

    modelFluxoCaixa.select();

    idLoja = data("idLoja").toInt();
    representacao = data("representacao").toBool();

    if (not representacao) { ui->tableFluxoCaixa->hideColumn("representacao"); }

    if (financeiro) {
      // TODO: 1quando estiver tudo pago bloquear correcao de fluxo
      modelFluxoCaixa2.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND (comissao = TRUE OR taxa = TRUE)");

      modelFluxoCaixa2.select();

      for (auto &widget : ui->frameRT->findChildren<QWidget *>()) { widget->show(); }

      ui->frameRT->show();
      ui->labelRT->show();
      ui->checkBoxPontuacaoIsento->hide();
      ui->checkBoxPontuacaoPadrao->hide();
      ui->checkBoxRT->hide();

      ui->doubleSpinBoxPontuacao->setValue(data("rt").toDouble());
      ui->doubleSpinBoxPontuacao->setEnabled(true);
      ui->doubleSpinBoxPontuacao->setReadOnly(true);
      ui->doubleSpinBoxPontuacao->setButtonSymbols(QDoubleSpinBox::NoButtons);

      // -------------------------------------------------------------------------

      if (representacao) { verificaFreteLoja(); }
    }

    //-----------------------------------------------------------------

    if (data("status") == "CANCELADO" or data("status") == "DEVOLVIDO") {
      ui->pushButtonCancelamento->hide();
      ui->pushButtonDevolucao->hide();
    }

    if (User::isAdministrativo()) {
      // TODO: renomear para itemBoxEnderecoEnt
      ui->itemBoxEndereco->setReadOnlyItemBox(false);
      ui->itemBoxEndereco->setFilter("idEndereco = 1 OR (idCliente = " + data("idCliente").toString() + " AND desativado = FALSE)");
      ui->pushButtonAtualizarEnderecoEnt->show();
      tipo = Tipo::Atualizar;
    }

    if (not User::isAdministrativo() and not User::isGerente()) { ui->pushButtonCancelamento->hide(); }

    if (not User::isAdministrativo()) { ui->pushButtonDevolucao->hide(); }

    if (data("devolucao").toBool()) { ui->pushButtonDevolucao->hide(); }

    if (data("idUsuarioConsultor").toInt() != 0) {
      ui->labelConsultor->show();
      ui->itemBoxConsultor->show();
    }

    calcularPesoTotal();

    return true;
  }();

  setConnections();

  return ok;
}

void Venda::criarComissaoProfissional() {
  if (ui->checkBoxPontuacaoPadrao->isChecked() and ui->itemBoxProfissional->getId() != 1) {
    const QDate date = ui->dateTimeEdit->date();
    const double valor = (ui->doubleSpinBoxSubTotalLiq->value() - ui->doubleSpinBoxDescontoGlobalReais->value()) * ui->doubleSpinBoxPontuacao->value() / 100;

    if (not qFuzzyIsNull(valor)) {
      SqlQuery query1;
      query1.prepare("INSERT INTO conta_a_pagar_has_pagamento (dataEmissao, idVenda, contraParte, idLoja, idConta, centroCusto, valor, tipo, dataPagamento, grupo) "
                     "VALUES (:dataEmissao, :idVenda, :contraParte, :idLoja, :idConta, :centroCusto, :valor, :tipo, :dataPagamento, :grupo)");
      query1.bindValue(":dataEmissao", date);
      query1.bindValue(":idVenda", ui->lineEditVenda->text());
      query1.bindValue(":contraParte", ui->itemBoxProfissional->text());
      query1.bindValue(":idLoja", idLoja);
      query1.bindValue(":idConta", 33);
      query1.bindValue(":centroCusto", idLoja);
      query1.bindValue(":valor", valor);
      query1.bindValue(":tipo", "1. TRANSF. ITAÚ");
      // 01-15 paga dia 30, 16-30 paga prox dia 15
      QDate quinzena1 = QDate(date.year(), date.month(), qMin(date.daysInMonth(), 30));
      QDate quinzena2 = date.addMonths(1);
      quinzena2.setDate(quinzena2.year(), quinzena2.month(), 15);
      query1.bindValue(":dataPagamento", date.day() <= 15 ? quinzena1 : quinzena2);
      query1.bindValue(":grupo", "RT's");

      if (not query1.exec()) { throw RuntimeException("Erro cadastrando pontuação: " + query1.lastError().text()); }
    }
  }
}

void Venda::verificaDisponibilidadeEstoque() {
  SqlQuery query;

  QStringList produtos;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.data(row, "estoque").toInt() != 1) { continue; }

    const QString idProduto = modelItem.data(row, "idProduto").toString();
    const QString quant = modelItem.data(row, "quant").toString();

    if (not query.exec("SELECT 0 FROM produto WHERE idProduto = " + idProduto + " AND estoqueRestante >= " + quant + " LIMIT 1")) {
      throw RuntimeException("Erro verificando a disponibilidade do estoque: " + query.lastError().text());
    }

    if (not query.first()) { produtos << modelItem.data(row, "produto").toString(); }
  }

  if (not produtos.isEmpty()) {
    throw RuntimeError("Os seguintes produtos de estoque não estão mais disponíveis na quantidade selecionada:\n    -" + produtos.join("\n    -") + "\n\nRemova ou diminua a quant. para prosseguir!");
  }
}

void Venda::verificaFreteLoja() {
  SqlQuery queryFornecedor;
  // TODO: e se tiver mais de um fornecedor?
  queryFornecedor.prepare("SELECT fretePagoLoja FROM fornecedor f LEFT JOIN produto p ON f.idFornecedor = p.idFornecedor WHERE p.idProduto = :idProduto");
  queryFornecedor.bindValue(":idProduto", modelItem.data(0, "idProduto"));

  if (not queryFornecedor.exec()) { throw RuntimeException("Erro buscando fretePagoLoja: " + queryFornecedor.lastError().text()); }

  if (not queryFornecedor.first()) { throw RuntimeException("Frete não encontrado para o produto com id: '" + modelItem.data(0, "idProduto").toString() + "'"); }

  const bool fretePagoLoja = queryFornecedor.value("fretePagoLoja").toBool();

  if (fretePagoLoja) { ui->widgetPgts->setFretePagoLoja(); }
}

void Venda::on_pushButtonVoltar_clicked() {
  auto *orcamento = new Orcamento(parentWidget());
  orcamento->viewRegisterById(ui->lineEditIdOrcamento->text());

  orcamento->show();

  close();
}

void Venda::montarFluxoCaixa() {
  qDebug() << "montarFluxoCaixa: " << QTime::currentTime();

  // TODO: verificar se esta funcao nao está rodando mais de uma vez por operacao

  if (ui->widgetPgts->isHidden()) { return; }

  modelFluxoCaixa.revertAll();
  modelFluxoCaixa2.revertAll();

  // TODO: colocar lógica igual da compra para poder escolher as linhas que serão substituidas

  if (financeiro) {
    for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) { modelFluxoCaixa.setData(row, "status", "SUBSTITUIDO"); }

    for (int row = 0; row < modelFluxoCaixa2.rowCount(); ++row) { modelFluxoCaixa2.setData(row, "status", "SUBSTITUIDO"); }
  }

  for (auto *pgt : qAsConst(ui->widgetPgts->pagamentos)) { processarPagamento(pgt); }

  if (ui->widgetPgts->pgtFrete) {
    ui->widgetPgts->pgtFrete->posicao = ui->widgetPgts->pagamentos.size() + 1;
    processarPagamento(ui->widgetPgts->pgtFrete);
  }
}

void Venda::on_doubleSpinBoxTotal_valueChanged(const double total) {
  unsetConnections();

  try {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();
    const double descontoReais = subTotalLiq + frete - total;
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      modelItem.setData(row, "descGlobal", descontoPorc * 100);

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc));
    }

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxDescontoGlobalReais->setValue(descontoReais);

    ui->widgetPgts->setTotal(total);
    montarFluxoCaixa();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Venda::on_checkBoxMostrarCancelados_toggled(const bool checked) {
  const QString filtroCancelado = (checked ? "" : " AND status NOT IN ('CANCELADO', 'SUBSTITUIDO')");

  modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND comissao = FALSE AND taxa = FALSE AND desativado = FALSE" + filtroCancelado);
  modelFluxoCaixa2.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND (comissao = TRUE OR taxa = TRUE) AND desativado = FALSE" + filtroCancelado);
}

void Venda::on_checkBoxFreteManual_clicked(const bool checked) {
  Q_UNUSED(checked)

  if (not canChangeFrete) {
    if (User::temPermissao("ajusteFrete")) {
      canChangeFrete = true;
    } else {
      qApp->enqueueInformation("Necessário autorização do administrativo!", this);

      LoginDialog dialog(LoginDialog::Tipo::Autorizacao, this);

      if (dialog.exec() != QDialog::Accepted) {
        ui->checkBoxFreteManual->setChecked(not checked);
        return;
      }

      canChangeFrete = true;
      ui->checkBoxFreteManual->setDisabled(true);
    }
  }

  if (User::temPermissao("ajusteFrete")) {
    ui->doubleSpinBoxFrete->setMinimum(0);
  } else {
    ui->doubleSpinBoxFrete->setMinimum(User::valorMinimoFrete);
    ui->doubleSpinBoxFrete->setValue(User::valorMinimoFrete);
    User::valorMinimoFrete = -1;
  }
}

void Venda::on_doubleSpinBoxFrete_valueChanged(const double frete) {
  const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
  const double desconto = ui->doubleSpinBoxDescontoGlobalReais->value();

  unsetConnections();

  try {
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - desconto + frete);
    ui->widgetPgts->setFrete(frete);
    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    montarFluxoCaixa();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Venda::on_doubleSpinBoxDescontoGlobal_valueChanged(const double descontoPorc) {
  unsetConnections();

  try {
    const double descontoPorc2 = descontoPorc / 100;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      modelItem.setData(row, "descGlobal", descontoPorc);

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc2));
    }

    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobalReais->setValue(subTotalLiq * descontoPorc2);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq * (1 - descontoPorc2) + frete);

    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    montarFluxoCaixa();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Venda::on_doubleSpinBoxDescontoGlobalReais_valueChanged(const double descontoReais) {
  unsetConnections();

  try {
    const double subTotalLiq = ui->doubleSpinBoxSubTotalLiq->value();
    const double descontoPorc = descontoReais / subTotalLiq;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      modelItem.setData(row, "descGlobal", descontoPorc * 100);

      const double parcialDesc = modelItem.data(row, "parcialDesc").toDouble();
      modelItem.setData(row, "total", parcialDesc * (1 - descontoPorc));
    }

    const double frete = ui->doubleSpinBoxFrete->value();

    ui->doubleSpinBoxDescontoGlobal->setValue(descontoPorc * 100);
    ui->doubleSpinBoxTotal->setValue(subTotalLiq - descontoReais + frete);

    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());
    montarFluxoCaixa();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Venda::on_pushButtonGerarPdf_clicked() {
  PDF pdf(data("idVenda").toString(), PDF::Tipo::Venda, this);
  pdf.gerarPdf();
}

void Venda::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Venda cadastrada com sucesso!", this); }

void Venda::on_pushButtonGerarExcel_clicked() {
  Excel excel(ui->lineEditVenda->text(), Excel::Tipo::Venda, this);
  excel.gerarExcel();
}

void Venda::atualizarCredito() {
  double creditoUsado = 0;
  bool update = false;

  for (auto *pgt : qAsConst(ui->widgetPgts->pagamentos)) {
    if (pgt->comboTipoPgt->currentText() == "CONTA CLIENTE") {
      creditoUsado += pgt->valorPgt->value();
      update = true;
    }
  }

  if (update) {
    SqlQuery query;
    // NOTE: usar 'credito = credito - X' porque se outro usuario estiver fazendo devolucao a query 'credito = creditoRestante' vai sobrescrever o valor
    query.prepare("UPDATE cliente SET credito = credito - :creditoUsado WHERE idCliente = :idCliente");
    query.bindValue(":creditoUsado", creditoUsado);
    query.bindValue(":idCliente", ui->itemBoxCliente->getId());

    if (not query.exec()) { throw RuntimeException("Erro atualizando crédito do cliente: " + query.lastError().text()); }
  }
}

void Venda::cadastrar() {
  try {
    qApp->startTransaction("Venda::cadastrar");

    if (tipo == Tipo::Atualizar) { throw RuntimeException("Erro Venda Cadastrar Tipo::Atualizar"); }

    currentRow = model.insertRowAtEnd();

    unsetConnections();
    savingProcedures();
    setConnections();

    atualizarCredito();
    criarComissaoProfissional();

    // -------------------------------------------------------------------------

    model.submitAll();

    // -------------------------------------------------------------------------

    primaryId = ui->lineEditVenda->text();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    // -------------------------------------------------------------------------

    for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) { modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text()); }

    modelFluxoCaixa.submitAll();

    // -------------------------------------------------------------------------

    for (int row = 0; row < modelFluxoCaixa2.rowCount(); ++row) { modelFluxoCaixa2.setData(row, "idVenda", ui->lineEditVenda->text()); }

    modelFluxoCaixa2.submitAll();

    // -------------------------------------------------------------------------

    for (int row = 0; row < modelItem.rowCount(); ++row) { modelItem.setData(row, "idVenda", ui->lineEditVenda->text()); }

    modelItem.submitAll();

    // -------------------------------------------------------------------------

    SqlQuery queryOrcamento;
    queryOrcamento.prepare("UPDATE orcamento SET status = 'FECHADO' WHERE idOrcamento = :idOrcamento");
    queryOrcamento.bindValue(":idOrcamento", ui->lineEditIdOrcamento->text());

    if (not queryOrcamento.exec()) { throw RuntimeException("Erro marcando orçamento como 'FECHADO': " + queryOrcamento.lastError().text()); }

    // -------------------------------------------------------------------------

    criarConsumos();

    Sql::updateVendaStatus(ui->lineEditVenda->text());
    Sql::updateFornecedoresVenda(ui->lineEditVenda->text());
    Sql::updateOrdemRepresentacaoVenda(ui->lineEditVenda->text());

    qApp->endTransaction();

    // -------------------------------------------------------------------------

    backupItem.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelItem.setFilter(primaryKey + " = '" + primaryId + "'");

    modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND comissao = FALSE AND taxa = FALSE AND desativado = FALSE");
  } catch (std::exception &) {
    qApp->rollbackTransaction();
    model.select();
    modelItem.select();

    for (auto &record : backupItem) { modelItem.insertRecord(-1, record); }

    if (tipo == Tipo::Cadastrar) { ui->lineEditVenda->setText("Auto gerado"); }

    throw;
  }
}

void Venda::criarConsumos() {
  SqlQuery query;
  query.prepare("SELECT p.idEstoque, vp2.idVendaProduto2, vp2.quant FROM venda_has_produto2 vp2 LEFT JOIN produto p ON vp2.idProduto = p.idProduto WHERE vp2.idVenda = :idVenda AND vp2.estoque > 0");
  query.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query.exec()) { throw RuntimeException("Erro buscando produtos estoque: " + query.lastError().text()); }

  while (query.next()) {
    auto estoque = Estoque(query.value("idEstoque").toString(), this);
    estoque.criarConsumo(query.value("idVendaProduto2").toInt(), query.value("quant").toDouble());
  }
}

void Venda::cancelamento() {
  // TODO: guardar na tabela de followup o usuario que cancelou e em qual data

  const QString idOrcamento = ui->lineEditIdOrcamento->text();

  SqlQuery query1;

  if (not idOrcamento.isEmpty()) {
    // TODO: não marcar orçamento como ativo se ele estiver fora da validade
    query1.prepare("UPDATE orcamento SET status = 'ATIVO' WHERE idOrcamento = :idOrcamento");
    query1.bindValue(":idOrcamento", idOrcamento);

    if (not query1.exec()) { throw RuntimeException("Erro reativando orçamento: " + query1.lastError().text()); }
  }

  // -------------------------------------------------------------------------

  SqlQuery query2;
  query2.prepare("UPDATE venda SET status = 'CANCELADO', statusFinanceiro = 'CANCELADO' WHERE idVenda = :idVenda");
  query2.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query2.exec()) { throw RuntimeException("Erro marcando venda como cancelada: " + query2.lastError().text()); }

  // -------------------------------------------------------------------------

  SqlQuery queryDelete;
  queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto2 IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda)");
  queryDelete.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not queryDelete.exec()) { throw RuntimeException("Erro removendo consumo estoque: " + queryDelete.lastError().text()); }

  // -------------------------------------------------------------------------

  SqlQuery query3;
  query3.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, `idVendaProduto2` = NULL WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda)");
  query3.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query3.exec()) { throw RuntimeException("Erro removendo vínculo da compra: " + query3.lastError().text()); }

  // -------------------------------------------------------------------------

  SqlQuery query4;
  query4.prepare("UPDATE venda_has_produto2 SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query4.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query4.exec()) { throw RuntimeException("Erro marcando produtos da venda como cancelados: " + query4.lastError().text()); }

  // -------------------------------------------------------------------------

  SqlQuery query5;
  query5.prepare("UPDATE venda_has_produto SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query5.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query5.exec()) { throw RuntimeException("Erro marcando produtos da venda como cancelados: " + query5.lastError().text()); }

  // -------------------------------------------------------------------------

  // TODO: no pedido ALPH-211411 não funcionou, testar

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    const QString status = modelFluxoCaixa.data(row, "status").toString();
    const QString tipo = modelFluxoCaixa.data(row, "tipo").toString();

    if ((status == "PENDENTE" or status == "RECEBIDO") and tipo.contains("CONTA CLIENTE")) {
      const double credito = modelFluxoCaixa.data(row, "valor").toDouble() * -1;

      SqlQuery query6;
      query6.prepare("UPDATE cliente SET credito = credito + :valor WHERE idCliente = :idCliente");
      query6.bindValue(":valor", credito);
      query6.bindValue(":idCliente", model.data(0, "idCliente"));

      if (not query6.exec()) { throw RuntimeException("Erro voltando credito do cliente: " + query6.lastError().text()); }
    }
  }

  // -------------------------------------------------------------------------

  SqlQuery query7;
  query7.prepare("UPDATE conta_a_receber_has_pagamento SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query7.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query7.exec()) { throw RuntimeException("Erro marcando contas como canceladas: " + query7.lastError().text()); }

  // -------------------------------------------------------------------------

  SqlQuery query8;
  query8.prepare("UPDATE conta_a_pagar_has_pagamento SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query8.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query8.exec()) { throw RuntimeException("Erro marcando RT como cancelado: " + query8.lastError().text()); }
}

void Venda::on_pushButtonCancelamento_clicked() {
  // TODO: perguntar e salvar motivo do cancelamento
  // TODO: caso haja agendamento de entrega cancelar o agendamento primeiro?

  if (not User::isAdmin()) {
    const QDate dataVenda = data("data").toDate();
    const QDate dataSeguinte = dataVenda.addMonths(1);
    const QDate dataAtual = qApp->serverDate();

    const bool mesmoMes = (dataAtual.year() == dataVenda.year() and dataAtual.month() == dataVenda.month());
    const bool mesSeguinte = (dataAtual.year() == dataSeguinte.year() and dataAtual.month() == dataSeguinte.month());

    if (not mesmoMes and not mesSeguinte) { throw RuntimeError("Fora do período de cancelamento!"); }

    if (mesSeguinte) {
      const int diaSemana = dataAtual.dayOfWeek();
      const int dia = dataAtual.day();

      bool allowed = false;

      // aceita qualquer dia da semana
      if (diaSemana >= 1 and diaSemana <= 5 and dia == 1) { allowed = true; }
      // sabado aceita segunda 3
      if (diaSemana == 6 and dia < 3) { allowed = true; }
      // domingo aceita segunda 2
      if (diaSemana == 7 and dia < 2) { allowed = true; }

      // ----------------------------------------------------

      SqlQuery queryData;
      QString dataStr = dataAtual.toString("yyyy-MM-dd");

      if (not queryData.exec("SELECT * FROM feriados_bancarios WHERE data = '" + dataStr + "'")) { throw RuntimeException("Erro verificando data: " + queryData.lastError().text()); }

      if (queryData.size() > 0) { allowed = true; }

      // ----------------------------------------------------

      if (not allowed) { throw RuntimeError("Não está no primeiro dia útil do mês seguinte!", this); }
    }
  }

  // -------------------------------------------------------------------------

  // TODO: tem 2 formas de cancelar pagamento, mudando status para cancelado ou setando flag desativado
  // portanto a regra não deve ser verificar se status == pendente

  bool ok = true;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.data(row, "estoque").toInt() > 0) { continue; }

    if (modelItem.data(row, "status").toString() != "PENDENTE") {
      ok = false;
      break;
    }
  }

  if (not ok) { throw RuntimeError("Um ou mais produtos não estão pendentes!", this); }

  // -------------------------------------------------------------------------

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    const QString status = modelFluxoCaixa.data(row, "status").toString();
    const QString tipo = modelFluxoCaixa.data(row, "tipo").toString();

    if (status == "RECEBIDO" and not tipo.contains("CONTA CLIENTE")) {
      throw RuntimeError("Um ou mais pagamentos foram recebidos!\nPeça para o financeiro cancelar esses pagamentos para dar continuidade ao cancelamento da venda!", this);
    }
  }

  // -------------------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar venda");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // -------------------------------------------------------------------------

  qApp->startTransaction("Venda::on_pushButtonCancelamento");

  cancelamento();

  qApp->endTransaction();

  const QString idOrcamento = ui->lineEditIdOrcamento->text();

  qApp->enqueueInformation(idOrcamento.isEmpty() ? "Não havia Orçamento associado para ativar!" : "Orçamento " + idOrcamento + " reativado!", this);

  qApp->enqueueInformation("Venda cancelada!", this);

  close();
}

void Venda::generateId() {
  const QString siglaLoja = User::fromLoja("sigla", ui->itemBoxVendedor->text()).toString();

  if (siglaLoja.isEmpty()) { throw RuntimeException("Erro buscando sigla da loja!"); }

  QString id = siglaLoja + "-" + ui->dateTimeEdit->date().toString("yy");

  SqlQuery query;
  query.prepare("SELECT MAX(idVenda) AS idVenda FROM venda WHERE idVenda LIKE :id");
  query.bindValue(":id", id + "%");

  if (not query.exec()) { throw RuntimeException("Erro na query: " + query.lastError().text()); }

  int last = 0;

  if (query.first()) { last = query.value("idVenda").toString().mid(7, 4).toInt(); }

  id += QString::number(last + 1).rightJustified(4, '0');

  if (id.size() != 11) { throw RuntimeException("Ocorreu algum erro ao gerar id: " + id); }

  id += representacao ? "R" : "";

  ui->lineEditVenda->setText(id);
}

void Venda::on_pushButtonDevolucao_clicked() {
  auto *devolucao = new Devolucao(data("idVenda").toString(), data("representacao").toBool(), this);
  connect(devolucao, &Devolucao::finished, [&] { viewRegisterById(ui->lineEditVenda->text()); });
  devolucao->show();
}

void Venda::on_dateTimeEdit_dateTimeChanged() {
  // TODO: colocar uma funcao em WidgetPagamentos para setar data e não precisar apagar pagamentos
  ui->widgetPgts->resetarPagamentos();
}

void Venda::setFinanceiro() {
  ui->groupBoxFinanceiro->show();
  ui->tableFluxoCaixa2->show();

  if (not User::isAdministrativo()) { ui->pushButtonCorrigirFluxo->hide(); }

  ui->frameButtons->hide();
  financeiro = true;
}

void Venda::show() {
  RegisterDialog::show();

  ui->groupBoxInfo->adjustSize();
  ui->groupBoxDados->adjustSize();
  ui->groupBoxValores->adjustSize();

  ui->groupBoxInfo->setMaximumHeight(ui->groupBoxInfo->height());
  ui->groupBoxDados->setMaximumHeight(ui->groupBoxDados->height());
  ui->groupBoxValores->setMaximumHeight(ui->groupBoxValores->height());
}

void Venda::financeiroSalvar() {
  atualizarCredito();

  setData("dataFinanceiro", qApp->serverDateTime());
  setData("statusFinanceiro", ui->comboBoxFinanceiro->currentText());

  model.submitAll();

  modelFluxoCaixa.submitAll();

  modelFluxoCaixa2.submitAll();
}

void Venda::on_pushButtonFinanceiroSalvar_clicked() {
  verifyFields();

  qApp->startTransaction("Venda::on_pushButtonFinanceiroSalvar");

  financeiroSalvar();

  qApp->endTransaction();

  qApp->enqueueInformation("Dados salvos com sucesso!", this);

  close();
}

void Venda::on_pushButtonCorrigirFluxo_clicked() {
  ui->widgetPgts->setIdLoja(idLoja);
  ui->widgetPgts->setRepresentacao(representacao);
  ui->widgetPgts->setFrete(ui->doubleSpinBoxFrete->value());
  ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());

  // -------------------------------------------------------------------------

  SqlQuery queryPag;
  queryPag.prepare("SELECT credito FROM cliente WHERE idCliente = :idCliente");
  queryPag.bindValue(":idCliente", data("idCliente"));

  if (not queryPag.exec()) { throw RuntimeException("Erro lendo credito cliente: " + queryPag.lastError().text(), this); }

  if (not queryPag.first()) { throw RuntimeException("Crédito não encontrado do cliente com id: '" + data("idCliente").toString() + "'"); }

  double credito = queryPag.value("credito").toDouble();

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    if (modelFluxoCaixa.data(row, "tipo").toString().contains("CONTA CLIENTE")) { credito += modelFluxoCaixa.data(row, "valor").toDouble(); }
  }

  ui->widgetPgts->setCredito(credito * -1);

  // -------------------------------------------------------------------------

  ui->tableFluxoCaixa->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed | QAbstractItemView::AnyKeyPressed);

  ui->tableFluxoCaixa2->show();
  ui->widgetPgts->show();
  ui->widgetPgts->resetarPagamentos();

  ui->checkBoxMostrarCancelados->setDisabled(true);

  correcao = true;
}

void Venda::on_checkBoxPontuacaoPadrao_toggled(const bool checked) {
  if (checked) {
    SqlQuery query;
    query.prepare("SELECT comissao FROM profissional WHERE idProfissional = :idProfissional");
    query.bindValue(":idProfissional", ui->itemBoxProfissional->getId());

    if (not query.exec()) { throw RuntimeException("Erro buscando pontuação: " + query.lastError().text(), this); }

    if (not query.first()) { throw RuntimeException("Pontuação não encontrada!"); }

    double comissao = query.value("comissao").toDouble();

    ui->checkBoxPontuacaoIsento->setChecked(false);
    ui->doubleSpinBoxPontuacao->setEnabled(true);

    if (qFuzzyIsNull(comissao)) { comissao = 5; }

    ui->doubleSpinBoxPontuacao->setMaximum(comissao);
    ui->doubleSpinBoxPontuacao->setValue(comissao);
  }

  // TODO: 5criar linha em contas_pagar de comissao/rt
}

void Venda::on_checkBoxPontuacaoIsento_toggled(const bool checked) {
  if (checked) {
    ui->checkBoxPontuacaoPadrao->setChecked(false);
    ui->doubleSpinBoxPontuacao->setValue(0);
    ui->doubleSpinBoxPontuacao->setDisabled(true);
  } else {
    ui->doubleSpinBoxPontuacao->setEnabled(true);
  }
}

void Venda::on_checkBoxRT_toggled(const bool checked) {
  for (auto &widget : ui->frameRT->findChildren<QWidget *>()) { widget->setVisible(checked); }

  ui->checkBoxRT->setText(checked ? "Pontuação" : "");
}

void Venda::on_itemBoxEndereco_idChanged() {
  if (User::isGerente()) { minimoGerente = 0.; }
  canChangeFrete = false;
  ui->checkBoxFreteManual->setChecked(false);
  ui->checkBoxFreteManual->setEnabled(true);

  if (not representacao) { ui->doubleSpinBoxFrete->setMinimum(0); }
  calcularFrete(true);
  if (not representacao) { ui->doubleSpinBoxFrete->setMinimum(not qFuzzyIsNull(minimoGerente) ? minimoGerente : ui->doubleSpinBoxFrete->value()); }

  const QString disclaimer = "O VALOR CALCULADO PARA O FRETE É VÁLIDO APENAS PARA AS REGIÕES DE SÃO PAULO, BARUERI E JUNDIAÍ.";
  QString observacao = ui->plainTextEditObs->toPlainText();

  if (ui->itemBoxEndereco->text() == "NÃO HÁ/RETIRA") {
    if (!observacao.contains(disclaimer, Qt::CaseInsensitive)) { observacao += disclaimer; }

  } else {
    observacao.remove(disclaimer, Qt::CaseInsensitive);
  }

  ui->plainTextEditObs->setPlainText(observacao);
}

bool Venda::verificaServicosEspeciais() {
  QStringList fornecedores;

  for (int row = 0; row < modelItem.rowCount(); ++row) { fornecedores << modelItem.data(row, "fornecedor").toString(); }

  fornecedores.removeDuplicates();

  if (fornecedores.size() == 1 and fornecedores.first() == "STACCATO SERVIÇOS ESPECIAIS (SSE)") {
    ui->doubleSpinBoxFrete->setMinimum(0);
    ui->doubleSpinBoxFrete->setValue(0);
    return true;
  }

  return false;
}

void Venda::calcularFrete(const bool updateSpinBox) {
  if (ui->checkBoxFreteManual->isChecked()) { return; }

  if (verificaServicosEspeciais()) { return; }

  double fretePorcentagem = ui->doubleSpinBoxSubTotalBruto->value() * porcFrete / 100.;
  double freteMaior = qMax(fretePorcentagem, minimoFrete);

  qDebug() << "fretePorcentagem: R$" << fretePorcentagem;
  qDebug() << "freteMinimo: R$" << minimoFrete;

  if (ui->itemBoxEndereco->text() != "NÃO HÁ/RETIRA") {
    double pesoSul = 0.;
    double pesoTotal = 0.;

    for (int row = 0; row < modelItem.rowCount(); ++row) {
      const QString idProduto = modelItem.data(row, "idProduto").toString();

      SqlQuery sqlQueryKgCx;

      if (not sqlQueryKgCx.exec("SELECT kgcx FROM produto WHERE idProduto = " + idProduto) or not sqlQueryKgCx.first()) {
        throw RuntimeException("Erro buscando peso do produto: " + sqlQueryKgCx.lastError().text());
      }

      const double kgcx = sqlQueryKgCx.value("kgcx").toDouble();
      const double caixas = modelItem.data(row, "caixas").toDouble();
      const double peso = caixas * kgcx;

      SqlQuery queryFornecedor;

      if (not queryFornecedor.exec("SELECT vemDoSul FROM fornecedor WHERE idFornecedor = (SELECT idFornecedor FROM produto WHERE idProduto = " + idProduto + ")")) {
        throw RuntimeException("Erro buscando se fornecedor é do sul: " + queryFornecedor.lastError().text());
      }

      if (not queryFornecedor.first()) { throw RuntimeException("Fornecedor não encontrado para produto com id: " + idProduto); }

      if (queryFornecedor.value("vemDoSul").toBool()) { pesoSul += peso; }

      pesoTotal += peso;
    }

    qDebug() << "pesoSul:" << pesoSul << "kg";
    qDebug() << "pesoTotal:" << pesoTotal << "kg";

    // --------------------------------------------

    CalculoFrete calculoFrete;
    calculoFrete.setOrcamento(ui->itemBoxEndereco->getId(), pesoSul, pesoTotal);

    double freteQualp = 0.;

    try {
      freteQualp = calculoFrete.getFrete();
    } catch (std::exception &e) { Log::createLog("Exceção", e.what()); }

    qDebug() << "freteQualp: R$" << freteQualp;

    freteMaior = qMax(freteMaior, freteQualp);

    if (User::isGerente()) {
      const double freteMenor = qMin(freteQualp, freteMaior);
      minimoGerente = qFuzzyIsNull(freteMenor) ? freteMaior : freteMenor * 1.2;
      qDebug() << "minimoGerente: R$" << minimoGerente;
    }
  }

  ui->doubleSpinBoxFrete->setMinimum(User::isGerente() ? minimoGerente : freteMaior);

  if (updateSpinBox) {
    qDebug() << "freteFinal: R$" << freteMaior;
    ui->doubleSpinBoxFrete->setValue(freteMaior);
  }

  qDebug() << "--------------------------------------";
}

void Venda::on_itemBoxProfissional_textChanged() {
  const bool naoHa = (ui->itemBoxProfissional->text() == "NÃO HÁ");

  ui->frameRT->setDisabled(naoHa);

  if (naoHa) { return ui->checkBoxPontuacaoIsento->setChecked(true); }

  on_checkBoxPontuacaoPadrao_toggled(ui->checkBoxPontuacaoPadrao->isChecked());
}

void Venda::copiaProdutosOrcamento() {
  SqlQuery queryProdutos;
  queryProdutos.prepare("SELECT * FROM orcamento_has_produto WHERE idOrcamento = :idOrcamento");
  queryProdutos.bindValue(":idOrcamento", ui->lineEditIdOrcamento->text());

  if (not queryProdutos.exec()) { throw RuntimeException("Erro buscando produtos: " + queryProdutos.lastError().text()); }

  while (queryProdutos.next()) {
    const int rowItem = modelItem.insertRowAtEnd();

    for (int column = 0, columnCount = queryProdutos.record().count(); column < columnCount; ++column) {
      const QString field = queryProdutos.record().fieldName(column);

      if (field == "created") { continue; }
      if (field == "lastUpdated") { continue; }
      if (modelItem.fieldIndex(field, true) == -1) { continue; }

      modelItem.setData(rowItem, field, queryProdutos.value(field));
    }

    if (modelItem.data(rowItem, "estoque").toInt() > 0) {
      SqlQuery queryStatus;

      if (not queryStatus.exec("SELECT e.status FROM estoque e LEFT JOIN produto p ON e.idEstoque = p.idEstoque WHERE p.idProduto = " + modelItem.data(rowItem, "idProduto").toString())) {
        throw RuntimeException("Erro buscando status do estoque: " + queryStatus.lastError().text());
      }

      if (not queryStatus.first()) { throw RuntimeException("Dados não encontrados para produto com id: '" + modelItem.data(rowItem, "idProduto").toString() + "'"); }

      modelItem.setData(rowItem, "status", queryStatus.value("status"));
    } else {
      modelItem.setData(rowItem, "status", "PENDENTE");
    }
  }

  for (int row = 0; row < modelItem.rowCount(); ++row) { backupItem.append(modelItem.record(row)); }
}

void Venda::on_pushButtonComprovantes_clicked() {
  auto *comprovantes = new Comprovantes(ui->lineEditVenda->text(), this);
  comprovantes->show();
}

void Venda::on_pushButtonAdicionarObservacao_clicked() {
  const auto selection = ui->treeView->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  //------------------------------------------------------

  for (const auto &index : selection) {
    if (not index.parent().isValid()) { throw RuntimeError("Deve selecionar apenas sublinhas!", this); }
  }

  const QString observacao = QInputDialog::getText(this, "Observação", "Obs: ");

  if (observacao.isEmpty()) { return; }

  //------------------------------------------------------

  for (const auto &index : selection) {
    const auto index2 = modelTree.proxyModel->mapToSource(index);
    const auto row = modelTree.mappedRow(index2);

    modelItem2.setData(row, "obs", observacao);
  }

  modelItem2.submitAll();

  modelTree.updateData();
}

void Venda::on_pushButtonModelo3d_clicked() {
  const auto selection = ui->treeView->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const int row = selection.first().row();

  const QString ip = qApp->getWebDavIp();
  const QString fornecedor = modelItem.data(row, "fornecedor").toString();
  const QString codComercial = modelItem.data(row, "codComercial").toString();

  const QString url = "https://" + ip + "/webdav/SISTEMA/MODELOS 3D/" + fornecedor + "/" + codComercial + ".skp";

  auto *manager = new QNetworkAccessManager(this);
  manager->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

  connect(manager, &QNetworkAccessManager::authenticationRequired, this, [&](QNetworkReply *reply, QAuthenticator *authenticator) {
    Q_UNUSED(reply)

    authenticator->setUser(User::usuario);
    authenticator->setPassword(User::senha);
  });

  auto *reply = manager->get(QNetworkRequest(QUrl(url)));

  connect(reply, &QNetworkReply::finished, this, [=, this] {
    if (reply->error() != QNetworkReply::NoError) {
      if (reply->error() == QNetworkReply::ContentNotFoundError) { throw RuntimeError("Produto não possui modelo 3D!"); }

      throw RuntimeException("Erro ao baixar arquivo: " + reply->errorString(), this);
    }

    const QString filename = QDir::currentPath() + "/arquivos/" + url.split("/").last();

    File file(filename);

    if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Erro abrindo arquivo para escrita: " + file.errorString(), this); }

    file.write(reply->readAll());

    file.close();

    if (not QDesktopServices::openUrl(QUrl::fromLocalFile(filename))) { throw RuntimeException("Não foi possível abrir o arquivo 3D!"); }
  });
}

void Venda::on_treeView_doubleClicked(const QModelIndex &index) {
  if (not index.parent().isValid()) { return; }

  // ---------------------------------------------------------------

  const auto index2 = modelTree.proxyModel->mapToSource(index);
  const auto row = modelTree.mappedRow(index2);

  if (User::isVendedorOrEspecial() or User::isGerente()) { return qApp->abrirNFe(modelItem2.data(row, "idNFeSaida")); }

  if (User::isAdmin() or User::isAdministrativo()) {
    const QString idVendaProduto2 = modelItem2.data(row, "idVendaProduto2").toString();

    QSqlQuery query;

    if (not query.exec("SELECT xml FROM nfe WHERE idNFe = (SELECT idNFe FROM estoque WHERE idEstoque = (SELECT idEstoque FROM estoque_has_consumo WHERE idVendaProduto2 = " + idVendaProduto2 + "))")) {
      throw RuntimeException("Erro buscando NF-e: " + query.lastError().text());
    }

    if (not query.first()) { throw RuntimeError("Linha não possui NF-e!"); }

    ACBrLib::gerarDanfe(query.value("xml").toString(), true);
  }
}

void Venda::calcularPesoTotal() {
  double total = 0;

  SqlQuery queryProduto;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (not queryProduto.exec("SELECT kgcx FROM produto WHERE idProduto = " + modelItem.data(row, "idProduto").toString())) {
      throw RuntimeException("Erro buscando kgcx: " + queryProduto.lastError().text());
    }

    if (not queryProduto.first()) { throw RuntimeException("Peso não encontrado do produto com id: '" + modelItem.data(row, "idProduto").toString() + "'"); }

    const double kgcx = queryProduto.value("kgcx").toDouble();
    total += modelItem.data(row, "caixas").toDouble() * kgcx;
  }

  // TODO: implicit conversion double -> int
  ui->spinBoxPesoTotal->setValue(total);
}

void Venda::connectLineEditsToDirty() {
  const auto children = ui->frame->findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &Venda::marcarDirty); }
}

void Venda::processarPagamento(Pagamento *pgt) {
  if (pgt->comboTipoPgt->currentText() == "ESCOLHA UMA OPÇÃO!") { return; }

  // TODO: lancamento de credito deve ser marcado direto como 'recebido' e statusFinanceiro == liberado

  //-----------------------------------------------------------------

  const QString tipoPgt = pgt->comboTipoPgt->currentText();
  const int parcelas = pgt->comboParcela->currentIndex() + 1;

  //-----------------------------------------------------------------

  if (tipoPgt == "CONTA CLIENTE") {
    const int row = modelFluxoCaixa.insertRowAtEnd();

    const QDate dataEmissao = (correcao ? modelFluxoCaixa.data(0, "dataEmissao").toDate() : qApp->serverDate());

    modelFluxoCaixa.setData(row, "contraParte", ui->itemBoxCliente->text());
    modelFluxoCaixa.setData(row, "dataEmissao", dataEmissao);
    modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text());
    modelFluxoCaixa.setData(row, "idLoja", idLoja);
    modelFluxoCaixa.setData(row, "dataPagamento", dataEmissao);
    modelFluxoCaixa.setData(row, "valor", pgt->valorPgt->value() * -1);
    modelFluxoCaixa.setData(row, "tipo", QString::number(pgt->posicao) + ". " + tipoPgt);
    modelFluxoCaixa.setData(row, "parcela", 1);
    modelFluxoCaixa.setData(row, "observacao", pgt->observacao->text());
    modelFluxoCaixa.setData(row, "representacao", false);
    const int creditoClientes = 11;
    modelFluxoCaixa.setData(row, "idConta", creditoClientes);
    modelFluxoCaixa.setData(row, "grupo", "Produtos - Venda");
    modelFluxoCaixa.setData(row, "subGrupo", "");
    modelFluxoCaixa.setData(row, "centroCusto", idLoja);

    return;
  }

  SqlQuery query2;
  query2.prepare("SELECT fp.idConta, fp.pula1Mes, fp.ajustaDiaUtil, fp.dMaisUm, fp.centavoSobressalente, fpt.taxa "
                 "FROM forma_pagamento fp "
                 "LEFT JOIN forma_pagamento_has_taxa fpt ON fp.idPagamento = fpt.idPagamento "
                 "WHERE fp.pagamento = :pagamento AND fpt.parcela = :parcela");
  query2.bindValue(":pagamento", tipoPgt);
  query2.bindValue(":parcela", parcelas);

  if (not query2.exec()) { throw RuntimeException("Erro buscando taxa: " + query2.lastError().text(), this); }

  if (not query2.first()) { throw RuntimeException("Dados do pagamento não encontrado!"); }

  const int idConta = query2.value("idConta").toInt();
  const bool pula1Mes = query2.value("pula1Mes").toInt();
  const bool ajustaDiaUtil = query2.value("ajustaDiaUtil").toBool();
  const bool dMaisUm = query2.value("dMaisUm").toBool();
  const bool centavoPrimeiraParcela = query2.value("centavoSobressalente").toBool();
  const double porcentagemTaxa = query2.value("taxa").toDouble() / 100;

  //-----------------------------------------------------------------
  // calcular pagamento

  const bool isRepresentacao = pgt->checkBoxRep->isChecked();
  const QString observacaoPgt = pgt->observacao->text();

  const double valor = pgt->valorPgt->value();
  const double valorParcela = qApp->roundDouble(valor / parcelas, 2);
  const double restoParcela = qApp->roundDouble(valor - (valorParcela * parcelas), 2);

  const double valorTaxa = qApp->roundDouble(valor * porcentagemTaxa, 2) * -1;
  const double parcelaTaxa = qApp->roundDouble(valorTaxa / parcelas, 2);
  const double restoTaxa = qApp->roundDouble(valorTaxa - (parcelaTaxa * parcelas), 2);

  const QDate dataEmissao = (correcao ? modelFluxoCaixa.data(0, "dataEmissao").toDate() : qApp->serverDate());

  for (int parcela = 0; parcela < parcelas; ++parcela) {
    const int row = modelFluxoCaixa.insertRowAtEnd();

    modelFluxoCaixa.setData(row, "contraParte", ui->itemBoxCliente->text());
    modelFluxoCaixa.setData(row, "dataEmissao", dataEmissao);
    modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text());
    modelFluxoCaixa.setData(row, "idLoja", idLoja);

    QDate dataPgt = pgt->dataPgt->date();
    if (dMaisUm) { dataPgt = dataPgt.addDays(1); }
    if (pula1Mes) { dataPgt = dataPgt.addMonths(1); }
    dataPgt = dataPgt.addMonths(parcela);
    if (ajustaDiaUtil) { dataPgt = qApp->ajustarDiaUtil(dataPgt); }

    modelFluxoCaixa.setData(row, "dataPagamento", dataPgt);

    const bool primeiraParcela = (parcela == 0);
    const bool ultimaParcela = (parcela == parcelas - 1);

    double val = valorParcela;

    if ((centavoPrimeiraParcela and primeiraParcela) or (not centavoPrimeiraParcela and ultimaParcela)) { val += restoParcela; }

    modelFluxoCaixa.setData(row, "valor", val);
    modelFluxoCaixa.setData(row, "tipo", QString::number(pgt->posicao) + ". " + tipoPgt);
    modelFluxoCaixa.setData(row, "parcela", parcela + 1);
    modelFluxoCaixa.setData(row, "observacao", observacaoPgt);
    modelFluxoCaixa.setData(row, "representacao", isRepresentacao);
    modelFluxoCaixa.setData(row, "idConta", idConta);
    modelFluxoCaixa.setData(row, "grupo", "Produtos - Venda");
    modelFluxoCaixa.setData(row, "subGrupo", "");
    modelFluxoCaixa.setData(row, "centroCusto", idLoja);

    //-----------------------------------------------------------------
    // calcular taxa

    const bool calculaTaxa = (not qFuzzyIsNull(porcentagemTaxa) and not isRepresentacao and not tipoPgt.contains("CONTA CLIENTE"));

    if (calculaTaxa) {
      const int rowTaxa = modelFluxoCaixa2.insertRowAtEnd();

      modelFluxoCaixa2.setData(rowTaxa, "contraParte", "Administradora Cartão");
      modelFluxoCaixa2.setData(rowTaxa, "dataEmissao", dataEmissao);
      modelFluxoCaixa2.setData(rowTaxa, "idVenda", ui->lineEditVenda->text());
      modelFluxoCaixa2.setData(rowTaxa, "idLoja", idLoja);
      modelFluxoCaixa2.setData(rowTaxa, "dataPagamento", dataPgt);

      double val1 = parcelaTaxa;

      if ((centavoPrimeiraParcela and primeiraParcela) or (not centavoPrimeiraParcela and ultimaParcela)) { val1 += restoTaxa; }

      modelFluxoCaixa2.setData(rowTaxa, "valor", val1);
      modelFluxoCaixa2.setData(rowTaxa, "tipo", QString::number(pgt->posicao) + ". Taxa Cartão");
      modelFluxoCaixa2.setData(rowTaxa, "parcela", parcela + 1);
      modelFluxoCaixa2.setData(rowTaxa, "taxa", true);
      modelFluxoCaixa2.setData(rowTaxa, "idConta", idConta);
      modelFluxoCaixa2.setData(rowTaxa, "centroCusto", idLoja);
      modelFluxoCaixa2.setData(rowTaxa, "grupo", "Tarifas Cartão");
    }

    //-----------------------------------------------------------------
    // calcular comissao loja

    const QString fornecedor = modelItem.data(0, "fornecedor").toString();

    SqlQuery query1;
    query1.prepare("SELECT comissaoLoja FROM fornecedor WHERE razaoSocial = :razaoSocial");
    query1.bindValue(":razaoSocial", fornecedor);

    if (not query1.exec()) { throw RuntimeException("Erro buscando comissão: " + query1.lastError().text(), this); }

    if (not query1.first()) { throw RuntimeException("Dados não encontrados do fornecedor: '" + fornecedor + "'"); }

    const double taxaComissao = query1.value("comissaoLoja").toDouble() / 100;

    const bool calculaComissao = (not qFuzzyIsNull(taxaComissao) and isRepresentacao and observacaoPgt != "FRETE");

    if (calculaComissao) {
      const double valorBase = modelFluxoCaixa.data(row, "valor").toDouble();
      const double valorComissao = valorBase * taxaComissao;

      const int rowComissao = modelFluxoCaixa2.insertRowAtEnd();

      modelFluxoCaixa2.setData(rowComissao, "contraParte", modelItem.data(0, "fornecedor"));
      modelFluxoCaixa2.setData(rowComissao, "dataEmissao", dataEmissao);
      modelFluxoCaixa2.setData(rowComissao, "idVenda", ui->lineEditVenda->text());
      modelFluxoCaixa2.setData(rowComissao, "idLoja", idLoja);
      modelFluxoCaixa2.setData(rowComissao, "dataPagamento", dataPgt.addMonths(1));
      modelFluxoCaixa2.setData(rowComissao, "valor", valorComissao);
      modelFluxoCaixa2.setData(rowComissao, "tipo", QString::number(pgt->posicao) + ". Comissão");
      modelFluxoCaixa2.setData(rowComissao, "parcela", parcela + 1);
      modelFluxoCaixa2.setData(rowComissao, "comissao", true);
      modelFluxoCaixa2.setData(rowComissao, "centroCusto", idLoja);
      modelFluxoCaixa2.setData(rowComissao, "grupo", "Comissão Representação");
    }
  }
}

void Venda::on_pushButtonAbrirOrcamento_clicked() {
  auto *orcamento = new Orcamento(this);
  orcamento->setAttribute(Qt::WA_DeleteOnClose);
  orcamento->viewRegisterById(ui->lineEditIdOrcamento->text());

  orcamento->show();
}

void Venda::on_pushButtonAtualizarEnderecoEnt_clicked() { trocarEnderecoEntrega(); }

void Venda::trocarEnderecoEntrega() {
  qApp->startTransaction("Venda::trocarEnderecoEntrega");

  SqlQuery query;

  if (not query.exec("UPDATE venda SET idEnderecoEntrega = " + ui->itemBoxEndereco->getId().toString() + " WHERE idVenda = '" + primaryId + "'")) {
    throw RuntimeException("Erro alterando endereço de entrega: " + query.lastError().text());
  }

  qApp->endTransaction();

  successMessage();
}

// TODO: 0no corrigir fluxo esta mostrando os botoes de 'frete pago a loja' e 'pagamento total a loja' em pedidos que nao sao de representacao
// TODO: 0quando for 'MATR' nao criar fluxo caixa
// TODO: verificar se um pedido nao deveria ter seu 'statusFinanceiro' alterado para 'liberado' ao ter todos os pagamentos recebidos ('status' e 'statusFinanceiro' deveriam ser vinculados?)
// TODO: prazoEntrega por produto
// TODO: bloquear desconto maximo por classe de funcionario
// TODO: em vez de ter uma caixinha 'un', concatenar em 'quant', 'minimo' e 'un/cx'
