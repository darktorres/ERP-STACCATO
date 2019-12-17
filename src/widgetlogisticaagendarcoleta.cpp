#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardItemModel>
#include <QUrl>

#include "acbr.h"
#include "application.h"
#include "doubledelegate.h"
#include "estoqueprazoproxymodel.h"
#include "inputdialog.h"
#include "ui_widgetlogisticaagendarcoleta.h"
#include "usersession.h"
#include "venda.h"
#include "widgetlogisticaagendarcoleta.h"

WidgetLogisticaAgendarColeta::WidgetLogisticaAgendarColeta(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaAgendarColeta) { ui->setupUi(this); }

WidgetLogisticaAgendarColeta::~WidgetLogisticaAgendarColeta() { delete ui; }

void WidgetLogisticaAgendarColeta::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxEstoque, &QCheckBox::toggled, this, &WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled, connectionType);
  connect(ui->dateTimeEdit, &QDateTimeEdit::dateChanged, this, &WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged, connectionType);
  connect(ui->itemBoxVeiculo, &ItemBox::textChanged, this, &WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged, connectionType);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked, connectionType);
  connect(ui->pushButtonAgendarColeta, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked, connectionType);
  connect(ui->pushButtonCancelarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked, connectionType);
  connect(ui->pushButtonDanfe, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked, connectionType);
  connect(ui->pushButtonMontarCarga, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked, connectionType);
  connect(ui->pushButtonRemoverProduto, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked, connectionType);
  connect(ui->pushButtonVenda, &QPushButton::clicked, this, &WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked, connectionType);
}

void WidgetLogisticaAgendarColeta::setupTables() {
  modelEstoque.setTable("view_agendar_coleta");

  modelEstoque.setSort("prazoEntrega");

  modelEstoque.setHeaderData("prazoEntrega", "Prazo Limite");
  modelEstoque.setHeaderData("dataRealFat", "Data Faturado");
  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");
  modelEstoque.setHeaderData("numeroNFe", "NFe");
  modelEstoque.setHeaderData("idVenda", "Venda");
  modelEstoque.setHeaderData("ordemCompra", "OC");
  modelEstoque.setHeaderData("produto", "Produto");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Cx.");
  modelEstoque.setHeaderData("kgcx", "Kg./Cx.");

  modelEstoque.proxyModel = new EstoquePrazoProxyModel(&modelEstoque, this);

  ui->tableEstoque->setModel(&modelEstoque);

  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));

  ui->tableEstoque->hideColumn("fornecedor");
  ui->tableEstoque->hideColumn("unCaixa");
  ui->tableEstoque->hideColumn("formComercial");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("idNFe");

  connect(ui->tableEstoque->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetLogisticaAgendarColeta::calcularPeso);

  // -------------------------------------------------------------------------

  modelTranspAtual.setTable("veiculo_has_produto");

  modelTranspAtual.setHeaderData("idEstoque", "Estoque");
  modelTranspAtual.setHeaderData("status", "Status");
  modelTranspAtual.setHeaderData("produto", "Produto");
  modelTranspAtual.setHeaderData("caixas", "Cx.");
  modelTranspAtual.setHeaderData("kg", "Kg");
  modelTranspAtual.setHeaderData("quant", "Quant.");
  modelTranspAtual.setHeaderData("un", "Un.");
  modelTranspAtual.setHeaderData("codComercial", "Cód. Com.");

  ui->tableTranspAtual->setModel(&modelTranspAtual);

  ui->tableTranspAtual->hideColumn("id");
  ui->tableTranspAtual->hideColumn("idEvento");
  ui->tableTranspAtual->hideColumn("idVeiculo");
  ui->tableTranspAtual->hideColumn("idVendaProduto1");
  ui->tableTranspAtual->hideColumn("idVendaProduto2");
  ui->tableTranspAtual->hideColumn("idCompra");
  ui->tableTranspAtual->hideColumn("idNFeSaida");
  ui->tableTranspAtual->hideColumn("fornecedor");
  ui->tableTranspAtual->hideColumn("idVenda");
  ui->tableTranspAtual->hideColumn("idLoja");
  ui->tableTranspAtual->hideColumn("idProduto");
  ui->tableTranspAtual->hideColumn("obs");
  ui->tableTranspAtual->hideColumn("unCaixa");
  ui->tableTranspAtual->hideColumn("formComercial");
  ui->tableTranspAtual->hideColumn("data");

  // -------------------------------------------------------------------------

  modelTranspAgend.setTable("veiculo_has_produto");

  modelTranspAgend.setHeaderData("data", "Agendado");
  modelTranspAgend.setHeaderData("idEstoque", "Estoque");
  modelTranspAgend.setHeaderData("status", "Status");
  modelTranspAgend.setHeaderData("produto", "Produto");
  modelTranspAgend.setHeaderData("caixas", "Cx.");
  modelTranspAgend.setHeaderData("kg", "Kg");
  modelTranspAgend.setHeaderData("quant", "Quant.");
  modelTranspAgend.setHeaderData("un", "Un.");
  modelTranspAgend.setHeaderData("codComercial", "Cód. Com.");

  ui->tableTranspAgend->setModel(&modelTranspAgend);

  ui->tableTranspAgend->hideColumn("id");
  ui->tableTranspAgend->hideColumn("idEvento");
  ui->tableTranspAgend->hideColumn("idVeiculo");
  ui->tableTranspAgend->hideColumn("idVendaProduto1");
  ui->tableTranspAgend->hideColumn("idVendaProduto2");
  ui->tableTranspAgend->hideColumn("idCompra");
  ui->tableTranspAgend->hideColumn("idNFeSaida");
  ui->tableTranspAgend->hideColumn("fornecedor");
  ui->tableTranspAgend->hideColumn("idVenda");
  ui->tableTranspAgend->hideColumn("idLoja");
  ui->tableTranspAgend->hideColumn("idProduto");
  ui->tableTranspAgend->hideColumn("unCaixa");
  ui->tableTranspAgend->hideColumn("obs");
  ui->tableTranspAgend->hideColumn("formComercial");
}

void WidgetLogisticaAgendarColeta::calcularPeso() {
  double peso = 0;
  double caixas = 0;

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  for (const auto &index : list) {
    const double kg = modelEstoque.data(index.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(index.row(), "caixas").toDouble();
    peso += kg * caixa;
    caixas += caixa;
  }

  ui->doubleSpinBoxPeso->setValue(peso);
  ui->doubleSpinBoxPeso->setSuffix(" Kg (" + QString::number(caixas) + " Cx.)");
}

void WidgetLogisticaAgendarColeta::updateTables() {
  if (not isSet) {
    ui->frameCaminhao->hide();
    ui->pushButtonCancelarCarga->hide();
    ui->dateTimeEdit->setDate(qApp->serverDate());
    ui->itemBoxVeiculo->setSearchDialog(SearchDialog::veiculo(this));
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelEstoque.select()) { return; }

  if (not modelTranspAgend.select()) { return; }
}

void WidgetLogisticaAgendarColeta::tableFornLogistica_clicked(const QString &fornecedor) {
  this->fornecedor = fornecedor;

  ui->lineEditBusca->clear();

  montaFiltro();
}

void WidgetLogisticaAgendarColeta::resetTables() { modelIsSet = false; }

void WidgetLogisticaAgendarColeta::on_pushButtonMontarCarga_clicked() {
  if (not ui->frameCaminhao->isVisible()) {
    ui->frameCaminhao->show();
    ui->pushButtonAgendarColeta->hide();
    ui->pushButtonCancelarCarga->show();
    return;
  }

  if (modelTranspAtual.rowCount() == 0) { return qApp->enqueueError("Nenhum item no veículo!", this); }

  QModelIndexList list;

  for (int row = 0; row < modelTranspAtual.rowCount(); ++row) { list << modelTranspAtual.index(row, 0); }

  if (not qApp->startTransaction()) { return; }

  if (not processRows(list, ui->dateTimeEdit->date(), true)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Agendado com sucesso!", this);

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();
}

void WidgetLogisticaAgendarColeta::on_pushButtonAgendarColeta_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarColeta, this);
  // TODO: 5colocar qual a linha/id esta sendo trabalhada para o usuario nao se perder ao trocar de janela e voltar

  if (input.exec() != InputDialog::Accepted) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not processRows(list, input.getNextDate())) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Agendado com sucesso!", this);
}

bool WidgetLogisticaAgendarColeta::processRows(const QModelIndexList &list, const QDate &dataPrevColeta, const bool montarCarga) {
  QSqlQuery queryTemp;
  queryTemp.prepare("SELECT codComercial FROM estoque WHERE idEstoque = :idEstoque");

  QSqlQuery query2;
  query2.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE status = 'EM COLETA' AND idPedido2 IN (SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = :idEstoque)");

  QSqlQuery query3;
  query3.prepare("UPDATE venda_has_produto2 SET dataPrevColeta = :dataPrevColeta WHERE status = 'EM COLETA' AND idVendaProduto2 IN (SELECT idVendaProduto2 FROM estoque_has_consumo WHERE idEstoque = "
                 ":idEstoque)");

  for (const auto &index : list) {
    int idEstoque;
    QString codComercial;

    if (montarCarga) {
      QSqlQuery query;
      query.exec("SELECT COALESCE(MAX(idEvento), 0) + 1 FROM veiculo_has_produto");
      query.first();

      const int idEvento = query.value(0).toInt();

      if (not modelTranspAtual.setData(index.row(), "data", dataPrevColeta)) { return false; }
      if (not modelTranspAtual.setData(index.row(), "idEvento", idEvento)) { return false; }

      idEstoque = modelTranspAtual.data(index.row(), "idEstoque").toInt();

      queryTemp.bindValue(":idEstoque", idEstoque);

      if (not queryTemp.exec() or not queryTemp.first()) { return qApp->enqueueError(false, "Erro buscando codComercial: " + queryTemp.lastError().text(), this); }

      codComercial = queryTemp.value("codComercial").toString();
    } else {
      idEstoque = modelEstoque.data(index.row(), "idEstoque").toInt();
      codComercial = modelEstoque.data(index.row(), "codComercial").toString();
    }

    query2.bindValue(":dataPrevColeta", dataPrevColeta);
    query2.bindValue(":idEstoque", idEstoque);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro salvando status no pedido_fornecedor: " + query2.lastError().text(), this); }

    query3.bindValue(":dataPrevColeta", dataPrevColeta);
    query3.bindValue(":idEstoque", idEstoque);

    if (not query3.exec()) { return qApp->enqueueError(false, "Erro salvando status na venda_produto: " + query3.lastError().text(), this); }
  }

  return modelTranspAtual.submitAll();
}

void WidgetLogisticaAgendarColeta::on_itemBoxVeiculo_textChanged(const QString &) {
  QSqlQuery query;
  query.prepare("SELECT capacidade FROM transportadora_has_veiculo WHERE idVeiculo = :idVeiculo");
  query.bindValue(":idVeiculo", ui->itemBoxVeiculo->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados veiculo: " + query.lastError().text(), this); }

  if (not modelTranspAtual.select()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + ui->dateTimeEdit->date().toString("yyyy-MM-dd") + "'");

  if (not modelTranspAgend.select()) { return; }

  ui->doubleSpinBoxCapacidade->setValue(query.value("capacidade").toDouble());
}

bool WidgetLogisticaAgendarColeta::adicionarProduto(const QModelIndexList &list) {
  for (const auto &index : list) {
    const double kg = modelEstoque.data(index.row(), "kgcx").toDouble();
    const double caixa = modelEstoque.data(index.row(), "caixas").toDouble();
    const double peso = kg * caixa;

    const int row = modelTranspAtual.insertRowAtEnd();

    if (not modelTranspAtual.setData(row, "fornecedor", modelEstoque.data(index.row(), "fornecedor"))) { return false; }
    if (not modelTranspAtual.setData(row, "unCaixa", modelEstoque.data(index.row(), "unCaixa"))) { return false; }
    if (not modelTranspAtual.setData(row, "formComercial", modelEstoque.data(index.row(), "formComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "idVeiculo", ui->itemBoxVeiculo->getId())) { return false; }
    if (not modelTranspAtual.setData(row, "idEstoque", modelEstoque.data(index.row(), "idEstoque"))) { return false; }
    if (not modelTranspAtual.setData(row, "idProduto", modelEstoque.data(index.row(), "idProduto"))) { return false; }
    if (not modelTranspAtual.setData(row, "produto", modelEstoque.data(index.row(), "produto"))) { return false; }
    if (not modelTranspAtual.setData(row, "codComercial", modelEstoque.data(index.row(), "codComercial"))) { return false; }
    if (not modelTranspAtual.setData(row, "un", modelEstoque.data(index.row(), "un"))) { return false; }
    if (not modelTranspAtual.setData(row, "caixas", modelEstoque.data(index.row(), "caixas"))) { return false; }
    if (not modelTranspAtual.setData(row, "kg", peso)) { return false; }
    if (not modelTranspAtual.setData(row, "quant", modelEstoque.data(index.row(), "quant"))) { return false; }
    if (not modelTranspAtual.setData(row, "status", "EM COLETA")) { return false; }
  }

  return true;
}

void WidgetLogisticaAgendarColeta::on_pushButtonAdicionarProduto_clicked() {
  if (ui->itemBoxVeiculo->getId().isNull()) { return qApp->enqueueError("Deve escolher uma transportadora antes!", this); }

  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  if (ui->doubleSpinBoxPeso->value() > ui->doubleSpinBoxCapacidade->value()) { qApp->enqueueWarning("Peso maior que capacidade do veículo!", this); }

  for (const auto &index : list) {
    const auto listMatch = modelTranspAtual.match("idEstoque", modelEstoque.data(index.row(), "idEstoque"), 1, Qt::MatchExactly);

    if (not listMatch.isEmpty()) { return qApp->enqueueError("Item '" + modelEstoque.data(index.row(), "produto").toString() + "' já inserido!", this); }
  }

  if (not adicionarProduto(list) and not modelTranspAtual.select()) { return; }
}

void WidgetLogisticaAgendarColeta::on_pushButtonRemoverProduto_clicked() {
  const auto list = ui->tableTranspAtual->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  for (const auto &index : list) { modelTranspAtual.removeRow(index.row()); }
}

void WidgetLogisticaAgendarColeta::on_pushButtonCancelarCarga_clicked() {
  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  ui->frameCaminhao->hide();
  ui->pushButtonAgendarColeta->show();
  ui->pushButtonCancelarCarga->hide();

  if (not modelTranspAtual.select()) { return; }
}

void WidgetLogisticaAgendarColeta::on_pushButtonDanfe_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  if (ACBr acbrLocal; not acbrLocal.gerarDanfe(modelEstoque.data(list.first().row(), "idNFe").toInt())) { return; }
}

void WidgetLogisticaAgendarColeta::on_lineEditBusca_textChanged(const QString &) { montaFiltro(); }

void WidgetLogisticaAgendarColeta::on_dateTimeEdit_dateChanged(const QDate &date) {
  if (ui->itemBoxVeiculo->text().isEmpty()) { return; }

  modelTranspAgend.setFilter("idVeiculo = " + ui->itemBoxVeiculo->getId().toString() + " AND status != 'FINALIZADO' AND DATE(data) = '" + date.toString("yyyy-MM-dd") + "'");
}

void WidgetLogisticaAgendarColeta::on_pushButtonVenda_clicked() {
  const auto list = ui->tableEstoque->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  for (const auto &index : list) {
    const QString idVenda = modelEstoque.data(index.row(), "idVenda").toString();
    const QStringList ids = idVenda.split(", ");

    if (ids.isEmpty()) { return; }

    for (const auto &id : ids) {
      auto *venda = new Venda(this);
      venda->setAttribute(Qt::WA_DeleteOnClose);
      venda->viewRegisterById(id);
    }
  }
}

void WidgetLogisticaAgendarColeta::on_checkBoxEstoque_toggled() { montaFiltro(); }

void WidgetLogisticaAgendarColeta::montaFiltro() {
  QStringList filtros;

  const QString filtroFornecedor = "fornecedor = '" + fornecedor + "'";

  if (not fornecedor.isEmpty()) { filtros << filtroFornecedor; }

  //-------------------------------------

  const QString filtroEstoque = "idVenda " + QString(ui->checkBoxEstoque->isChecked() ? "IS NULL" : "IS NOT NULL");

  if (not filtroEstoque.isEmpty()) { filtros << filtroEstoque; }

  //-------------------------------------

  const QString text = ui->lineEditBusca->text();
  // TODO: OC não está aparecendo nessa tabela
  const QString filtroBusca = "(numeroNFe LIKE '%" + text + "%' OR produto LIKE '%" + text + "%' OR idVenda LIKE '%" + text + "%' OR ordemCompra LIKE '%" + text + "%')";

  if (not text.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  modelEstoque.setFilter(filtros.join(" AND "));
}

// TODO: 5importar nota de amostra nesta tela dizendo para qual loja ela vai e no final do fluxo gerar nota de
// tranferencia
