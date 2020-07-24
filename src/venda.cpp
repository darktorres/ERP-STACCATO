#include "venda.h"
#include "ui_venda.h"

#include "application.h"
#include "cadastrocliente.h"
#include "checkboxdelegate.h"
#include "comprovantes.h"
#include "devolucao.h"
#include "doubledelegate.h"
#include "editdelegate.h"
#include "estoque.h"
#include "excel.h"
#include "logindialog.h"
#include "noeditdelegate.h"
#include "orcamento.h"
#include "pdf.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "searchdialogproxymodel.h"
#include "sql.h"
#include "usersession.h"

#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

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

  if (UserSession::tipoUsuario() == "ADMINISTRADOR" or UserSession::tipoUsuario() == "ADMINISTRATIVO") {
    ui->dateTimeEdit->setReadOnly(false);
    ui->dateTimeEdit->setCalendarPopup(true);
  }

  ui->splitter->setStretchFactor(0, 255);
  ui->splitter->setStretchFactor(1, 1);
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

  ui->treeView->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));
  ui->treeView->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("parcial", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("parcialDesc", new ReaisDelegate(this));
  ui->treeView->setItemDelegateForColumn("desconto", new PorcentagemDelegate(false, this));
  ui->treeView->setItemDelegateForColumn("descGlobal", new PorcentagemDelegate(false, this));
  ui->treeView->setItemDelegateForColumn("total", new ReaisDelegate(this));
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
  connect(ui->pushButtonComprovantes, &QPushButton::clicked, this, &Venda::on_pushButtonComprovantes_clicked, connectionType);
  connect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &Venda::on_pushButtonCorrigirFluxo_clicked, connectionType);
  connect(ui->pushButtonDevolucao, &QPushButton::clicked, this, &Venda::on_pushButtonDevolucao_clicked, connectionType);
  connect(ui->pushButtonFinanceiroSalvar, &QPushButton::clicked, this, &Venda::on_pushButtonFinanceiroSalvar_clicked, connectionType);
  connect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Venda::on_pushButtonGerarExcel_clicked, connectionType);
  connect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Venda::on_pushButtonGerarPdf_clicked, connectionType);
  connect(ui->pushButtonVoltar, &QPushButton::clicked, this, &Venda::on_pushButtonVoltar_clicked, connectionType);
  connect(ui->pushButtonAdicionarObservacao, &QPushButton::clicked, this, &Venda::on_pushButtonAdicionarObservacao_clicked, connectionType);
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
  disconnect(ui->pushButtonComprovantes, &QPushButton::clicked, this, &Venda::on_pushButtonComprovantes_clicked);
  disconnect(ui->pushButtonCorrigirFluxo, &QPushButton::clicked, this, &Venda::on_pushButtonCorrigirFluxo_clicked);
  disconnect(ui->pushButtonDevolucao, &QPushButton::clicked, this, &Venda::on_pushButtonDevolucao_clicked);
  disconnect(ui->pushButtonFinanceiroSalvar, &QPushButton::clicked, this, &Venda::on_pushButtonFinanceiroSalvar_clicked);
  disconnect(ui->pushButtonGerarExcel, &QPushButton::clicked, this, &Venda::on_pushButtonGerarExcel_clicked);
  disconnect(ui->pushButtonGerarPdf, &QPushButton::clicked, this, &Venda::on_pushButtonGerarPdf_clicked);
  disconnect(ui->pushButtonVoltar, &QPushButton::clicked, this, &Venda::on_pushButtonVoltar_clicked);
  disconnect(ui->pushButtonAdicionarObservacao, &QPushButton::clicked, this, &Venda::on_pushButtonAdicionarObservacao_clicked);
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
  ui->tableFluxoCaixa->setItemDelegateForColumn("observacao", new EditDelegate(this));
  ui->tableFluxoCaixa->setItemDelegateForColumn("valor", new ReaisDelegate(2, true, this));

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

  if (not copiaProdutosOrcamento()) { return; }

  setTreeView();

  // -------------------------------------------------------------------------

  QSqlQuery queryOrc;
  queryOrc.prepare("SELECT idUsuario, idLoja, idUsuarioConsultor, idCliente, idEnderecoEntrega, idProfissional, data, subTotalBru, subTotalLiq, frete, "
                   "freteManual, descontoPorc, descontoReais, total, status, observacao, prazoEntrega, representacao FROM orcamento WHERE idOrcamento = :idOrcamento");
  queryOrc.bindValue(":idOrcamento", idOrcamento);

  if (not queryOrc.exec() or not queryOrc.first()) { return qApp->enqueueException("Erro buscando orçamento: " + queryOrc.lastError().text(), this); }

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

  if (not queryFrete.exec() or not queryFrete.first()) { return qApp->enqueueException("Erro buscando parâmetros do frete: " + queryFrete.lastError().text(), this); }

  minimoFrete = queryFrete.value("valorMinimoFrete").toDouble();
  porcFrete = queryFrete.value("porcentagemFrete").toDouble();

  if (not representacao) { ui->tableFluxoCaixa->hideColumn("representacao"); }

  // -------------------------------------------------------------------------

  QSqlQuery queryCredito;
  queryCredito.prepare("SELECT credito FROM cliente WHERE idCliente = :idCliente");
  queryCredito.bindValue(":idCliente", queryOrc.value("idCliente"));

  if (not queryCredito.exec() or not queryCredito.first()) { return qApp->enqueueException("Erro buscando crédito cliente: " + queryCredito.lastError().text(), this); }

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

  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT fretePagoLoja FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", modelItem.data(0, "fornecedor"));

  if (not queryFornecedor.exec() or not queryFornecedor.first()) { return qApp->enqueueException("Erro buscando fretePagoLoja: " + queryFornecedor.lastError().text()); }

  const bool fretePagoLoja = queryFornecedor.value("fretePagoLoja").toBool();

  if (fretePagoLoja) { ui->widgetPgts->setFretePagoLoja(); }

  // -------------------------------------------------------------------------

  setConnections();
}

bool Venda::verifyFields() {
  // TODO: pintar campos errados de vermelho
  // TODO: pintar campos necessarios de amarelo
  // TODO: pintar campos certos de verde
  // TODO: pintar totalPag de vermelho enquanto o total for diferente

  if (not financeiro and not verificaDisponibilidadeEstoque()) { return false; }

  if (ui->widgetPgts->isVisible()) {
    if (not qFuzzyCompare(ui->widgetPgts->getTotalPag(), ui->doubleSpinBoxTotal->value())) { return qApp->enqueueError(false, "Total dos pagamentos difere do total do pedido!", this); }

    if (not ui->widgetPgts->verifyFields()) { return false; }

    if (modelFluxoCaixa.rowCount() == 0) { return qApp->enqueueError(false, "Sem linhas de pagamento!", this); }
  }

  if (ui->spinBoxPrazoEntrega->value() == 0) {
    qApp->enqueueError("Por favor preencha o prazo de entrega!", this);
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
  addMapping(ui->plainTextEdit, "observacao");
  addMapping(ui->spinBoxPrazoEntrega, "prazoEntrega");
}

void Venda::on_pushButtonCadastrarPedido_clicked() { save(); }

bool Venda::savingProcedures() {
  if (not setData("data", ui->dateTimeEdit->isReadOnly() ? qApp->serverDateTime() : ui->dateTimeEdit->dateTime())) { return false; }
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
  ui->checkBoxFreteManual->show();
  ui->framePagamentos->show();
  ui->pushButtonCadastrarPedido->show();
  ui->pushButtonVoltar->show();

  ui->groupBoxFinanceiro->hide();
  ui->itemBoxConsultor->hide();
  ui->labelConsultor->hide();
  ui->pushButtonAdicionarObservacao->hide();
  ui->pushButtonCancelamento->hide();
  ui->pushButtonComprovantes->hide();
  ui->pushButtonDevolucao->hide();
  ui->pushButtonGerarExcel->hide();
  ui->pushButtonGerarPdf->hide();
  ui->tableFluxoCaixa2->hide();
  ui->labelRT->hide();

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
  ui->pushButtonCadastrarPedido->hide();
  ui->pushButtonVoltar->hide();
  ui->tableFluxoCaixa2->hide();
  ui->tableFluxoCaixa2->hide();
  ui->widgetPgts->hide();
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
  ui->plainTextEdit->setReadOnly(true);
  ui->spinBoxPrazoEntrega->setReadOnly(true);

  ui->doubleSpinBoxDescontoGlobal->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxDescontoGlobalReais->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxFrete->setButtonSymbols(QDoubleSpinBox::NoButtons);
  ui->doubleSpinBoxTotal->setButtonSymbols(QDoubleSpinBox::NoButtons);

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

    if (not modelItem.select()) { return false; }

    modelItem2.setFilter("idVenda = '" + model.data(0, "idVenda").toString() + "'");

    if (not modelItem2.select()) { return false; }

    setTreeView();

    modelFluxoCaixa.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND comissao = FALSE AND taxa = FALSE");

    if (not modelFluxoCaixa.select()) { return false; }

    idLoja = data("idLoja").toInt();
    representacao = data("representacao").toBool();

    if (financeiro) {
      // TODO: 1quando estiver tudo pago bloquear correcao de fluxo
      modelFluxoCaixa2.setFilter("idVenda = '" + ui->lineEditVenda->text() + "' AND status NOT IN ('CANCELADO', 'SUBSTITUIDO') AND (comissao = TRUE OR taxa = TRUE)");

      if (not modelFluxoCaixa2.select()) { return false; }

      for (auto &widget : ui->frameRT->findChildren<QWidget *>()) { widget->setVisible(true); }

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

      if (representacao) {
        QSqlQuery queryFornecedor;
        queryFornecedor.prepare("SELECT fretePagoLoja FROM fornecedor WHERE razaoSocial = :razaoSocial");
        queryFornecedor.bindValue(":razaoSocial", modelItem.data(0, "fornecedor"));

        if (not queryFornecedor.exec() or not queryFornecedor.first()) { return qApp->enqueueException(false, "Erro buscando fretePagoLoja: " + queryFornecedor.lastError().text()); }

        const bool fretePagoLoja = queryFornecedor.value("fretePagoLoja").toBool();

        if (fretePagoLoja) { ui->widgetPgts->setFretePagoLoja(); }
      }
    }

    //-----------------------------------------------------------------

    if (data("status") == "CANCELADO" or data("status") == "DEVOLVIDO") {
      ui->pushButtonCancelamento->hide();
      ui->pushButtonDevolucao->hide();
    }

    const QString tipoUsuario = UserSession::tipoUsuario();

    if (not tipoUsuario.contains("GERENTE") and tipoUsuario != "DIRETOR" and tipoUsuario != "ADMINISTRADOR" and tipoUsuario != "ADMINISTRATIVO") {
      ui->pushButtonDevolucao->hide();
      ui->pushButtonCancelamento->hide();
    }

    if (data("devolucao").toBool()) { ui->pushButtonDevolucao->hide(); }

    if (data("idUsuarioConsultor").toInt() != 0) {
      ui->labelConsultor->show();
      ui->itemBoxConsultor->show();
    }

    ui->widgetPgts->setIdOrcamento(ui->lineEditIdOrcamento->text());
    ui->widgetPgts->setRepresentacao(representacao);
    ui->widgetPgts->setFrete(ui->doubleSpinBoxFrete->value());
    ui->widgetPgts->setTotal(ui->doubleSpinBoxTotal->value());

    return true;
  }();

  setConnections();

  return ok;
}

bool Venda::criarComissaoProfissional() {
  if (ui->checkBoxPontuacaoPadrao->isChecked() and ui->itemBoxProfissional->getId() != 1) {
    const QDate date = ui->dateTimeEdit->date();
    const double valor = (ui->doubleSpinBoxSubTotalLiq->value() - ui->doubleSpinBoxDescontoGlobalReais->value()) * ui->doubleSpinBoxPontuacao->value() / 100;

    if (not qFuzzyIsNull(valor)) {
      QSqlQuery query1;
      query1.prepare("INSERT INTO conta_a_pagar_has_pagamento (dataEmissao, idVenda, contraParte, idLoja, centroCusto, valor, tipo, dataPagamento, grupo) "
                     "VALUES (:dataEmissao, :idVenda, :contraParte, :idLoja, :centroCusto, :valor, :tipo, :dataPagamento, :grupo)");
      query1.bindValue(":dataEmissao", date);
      query1.bindValue(":idVenda", ui->lineEditVenda->text());
      query1.bindValue(":contraParte", ui->itemBoxProfissional->text());
      query1.bindValue(":idLoja", idLoja);
      query1.bindValue(":centroCusto", idLoja);
      query1.bindValue(":valor", valor);
      query1.bindValue(":tipo", "1. Dinheiro");
      // 01-15 paga dia 30, 16-30 paga prox dia 15
      QDate quinzena1 = QDate(date.year(), date.month(), qMin(date.daysInMonth(), 30));
      QDate quinzena2 = date.addMonths(1);
      quinzena2.setDate(quinzena2.year(), quinzena2.month(), 15);
      query1.bindValue(":dataPagamento", date.day() <= 15 ? quinzena1 : quinzena2);
      query1.bindValue(":grupo", "RT's");

      if (not query1.exec()) { return qApp->enqueueException(false, "Erro cadastrando pontuação: " + query1.lastError().text(), this); }
    }
  }

  return true;
}

bool Venda::verificaDisponibilidadeEstoque() {
  QSqlQuery query;

  QStringList produtos;

  for (int row = 0; row < modelItem.rowCount(); ++row) {
    if (modelItem.data(row, "estoque").toInt() != 1) { continue; }

    const QString idProduto = modelItem.data(row, "idProduto").toString();
    const QString quant = modelItem.data(row, "quant").toString();

    if (not query.exec("SELECT 0 FROM produto WHERE idProduto = " + idProduto + " AND estoqueRestante >= " + quant)) {
      return qApp->enqueueException(false, "Erro verificando a disponibilidade do estoque: " + query.lastError().text(), this);
    }

    if (not query.first()) { produtos << modelItem.data(row, "produto").toString(); }
  }

  if (not produtos.isEmpty()) {
    return qApp->enqueueError(
        false, "Os seguintes produtos de estoque não estão mais disponíveis na quantidade selecionada:\n    -" + produtos.join("\n    -") + "\n\nRemova ou diminua a quant. para prosseguir!", this);
  }

  return true;
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
    for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
      if (not modelFluxoCaixa.setData(row, "status", "SUBSTITUIDO")) { return; }
    }

    for (int row = 0; row < modelFluxoCaixa2.rowCount(); ++row) {
      if (not modelFluxoCaixa2.setData(row, "status", "SUBSTITUIDO")) { return; }
    }
  }

  const QString fornecedor = modelItem.data(0, "fornecedor").toString();
  const double totalVenda = ui->doubleSpinBoxTotal->value();
  const double frete = ui->doubleSpinBoxFrete->value();

  QSqlQuery query1;
  query1.prepare("SELECT comissaoLoja FROM fornecedor WHERE razaoSocial = :razaoSocial");
  query1.bindValue(":razaoSocial", fornecedor);

  if (not query1.exec() or not query1.first()) { return qApp->enqueueException("Erro buscando comissão: " + query1.lastError().text(), this); }

  const double taxaComissao = query1.value("comissaoLoja").toDouble() / 100;

  for (int pagamento = 0; pagamento < ui->widgetPgts->pagamentos; ++pagamento) {
    if (ui->widgetPgts->listTipoPgt.at(pagamento)->currentText() == "ESCOLHA UMA OPÇÃO!") { continue; }

    // TODO: lancamento de credito deve ser marcado direto como 'recebido' e statusFinanceiro == liberado

    //-----------------------------------------------------------------

    const QString tipoPgt = ui->widgetPgts->listTipoPgt.at(pagamento)->currentText();
    const int parcelas = ui->widgetPgts->listParcela.at(pagamento)->currentIndex() + 1;

    //-----------------------------------------------------------------

    if (tipoPgt == "CONTA CLIENTE") {
      const int row = modelFluxoCaixa.insertRowAtEnd();

      const QDate dataEmissao = correcao ? modelFluxoCaixa.data(0, "dataEmissao").toDate() : qApp->serverDate();

      if (not modelFluxoCaixa.setData(row, "contraParte", ui->itemBoxCliente->text())) { return; }
      if (not modelFluxoCaixa.setData(row, "dataEmissao", dataEmissao)) { return; }
      if (not modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text())) { return; }
      if (not modelFluxoCaixa.setData(row, "idLoja", idLoja)) { return; }
      if (not modelFluxoCaixa.setData(row, "dataPagamento", dataEmissao)) { return; }
      if (not modelFluxoCaixa.setData(row, "valor", ui->widgetPgts->listValorPgt.at(pagamento)->value())) { return; }
      if (not modelFluxoCaixa.setData(row, "tipo", QString::number(pagamento + 1) + ". " + tipoPgt)) { return; }
      if (not modelFluxoCaixa.setData(row, "parcela", 1)) { return; }
      if (not modelFluxoCaixa.setData(row, "observacao", ui->widgetPgts->listObservacao.at(pagamento)->text())) { return; }
      if (not modelFluxoCaixa.setData(row, "representacao", false)) { return; }
      const int creditoClientes = 11;
      if (not modelFluxoCaixa.setData(row, "idConta", creditoClientes)) { return; }
      if (not modelFluxoCaixa.setData(row, "grupo", "Produtos - Venda")) { return; }
      if (not modelFluxoCaixa.setData(row, "subGrupo", "")) { return; }
      if (not modelFluxoCaixa.setData(row, "centroCusto", idLoja)) { return; }

      continue;
    }

    QSqlQuery query2;
    query2.prepare("SELECT fp.idConta, fp.pula1Mes, fp.ajustaDiaUtil, fp.dMaisUm, fp.centavoSobressalente, fpt.taxa FROM forma_pagamento fp LEFT JOIN forma_pagamento_has_taxa fpt ON "
                   "fp.idPagamento = fpt.idPagamento WHERE fp.pagamento = :pagamento AND fpt.parcela = :parcela");
    query2.bindValue(":pagamento", tipoPgt);
    query2.bindValue(":parcela", parcelas);

    if (not query2.exec() or not query2.first()) { return qApp->enqueueException("Erro buscando taxa: " + query2.lastError().text(), this); }

    const int idConta = query2.value("idConta").toInt();
    const bool pula1Mes = query2.value("pula1Mes").toInt();
    const bool ajustaDiaUtil = query2.value("ajustaDiaUtil").toBool();
    const bool dMaisUm = query2.value("dMaisUm").toBool();
    const bool centavoSobressalente = query2.value("centavoSobressalente").toBool();
    const double porcentagemTaxa = query2.value("taxa").toDouble() / 100;

    //-----------------------------------------------------------------
    // calcular pagamento

    const bool isRepresentacao = ui->widgetPgts->listCheckBoxRep.at(pagamento)->isChecked();
    const QString observacaoPgt = ui->widgetPgts->listObservacao.at(pagamento)->text();

    const double valor = ui->widgetPgts->listValorPgt.at(pagamento)->value();
    const double valorParcela = qApp->roundDouble(valor / parcelas, 2);
    const double restoParcela = qApp->roundDouble(valor - (valorParcela * parcelas), 2);

    const double valorTaxa = qApp->roundDouble(valor * porcentagemTaxa, 2) * -1;
    const double parcelaTaxa = qApp->roundDouble(valorTaxa / parcelas, 2);
    const double restoTaxa = qApp->roundDouble(valorTaxa - (parcelaTaxa * parcelas), 2);

    const QDate dataEmissao = correcao ? modelFluxoCaixa.data(0, "dataEmissao").toDate() : qApp->serverDate();

    for (int parcela = 0; parcela < parcelas; ++parcela) {
      const int row = modelFluxoCaixa.insertRowAtEnd();

      if (not modelFluxoCaixa.setData(row, "contraParte", ui->itemBoxCliente->text())) { return; }
      if (not modelFluxoCaixa.setData(row, "dataEmissao", dataEmissao)) { return; }
      if (not modelFluxoCaixa.setData(row, "idVenda", ui->lineEditVenda->text())) { return; }
      if (not modelFluxoCaixa.setData(row, "idLoja", idLoja)) { return; }

      QDate dataPgt = ui->widgetPgts->listDataPgt.at(pagamento)->date();
      if (dMaisUm) { dataPgt = dataPgt.addDays(1); }
      if (pula1Mes) { dataPgt = dataPgt.addMonths(1); }
      dataPgt = dataPgt.addMonths(parcela);
      if (ajustaDiaUtil) { dataPgt = qApp->ajustarDiaUtil(dataPgt); }

      if (not modelFluxoCaixa.setData(row, "dataPagamento", dataPgt)) { return; }

      double val;

      if (centavoSobressalente and parcela == 0) {
        val = valorParcela + restoParcela;
      } else if (not centavoSobressalente and parcela == parcelas - 1) {
        val = valorParcela + restoParcela;
      } else {
        val = valorParcela;
      }

      if (not modelFluxoCaixa.setData(row, "valor", val)) { return; }
      if (not modelFluxoCaixa.setData(row, "tipo", QString::number(pagamento + 1) + ". " + tipoPgt)) { return; }
      if (not modelFluxoCaixa.setData(row, "parcela", parcela + 1)) { return; }
      if (not modelFluxoCaixa.setData(row, "observacao", observacaoPgt)) { return; }
      if (not modelFluxoCaixa.setData(row, "representacao", isRepresentacao)) { return; }
      if (not modelFluxoCaixa.setData(row, "idConta", idConta)) { return; }
      if (not modelFluxoCaixa.setData(row, "grupo", "Produtos - Venda")) { return; }
      if (not modelFluxoCaixa.setData(row, "subGrupo", "")) { return; }
      if (not modelFluxoCaixa.setData(row, "centroCusto", idLoja)) { return; }

      //-----------------------------------------------------------------
      // calcular taxa

      const bool calculaTaxa = (not qFuzzyIsNull(porcentagemTaxa) and not isRepresentacao and not tipoPgt.contains("CONTA CLIENTE"));

      if (calculaTaxa) {
        const int rowTaxa = modelFluxoCaixa2.insertRowAtEnd();

        if (not modelFluxoCaixa2.setData(rowTaxa, "contraParte", "Administradora Cartão")) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "dataEmissao", dataEmissao)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "idVenda", ui->lineEditVenda->text())) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "idLoja", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "dataPagamento", dataPgt)) { return; }

        double val1;

        if (centavoSobressalente and parcela == 0) {
          val1 = parcelaTaxa + restoTaxa;
        } else if (not centavoSobressalente and parcela == parcelas - 1) {
          val1 = parcelaTaxa + restoTaxa;
        } else {
          val1 = parcelaTaxa;
        }

        if (not modelFluxoCaixa2.setData(rowTaxa, "valor", val1)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "tipo", QString::number(pagamento + 1) + ". Taxa Cartão")) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "parcela", parcela + 1)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "taxa", true)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "idConta", idConta)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "centroCusto", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(rowTaxa, "grupo", "Tarifas Cartão")) { return; }
      }

      //-----------------------------------------------------------------
      // calcular comissao loja

      const bool calculaComissao = (not qFuzzyIsNull(taxaComissao) and isRepresentacao and observacaoPgt != "FRETE");

      if (calculaComissao) {
        const double valorBase = modelFluxoCaixa.data(row, "valor").toDouble();
        const double valorComissao1 = valorBase * taxaComissao;
        const double valorComissao2 = taxaComissao * (valorBase - (valorBase / totalVenda * frete));
        const bool isFreteLoja = (observacaoPgt == "FRETE");
        const double valorAjustado = isFreteLoja ? valorComissao1 : valorComissao2;

        const int rowComissao = modelFluxoCaixa2.insertRowAtEnd();

        if (not modelFluxoCaixa2.setData(rowComissao, "contraParte", modelItem.data(0, "fornecedor"))) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "dataEmissao", dataEmissao)) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "idVenda", ui->lineEditVenda->text())) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "idLoja", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "dataPagamento", dataPgt.addMonths(1))) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "valor", valorAjustado)) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "tipo", QString::number(pagamento + 1) + ". Comissão")) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "parcela", parcela + 1)) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "comissao", true)) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "centroCusto", idLoja)) { return; }
        if (not modelFluxoCaixa2.setData(rowComissao, "grupo", "Comissão Representação")) { return; }
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

void Venda::on_pushButtonGerarPdf_clicked() {
  PDF pdf(data("idVenda").toString(), PDF::Tipo::Venda, this);
  pdf.gerarPdf();
}

void Venda::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Venda cadastrada com sucesso!", this); }

void Venda::on_pushButtonGerarExcel_clicked() {
  Excel excel(ui->lineEditVenda->text(), Excel::Tipo::Venda);
  excel.gerarExcel();
}

bool Venda::atualizarCredito() {
  double creditoRestante = ui->widgetPgts->getCredito();
  bool update = false;

  for (int i = 0; i < ui->widgetPgts->pagamentos; ++i) {
    if (ui->widgetPgts->listTipoPgt.at(i)->currentText() == "CONTA CLIENTE") {
      creditoRestante -= ui->widgetPgts->listValorPgt.at(i)->value();
      update = true;
    }
  }

  if (update) {
    QSqlQuery query;
    query.prepare("UPDATE cliente SET credito = :credito WHERE idCliente = :idCliente");
    query.bindValue(":credito", creditoRestante);
    query.bindValue(":idCliente", ui->itemBoxCliente->getId());

    if (not query.exec()) { return qApp->enqueueException(false, "Erro atualizando crédito do cliente: " + query.lastError().text(), this); }
  }

  return true;
}

bool Venda::cadastrar() {
  if (not qApp->startTransaction("Venda::cadastrar")) { return false; }

  const bool success = [&] {
    if (tipo == Tipo::Cadastrar) { // TODO: como sempre é cadastro, verificar se esse if pode ser removido
      if (not generateId()) { return false; }

      currentRow = model.insertRowAtEnd();
    }

    if (not savingProcedures()) { return false; }

    if (not atualizarCredito()) { return false; }

    if (not criarComissaoProfissional()) { return false; }

    // -------------------------------------------------------------------------

    if (not model.submitAll()) { return false; }

    // -------------------------------------------------------------------------

    primaryId = ui->lineEditVenda->text();

    if (primaryId.isEmpty()) { return qApp->enqueueException(false, "Id vazio!", this); }

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

    if (not query3.exec()) { return qApp->enqueueException(false, "Erro marcando orçamento como 'FECHADO': " + query3.lastError().text(), this); }

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

  if (not query.exec()) { return qApp->enqueueException(false, "Erro buscando produtos estoque: " + query.lastError().text(), this); }

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

    if (not query1.exec()) { return qApp->enqueueException(false, "Erro reativando orçamento: " + query1.lastError().text(), this); }
  }

  // -------------------------------------------------------------------------

  QSqlQuery query2;
  query2.prepare("UPDATE venda SET status = 'CANCELADO', statusFinanceiro = 'CANCELADO' WHERE idVenda = :idVenda");
  query2.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query2.exec()) { return qApp->enqueueException(false, "Erro marcando venda como cancelada: " + query2.lastError().text(), this); }

  // -------------------------------------------------------------------------

  QSqlQuery queryDelete;
  queryDelete.prepare("DELETE FROM estoque_has_consumo WHERE idVendaProduto2 IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda)");
  queryDelete.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not queryDelete.exec()) { return qApp->enqueueException(false, "Erro removendo consumo estoque: " + queryDelete.lastError().text(), this); }

  // -------------------------------------------------------------------------

  QSqlQuery query3;
  query3.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, `idVendaProduto2` = NULL WHERE `idVendaProduto2` IN (SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda)");
  query3.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query3.exec()) { return qApp->enqueueException(false, "Erro removendo vínculo da compra: " + query3.lastError().text(), this); }

  // -------------------------------------------------------------------------

  QSqlQuery query4;
  query4.prepare("UPDATE venda_has_produto2 SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query4.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query4.exec()) { return qApp->enqueueException(false, "Erro marcando produtos da venda como cancelados: " + query4.lastError().text(), this); }

  QSqlQuery query5;
  query5.prepare("UPDATE venda_has_produto SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query5.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query5.exec()) { return qApp->enqueueException(false, "Erro marcando produtos da venda como cancelados: " + query5.lastError().text(), this); }

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    if (modelFluxoCaixa.data(row, "tipo").toString().contains("CONTA CLIENTE")) {
      // TODO: é gerado crédito mesmo se a conta nao chegou a ser paga?
      if (modelFluxoCaixa.data(row, "status").toString() == "CANCELADO") { continue; }
      const double credito = modelFluxoCaixa.data(row, "valor").toDouble();

      QSqlQuery query6;
      query6.prepare("UPDATE cliente SET credito = credito + :valor WHERE idCliente = :idCliente");
      query6.bindValue(":valor", credito);
      query6.bindValue(":idCliente", model.data(0, "idCliente"));

      if (not query6.exec()) { return qApp->enqueueException(false, "Erro voltando credito do cliente: " + query6.lastError().text(), this); }
    }
  }

  QSqlQuery query7;
  query7.prepare("UPDATE conta_a_receber_has_pagamento SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query7.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query7.exec()) { return qApp->enqueueException(false, "Erro marcando contas como canceladas: " + query7.lastError().text(), this); }

  QSqlQuery query8;
  query8.prepare("UPDATE conta_a_pagar_has_pagamento SET status = 'CANCELADO' WHERE idVenda = :idVenda");
  query8.bindValue(":idVenda", ui->lineEditVenda->text());

  if (not query8.exec()) { return qApp->enqueueException(false, "Erro marcando RT como cancelado: " + query8.lastError().text(), this); }

  return true;
}

void Venda::on_pushButtonCancelamento_clicked() {
  // TODO: perguntar e salvar motivo do cancelamento
  // TODO: caso haja agendamento de entrega cancelar o agendamento primeiro?

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

  // TODO: readd this later
  //  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
  //    const QString status = modelFluxoCaixa.data(row, "status").toString();

  //    if (status == "RECEBIDO") {
  //      return qApp->enqueueError("Um ou mais pagamentos foram recebidos!\nPedir para o financeiro cancelar esses pagamentos para dar continuidade ao cancelamento da venda!", this);
  //    }
  //  }

  // -------------------------------------------------------------------------

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar venda");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  // -------------------------------------------------------------------------

  if (not qApp->startTransaction("Venda::on_pushButtonCancelamento")) { return; }

  if (not cancelamento()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  const QString idOrcamento = ui->lineEditIdOrcamento->text();

  qApp->enqueueInformation(idOrcamento.isEmpty() ? "Não havia Orçamento associado para ativar!" : "Orçamento " + idOrcamento + " reativado!", this);

  qApp->enqueueInformation("Venda cancelada!", this);
  close();
}

bool Venda::generateId() {
  const auto siglaLoja = UserSession::fromLoja("sigla", ui->itemBoxVendedor->text());

  if (not siglaLoja) { return qApp->enqueueException(false, "Erro buscando sigla da loja!", this); }

  QString id = siglaLoja->toString() + "-" + ui->dateTimeEdit->date().toString("yy");

  QSqlQuery query;
  query.prepare("SELECT MAX(idVenda) AS idVenda FROM venda WHERE idVenda LIKE :id");
  query.bindValue(":id", id + "%");

  if (not query.exec()) { return qApp->enqueueException(false, "Erro na query: " + query.lastError().text(), this); }

  const int last = query.first() ? query.value("idVenda").toString().remove(id).leftRef(4).toInt() : 0;

  id += QString("%1").arg(last + 1, 4, 10, QChar('0'));

  if (id.size() != 11) { return qApp->enqueueException(false, "Ocorreu algum erro ao gerar id: " + id, this); }

  id += representacao ? "R" : "";

  ui->lineEditVenda->setText(id);

  return true;
}

void Venda::on_pushButtonDevolucao_clicked() {
  auto *devolucao = new Devolucao(data("idVenda").toString(), data("representacao").toBool(), this);
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

  if (not qApp->startTransaction("Venda::on_pushButtonFinanceiroSalvar")) { return; }

  if (not financeiroSalvar()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Dados salvos com sucesso!", this);
  close();
}

void Venda::on_pushButtonCorrigirFluxo_clicked() {
  // TODO: usar um delegate para colocar o texto das linhas substituidas como tachado

  QSqlQuery queryPag;
  queryPag.prepare("SELECT credito FROM cliente WHERE idCliente = :idCliente");
  queryPag.bindValue(":idCliente", data("idCliente"));

  if (not queryPag.exec() or not queryPag.first()) { return qApp->enqueueException("Erro lendo credito cliente: " + queryPag.lastError().text(), this); }

  double credito = queryPag.value("credito").toDouble();

  for (int row = 0; row < modelFluxoCaixa.rowCount(); ++row) {
    if (modelFluxoCaixa.data(row, "tipo").toString().contains("CONTA CLIENTE")) { credito += modelFluxoCaixa.data(row, "valor").toDouble(); }
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

    if (not query.exec() or not query.first()) { return qApp->enqueueException("Erro buscando pontuação: " + query.lastError().text(), this); }

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

  if (not queryProdutos.exec()) { return qApp->enqueueException(false, "Erro buscando produtos: " + queryProdutos.lastError().text(), this); }

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
        return qApp->enqueueException(false, "Erro buscando status do estoque: " + queryStatus.lastError().text(), this);
      }

      if (not modelItem.setData(rowItem, "status", queryStatus.value("status"))) { return false; }
    } else {
      if (not modelItem.setData(rowItem, "status", "PENDENTE")) { return false; }
    }
  }

  for (int row = 0; row < modelItem.rowCount(); ++row) { backupItem.append(modelItem.record(row)); }

  return true;
}

void Venda::on_pushButtonComprovantes_clicked() {
  auto *comprovantes = new Comprovantes(ui->lineEditVenda->text(), this);
  comprovantes->show();
}

void Venda::on_pushButtonAdicionarObservacao_clicked() {
  const auto selection = ui->treeView->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

  //------------------------------------------------------

  for (const auto &index : selection) {
    if (not index.parent().isValid()) { return qApp->enqueueError("Deve selecionar apenas sublinhas!", this); }
  }

  const QString observacao = QInputDialog::getText(this, "Observação", "Obs: ");

  if (observacao.isEmpty()) { return; }

  //------------------------------------------------------

  for (const auto &index : selection) {
    const auto index2 = modelTree.proxyModel->mapToSource(index);
    const auto row = modelTree.mappedRow(index2);

    if (not modelItem2.setData(row, "obs", observacao)) { return; }
  }

  if (not modelItem2.submitAll()) { return; }

  modelTree.updateData();
}

// TODO: 0no corrigir fluxo esta mostrando os botoes de 'frete pago a loja' e 'pagamento total a loja' em pedidos que nao sao de representacao
// TODO: 0quando for 'MATR' nao criar fluxo caixa
// TODO: verificar se um pedido nao deveria ter seu 'statusFinanceiro' alterado para 'liberado' ao ter todos os pagamentos recebidos ('status' e 'statusFinanceiro' deveriam ser vinculados?)
// TODO: prazoEntrega por produto
// TODO: bloquear desconto maximo por classe de funcionario
// TODO: em vez de ter uma caixinha 'un', concatenar em 'quant', 'minimo' e 'un/cx'
