#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>

#include <QSqlError>
#include <QSqlRecord>

#include "application.h"
#include "cadastrarnfe.h"
#include "doubledelegate.h"
#include "financeiroproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "ui_widgetlogisticaagendarentrega.h"
#include "usersession.h"
#include "widgetlogisticaagendarentrega.h"

WidgetLogisticaAgendarEntrega::WidgetLogisticaAgendarEntrega(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarEntrega) { ui->setupUi(this); }

WidgetLogisticaAgendarEntrega::~WidgetLogisticaAgendarEntrega() { delete ui; }

void WidgetLogisticaAgendarEntrega::setupTables() {
  modelVendas.setTable("view_entrega_pendente");

  modelVendas.setSort("prazoEntrega");

  modelVendas.setHeaderData("prazoEntrega", "Prazo Limite");
  modelVendas.setHeaderData("novoPrazoEntrega", "Novo Prazo");
  modelVendas.setHeaderData("dataRealReceb", "Receb.");
  modelVendas.setHeaderData("statusFinanceiro", "Financeiro");
  modelVendas.setHeaderData("idVenda", "Venda");

  modelVendas.proxyModel = new FinanceiroProxyModel(&modelVendas, this);

  ui->tableVendas->setModel(&modelVendas);

  ui->tableVendas->setItemDelegate(new DoubleDelegate(this));

  ui->tableVendas->hideColumn("data");

  // -----------------------------------------------------------------

  modelProdutos.setTable("view_agendar_entrega");

  modelProdutos.setHeaderData("dataPrevEnt", "Prev. Ent.");
  modelProdutos.setHeaderData("status", "Status");
  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("idNFeSaida", "NFe");
  modelProdutos.setHeaderData("idNFeFutura", "NFe Futura");
  modelProdutos.setHeaderData("produto", "Produto");
  modelProdutos.setHeaderData("idEstoque", "Estoque");
  modelProdutos.setHeaderData("lote", "Lote");
  modelProdutos.setHeaderData("local", "Local");
  modelProdutos.setHeaderData("bloco", "Bloco");
  modelProdutos.setHeaderData("caixas", "Caixas");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("unCaixa", "Un./Cx.");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataRealEnt");
  ui->tableProdutos->hideColumn("idConsumo");

  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarEntrega::calcularPeso);

  // -----------------------------------------------------------------

  modelTranspAtual.setTable("veiculo_has_produto");

  modelTranspAtual.setHeaderData("idVenda", "Venda");
  modelTranspAtual.setHeaderData("status", "Status");
  modelTranspAtual.setHeaderData("fornecedor", "Fornecedor");
  modelTranspAtual.setHeaderData("produto", "Produto");
  modelTranspAtual.setHeaderData("caixas", "Cx.");
  modelTranspAtual.setHeaderData("kg", "Kg.");
  modelTranspAtual.setHeaderData("quant", "Quant.");
  modelTranspAtual.setHeaderData("un", "Un.");
  modelTranspAtual.setHeaderData("unCaixa", "Un./Cx.");
  modelTranspAtual.setHeaderData("codComercial", "Cód. Com.");
  modelTranspAtual.setHeaderData("formComercial", "Form. Com.");

  ui->tableTranspAtual->setModel(&modelTranspAtual);

  ui->tableTranspAtual->hideColumn("id");
  ui->tableTranspAtual->hideColumn("idEstoque");
  ui->tableTranspAtual->hideColumn("idEvento");
  ui->tableTranspAtual->hideColumn("idVeiculo");
  ui->tableTranspAtual->hideColumn("idCompra");
  ui->tableTranspAtual->hideColumn("idVendaProduto");
  ui->tableTranspAtual->hideColumn("idNFeSaida");
  ui->tableTranspAtual->hideColumn("idLoja");
  ui->tableTranspAtual->hideColumn("idProduto");
  ui->tableTranspAtual->hideColumn("obs");
  ui->tableTranspAtual->hideColumn("data");

  // -----------------------------------------------------------------

  modelTranspAgend.setTable("veiculo_has_produto");

  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
  modelTranspAgend.setHeaderData("idVenda", "Venda");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("fornecedor", "Fornecedor");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("kg", "Kg.");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("unCaixa", "Un./Cx.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");
  modelTranspAgend.setHeaderData("formComercial", "Form. Com.");

  ui->tableTranspAgend->setModel(&modelTranspAgend);

  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idVendaProduto");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("obs");
}

void WidgetLogisticaAgendarEntrega::calcularPeso() {
  double peso = 0;

  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

  const auto selectedRows = ui->tableProdutos->selectionModel()->selectedRows();

  for (const auto &item : selectedRows) {
    query.bindValue(":idProduto", modelProdutos.data(item.row(), "idProduto"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando peso do produto: " + query.lastError().text(), this); }

    const double kg = query.value("kgcx").toDouble();
    const double caixas = modelProdutos.data(item.row(), "caixas").toDouble();
    peso += kg * caixas;
  }

  ui->doubleSpinBoxPeso->setValue(peso);

  ui->doubleSpinBoxPeso->setStyleSheet(ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value() ? "color: rgb(255, 0, 0);" : "");
}

void WidgetLogisticaAgendarEntrega::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetLogisticaAgendarEntrega::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetLogisticaAgendarEntrega::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaAgendarEntrega::montaFiltro, connectionType);
  connect(ui->pushButtonAdicionarParcial, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarParcial_clicked, connectionType);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarProduto_clicked, connectionType);
  connect(ui->pushButtonAgendarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonAgendarCarga_clicked, connectionType);
  connect(ui->pushButtonGerarNFeFutura, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonGerarNFeFutura_clicked, connectionType);
  connect(ui->pushButtonImportarNFe, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonImportarNFe_clicked, connectionType);
  connect(ui->pushButtonReagendarPedido, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonReagendarPedido_clicked, connectionType);
  connect(ui->pushButtonRemoverProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarEntrega::on_pushButtonRemoverProduto_clicked, connectionType);
  connect(ui->radioButtonEntregaLimpar, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro, connectionType);
  connect(ui->radioButtonParcialEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro, connectionType);
  connect(ui->radioButtonSemEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro, connectionType);
  connect(ui->radioButtonTotalEstoque, &QRadioButton::clicked, this, &WidgetLogisticaAgendarEntrega::montaFiltro, connectionType);
  connect(ui->tableVendas, &TableView::clicked, this, &WidgetLogisticaAgendarEntrega::on_tableVendas_clicked, connectionType);
  connect(ui->tableVendas, &TableView::doubleClicked, this, &WidgetLogisticaAgendarEntrega::on_tableVendas_doubleClicked, connectionType);
}

void WidgetLogisticaAgendarEntrega::updateTables() {
  if (not isSet) {
    if (UserSession::tipoUsuario() == "VENDEDOR") {
      ui->tableVendas->hide();
      ui->labelEntregasCliente->hide();
    }

    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    ui->dateTimeEdit->setDate(QDate::currentDate());

    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelVendas.select()) { return; }

  // -------------------------------------------------------------------------

  if (not modelProdutos.select()) { return; }

  // -------------------------------------------------------------------------

  if (not modelTranspAgend.select()) { return; }

  // -------------------------------------------------------------------------

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::resetTables() { modelIsSet = false; }

void WidgetLogisticaAgendarEntrega::on_tableVendas_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString idVenda = modelVendas.data(index.row(), "idVenda").toString();

  modelProdutos.setFilter("idVenda = '" + idVenda + "'");

  if (not modelProdutos.select()) { return qApp->enqueueError("Erro buscando produtos: " + modelProdutos.lastError().text()); }

  ui->lineEditAviso->setText(modelVendas.data(index.row(), "statusFinanceiro").toString() != "LIBERADO" ? "Financeiro não liberou!" : "");
}

void WidgetLogisticaAgendarEntrega::montaFiltro() {
  QStringList filtros;

  QString filtroCheck;

  if (ui->radioButtonEntregaLimpar->isChecked()) { filtroCheck = "(Estoque > 0 OR Outros > 0)"; }
  if (ui->radioButtonTotalEstoque->isChecked()) { filtroCheck = "Estoque > 0 AND Outros = 0"; }
  if (ui->radioButtonParcialEstoque->isChecked()) { filtroCheck = "Estoque > 0 AND Outros > 0"; }
  if (ui->radioButtonSemEstoque->isChecked()) { filtroCheck = "Estoque = 0 AND Outros > 0"; }

  if (not filtroCheck.isEmpty()) { filtros << filtroCheck; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();

  const QString filtroBusca = "(idVenda LIKE '%" + textoBusca + "%' OR Bairro LIKE '%" + textoBusca + "%' OR Logradouro LIKE '%" + textoBusca + "%' OR Cidade LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelVendas.setFilter(filtros.join(" AND "));

  modelProdutos.setFilter("0");
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAgendarCarga_clicked() {
  // TODO: não deixar agendar carga com mistura de produtos com e sem nfe futura

  if (modelTranspAtual.rowCount() == 0) { return qApp->enqueueError("Carga vazia!", this); }

  if (not qApp->startTransaction()) { return; }

  if (not processRows()) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Confirmado agendamento!", this);
}

bool WidgetLogisticaAgendarEntrega::processRows() {
  const QDateTime dataPrevEnt = ui->dateTimeEdit->dateTime();

  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto") or not query.first()) {
    return qApp->enqueueError(false, "Erro comunicando com o banco de dados: " + query.lastError().text(), this);
  }

  const int idEvento = query.value(0).toInt();

  QSqlQuery query1;
  query1.prepare("SELECT idVenda, codComercial FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'ENTREGA AGEND.', dataPrevEnt = :dataPrevEnt WHERE status = 'ESTOQUE' AND idVendaProduto = :idVendaProduto");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto SET status = 'ENTREGA AGEND.', dataPrevEnt = :dataPrevEnt WHERE status = 'ESTOQUE' AND idVendaProduto = :idVendaProduto");

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) {
    if (not modelTranspAtual.setData(row, "data", dataPrevEnt)) { return false; }
    if (not modelTranspAtual.setData(row, "idEvento", idEvento)) { return false; }

    const int idVendaProduto = modelTranspAtual.data(row, "idVendaProduto").toInt();

    query1.bindValue(":idVendaProduto", idVendaProduto);

    if (not query1.exec() or not query1.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + query1.lastError().text(), this); }

    query2.bindValue(":dataPrevEnt", dataPrevEnt);
    query2.bindValue(":idVendaProduto", idVendaProduto);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + query2.lastError().text(), this); }

    query3.bindValue(":dataPrevEnt", dataPrevEnt);
    query3.bindValue(":idVendaProduto", idVendaProduto);

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro atualizando produtos venda: " + query3.lastError().text(), this); }
  }

  return modelTranspAtual.submitAll();
}

bool WidgetLogisticaAgendarEntrega::adicionarProduto(const QModelIndexList &list) {
  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");

  for (const auto &item : list) {
    query.bindValue(":idProduto", modelProdutos.data(item.row(), "idProduto"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando peso do produto: " + query.lastError().text(), this); }

    const double kg = query.value("kgcx").toDouble();
    const double caixas = modelProdutos.data(item.row(), "caixas").toDouble();
    const double peso = kg * caixas;

    // -------------------------------------------------------------------------

    const int row = modelTranspAtual.insertRowAtEnd();

    if (not modelTranspAtual.setData(row, "fornecedor", modelProdutos.data(item.row(), "fornecedor"))) { return false; }
    if (not modelTranspAtual.setData(row, "unCaixa", modelProdutos.data(item.row(), "unCaixa"))) { return false; }
    if (not modelTranspAtual.setData(row, "formComercial", modelProdutos.data(item.row(), "formComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "idVeiculo", ui->itemBoxVeiculo->getId())) { return false; }
    if (not modelTranspAtual.setData(row, "idVenda", modelProdutos.data(item.row(), "idVenda"))) { return false; }
    if (not modelTranspAtual.setData(row, "idVendaProduto", modelProdutos.data(item.row(), "idVendaProduto"))) { return false; }
    if (not modelTranspAtual.setData(row, "idProduto", modelProdutos.data(item.row(), "idProduto"))) { return false; }
    if (not modelTranspAtual.setData(row, "produto", modelProdutos.data(item.row(), "produto"))) { return false; }
    if (not modelTranspAtual.setData(row, "codComercial", modelProdutos.data(item.row(), "codComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "un", modelProdutos.data(item.row(), "un"))) { return false; }
    if (not modelTranspAtual.setData(row, "caixas", modelProdutos.data(item.row(), "caixas"))) { return false; }
    if (not modelTranspAtual.setData(row, "kg", peso)) { return false; }
    if (not modelTranspAtual.setData(row, "quant", modelProdutos.data(item.row(), "quant"))) { return false; }
    if (not modelTranspAtual.setData(row, "status", "ENTREGA AGEND.")) { return false; }
  }

  return true;
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { return qApp->enqueueError("Deve escolher uma transportadora antes!", this); }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!", this); }

  for (const auto &item : list) {
    const auto listMatch = modelTranspAtual.match("idVendaProduto", modelProdutos.data(item.row(), "idVendaProduto"), 1, Qt::MatchExactly);

    if (not listMatch.isEmpty()) { return qApp->enqueueError("Item '" + modelProdutos.data(item.row(), "produto").toString() + "' já inserido!", this); }

    if (modelProdutos.data(item.row(), "status").toString() != "ESTOQUE") { return qApp->enqueueError("Produto não está em estoque!", this); }

    if (modelProdutos.data(item.row(), "idConsumo").toString().isEmpty()) { return qApp->enqueueError("Produto ainda não possui nota de entrada!", this); }

    if (not modelProdutos.data(item.row(), "dataPrevEnt").isNull()) { return qApp->enqueueError("Produto já agendado!", this); }
  }

  if (not adicionarProduto(list) and not modelTranspAtual.select()) { return; }

  ui->tableProdutos->clearSelection();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonRemoverProduto_clicked() {
  const auto list = ui->tableTranspAtual->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  for (const auto &item : list) { modelTranspAtual.removeRow(item.row()); }
}

void WidgetLogisticaAgendarEntrega::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados veiculo: " + query.lastError().text(), this); }

  if (not modelTranspAtual.select()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::calcularDisponivel() {
  double peso = 0;

  for (int row = 0; row < modelTranspAgend.rowCount(); ++row) {
    const int hour = modelTranspAgend.data(row, "data").toDateTime().time().hour();

    if (hour != ui->dateTimeEdit->time().hour()) { continue; }

    peso += modelTranspAgend.data(row, "kg").toDouble();
  }

  const double capacidade = ui->doubleSpinBoxCapacidade->value();

  ui->doubleSpinBoxDisponivel->setValue(capacidade - peso);
}

void WidgetLogisticaAgendarEntrega::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");

  calcularDisponivel();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonAdicionarParcial_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { return qApp->enqueueError("Deve escolher uma transportadora antes!", this); }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  if (list.size() > 1) { return qApp->enqueueError("Deve selecionar apenas um item para agendamento parcial!", this); }

  const int row = list.first().row();

  const auto list2 = modelTranspAtual.match("idVendaProduto", modelProdutos.data(row, "idVendaProduto"), 1, Qt::MatchExactly);

  if (not list2.isEmpty()) { return qApp->enqueueError("Item '" + modelProdutos.data(row, "produto").toString() + "' já inserido!", this); }

  if (modelProdutos.data(row, "status").toString() != "ESTOQUE") { return qApp->enqueueError("Produto não está em estoque!", this); }

  // TODO: calcular o peso parcial e não o total para comparar
  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxDisponivel->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!", this); }

  if (modelProdutos.data(row, "idConsumo").toString().isEmpty()) { return qApp->enqueueError("Produto ainda não possui nota de entrada!", this); }

  if (modelProdutos.data(row, "caixas").toInt() == 1) { return qApp->enqueueError("Produto tem apenas uma caixa!", this); }

  if (not modelProdutos.data(row, "dataPrevEnt").isNull()) { return qApp->enqueueError("Produto já agendado!", this); }

  // perguntar quantidade
  const int caixasTotal = modelProdutos.data(row, "caixas").toInt();

  bool ok;

  const int caixasAgendar = QInputDialog::getInt(this, "Agendar", "Quantidade de caixas: ", caixasTotal, 0, caixasTotal, 1, &ok);

  if (caixasAgendar == 0 or not ok) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not adicionarProdutoParcial(row, caixasAgendar, caixasTotal)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  ui->tableProdutos->clearSelection();
}

bool WidgetLogisticaAgendarEntrega::adicionarProdutoParcial(const int row, const int caixasAgendar, const int caixasTotal) {
  if (caixasAgendar < caixasTotal) {
    if (not dividirProduto(row, caixasAgendar, caixasTotal)) { return false; }
  }

  QSqlQuery query;
  query.prepare("SELECT kgcx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelProdutos.data(row, "idProduto"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando peso do produto: " + query.lastError().text(), this); }

  const double unCaixa = modelProdutos.data(row, "unCaixa").toDouble();
  const double kg = query.value("kgcx").toDouble();

  const int newRow = modelTranspAtual.insertRowAtEnd();

  if (not modelTranspAtual.setData(newRow, "fornecedor", modelProdutos.data(row, "fornecedor"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "unCaixa", modelProdutos.data(row, "unCaixa"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "formComercial", modelProdutos.data(row, "formComercial"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "idVeiculo", ui->itemBoxVeiculo->getId())) { return false; }
  if (not modelTranspAtual.setData(newRow, "idVenda", modelProdutos.data(row, "idVenda"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "idVendaProduto", modelProdutos.data(row, "idVendaProduto"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "idProduto", modelProdutos.data(row, "idProduto"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "produto", modelProdutos.data(row, "produto"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "codComercial", modelProdutos.data(row, "codComercial"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "un", modelProdutos.data(row, "un"))) { return false; }
  if (not modelTranspAtual.setData(newRow, "caixas", caixasAgendar)) { return false; }
  if (not modelTranspAtual.setData(newRow, "kg", kg * caixasAgendar)) { return false; }
  if (not modelTranspAtual.setData(newRow, "quant", caixasAgendar * unCaixa)) { return false; }
  if (not modelTranspAtual.setData(newRow, "status", "ENTREGA AGEND.")) { return false; }

  return true;
}

bool WidgetLogisticaAgendarEntrega::dividirProduto(const int row, const int caixasAgendar, const int caixasTotal) {
  // TODO: quebrar linha em pedido_fornecedor tambem para manter 1:1

  SqlRelationalTableModel modelProdutosTemp;
  modelProdutosTemp.setTable("venda_has_produto");

  modelProdutosTemp.setFilter("idVendaProduto = " + modelProdutos.data(row, "idVendaProduto").toString());

  if (not modelProdutosTemp.select()) { return false; }

  const int newRow = modelProdutosTemp.insertRowAtEnd();
  // NOTE: *quebralinha venda_produto

  // copiar colunas
  for (int column = 0, columnCount = modelProdutosTemp.columnCount(); column < columnCount; ++column) {
    if (modelProdutosTemp.fieldIndex("idVendaProduto") == column) { continue; }
    if (modelProdutosTemp.fieldIndex("created") == column) { continue; }
    if (modelProdutosTemp.fieldIndex("lastUpdated") == column) { continue; }

    const QVariant value = modelProdutosTemp.data(0, column);

    if (not modelProdutosTemp.setData(newRow, column, value)) { return false; }
  }

  const QDecDouble caixasAgendar2 = caixasAgendar;
  const QDecDouble caixasTotal2 = caixasTotal;

  const QDecDouble unCaixa = modelProdutosTemp.data(0, "unCaixa").toDouble();

  const QDecDouble proporcao = caixasAgendar2 / caixasTotal2;
  const QDecDouble parcial = QDecDouble(modelProdutosTemp.data(0, "parcial").toDouble()) * proporcao;
  const QDecDouble parcialDesc = QDecDouble(modelProdutosTemp.data(0, "parcialDesc").toDouble()) * proporcao;
  const QDecDouble total = QDecDouble(modelProdutosTemp.data(0, "total").toDouble()) * proporcao;

  if (not modelProdutosTemp.setData(0, "quant", (caixasAgendar2 * unCaixa).toDouble())) { return false; }
  if (not modelProdutosTemp.setData(0, "caixas", caixasAgendar2.toDouble())) { return false; }
  if (not modelProdutosTemp.setData(0, "parcial", parcial.toDouble())) { return false; }
  if (not modelProdutosTemp.setData(0, "parcialDesc", parcialDesc.toDouble())) { return false; }
  if (not modelProdutosTemp.setData(0, "total", total.toDouble())) { return false; }

  // alterar quant, precos, etc da linha nova
  const QDecDouble proporcaoNovo = (caixasTotal2 - caixasAgendar2) / caixasTotal2;
  const QDecDouble parcialNovo = QDecDouble(modelProdutosTemp.data(newRow, "parcial").toDouble()) * proporcaoNovo;
  const QDecDouble parcialDescNovo = QDecDouble(modelProdutosTemp.data(newRow, "parcialDesc").toDouble()) * proporcaoNovo;
  const QDecDouble totalNovo = QDecDouble(modelProdutosTemp.data(newRow, "total").toDouble()) * proporcaoNovo;

  if (not modelProdutosTemp.setData(newRow, "idRelacionado", modelProdutosTemp.data(0, "idVendaProduto"))) { return false; }
  if (not modelProdutosTemp.setData(newRow, "quant", ((caixasTotal2 - caixasAgendar2) * unCaixa).toDouble())) { return false; }
  if (not modelProdutosTemp.setData(newRow, "caixas", (caixasTotal2 - caixasAgendar2).toDouble())) { return false; }
  if (not modelProdutosTemp.setData(newRow, "parcial", parcialNovo.toDouble())) { return false; }
  if (not modelProdutosTemp.setData(newRow, "parcialDesc", parcialDescNovo.toDouble())) { return false; }
  if (not modelProdutosTemp.setData(newRow, "total", totalNovo.toDouble())) { return false; }

  if (not modelProdutosTemp.submitAll()) { return false; }

  // TODO: em vez de quebrar consumo, mandar para a funcao Estoque::criarConsumo?
  if (not dividirConsumo(row, proporcao, proporcaoNovo, modelProdutosTemp.query().lastInsertId().toInt())) { return false; }

  return true;
}

bool WidgetLogisticaAgendarEntrega::dividirConsumo(const int row, const QDecDouble proporcao, const QDecDouble proporcaoNovo, const int idVendaProduto) {
  SqlRelationalTableModel modelConsumoTemp;
  modelConsumoTemp.setTable("estoque_has_consumo");

  modelConsumoTemp.setFilter("idVendaProduto = " + modelProdutos.data(row, "idVendaProduto").toString());

  if (not modelConsumoTemp.select()) { return false; }

  const int rowConsumo = modelConsumoTemp.insertRowAtEnd();

  for (int column = 0, columnCount = modelConsumoTemp.columnCount(); column < columnCount; ++column) {
    if (modelConsumoTemp.fieldIndex("idConsumo") == column) { continue; }
    if (modelConsumoTemp.fieldIndex("idVendaProduto") == column) { continue; }
    if (modelConsumoTemp.fieldIndex("created") == column) { continue; }
    if (modelConsumoTemp.fieldIndex("lastUpdated") == column) { continue; }

    const QVariant value = modelConsumoTemp.data(0, column);

    if (not modelConsumoTemp.setData(rowConsumo, column, value)) { return false; }
  }

  // alterar quant, caixas, valor

  const QDecDouble quantConsumo = QDecDouble(modelConsumoTemp.data(0, "quant").toDouble()) * proporcao;
  const QDecDouble caixasConsumo = QDecDouble(modelConsumoTemp.data(0, "caixas").toDouble()) * proporcao;
  const QDecDouble valorConsumo = QDecDouble(modelConsumoTemp.data(0, "valor").toDouble()) * proporcao;

  const QDecDouble desconto = QDecDouble(modelConsumoTemp.data(0, "desconto").toDouble()) * proporcao;
  const QDecDouble vBC = QDecDouble(modelConsumoTemp.data(0, "vBC").toDouble()) * proporcao;
  const QDecDouble vICMS = QDecDouble(modelConsumoTemp.data(0, "vICMS").toDouble()) * proporcao;
  const QDecDouble vBCST = QDecDouble(modelConsumoTemp.data(0, "vBCST").toDouble()) * proporcao;
  const QDecDouble vICMSST = QDecDouble(modelConsumoTemp.data(0, "vICMSST").toDouble()) * proporcao;
  const QDecDouble vBCPIS = QDecDouble(modelConsumoTemp.data(0, "vBCPIS").toDouble()) * proporcao;
  const QDecDouble vPIS = QDecDouble(modelConsumoTemp.data(0, "vPIS").toDouble()) * proporcao;
  const QDecDouble vBCCOFINS = QDecDouble(modelConsumoTemp.data(0, "vBCCOFINS").toDouble()) * proporcao;
  const QDecDouble vCOFINS = QDecDouble(modelConsumoTemp.data(0, "vCOFINS").toDouble()) * proporcao;

  if (not modelConsumoTemp.setData(0, "quant", quantConsumo.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "caixas", caixasConsumo.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "valor", valorConsumo.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "desconto", desconto.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vBC", vBC.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vICMS", vICMS.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vBCST", vBCST.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vICMSST", vICMSST.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vBCPIS", vBCPIS.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vPIS", vPIS.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vBCCOFINS", vBCCOFINS.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(0, "vCOFINS", vCOFINS.toDouble())) { return false; }

  // alterar linha nova
  const QDecDouble quantConsumo2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "quant").toDouble()) * proporcaoNovo;
  const QDecDouble caixasConsumo2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "caixas").toDouble()) * proporcaoNovo;
  const QDecDouble valorConsumo2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "valor").toDouble()) * proporcaoNovo;

  const QDecDouble desconto2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "desconto").toDouble()) * proporcaoNovo;
  const QDecDouble vBC2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vBC").toDouble()) * proporcaoNovo;
  const QDecDouble vICMS2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vICMS").toDouble()) * proporcaoNovo;
  const QDecDouble vBCST2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vBCST").toDouble()) * proporcaoNovo;
  const QDecDouble vICMSST2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vICMSST").toDouble()) * proporcaoNovo;
  const QDecDouble vBCPIS2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vBCPIS").toDouble()) * proporcaoNovo;
  const QDecDouble vPIS2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vPIS").toDouble()) * proporcaoNovo;
  const QDecDouble vBCCOFINS2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vBCCOFINS").toDouble()) * proporcaoNovo;
  const QDecDouble vCOFINS2 = QDecDouble(modelConsumoTemp.data(rowConsumo, "vCOFINS").toDouble()) * proporcaoNovo;

  if (not modelConsumoTemp.setData(rowConsumo, "idVendaProduto", idVendaProduto)) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "quant", quantConsumo2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "caixas", caixasConsumo2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "valor", valorConsumo2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "desconto", desconto2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vBC", vBC2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vICMS", vICMS2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vBCST", vBCST2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vICMSST", vICMSST2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vBCPIS", vBCPIS2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vPIS", vPIS2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vBCCOFINS", vBCCOFINS2.toDouble())) { return false; }
  if (not modelConsumoTemp.setData(rowConsumo, "vCOFINS", vCOFINS2.toDouble())) { return false; }

  if (not modelConsumoTemp.submitAll()) { return false; }

  return true;
}

void WidgetLogisticaAgendarEntrega::on_pushButtonReagendarPedido_clicked() {
  // get idVenda from view_entrega_pendente/modelVendas and set 'novoPrazo'

  const auto list = ui->tableVendas->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::ReagendarPedido, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not reagendar(list, input.getNextDate(), input.getObservacao())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

bool WidgetLogisticaAgendarEntrega::reagendar(const QModelIndexList &list, const QDate &dataPrev, const QString &observacao) {
  QSqlQuery query1;
  query1.prepare("UPDATE venda SET novoPrazoEntrega = :novoPrazoEntrega WHERE idVenda = :idVenda");

  QSqlQuery query2;
  query2.prepare("INSERT INTO venda_has_followup (idVenda, idVendaBase, idLoja, idUsuario, tipoOperacao, observacao, dataFollowup) VALUES (:idVenda, :idVendaBase, :idLoja, :idUsuario, :tipoOperacao, "
                 ":observacao, :dataFollowup)");

  for (const auto &item : list) {
    query1.bindValue(":novoPrazoEntrega", modelVendas.data(item.row(), "data").toDate().daysTo(dataPrev));
    query1.bindValue(":idVenda", modelVendas.data(item.row(), "idVenda"));

    if (not query1.exec()) { return qApp->enqueueError(false, "Erro atualizando novo prazo: " + query1.lastError().text(), this); }

    query2.bindValue(":idVenda", modelVendas.data(item.row(), "idVenda"));
    query2.bindValue(":idVendaBase", modelVendas.data(item.row(), "idVenda").toString().left(11));
    query2.bindValue(":idLoja", UserSession::idLoja());
    query2.bindValue(":idUsuario", UserSession::idUsuario());
    query2.bindValue(":tipoOperacao", "Alteração do prazo de entrega");
    query2.bindValue(":observacao", observacao);
    query2.bindValue(":dataFollowup", QDate::currentDate());

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando followup: " + query2.lastError().text(), this); }
  }

  return true;
}

void WidgetLogisticaAgendarEntrega::on_tableVendas_doubleClicked(const QModelIndex &index) {
  if (index.column() == modelVendas.record().indexOf("novoPrazoEntrega")) {
    const auto list = ui->tableVendas->selectionModel()->selectedRows();

    if (list.isEmpty()) { return qApp->enqueueError("Nenhuma linha selecionada!", this); }

    FollowUp *followup = new FollowUp(modelVendas.data(list.first().row(), "idVenda").toString(), FollowUp::Tipo::Venda, this);
    followup->setAttribute(Qt::WA_DeleteOnClose);
    followup->show();
  }
}

void WidgetLogisticaAgendarEntrega::on_pushButtonGerarNFeFutura_clicked() {
  if (not ui->tableProdutos->model()) { return; }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  for (const auto &item : list) {
    if (not modelProdutos.data(item.row(), "idNFeFutura").isNull()) { return qApp->enqueueError("Produto já possui nota futura!", this); }

    if (modelProdutos.data(item.row(), "idConsumo").toInt() == 0) { return qApp->enqueueError("Nem todos os produtos selecionados possuem nota de entrada!", this); }
  }

  const QString idVenda = modelProdutos.data(list.first().row(), "idVenda").toString();

  if (idVenda.isEmpty()) { return qApp->enqueueError("Erro buscando 'Venda'!", this); }

  QList<int> lista;

  for (const auto &item : list) { lista << modelProdutos.data(item.row(), "idVendaProduto").toInt(); }

  auto *nfe = new CadastrarNFe(idVenda, lista, CadastrarNFe::Tipo::Futura, this);
  nfe->setAttribute(Qt::WA_DeleteOnClose);

  nfe->show();
}

void WidgetLogisticaAgendarEntrega::on_pushButtonImportarNFe_clicked() {
  const auto selection = ui->tableProdutos->selectionModel()->selectedRows();

  if (selection.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma linha!", this); }

  for (const auto &index : selection) {
    if (modelProdutos.data(index.row(), "idNFeSaida").toInt() > 0) { return qApp->enqueueError("Pelo menos uma linha selecionada já possui nota!", this); }
  }

  const QString filePath = QFileDialog::getOpenFileName(this, "Arquivo XML", QDir::currentPath(), "XML (*.xml)");

  if (filePath.isEmpty()) { return; }

  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueError("Erro lendo arquivo: " + file.errorString(), this); }

  XML xml(file.readAll(), file.fileName());

  if (not verificaCNPJ(xml) or verificaExiste(xml) or not verificaValido(xml)) { return; }

  QSqlQuery queryNFe;
  queryNFe.prepare("INSERT INTO nfe (numeroNFe, tipo, xml, status, cnpjOrig, chaveAcesso, valor) VALUES (:numeroNFe, 'SAÍDA', :xml, 'AUTORIZADO', :cnpjOrig, :chaveAcesso, :valor)");
  queryNFe.bindValue(":numeroNFe", xml.nNF);
  queryNFe.bindValue(":xml", xml.fileContent);
  queryNFe.bindValue(":cnpjOrig", xml.cnpjDest);
  queryNFe.bindValue(":chaveAcesso", xml.chaveAcesso);
  queryNFe.bindValue(":valor", xml.vNF_Total);

  if (not queryNFe.exec()) { return qApp->enqueueError("Erro importando NFe: " + queryNFe.lastError().text(), this); }

  const QVariant id = queryNFe.lastInsertId();

  // update other tables

  QSqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto SET idNFeSaida = :idNFeSaida WHERE idVendaProduto = :idVendaProduto");

  for (const auto &index : selection) {
    const int row = index.row();
    queryVenda.bindValue(":idNFeSaida", id);
    queryVenda.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

    if (not queryVenda.exec()) { return qApp->enqueueError("Erro salvando NFe nos produtos: " + queryVenda.lastError().text(), this); }
  }

  updateTables();
  qApp->enqueueInformation("Nota importada com sucesso!", this);
}

bool WidgetLogisticaAgendarEntrega::verificaCNPJ(const XML &xml) {
  QSqlQuery queryLoja;

  const QString cnpj = QString(xml.cnpjOrig).insert(2, ".").insert(6, ".").insert(10, "/").insert(15, "-");

  if (not queryLoja.exec("SELECT cnpj FROM loja WHERE cnpj = '" + cnpj + "'")) { return qApp->enqueueError(false, "Erro na query CNPJ: " + queryLoja.lastError().text(), this); }

  if (not queryLoja.first()) { return qApp->enqueueError(false, "CNPJ da NFe difere dos CNPJs da loja!", this); }

  return true;
}

bool WidgetLogisticaAgendarEntrega::verificaExiste(const XML &xml) {
  QSqlQuery query;
  query.prepare("SELECT idNFe FROM nfe WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", xml.chaveAcesso);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se nota já cadastrada: " + query.lastError().text(), this); }

  if (query.first()) { return qApp->enqueueError(true, "Nota já cadastrada!", this); }

  return false;
}

bool WidgetLogisticaAgendarEntrega::verificaValido(const XML &xml) {
  if (not xml.fileContent.contains("nProt")) { return qApp->enqueueError(false, "NFe não está autorizada pela SEFAZ!", this); }

  return true;
}

// TODO: 1'em entrega' deve entrar na categoria 100% estoque?
// TODO: 5adicionar botao para cancelar agendamento (verificar com Anderson)
// TODO: 5refazer filtros do estoque (casos 'devolvido', 'cancelado', 'em entrega')
