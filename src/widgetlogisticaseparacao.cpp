#include "widgetlogisticaseparacao.h"
#include "ui_widgetlogisticaseparacao.h"

#include "application.h"
#include "permissao.h"
#include "romaneio.h"
#include "sql.h"
#include "sqlquery.h"

#include <QSqlError>

WidgetLogisticaSeparacao::WidgetLogisticaSeparacao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaSeparacao) { ui->setupUi(this); }

WidgetLogisticaSeparacao::~WidgetLogisticaSeparacao() { delete ui; }

void WidgetLogisticaSeparacao::resetTables() { setupTables(); }

void WidgetLogisticaSeparacao::updateTables() {
  if (not isSet) {
    ui->lineEditBuscaProduto->setDelayed();
    ui->lineEditBuscaPedido->setDelayed();

    setupTables();

    setConnections();

    Permissao::aplicarTela(this, "logistica.separacao");

    isSet = true;
  }

  buscarProdutoLocal();
  buscarPedido();
}

void WidgetLogisticaSeparacao::setupTables() {
  modelProdutoLocal.setQuery(Sql::view_produto_localizacao() + " LIMIT 0");

  modelProdutoLocal.select();

  modelProdutoLocal.setHeaderData("tipo", "Tipo");
  modelProdutoLocal.setHeaderData("status", "Status");
  modelProdutoLocal.setHeaderData("idVenda", "Venda");
  modelProdutoLocal.setHeaderData("codComercial", "Cód. Com.");
  modelProdutoLocal.setHeaderData("descricao", "Produto");
  modelProdutoLocal.setHeaderData("formComercial", "Formato");
  modelProdutoLocal.setHeaderData("caixas", "Cx.");
  modelProdutoLocal.setHeaderData("quant", "Quant.");
  modelProdutoLocal.setHeaderData("un", "Un.");
  modelProdutoLocal.setHeaderData("lote", "Lote");
  modelProdutoLocal.setHeaderData("numeroNFe", "NF-e");
  modelProdutoLocal.setHeaderData("bloco", "Bloco");

  ui->tableProdutoLocal->setModel(&modelProdutoLocal);

  ui->tableProdutoLocal->hideColumn("idVendaProduto2");
  ui->tableProdutoLocal->hideColumn("idEstoque");

  // -----------------------------------------------------------------

  modelPedido.setQuery(Sql::view_agendar_entrega() + " LIMIT 0");

  modelPedido.select();

  modelPedido.setHeaderData("dataPrevEnt", "Prev. Ent.");
  modelPedido.setHeaderData("status", "Status");
  modelPedido.setHeaderData("fornecedor", "Fornecedor");
  modelPedido.setHeaderData("idVenda", "Venda");
  modelPedido.setHeaderData("nFeSaida", "NF-e");
  modelPedido.setHeaderData("nfeFutura", "NF-e Futura");
  modelPedido.setHeaderData("produto", "Produto");
  modelPedido.setHeaderData("idEstoque", "Estoque");
  modelPedido.setHeaderData("lote", "Lote");
  modelPedido.setHeaderData("local", "Local");
  modelPedido.setHeaderData("bloco", "Bloco");
  modelPedido.setHeaderData("caixas", "Caixas");
  modelPedido.setHeaderData("quant", "Quant.");
  modelPedido.setHeaderData("un", "Un.");
  modelPedido.setHeaderData("quantCaixa", "Quant./Cx.");
  modelPedido.setHeaderData("codComercial", "Cód. Com.");
  modelPedido.setHeaderData("formComercial", "Form. Com.");
  modelPedido.setHeaderData("dataFollowup", "Data Followup");
  modelPedido.setHeaderData("observacao", "Observação");

  ui->tablePedido->setModel(&modelPedido);

  ui->tablePedido->hideColumn("idVendaProduto2");
  ui->tablePedido->hideColumn("idProduto");
  ui->tablePedido->hideColumn("dataRealEnt");
  ui->tablePedido->hideColumn("idNFeSaida");
  ui->tablePedido->hideColumn("idNFeFutura");
  ui->tablePedido->hideColumn("idConsumo");
}

void WidgetLogisticaSeparacao::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBuscaProduto, &LineEdit::delayedTextChanged, this, &WidgetLogisticaSeparacao::buscarProdutoLocal, connectionType);
  connect(ui->lineEditBuscaPedido, &LineEdit::delayedTextChanged, this, &WidgetLogisticaSeparacao::buscarPedido, connectionType);
  connect(ui->tableProdutoLocal, &TableView::doubleClicked, this, &WidgetLogisticaSeparacao::on_tableProdutoLocal_doubleClicked, connectionType);
  connect(ui->tablePedido, &TableView::doubleClicked, this, &WidgetLogisticaSeparacao::on_tablePedido_doubleClicked, connectionType);
  connect(ui->pushButtonSeparar, &QPushButton::clicked, this, &WidgetLogisticaSeparacao::on_pushButtonSeparar_clicked, connectionType);
  connect(ui->pushButtonRomaneio, &QPushButton::clicked, this, &WidgetLogisticaSeparacao::on_pushButtonRomaneio_clicked, connectionType);
}

void WidgetLogisticaSeparacao::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->lineEditBuscaProduto, &LineEdit::delayedTextChanged, this, &WidgetLogisticaSeparacao::buscarProdutoLocal);
  disconnect(ui->lineEditBuscaPedido, &LineEdit::delayedTextChanged, this, &WidgetLogisticaSeparacao::buscarPedido);
  disconnect(ui->tableProdutoLocal, &TableView::doubleClicked, this, &WidgetLogisticaSeparacao::on_tableProdutoLocal_doubleClicked);
  disconnect(ui->tablePedido, &TableView::doubleClicked, this, &WidgetLogisticaSeparacao::on_tablePedido_doubleClicked);
  disconnect(ui->pushButtonSeparar, &QPushButton::clicked, this, &WidgetLogisticaSeparacao::on_pushButtonSeparar_clicked);
  disconnect(ui->pushButtonRomaneio, &QPushButton::clicked, this, &WidgetLogisticaSeparacao::on_pushButtonRomaneio_clicked);
}

void WidgetLogisticaSeparacao::buscarProdutoLocal() {
  const QString textoBusca = qApp->escaparBusca(ui->lineEditBuscaProduto->text());

  modelProdutoLocal.setQuery(Sql::view_produto_localizacao(textoBusca) + (textoBusca.isEmpty() ? " LIMIT 0" : ""));

  modelProdutoLocal.select();
}

void WidgetLogisticaSeparacao::buscarPedido() {
  const QString idVenda = qApp->escaparBusca(ui->lineEditBuscaPedido->text());

  modelPedido.setQuery(Sql::view_agendar_entrega(idVenda, "vp2.quant != 0") + (idVenda.isEmpty() ? " LIMIT 0" : ""));

  modelPedido.select();
}

void WidgetLogisticaSeparacao::on_tableProdutoLocal_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString idVenda = modelProdutoLocal.data(index.row(), "idVenda").toString();

  if (idVenda.isEmpty()) { return qApp->enqueueInformation("Este item não está vinculado a nenhum pedido.", this); }

  ui->lineEditBuscaPedido->setText(idVenda);

  buscarPedido();
}

void WidgetLogisticaSeparacao::on_tablePedido_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelPedido.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelPedido.data(index.row(), "idVenda")); }

  if (header == "NF-e") { return qApp->abrirNFe(modelPedido.data(index.row(), "idNFeSaida")); }

  if (header == "NF-e Futura") { return qApp->abrirNFe(modelPedido.data(index.row(), "idNFeFutura")); }

  if (header == "Estoque") { return qApp->abrirEstoque(modelPedido.data(index.row(), "idEstoque")); }
}

void WidgetLogisticaSeparacao::on_pushButtonSeparar_clicked() {
  const auto selection = ui->tablePedido->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QStringList idVendas;
  QList<int> idsVendaProduto2;
  bool temEstoque = false;
  bool temSeparado = false;

  for (const auto &index : selection) {
    const QString status = modelPedido.data(index.row(), "status").toString();

    if (status == "ESTOQUE") {
      temEstoque = true;
    } else if (status == "SEPARADO") {
      temSeparado = true;
    } else {
      throw RuntimeError("Produto '" + modelPedido.data(index.row(), "produto").toString() + "' não está em estoque!", this);
    }

    idVendas << modelPedido.data(index.row(), "idVenda").toString();
    idsVendaProduto2 << modelPedido.data(index.row(), "idVendaProduto2").toInt();
  }

  if (temEstoque and temSeparado) { throw RuntimeError("Selecione apenas itens em estoque OU apenas itens separados!", this); }

  const bool marcarSeparado = temEstoque;

  qApp->startTransaction("WidgetLogisticaSeparacao::on_pushButtonSeparar");

  Sql::separarProdutos(idsVendaProduto2, marcarSeparado);

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  buscarPedido();

  qApp->enqueueInformation(marcarSeparado ? "Separação confirmada!" : "Separação desfeita!", this);
}

void WidgetLogisticaSeparacao::on_pushButtonRomaneio_clicked() {
  const QString idVenda = ui->lineEditBuscaPedido->text();

  SqlQuery query;
  // mesmo filtro de status usado em Romaneio::gerar para montar o manifesto - sem isso, MAX()
  // pega a data de itens ja ENTREGUE/DEVOLVIDO/CANCELADO/QUEBRADO de meses atras em vez da data
  // dos itens que realmente entram no romaneio agora.
  query.prepare("SELECT MAX(dataPrevEnt) AS dataPrevEnt FROM venda_has_produto2 WHERE idVenda = :idVenda AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'ENTREGUE', 'QUEBRADO')");
  query.bindValue(":idVenda", idVenda);

  if (not query.exec()) { throw RuntimeException("Erro buscando data de entrega: " + query.lastError().text(), this); }

  QDate dataEntrega = qApp->serverDate();

  if (query.first() and not query.value("dataPrevEnt").isNull()) { dataEntrega = query.value("dataPrevEnt").toDate(); }

  const QString fileName = Romaneio::gerar(idVenda, dataEntrega, this);

  qApp->enqueueInformation("Romaneio salvo como:\n" + fileName, this);
}
