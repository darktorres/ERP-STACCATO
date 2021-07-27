#include "widgetlogisticaentregas.h"
#include "ui_widgetlogisticaentregas.h"

#include "acbrlib.h"
#include "application.h"
#include "cadastrarnfe.h"
#include "doubledelegate.h"
#include "file.h"
#include "followup.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sql.h"
#include "user.h"
#include "xlsxdocument.h"

#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QMessageBox>
#include <QSqlError>
#include <QUrl>

WidgetLogisticaEntregas::WidgetLogisticaEntregas(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaEntregas) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetLogisticaEntregas::~WidgetLogisticaEntregas() { delete ui; }

void WidgetLogisticaEntregas::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetLogisticaEntregas::on_lineEditBuscar_textChanged, connectionType);
  connect(ui->lineEditBuscar, &QLineEdit::textChanged, this, &WidgetLogisticaEntregas::delayFiltro, connectionType);
  connect(ui->pushButtonCancelarEntrega, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonCancelarEntrega_clicked, connectionType);
  connect(ui->pushButtonConfirmarEntrega, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonConfirmarEntrega_clicked, connectionType);
  connect(ui->pushButtonConsultarNFe, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonConsultarNFe_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonGerarNFe, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonGerarNFe_clicked, connectionType);
  connect(ui->pushButtonImprimirDanfe, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonImprimirDanfe_clicked, connectionType);
  connect(ui->pushButtonProtocoloEntrega, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonProtocoloEntrega_clicked, connectionType);
  connect(ui->pushButtonReagendar, &QPushButton::clicked, this, &WidgetLogisticaEntregas::on_pushButtonReagendar_clicked, connectionType);
  connect(ui->tableCalendario, &TableView::clicked, this, &WidgetLogisticaEntregas::on_tableCalendario_clicked, connectionType);
  connect(ui->tableCarga, &TableView::clicked, this, &WidgetLogisticaEntregas::on_tableCarga_clicked, connectionType);
}

void WidgetLogisticaEntregas::updateTables() {
  if (not isSet) {
    timer.setSingleShot(true);
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  // -----------------------------------------------------------------

  modelCalendario.select();

  // -----------------------------------------------------------------

  modelCarga.select();

  // -----------------------------------------------------------------

  if (modelCarga.rowCount() == 0) { modelProdutos.setFilter("0"); }

  modelProdutos.select();
}

void WidgetLogisticaEntregas::resetTables() { modelIsSet = false; }

void WidgetLogisticaEntregas::setupTables() {
  modelCalendario.setTable("view_calendario_entrega");

  modelCalendario.setFilter("");

  modelCalendario.setHeaderData("data", "Agendado");
  modelCalendario.setHeaderData("modelo", "Modelo");
  modelCalendario.setHeaderData("razaoSocial", "Transp.");
  modelCalendario.setHeaderData("kg", "Kg.");
  modelCalendario.setHeaderData("idVenda", "Venda");

  ui->tableCalendario->setModel(&modelCalendario);

  ui->tableCalendario->hideColumn("idVeiculo");

  ui->tableCalendario->setItemDelegateForColumn("kg", new DoubleDelegate(this));

  // -----------------------------------------------------------------

  modelCarga.setTable("view_calendario_carga");

  modelCarga.setHeaderData("dataPrevEnt", "Agendado");
  modelCarga.setHeaderData("numeroNFe", "NFe");
  modelCarga.setHeaderData("idVenda", "Venda");
  modelCarga.setHeaderData("status", "Status");
  modelCarga.setHeaderData("produtos", "Produtos");
  modelCarga.setHeaderData("kg", "Kg.");

  ui->tableCarga->setModel(&modelCarga);

  ui->tableCarga->hideColumn("idEvento");
  ui->tableCarga->hideColumn("idNFe");
  ui->tableCarga->hideColumn("idVeiculo");

  ui->tableCarga->setItemDelegateForColumn("kg", new DoubleDelegate(this));

  // -----------------------------------------------------------------

  modelProdutos.setTable("view_calendario_produto");

  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("produto", "Produto");
  modelProdutos.setHeaderData("codComercial", "Código");
  modelProdutos.setHeaderData("formComercial", "Formato");
  modelProdutos.setHeaderData("caixas", "Cx.");
  modelProdutos.setHeaderData("kg", "Kg.");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("idEstoque", "Estoque");
  modelProdutos.setHeaderData("lote", "Lote");
  modelProdutos.setHeaderData("local", "Local");
  modelProdutos.setHeaderData("bloco", "Bloco");
  modelProdutos.setHeaderData("isEstoque", "Estoque?");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->hideColumn("idEvento");
  ui->tableProdutos->hideColumn("idVendaProduto2");

  ui->tableProdutos->setItemDelegateForColumn("kg", new DoubleDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(this));
}

void WidgetLogisticaEntregas::on_pushButtonReagendar_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  InputDialog input(InputDialog::Tipo::AgendarEntrega, this);

  if (input.exec() != InputDialog::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaEntregas::on_pushButtonReagendar");

  reagendar(list, input.getNextDate());

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Reagendado com sucesso!", this);
}

void WidgetLogisticaEntregas::reagendar(const QModelIndexList &list, const QDate &dataPrevEnt) {
  SqlQuery query1;
  query1.prepare("UPDATE venda_has_produto2 SET dataPrevEnt = :dataPrevEnt WHERE `idVendaProduto2` = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");

  SqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto2 SET dataPrevEnt = :dataPrevEnt WHERE `idVendaProduto2` = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");

  SqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET data = :data WHERE idEvento = :idEvento");

  for (const auto &index : list) {
    for (int row = 0; row < modelProdutos.rowCount(); ++row) {
      query1.bindValue(":dataPrevEnt", dataPrevEnt);
      query1.bindValue(":idVendaProduto2", modelProdutos.data(row, "idVendaProduto2"));

      if (not query1.exec()) { throw RuntimeException("Erro atualizando data venda: " + query1.lastError().text()); }

      query2.bindValue(":dataPrevEnt", dataPrevEnt);
      query2.bindValue(":idVendaProduto2", modelProdutos.data(row, "idVendaProduto2"));

      if (not query2.exec()) { throw RuntimeException("Erro atualizando data pedido_fornecedor: " + query2.lastError().text()); }
    }

    query3.bindValue(":data", dataPrevEnt);
    query3.bindValue(":idEvento", modelCarga.data(index.row(), "idEvento"));

    if (not query3.exec()) { throw RuntimeException("Erro atualizando data carga: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaEntregas::on_pushButtonGerarNFe_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const QString idVenda = modelCarga.data(list.first().row(), "idVenda").toString();

  QStringList lista;

  for (int row = 0; row < modelProdutos.rowCount(); ++row) { lista << modelProdutos.data(row, "idVendaProduto2").toString(); }

  lista.removeDuplicates();

  const CadastrarNFe::Tipo tipo = (modelCarga.data(list.first().row(), "NFe Futura").toInt() == 0) ? CadastrarNFe::Tipo::Saida : CadastrarNFe::Tipo::SaidaAposFutura;

  auto *nfe = new CadastrarNFe(idVenda, lista, tipo, this);
  nfe->setAttribute(Qt::WA_DeleteOnClose);

  nfe->show();
}

void WidgetLogisticaEntregas::on_tableCalendario_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString data = modelCalendario.data(index.row(), "data").toString();
  const QString veiculo = modelCalendario.data(index.row(), "idVeiculo").toString();

  modelCarga.setFilter("CAST(dataPrevEnt AS DATE) = '" + data + "' AND idVeiculo = " + veiculo);

  //--------------------------------------

  modelProdutos.setFilter("0");

  //--------------------------------------

  ui->pushButtonReagendar->setDisabled(true);
  ui->pushButtonConfirmarEntrega->setDisabled(true);
  ui->pushButtonGerarNFe->setDisabled(true);
  ui->pushButtonImprimirDanfe->setDisabled(true);
  ui->pushButtonCancelarEntrega->setDisabled(true);
}

void WidgetLogisticaEntregas::on_tableCarga_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString idVenda = modelCarga.data(index.row(), "idVenda").toString();
  const QString idEvento = modelCarga.data(index.row(), "idEvento").toString();

  modelProdutos.setFilter("idVenda = '" + idVenda + "' AND idEvento = " + idEvento);

  const QString status = modelCarga.data(index.row(), "status").toString();
  const QString nfeStatus = modelCarga.data(index.row(), "NFe Status").toString();

  ui->pushButtonReagendar->setEnabled(true);
  ui->pushButtonCancelarEntrega->setEnabled(true);

  if (status == "ENTREGA AGEND.") {
    ui->pushButtonGerarNFe->setEnabled(true);
    ui->pushButtonConfirmarEntrega->setEnabled(true);
    ui->pushButtonImprimirDanfe->setDisabled(true);
  }

  if (status == "EM ENTREGA") {
    ui->pushButtonGerarNFe->setDisabled(true);
    ui->pushButtonConfirmarEntrega->setEnabled(true);
    ui->pushButtonImprimirDanfe->setEnabled(true);
  }

  if (status == "NOTA PENDENTE") {
    ui->pushButtonGerarNFe->setDisabled(true);
    ui->pushButtonConfirmarEntrega->setDisabled(true);
    ui->pushButtonImprimirDanfe->setDisabled(true);
  }

  const bool isValid = nfeStatus == "NOTA PENDENTE" or nfeStatus == "AUTORIZADO";

  ui->pushButtonConsultarNFe->setEnabled(isValid);
}

void WidgetLogisticaEntregas::confirmarEntrega(const QDate &dataRealEnt, const QString &entregou, const QString &recebeu) {
  SqlQuery query1;
  query1.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGUE' WHERE status IN ('ENTREGA AGEND.', 'EM ENTREGA') AND idVendaProduto2 = :idVendaProduto2");

  SqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'ENTREGUE', dataRealEnt = :dataRealEnt WHERE status IN ('ENTREGA AGEND.', 'EM ENTREGA') AND idVendaProduto2 = :idVendaProduto2");

  SqlQuery query3;
  query3.prepare("UPDATE venda_has_produto2 SET status = 'ENTREGUE', entregou = :entregou, recebeu = :recebeu, dataRealEnt = :dataRealEnt WHERE status IN ('ENTREGA AGEND.', 'EM ENTREGA') AND "
                 "idVendaProduto2 = :idVendaProduto2");

  for (int row = 0; row < modelProdutos.rowCount(); ++row) {
    const int idVendaProduto2 = modelProdutos.data(row, "idVendaProduto2").toInt();

    query1.bindValue(":idVendaProduto2", idVendaProduto2);

    if (not query1.exec()) { throw RuntimeException("Erro salvando veiculo_has_produto: " + query1.lastError().text()); }

    query2.bindValue(":dataRealEnt", dataRealEnt);
    query2.bindValue(":idVendaProduto2", idVendaProduto2);

    if (not query2.exec()) { throw RuntimeException("Erro salvando pedido_fornecedor: " + query2.lastError().text()); }

    query3.bindValue(":entregou", entregou);
    query3.bindValue(":recebeu", recebeu);
    query3.bindValue(":dataRealEnt", dataRealEnt);
    query3.bindValue(":idVendaProduto2", idVendaProduto2);

    if (not query3.exec()) { throw RuntimeException("Erro salvando venda_produto: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaEntregas::delayFiltro() { timer.start(500); }

void WidgetLogisticaEntregas::on_pushButtonConfirmarEntrega_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  // TODO: see if this can be read after InputDialogConfirmacao and place inside confirmarEntrega
  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelCarga.data(index.row(), "idVenda").toString(); }

  const int row = list.first().row();

  InputDialogConfirmacao inputDlg(InputDialogConfirmacao::Tipo::Entrega, this);
  inputDlg.setFilterEntrega(modelCarga.data(row, "idVenda").toString(), modelCarga.data(row, "idEvento").toString());

  if (inputDlg.exec() != InputDialogConfirmacao::Accepted) { return; }

  qApp->startTransaction("WidgetLogisticaEntregas::on_pushButtonConfirmarEntrega");

  confirmarEntrega(inputDlg.getDate(), inputDlg.getEntregou(), inputDlg.getRecebeu());

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Entrega confirmada!", this);
}

void WidgetLogisticaEntregas::on_pushButtonImprimirDanfe_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  ACBrLib::gerarDanfe(modelCarga.data(list.first().row(), "idNFe").toInt());
}

void WidgetLogisticaEntregas::on_lineEditBuscar_textChanged() { montaFiltro(); }

void WidgetLogisticaEntregas::montaFiltro() {
  const QString text = ui->lineEditBuscar->text();

  modelCarga.setFilter(text.isEmpty() ? "0" : "idVenda LIKE '%" + text + "%'");

  //--------------------------------------

  modelProdutos.setFilter("0");
}

void WidgetLogisticaEntregas::on_pushButtonCancelarEntrega_clicked() { // TODO: cancelar entrega não desvincula nfe
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelCarga.data(index.row(), "idVenda").toString(); }

  qApp->startTransaction("WidgetLogisticaEntregas::on_pushButtonCancelarEntrega");

  cancelarEntrega(list);

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Operação realizada com sucesso!", this);
}

void WidgetLogisticaEntregas::cancelarEntrega(const QModelIndexList &list) {
  const int idEvento = modelCarga.data(list.first().row(), "idEvento").toInt();

  SqlQuery queryEvento;
  queryEvento.prepare(
      "SELECT vhp.idVendaProduto2, ehc.idConsumo FROM veiculo_has_produto vhp LEFT JOIN estoque_has_consumo ehc ON vhp.idVendaProduto2 = ehc.idVendaProduto2 WHERE idEvento = :idEvento");
  queryEvento.bindValue(":idEvento", idEvento);

  if (not queryEvento.exec() or queryEvento.size() == 0) { throw RuntimeException("Erro buscando produtos: " + queryEvento.lastError().text()); }

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = :status, dataPrevEnt = NULL WHERE `idVendaProduto2` = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");

  SqlQuery queryCompra;
  queryCompra.prepare(
      "UPDATE pedido_fornecedor_has_produto2 SET status = 'ESTOQUE', dataPrevEnt = NULL WHERE `idVendaProduto2` = :idVendaProduto2 AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')");

  while (queryEvento.next()) {
    const QString status = (queryEvento.value("idConsumo").toInt() == 0) ? "PENDENTE" : "ESTOQUE";

    queryVenda.bindValue(":status", status);
    queryVenda.bindValue(":idVendaProduto2", queryEvento.value("idVendaProduto2"));

    if (not queryVenda.exec()) { throw RuntimeException("Erro voltando status produto: " + queryVenda.lastError().text()); }

    //----------------------------------------
    // linhas que não possuem consumo não irão ter linha de compra

    queryCompra.bindValue(":idVendaProduto2", queryEvento.value("idVendaProduto2"));

    if (not queryCompra.exec()) { throw RuntimeException("Erro voltando status produto compra: " + queryCompra.lastError().text()); }
  }

  SqlQuery queryVeiculo;
  queryVeiculo.prepare("DELETE FROM veiculo_has_produto WHERE idEvento = :idEvento");
  queryVeiculo.bindValue(":idEvento", idEvento);

  if (not queryVeiculo.exec()) { throw RuntimeException("Erro deletando evento: " + queryVeiculo.lastError().text()); }
}

void WidgetLogisticaEntregas::on_pushButtonConsultarNFe_clicked() {
  const auto selection = ui->tableCarga->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const int idNFe = modelCarga.data(selection.first().row(), "idNFe").toInt();

  ACBr acbrRemoto;

  try {
    const auto [xml, resposta] = acbrRemoto.consultarNFe(idNFe);

    qApp->startTransaction("WidgetLogisticaEntregas::on_pushButtonConsultarNFe");

    processarConsultaNFe(idNFe, xml);

    qApp->endTransaction();

    const int xMotivoIndex = resposta.indexOf("XMotivo=", Qt::CaseInsensitive);

    if (xMotivoIndex == -1) { throw RuntimeException("Não encontrou o campo 'XMotivo': " + resposta); }

    const QString xMotivo = resposta.mid(xMotivoIndex + 8).split("\r\n").first();

    qApp->enqueueInformation(xMotivo, this);
  } catch (std::exception &) {
    updateTables();
    throw;
  }

  updateTables();
}

void WidgetLogisticaEntregas::processarConsultaNFe(const int idNFe, const QString &xml) {
  SqlQuery query;
  query.prepare("UPDATE nfe SET status = 'AUTORIZADO', xml = :xml WHERE idNFe = :idNFe");
  query.bindValue(":xml", xml);
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) { throw RuntimeException("Erro marcando nota como 'AUTORIZADO': " + query.lastError().text()); }

  SqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'EM ENTREGA' WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2");

  SqlQuery query2;
  query2.prepare("UPDATE venda_has_produto2 SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2");

  SqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE status = 'ENTREGA AGEND.' AND idVendaProduto2 = :idVendaProduto2");

  for (int row = 0; row < modelProdutos.rowCount(); ++row) {
    query1.bindValue(":idVendaProduto2", modelProdutos.data(row, "idVendaProduto2"));

    if (not query1.exec()) { throw RuntimeException("Erro atualizando status do pedido_fornecedor: " + query1.lastError().text()); }

    query2.bindValue(":idNFeSaida", idNFe);
    query2.bindValue(":idVendaProduto2", modelProdutos.data(row, "idVendaProduto2"));

    if (not query2.exec()) { throw RuntimeException("Erro salvando NFe nos produtos: " + query2.lastError().text()); }

    query3.bindValue(":idVendaProduto2", modelProdutos.data(row, "idVendaProduto2"));
    query3.bindValue(":idNFeSaida", idNFe);

    if (not query3.exec()) { throw RuntimeException("Erro atualizando carga veiculo: " + query3.lastError().text()); }
  }
}

void WidgetLogisticaEntregas::on_pushButtonProtocoloEntrega_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  // -------------------------------------------------------------------------

  const QString idVenda = modelCarga.data(list.first().row(), "idVenda").toString();
  const QString idEvento = modelCarga.data(list.first().row(), "idEvento").toString();

  SqlQueryModel modelProdutosAgrupado;

  // TODO: move query to Sql class
  modelProdutosAgrupado.setQuery("SELECT idEvento, idVenda, fornecedor, ANY_VALUE(produto) AS produto, codComercial, ANY_VALUE(lote) AS lote, SUM(caixas) AS caixas, SUM(kg) AS kg, SUM(quant) AS "
                                 "quant, ANY_VALUE(un) AS un, ANY_VALUE(isEstoque) AS isEstoque "
                                 "FROM view_calendario_produto WHERE idVenda = '" +
                                 idVenda + "' AND idEvento = '" + idEvento + "' GROUP BY fornecedor, codComercial");

  modelProdutosAgrupado.select();

  // -------------------------------------------------------------------------

  if (modelProdutosAgrupado.rowCount() > 60) { throw RuntimeException("Mais produtos do que cabe no modelo do Excel!", this); }

  // -------------------------------------------------------------------------

  const QString folderKey = User::getSetting("User/EntregasPdfFolder").toString();

  if (folderKey.isEmpty()) { throw RuntimeError("Não há uma pasta definida para salvar PDF. Por favor escolha uma nas configurações do ERP!", this); }

  // -------------------------------------------------------------------------

  SqlQuery queryCliente;
  queryCliente.prepare("SELECT v.idProfissional, c.nome_razao, c.tel AS clienteTel, c.telCel AS clienteCel, p.tel AS profissionalTel, p.telCel AS profissionalCel FROM venda v LEFT JOIN cliente c ON "
                       "v.idCliente = c.idCliente LEFT JOIN profissional p ON v.idProfissional = p.idProfissional WHERE v.idVenda = :idVenda");
  queryCliente.bindValue(":idVenda", idVenda);

  if (not queryCliente.exec() or not queryCliente.first()) { throw RuntimeException("Erro buscando dados cliente: " + queryCliente.lastError().text(), this); }

  const QString cliente = queryCliente.value("nome_razao").toString();

  QString telefones;

  if (queryCliente.value("idProfissional").toInt() != 1) { // NÃO HÁ
    const QString profissionalTel = queryCliente.value("profissionalTel").toString();
    const QString profissionalCel = queryCliente.value("profissionalCel").toString();

    if (not profissionalTel.isEmpty() and not profissionalCel.isEmpty()) {
      telefones = profissionalTel + " - " + profissionalCel;
    } else if (not profissionalTel.isEmpty()) {
      telefones = profissionalTel;
    } else if (not profissionalCel.isEmpty()) {
      telefones = profissionalCel;
    }
  }

  SqlQuery queryEndereco;
  queryEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = (SELECT idEnderecoEntrega FROM venda WHERE idVenda = :idVenda)");
  queryEndereco.bindValue(":idVenda", idVenda);

  if (not queryEndereco.exec()) { throw RuntimeException("Erro buscando endereço: " + queryEndereco.lastError().text(), this); }

  QString endereco;
  QString cep;

  if (queryEndereco.first()) {
    endereco = queryEndereco.value("logradouro").toString() + " " + queryEndereco.value("numero").toString() + " " + queryEndereco.value("complemento").toString() + " - " +
               queryEndereco.value("bairro").toString() + ", " + queryEndereco.value("cidade").toString();
    cep = queryEndereco.value("cep").toString();
  }

  // -------------------------------------------------------------------------

  const QString fileName = gerarProtocolo(folderKey, idEvento, idVenda, cliente, telefones, endereco, cep, modelProdutosAgrupado);
  const QString fileName2 = gerarChecklist(folderKey, idEvento, idVenda, cliente, endereco, cep, modelProdutosAgrupado);

  // -------------------------------------------------------------------------

  qApp->enqueueInformation("Protocolo/checklist salvo como:\n" + fileName + "\n" + fileName2, this);
}

QString WidgetLogisticaEntregas::gerarProtocolo(const QString &folderKey, const QString &idEvento, const QString &idVenda, const QString &cliente, const QString &telefones, const QString &endereco,
                                                const QString &cep, const SqlQueryModel &modelProdutosAgrupado) {
  const QString arquivoModelo = QDir::currentPath() + "/modelos/espelho_entrega.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do protocolo!", this); }

  const QString fileName = folderKey + "/" + idEvento + "_" + idVenda + ".xlsx";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), this); }

  file.close();

  QXlsx::Document xlsx(arquivoModelo, this);

  xlsx.currentWorksheet()->setFitToPage(true);
  xlsx.currentWorksheet()->setFitToHeight(true);
  xlsx.currentWorksheet()->setOrientation(QXlsx::Worksheet::Orientation::Vertical);

  xlsx.write("AA5", idVenda);
  xlsx.write("G11", cliente);
  xlsx.write("Y11", telefones);
  xlsx.write("J19", endereco);
  xlsx.write("I17", cep);

  const int itens = modelProdutosAgrupado.rowCount();

  for (int row = 27; row < itens * 2 + 35; ++row) { xlsx.setRowHidden(row, false); }

  for (int row = 27, index = 0; index < itens; row += 2, ++index) {
    const QString fornecedor = modelProdutosAgrupado.data(index, "fornecedor").toString();
    const QString produto = modelProdutosAgrupado.data(index, "produto").toString();
    const QString codComercial = modelProdutosAgrupado.data(index, "codComercial").toString();
    const QString lote = (fornecedor == "PORTINARI") ? modelProdutosAgrupado.data(index, "lote").toString() : "";
    const QString quant = modelProdutosAgrupado.data(index, "quant").toString();
    const QString un = modelProdutosAgrupado.data(index, "un").toString();
    const QString caixas = modelProdutosAgrupado.data(index, "caixas").toString();
    const QString isEstoque = modelProdutosAgrupado.data(index, "isEstoque").toString();

    xlsx.write("D" + QString::number(row), fornecedor);
    xlsx.write("I" + QString::number(row), produto + " - " + codComercial);
    xlsx.write("W" + QString::number(row), lote);
    xlsx.write("Y" + QString::number(row), quant + " " + un);
    xlsx.write("AC" + QString::number(row), isEstoque);
    xlsx.write("AD" + QString::number(row), caixas.toDouble());
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o protocolo!", this); }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));

  return fileName;
}

QString WidgetLogisticaEntregas::gerarChecklist(const QString &folderKey, const QString &idEvento, const QString &idVenda, const QString &cliente, const QString &endereco, const QString &cep,
                                                const SqlQueryModel &modelProdutosAgrupado) {
  const QString arquivoModelo = QDir::currentPath() + "/modelos/modelo_checklist.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do checklist!", this); }

  const QString fileName = folderKey + "/" + idEvento + "_" + idVenda + "_checklist.xlsx";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), this); }

  file.close();

  QXlsx::Document xlsx(arquivoModelo, this);

  xlsx.currentWorksheet()->setFitToPage(true);
  xlsx.currentWorksheet()->setFitToHeight(true);
  xlsx.currentWorksheet()->setOrientation(QXlsx::Worksheet::Orientation::Vertical);

  xlsx.write("AA5", idVenda);
  xlsx.write("G11", cliente);
  xlsx.write("J19", endereco);
  xlsx.write("I17", cep);

  const int itens = modelProdutosAgrupado.rowCount();

  for (int row = 28; row < itens * 2 + 35; ++row) { xlsx.setRowHidden(row, false); }

  for (int row = 28, index = 0; index < itens; row += 2, ++index) {
    const QString fornecedor = modelProdutosAgrupado.data(index, "fornecedor").toString();
    const QString produto = modelProdutosAgrupado.data(index, "produto").toString();
    const QString codComercial = modelProdutosAgrupado.data(index, "codComercial").toString();
    const QString lote = (fornecedor == "PORTINARI") ? modelProdutosAgrupado.data(index, "lote").toString() : "";
    const QString quant = modelProdutosAgrupado.data(index, "quant").toString();
    const QString un = modelProdutosAgrupado.data(index, "un").toString();
    const QString caixas = modelProdutosAgrupado.data(index, "caixas").toString();
    const QString isEstoque = modelProdutosAgrupado.data(index, "isEstoque").toString();

    xlsx.write("D" + QString::number(row), fornecedor);
    xlsx.write("I" + QString::number(row), produto + " - " + codComercial);
    xlsx.write("W" + QString::number(row), lote);
    xlsx.write("Y" + QString::number(row), quant + " " + un);
    xlsx.write("AC" + QString::number(row), isEstoque);
    xlsx.write("AD" + QString::number(row), caixas.toDouble());
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o checklist!", this); }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));

  return fileName;
}

void WidgetLogisticaEntregas::on_pushButtonFollowup_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString codigo = modelCarga.data(list.first().row(), "idVenda").toString();

  FollowUp *followup = new FollowUp(codigo, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

// TODO: 2quando cancelar/devolver um produto cancelar/devolver na logistica/veiculo_has_produto
// TODO: 0no filtro de 'parte estoque' nao considerar 'devolvido' e 'cancelado'
