#include "widgetcompragerar.h"
#include "ui_widgetcompragerar.h"

#include "application.h"
#include "excel.h"
#include "file.h"
#include "followup.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "sendmail.h"
#include "sql.h"
#include "user.h"
#include "xlsxdocument.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

WidgetCompraGerar::WidgetCompraGerar(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraGerar) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetCompraGerar::~WidgetCompraGerar() { delete ui; }

void WidgetCompraGerar::calcularPreco() {
  double preco = 0;

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  for (const auto &index : list) { preco += modelProdutos.data(index.row(), "preco").toDouble(); }

  ui->doubleSpinBox->setValue(preco);
}

void WidgetCompraGerar::setupTables() {
  modelResumo.setTable("view_fornecedor_compra_gerar");

  modelResumo.setFilter("");

  ui->tableResumo->setModel(&modelResumo);

  //---------------------------------------------------------------------------------------

  modelProdutos.setTable("pedido_fornecedor_has_produto");

  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("descricao", "Descrição");
  modelProdutos.setHeaderData("colecao", "Coleção");
  modelProdutos.setHeaderData("caixas", "Caixas");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("prcUnitario", "Custo Unit.");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("preco", "Custo Total");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("dataPrevCompra", "Prev. Compra");
  modelProdutos.setHeaderData("obs", "Obs.");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  ui->tableProdutos->hideColumn("status");
  ui->tableProdutos->hideColumn("idRelacionado");
  ui->tableProdutos->hideColumn("ordemRepresentacao");
  ui->tableProdutos->hideColumn("idVendaProduto1");
  ui->tableProdutos->hideColumn("idVendaProduto2");
  ui->tableProdutos->hideColumn("codFornecedor");
  ui->tableProdutos->hideColumn("statusFinanceiro");
  ui->tableProdutos->hideColumn("selecionado");
  ui->tableProdutos->hideColumn("ordemCompra");
  ui->tableProdutos->hideColumn("idCompra");
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
  ui->tableProdutos->hideColumn("un2");
  ui->tableProdutos->hideColumn("codBarras");
  ui->tableProdutos->hideColumn("kgcx");
  ui->tableProdutos->hideColumn("formComercial");

  connect(ui->tableProdutos->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetCompraGerar::calcularPreco);
}

void WidgetCompraGerar::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetCompraGerar::on_checkBoxMarcarTodos_clicked, connectionType);
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonCancelarCompra_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonGerarCompra, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonGerarCompra_clicked, connectionType);
  connect(ui->pushButtonLimparFiltro, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonLimparFiltro_clicked, connectionType);
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

  modelResumo.select();

  if (not selection.isEmpty()) { ui->tableResumo->selectRow(selection.first().row()); }

  modelProdutos.select();
}

void WidgetCompraGerar::resetTables() { modelIsSet = false; }

void WidgetCompraGerar::gerarCompra(const QModelIndexList &list, const QDate dataCompra, const QDate dataPrevista, const int ordemCompra) {
  SqlQuery queryId;

  if (not queryId.exec("SELECT COALESCE(MAX(idCompra), 0) + 1 AS idCompra FROM pedido_fornecedor_has_produto")) { throw RuntimeException("Erro buscando idCompra: " + queryId.lastError().text()); }

  if (not queryId.first()) { throw RuntimeException("Erro buscando idCompra!"); }

  const int idCompra = queryId.value("idCompra").toInt();

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = 'EM COMPRA', idCompra = :idCompra, dataRealCompra = :dataRealCompra, dataPrevConf = :dataPrevConf WHERE status = 'INICIADO' AND "
                     "idVendaProdutoFK = :idVendaProduto1");

  SqlQuery queryCompra1;
  queryCompra1.prepare("UPDATE pedido_fornecedor_has_produto set STATUS = 'EM COMPRA', idCompra = :idCompra, ordemCompra = :ordemCompra, dataRealCompra = :dataRealCompra, dataPrevConf = "
                       ":dataPrevConf WHERE status = 'PENDENTE' AND idPedido1 = :idPedido1");

  SqlQuery queryCompra2;
  queryCompra2.prepare("UPDATE pedido_fornecedor_has_produto2 set STATUS = 'EM COMPRA', idCompra = :idCompra, ordemCompra = :ordemCompra, dataRealCompra = :dataRealCompra, dataPrevConf = "
                       ":dataPrevConf WHERE status = 'PENDENTE' AND idPedidoFK = :idPedido1");

  for (const auto &index : list) {
    // salvar status na venda

    const int idVendaProduto1 = modelProdutos.data(index.row(), "idVendaProduto1").toInt();
    const int idPedido1 = modelProdutos.data(index.row(), "idPedido1").toInt();

    if (idVendaProduto1 != 0) {
      queryVenda.bindValue(":idCompra", idCompra);
      queryVenda.bindValue(":dataRealCompra", dataCompra);
      queryVenda.bindValue(":dataPrevConf", dataPrevista);
      queryVenda.bindValue(":idVendaProduto1", idVendaProduto1);

      if (not queryVenda.exec()) { throw RuntimeException("Erro atualizando status da venda: " + queryVenda.lastError().text()); }
    }

    // ---------------------------------------------------------

    queryCompra1.bindValue(":idCompra", idCompra);
    queryCompra1.bindValue(":ordemCompra", ordemCompra);
    queryCompra1.bindValue(":dataRealCompra", dataCompra);
    queryCompra1.bindValue(":dataPrevConf", dataPrevista);
    queryCompra1.bindValue(":idPedido1", idPedido1);

    if (not queryCompra1.exec()) { throw RuntimeException("Erro atualizando compra: " + queryCompra1.lastError().text()); }

    // ---------------------------------------------------------

    queryCompra2.bindValue(":idCompra", idCompra);
    queryCompra2.bindValue(":ordemCompra", ordemCompra);
    queryCompra2.bindValue(":dataRealCompra", dataCompra);
    queryCompra2.bindValue(":dataPrevConf", dataPrevista);
    queryCompra2.bindValue(":idPedido1", idPedido1);

    if (not queryCompra2.exec()) { throw RuntimeException("Erro atualizando compra: " + queryCompra2.lastError().text()); }
  }
}

void WidgetCompraGerar::on_pushButtonGerarCompra_clicked() {
  const QString folderKey = User::getSetting("User/ComprasFolder").toString();

  if (folderKey.isEmpty()) { throw RuntimeError("Por favor selecione uma pasta para salvar os arquivos nas configurações do usuário!", this); }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  //

  const bool isRepresentacao = verificaRepresentacao(list);

  // oc

  const int ordemCompra = getOrdemCompra();

  const auto [dataCompra, dataPrevista] = getDates(list);

  // email --------------------------------

  // reload data after getDates
  modelProdutos.select();

  const QString razaoSocial = modelProdutos.data(list.first().row(), "fornecedor").toString();

  const QString anexo = gerarExcel(list, ordemCompra, isRepresentacao);

  // -------------------------------------------------------------------------

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  qApp->startTransaction("WidgetCompraGerar::on_pushButtonGerarCompra");

  gerarCompra(list, dataCompra, dataPrevista, ordemCompra);

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  enviarEmail(razaoSocial, anexo);

  // -------------------------------------------------------------------------

  updateTables();

  qApp->enqueueInformation("Compra gerada com sucesso!", this);

  emit finished();
}

void WidgetCompraGerar::enviarEmail(const QString &razaoSocial, const QString &anexo) {
  QMessageBox msgBox(QMessageBox::Question, "Enviar E-mail?", "Deseja enviar e-mail?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Enviar");
  msgBox.button(QMessageBox::No)->setText("Pular");

  if (msgBox.exec() == QMessageBox::Yes) {
    auto *mail = new SendMail(SendMail::Tipo::GerarCompra, anexo, razaoSocial, this);
    mail->setAttribute(Qt::WA_DeleteOnClose);

    mail->exec();
  }
}

std::tuple<QDate, QDate> WidgetCompraGerar::getDates(const QModelIndexList &list) {
  QStringList ids;

  for (const auto &index : list) { ids << modelProdutos.data(index.row(), "idPedido1").toString(); }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::GerarCompra, this);
  inputDlg.setFilter(ids);

  if (inputDlg.exec() != InputDialogProduto::Accepted) { throw std::exception(); }

  return std::make_tuple<>(inputDlg.getDate(), inputDlg.getNextDate());
}

int WidgetCompraGerar::getOrdemCompra() {
  SqlQuery queryOC;

  if (not queryOC.exec("SELECT COALESCE(MAX(ordemCompra), 0) + 1 AS ordemCompra FROM pedido_fornecedor_has_produto")) {
    throw RuntimeException("Erro buscando próxima OC: " + queryOC.lastError().text());
  }

  if (not queryOC.first()) { throw RuntimeException("Erro buscando próxima OC!"); }

  bool ok = false;

  int oc = QInputDialog::getInt(this, "OC", "Qual a Ordem de Compra?", queryOC.value("ordemCompra").toInt(), 0, 999999, 1, &ok);

  if (not ok) { throw std::exception(); }

  SqlQuery query2;
  query2.prepare("SELECT ordemCompra FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra LIMIT 1");

  while (true) {
    query2.bindValue(":ordemCompra", oc);

    if (not query2.exec()) { throw RuntimeException("Erro buscando O.C.!"); }

    if (not query2.first()) { break; }

    if (query2.first()) {
      QMessageBox msgBox(QMessageBox::Question, "Atenção!", "OC já existe! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
      msgBox.button(QMessageBox::Yes)->setText("Continuar");
      msgBox.button(QMessageBox::No)->setText("Voltar");

      const int choice = msgBox.exec();

      if (choice == QMessageBox::Yes) { break; }

      bool ok2 = false;

      oc = QInputDialog::getInt(this, "OC", "Qual a Ordem de Compra?", query2.value("ordemCompra").toInt(), 0, 999999, 1, &ok2);

      if (not ok2) { throw std::exception(); }
    }
  }

  return oc;
}

bool WidgetCompraGerar::verificaRepresentacao(const QModelIndexList &list) {
  const int row = list.first().row();
  const QString fornecedor = modelProdutos.data(row, "fornecedor").toString();

  SqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  query.bindValue(":razaoSocial", fornecedor);

  if (not query.exec()) { throw RuntimeException("Erro buscando dados do fornecedor: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para o fornecedor: " + fornecedor); }

  const bool isRepresentacao = query.value("representacao").toBool();

  if (isRepresentacao) {
    if (modelProdutos.data(row, "idVenda").toString().isEmpty()) {
      // TODO: verificar quando a loja precisar comprar avulso
      throw RuntimeError("'Venda' vazio!");
    }

    QStringList idVendaList;

    for (const auto &index : list) { idVendaList << modelProdutos.data(index.row(), "idVenda").toString(); }

    idVendaList.removeDuplicates();

    if (idVendaList.size() > 1) { throw RuntimeError("Não pode misturar produtos de vendas diferentes na representação!"); }
  }

  return isRepresentacao;
}

QString WidgetCompraGerar::gerarExcel(const QModelIndexList &list, const int ordemCompra, const bool isRepresentacao) {
  const QString folderKey = User::getSetting("User/ComprasFolder").toString();

  if (folderKey.isEmpty()) { throw RuntimeError("Não há uma pasta definida para salvar PDF/Excel. Por favor escolha uma nas configurações do ERP!"); }

  const int firstRow = list.first().row();
  const QString fornecedor = modelProdutos.data(firstRow, "fornecedor").toString();

  if (isRepresentacao) {
    const QString idVenda = modelProdutos.data(firstRow, "idVenda").toString();
    QString fileName = folderKey + "/" + QString::number(ordemCompra) + " " + idVenda + " " + fornecedor + ".xlsx";

    Excel excel(idVenda, Excel::Tipo::Venda, this);
    excel.ordemCompra = ordemCompra;
    excel.anexoCompra = true;
    excel.customFileName = fileName;
    excel.gerarExcel();

    return fileName;
  }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  idVendas.removeDuplicates();

  const QString idVenda = (fornecedor == "QUARTZOBRAS" or fornecedor == "MC BAUCHEMIE") ? "" : idVendas.join(", ");

  const QString arquivoModelo = QDir::currentPath() + "/modelos/compras.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do Excel!"); }

  QString fileName = folderKey + "/" + QString::number(ordemCompra) + " " + idVenda + " " + fornecedor + ".xlsx";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  SqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) { throw RuntimeException("Erro buscando contato do fornecedor: " + queryFornecedor.lastError().text()); }

  QXlsx::Document xlsx(arquivoModelo, this);

  //  xlsx.currentWorksheet()->setFitToPage(true);
  //  xlsx.currentWorksheet()->setFitToHeight(true);
  //  xlsx.currentWorksheet()->setOrientationVertical(false);

  xlsx.write("E4", ordemCompra);
  xlsx.write("E5", idVenda);
  xlsx.write("E6", fornecedor);
  xlsx.write("E7", queryFornecedor.first() ? queryFornecedor.value("contatoNome").toString() : "");
  xlsx.write("E8", qApp->serverDateTime().toString("dddd dd 'de' MMMM 'de' yyyy hh:mm"));

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

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o arquivo!"); }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como: " + fileName, this);

  return fileName;
}

void WidgetCompraGerar::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->tableProdutos->selectAll() : ui->tableProdutos->clearSelection(); }

void WidgetCompraGerar::on_tableResumo_clicked(const QModelIndex &index) {
  const QString fornecedor = index.isValid() ? modelResumo.data(index.row(), "fornecedor").toString() : "";

  const QString filtro = fornecedor.isEmpty() ? "0" : "status = 'PENDENTE' AND fornecedor = '" + fornecedor + "'";

  modelProdutos.setFilter(filtro);
}

void WidgetCompraGerar::cancelar(const QModelIndexList &list) {
  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'CANCELADO', idVenda = NULL, idVendaProduto2 = NULL WHERE idPedidoFK = :idPedidoFK");

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = CASE WHEN reposicaoEntrega THEN 'REPO. ENTREGA' WHEN reposicaoReceb THEN 'REPO. RECEB.' ELSE 'PENDENTE' END, idCompra = NULL, "
                     "dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
                     "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE status = 'INICIADO' AND "
                     "idVendaProdutoFK = :idVendaProduto1");

  for (const auto &index : list) {
    queryCompra.bindValue(":idPedidoFK", modelProdutos.data(index.row(), "idPedido1"));

    if (not queryCompra.exec()) { throw RuntimeException("Erro cancelando compra: " + queryCompra.lastError().text()); }

    // ---------------------------------------------------------

    queryVenda.bindValue(":idVendaProduto1", modelProdutos.data(index.row(), "idVendaProduto1"));

    if (not queryVenda.exec()) { throw RuntimeException("Erro voltando status do produto: " + queryVenda.lastError().text()); }
  }
}

void WidgetCompraGerar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Cancelar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  qApp->startTransaction("WidgetCompraGerar::on_pushButtonCancelarCompra");

  cancelar(list);

  Sql::updateVendaStatus(idVendas);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Itens cancelados!", this);
}

void WidgetCompraGerar::on_pushButtonFollowup_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = modelProdutos.data(list.first().row(), "idVenda").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetCompraGerar::on_pushButtonLimparFiltro_clicked() {
  ui->tableResumo->clearSelection();

  const QString fornecedor = "";

  const QString filtro = fornecedor.isEmpty() ? "0" : "status = 'PENDENTE' AND fornecedor = '" + fornecedor + "'";

  modelProdutos.setFilter(filtro);
}

// TODO: 2vincular compras geradas com loja selecionada em configuracoes
// TODO: 5colocar tamanho minimo da tabela da esquerda para mostrar todas as colunas
// TODO: no caso da quartzobras se for mais de um pedido deixar o campo 'PEDIDO DE VENDA NR.' vazio
// TODO: no caso da quartzobras ordenar por cod. produto em vez de por pedido

// TODO: ao confirmar email voltar tela para pendentes
