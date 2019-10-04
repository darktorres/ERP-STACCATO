#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "excel.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "sendmail.h"
#include "ui_widgetcompragerar.h"
#include "usersession.h"
#include "widgetcompragerar.h"
#include "xlsxdocument.h"

WidgetCompraGerar::WidgetCompraGerar(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraGerar) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetCompraGerar::~WidgetCompraGerar() { delete ui; }

void WidgetCompraGerar::calcularPreco() {
  double preco = 0;

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  for (const auto &item : list) { preco += modelProdutos.data(item.row(), "preco").toDouble(); }

  ui->doubleSpinBox->setValue(preco);
}

void WidgetCompraGerar::setupTables() {
  modelResumo.setTable("view_fornecedor_compra_gerar");

  modelResumo.setFilter("");

  ui->tableResumo->setModel(&modelResumo);

  //---------------------------------------------------------------------------------------

  modelProdutos.setTable("pedido_fornecedor_has_produto");

  modelProdutos.setHeaderData("idVenda", "Código");
  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("descricao", "Descrição");
  modelProdutos.setHeaderData("colecao", "Coleção");
  modelProdutos.setHeaderData("caixas", "Caixas");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("prcUnitario", "Custo Unit.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("un2", "Un.2");
  modelProdutos.setHeaderData("preco", "Custo Total");
  modelProdutos.setHeaderData("kgcx", "Kg./Cx.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("codBarras", "Cód. Bar.");
  modelProdutos.setHeaderData("dataPrevCompra", "Prev. Compra");
  modelProdutos.setHeaderData("dataCompra", "Data Compra");
  modelProdutos.setHeaderData("obs", "Obs.");
  modelProdutos.setHeaderData("status", "Status");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  ui->tableProdutos->hideColumn("ordemRepresentacao");
  ui->tableProdutos->hideColumn("idVendaProduto1");
  ui->tableProdutos->hideColumn("statusFinanceiro");
  ui->tableProdutos->hideColumn("selecionado");
  ui->tableProdutos->hideColumn("ordemCompra");
  ui->tableProdutos->hideColumn("idCompra");
  ui->tableProdutos->hideColumn("quantConsumida");
  ui->tableProdutos->hideColumn("quantUpd");
  ui->tableProdutos->hideColumn("idPedido1");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("dataRealCompra");
  ui->tableProdutos->hideColumn("dataPrevConf");
  ui->tableProdutos->hideColumn("dataRealConf");
  ui->tableProdutos->hideColumn("dataPrevFat");
  ui->tableProdutos->hideColumn("dataRealFat");
  ui->tableProdutos->hideColumn("dataPrevColeta");
  ui->tableProdutos->hideColumn("dataRealColeta");
  ui->tableProdutos->hideColumn("dataPrevEnt");
  ui->tableProdutos->hideColumn("dataRealEnt");
  ui->tableProdutos->hideColumn("dataPrevReceb");
  ui->tableProdutos->hideColumn("dataRealReceb");
  ui->tableProdutos->hideColumn("aliquotaSt");
  ui->tableProdutos->hideColumn("st");

  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetCompraGerar::calcularPreco);
}

void WidgetCompraGerar::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetCompraGerar::on_checkBoxMarcarTodos_clicked, connectionType);
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonCancelarCompra_clicked, connectionType);
  connect(ui->pushButtonGerarCompra, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonGerarCompra_clicked, connectionType);
  connect(ui->tableResumo, &TableView::clicked, this, &WidgetCompraGerar::on_tableResumo_clicked, connectionType);
}

void WidgetCompraGerar::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  const auto selection = ui->tableResumo->selectionModel()->selectedRows();

  if (not modelResumo.select()) { return; }

  if (not selection.isEmpty()) { ui->tableResumo->selectRow(selection.first().row()); }

  if (not modelProdutos.select()) { return; }
}

void WidgetCompraGerar::resetTables() { modelIsSet = false; }

bool WidgetCompraGerar::gerarCompra(const QList<QModelIndex> &list, const QDate &dataCompra, const QDate &dataPrevista, const int ordemCompra) {
  QSqlQuery queryId;

  if (not queryId.exec("SELECT COALESCE(MAX(idCompra), 0) + 1 AS idCompra FROM pedido_fornecedor_has_produto") or not queryId.first()) {
    return qApp->enqueueError(false, "Erro buscando idCompra: " + queryId.lastError().text(), this);
  }

  const int idCompra = queryId.value("idCompra").toInt();

  QSqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM COMPRA', idCompra = :idCompra, dataRealCompra = :dataRealCompra, dataPrevConf = :dataPrevConf WHERE status = 'INICIADO' AND "
                     "idVendaProduto2 = :idVendaProduto2");

  for (const auto &index : list) {
    const auto row = index.row();

    if (not modelProdutos.setData(row, "status", "EM COMPRA")) { return false; }
    if (not modelProdutos.setData(row, "idCompra", idCompra)) { return false; }
    if (not modelProdutos.setData(row, "ordemCompra", oc)) { return false; }
    if (not modelProdutos.setData(row, "dataRealCompra", dataCompra)) { return false; }
    if (not modelProdutos.setData(row, "dataPrevConf", dataPrevista)) { return false; }

    // salvar status na venda

    const int idVendaProduto2 = modelProdutos.data(row, "idVendaProduto2").toInt();

    if (idVendaProduto2 != 0) {
      queryVenda.bindValue(":idCompra", idCompra);
      queryVenda.bindValue(":dataRealCompra", dataCompra);
      queryVenda.bindValue(":dataPrevConf", dataPrevista);
      queryVenda.bindValue(":idVendaProduto2", idVendaProduto2);

      if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro atualizando status da venda: " + queryVenda.lastError().text(), this); }
    }
  }

  if (not modelProdutos.submitAll()) { return false; }

  return true;
}

void WidgetCompraGerar::on_pushButtonGerarCompra_clicked() {
  const auto folderKey = UserSession::getSetting("User/ComprasFolder");

  if (not folderKey) { return qApp->enqueueError("Por favor selecione uma pasta para salvar os arquivos nas configurações do usuário!", this); }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  //

  const auto isRepresentacao = verificaRepresentacao(list);

  if (not isRepresentacao) { return; }

  // oc

  const auto ordemCompra = getOrdemCompra();

  if (not ordemCompra) { return; }

  const int oc = ordemCompra.value();

  const auto dates = getDates(list);

  if (not dates) { return; }

  const auto [dataCompra, dataPrevista] = dates.value();

  // email --------------------------------

  // reload data after getDates
  if (not modelProdutos.select()) { return; }

  const QString razaoSocial = modelProdutos.data(list.first().row(), "fornecedor").toString();

  const auto anexo = gerarExcel(list, oc, isRepresentacao.value());

  if (not anexo) { return; }

  // -------------------------------------------------------------------------

  if (not qApp->startTransaction()) { return; }

  if (not gerarCompra(list, dataCompra, dataPrevista, oc)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  enviarEmail(razaoSocial, anexo.value());

  // -------------------------------------------------------------------------

  updateTables();
  qApp->enqueueInformation("Compra gerada com sucesso!", this);

  emit finished();
}

void WidgetCompraGerar::enviarEmail(const QString &razaoSocial, const QString &anexo) {
  QMessageBox msgBox(QMessageBox::Question, "Enviar E-mail?", "Deseja enviar e-mail?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Enviar");
  msgBox.setButtonText(QMessageBox::No, "Pular");

  if (msgBox.exec() == QMessageBox::Yes) {
    auto *mail = new SendMail(SendMail::Tipo::GerarCompra, anexo, razaoSocial, this);
    mail->setAttribute(Qt::WA_DeleteOnClose);

    mail->exec();
  }
}

std::optional<std::tuple<QDate, QDate>> WidgetCompraGerar::getDates(const QList<QModelIndex> &list) {
  QStringList ids;

  for (const auto &index : list) { ids << modelProdutos.data(index.row(), "idPedido1").toString(); }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::GerarCompra, this);
  if (not inputDlg.setFilter(ids)) { return {}; }

  if (inputDlg.exec() != InputDialogProduto::Accepted) { return {}; }

  return std::make_tuple<>(inputDlg.getDate(), inputDlg.getNextDate());
}

std::optional<int> WidgetCompraGerar::getOrdemCompra() {
  QSqlQuery queryOC;

  if (not queryOC.exec("SELECT COALESCE(MAX(ordemCompra), 0) + 1 AS ordemCompra FROM pedido_fornecedor_has_produto") or not queryOC.first()) {
    qApp->enqueueError("Erro buscando próximo O.C.!", this);
    return {};
  }

  bool ok;
  int oc = QInputDialog::getInt(this, "OC", "Qual a OC?", queryOC.value("ordemCompra").toInt(), 0, 999999, 1, &ok);
  if (not ok) { return {}; }

  QSqlQuery query2;
  query2.prepare("SELECT ordemCompra FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra LIMIT 1");

  while (true) {
    query2.bindValue(":ordemCompra", oc);

    if (not query2.exec()) {
      qApp->enqueueError("Erro buscando O.C.!", this);
      return {};
    }

    if (not query2.first()) { break; }

    if (query2.first()) {
      QMessageBox msgBox(QMessageBox::Question, "Atenção!", "OC já existe! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
      msgBox.setButtonText(QMessageBox::Yes, "Continuar");
      msgBox.setButtonText(QMessageBox::No, "Voltar");

      const int choice = msgBox.exec();

      if (choice == QMessageBox::Yes) { break; }

      bool ok2;
      oc = QInputDialog::getInt(this, "OC", "Qual a OC?", query2.value("ordemCompra").toInt(), 0, 999999, 1, &ok2);
      if (not ok2) { return {}; }
    }
  }

  return oc;
}

std::optional<bool> WidgetCompraGerar::verificaRepresentacao(const QList<QModelIndex> &list) {
  const int row = list.first().row();
  const QString fornecedor = modelProdutos.data(row, "fornecedor").toString();

  QSqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  query.bindValue(":razaoSocial", fornecedor);

  if (not query.exec() or not query.first()) {
    qApp->enqueueError("Erro buscando dados do fornecedor: " + query.lastError().text(), this);
    return {};
  }

  const bool isRepresentacao = query.value("representacao").toBool();

  if (isRepresentacao) {
    if (modelProdutos.data(row, "idVenda").toString().isEmpty()) {
      // TODO: verificar quando a loja precisar comprar avulso
      qApp->enqueueError("'Venda' vazio!", this);
      return {};
    }

    QStringList idVendaList;

    for (const auto &index : list) { idVendaList << modelProdutos.data(index.row(), "idVenda").toString(); }

    idVendaList.removeDuplicates();

    if (idVendaList.size() > 1) {
      qApp->enqueueError("Não pode misturar produtos de vendas diferentes na representação!", this);
      return {};
    }
  }

  return isRepresentacao;
}

std::optional<QString> WidgetCompraGerar::gerarExcel(const QList<QModelIndex> &list, const int oc, const bool isRepresentacao) {
  const int firstRow = list.first().row();
  const QString fornecedor = modelProdutos.data(firstRow, "fornecedor").toString();

  if (isRepresentacao) {
    const QString idVenda = modelProdutos.data(firstRow, "idVenda").toString();
    Excel excel(idVenda, Excel::Tipo::Venda);
    const QString representacao = "OC " + QString::number(oc) + " " + idVenda + " " + fornecedor;

    if (not excel.gerarExcel(oc, true, representacao)) { return {}; }

    return excel.getFileName();
  }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  idVendas.removeDuplicates();

  const QString idVenda = (fornecedor == "QUARTZOBRAS") ? "" : idVendas.join(", ");

  const QString arquivoModelo = "modelo compras.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    qApp->enqueueError("Não encontrou o modelo do Excel!", this);
    return {};
  }

  const auto folderKey = UserSession::getSetting("User/ComprasFolder");

  if (not folderKey) { return {}; }

  const QString fileName = folderKey.value().toString() + "/" + QString::number(oc) + " " + idVenda + " " + fornecedor + ".xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) {
    qApp->enqueueError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), this);
    return {};
  }

  file.close();

  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    qApp->enqueueError("Erro buscando contato do fornecedor: " + queryFornecedor.lastError().text(), this);
    return {};
  }

  QXlsx::Document xlsx(arquivoModelo);

  //  xlsx.currentWorksheet()->setFitToPage(true);
  //  xlsx.currentWorksheet()->setFitToHeight(true);
  //  xlsx.currentWorksheet()->setOrientationVertical(false);

  xlsx.write("E4", oc);
  xlsx.write("E5", idVenda);
  xlsx.write("E6", fornecedor);
  xlsx.write("E7", queryFornecedor.first() ? queryFornecedor.value("contatoNome").toString() : "");
  xlsx.write("E8", QDateTime::currentDateTime().toString("dddd dd 'de' MMMM 'de' yyyy hh:mm"));

  double total = 0;
  int excelRow = 0;

  for (const auto &index : list) {
    const int currentRow = index.row();

    xlsx.write("A" + QString::number(13 + excelRow), QString::number(excelRow + 1));
    xlsx.write("B" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "codComercial"));
    QString formato = modelProdutos.data(currentRow, "formComercial").toString();
    QString produto = modelProdutos.data(currentRow, "descricao").toString() + (formato.isEmpty() ? "" : " - " + formato);
    xlsx.write("C" + QString::number(13 + excelRow), produto);
    xlsx.write("E" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "prcUnitario"));
    xlsx.write("F" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "un"));
    xlsx.write("G" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "quant"));
    xlsx.write("H" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "preco"));
    if (idVendas.size() > 1) { xlsx.write("I" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "idVenda")); }
    const QString st = modelProdutos.data(currentRow, "st").toString();

    if (st == "ST Fornecedor") {
      xlsx.write("I" + QString::number(13 + excelRow), modelProdutos.data(currentRow, "idVenda"));
      total += modelProdutos.data(currentRow, "preco").toDouble();
    }

    excelRow++;
  }

  const QString st = modelProdutos.data(firstRow, "st").toString();

  if (st == "ST Fornecedor") {
    xlsx.write("G200", "ST:");
    xlsx.write("H200", total * modelProdutos.data(firstRow, "aliquotaSt").toDouble() / 100);
  }

  for (int row = list.size() + 13; row < 200; ++row) { xlsx.setRowHidden(row, true); }

  if (not xlsx.saveAs(fileName)) {
    qApp->enqueueError("Ocorreu algum erro ao salvar o arquivo.", this);
    return {};
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como: " + fileName, this);

  return fileName;
}

void WidgetCompraGerar::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->tableProdutos->selectAll() : ui->tableProdutos->clearSelection(); }

void WidgetCompraGerar::on_tableResumo_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString fornecedor = modelResumo.data(index.row(), "fornecedor").toString();

  modelProdutos.setFilter("fornecedor = '" + fornecedor + "' AND status = 'PENDENTE'");
}

bool WidgetCompraGerar::cancelar(const QModelIndexList &list) {
  QSqlQuery query;
  query.prepare("UPDATE venda_has_produto2 SET status = CASE WHEN reposicaoEntrega THEN 'REPO. ENTREGA' WHEN reposicaoReceb THEN 'REPO. RECEB.' ELSE 'PENDENTE' END WHERE status = 'INICIADO' AND "
                "idVendaProduto2 = :idVendaProduto2");

  for (const auto &index : list) {
    if (not modelProdutos.setData(index.row(), "status", "CANCELADO")) { return false; }

    query.bindValue(":idVendaProduto2", modelProdutos.data(index.row(), "idVendaProduto2"));

    if (not query.exec()) { return qApp->enqueueError(false, "Erro voltando status do produto: " + query.lastError().text(), this); }
  }

  return modelProdutos.submitAll();
}

void WidgetCompraGerar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(list)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  updateTables();
  qApp->enqueueInformation("Itens cancelados!", this);
}

// TODO: 2vincular compras geradas com loja selecionada em configuracoes
// TODO: 5colocar tamanho minimo da tabela da esquerda para mostrar todas as colunas
// TODO: avulso
// TODO: no caso da quartzobras se for mais de um pedido deixar o campo 'PEDIDO DE VENDA NR.' vazio
// TODO: no caso da quartzobras ordenar por cod. produto em vez de por pedido

// TODO: ao confirmar email voltar tela para pendentes
