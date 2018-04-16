#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

#include "excel.h"
#include "inputdialogproduto.h"
#include "reaisdelegate.h"
#include "sendmail.h"
#include "sql.h"
#include "ui_widgetcompragerar.h"
#include "usersession.h"
#include "widgetcompragerar.h"
#include "xlsxdocument.h"

WidgetCompraGerar::WidgetCompraGerar(QWidget *parent) : Widget(parent), ui(new Ui::WidgetCompraGerar) {
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

  modelResumo.setFilter("(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  modelResumo.setHeaderData("fornecedor", "Fornecedor");
  modelResumo.setHeaderData("COUNT(fornecedor)", "Itens");

  ui->tableResumo->setModel(&modelResumo);
  ui->tableResumo->hideColumn("idVenda");

  modelProdutos.setTable("pedido_fornecedor_has_produto");
  modelProdutos.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelProdutos.setHeaderData("selecionado", "");
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

  modelProdutos.setFilter("0");

  ui->tableProdutos->setModel(&modelProdutos);
  ui->tableProdutos->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->hideColumn("statusFinanceiro");
  ui->tableProdutos->hideColumn("selecionado");
  ui->tableProdutos->hideColumn("ordemCompra");
  ui->tableProdutos->hideColumn("idCompra");
  ui->tableProdutos->hideColumn("quantConsumida");
  ui->tableProdutos->hideColumn("idNfe");
  ui->tableProdutos->hideColumn("idEstoque");
  ui->tableProdutos->hideColumn("quantUpd");
  ui->tableProdutos->hideColumn("idPedido");
  ui->tableProdutos->hideColumn("idLoja");
  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("parcial");
  ui->tableProdutos->hideColumn("desconto");
  ui->tableProdutos->hideColumn("parcialDesc");
  ui->tableProdutos->hideColumn("descGlobal");
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
  connect(ui->checkBoxMarcarTodos, &QCheckBox::clicked, this, &WidgetCompraGerar::on_checkBoxMarcarTodos_clicked);
  connect(ui->checkBoxMostrarSul, &QCheckBox::toggled, this, &WidgetCompraGerar::on_checkBoxMostrarSul_toggled);
  connect(ui->pushButtonCancelarCompra, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonCancelarCompra_clicked);
  connect(ui->pushButtonGerarCompra, &QPushButton::clicked, this, &WidgetCompraGerar::on_pushButtonGerarCompra_clicked);
  connect(ui->tableProdutos, &TableView::entered, this, &WidgetCompraGerar::on_tableProdutos_entered);
  connect(ui->tableResumo, &TableView::activated, this, &WidgetCompraGerar::on_tableResumo_activated);
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

  if (selection.size() > 0) { ui->tableResumo->selectRow(selection.first().row()); }

  ui->tableResumo->resizeColumnsToContents();

  if (not modelProdutos.select()) { return; }

  ui->tableProdutos->resizeColumnsToContents();
}

void WidgetCompraGerar::resetTables() { modelIsSet = false; }

bool WidgetCompraGerar::gerarCompra(const QList<int> &lista, const QDateTime &dataCompra, const QDateTime &dataPrevista) {
  QStringList produtos;

  for (const auto &row : lista) {
    produtos.append(modelProdutos.data(row, "descricao").toString() + ", Quant: " + modelProdutos.data(row, "quant").toString() + ", R$ " +
                    modelProdutos.data(row, "preco").toString().replace(".", ","));
  }

  QSqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto SET status = 'EM COMPRA', idCompra = :idCompra, dataRealCompra = :dataRealCompra, dataPrevConf = :dataPrevConf WHERE idVendaProduto = :idVendaProduto");

  for (const auto &row : lista) {
    if (not modelProdutos.setData(row, "status", "EM COMPRA")) { return false; }

    QSqlQuery queryId;

    if (not queryId.exec("SELECT COALESCE(MAX(idCompra), 0) + 1 AS idCompra FROM pedido_fornecedor_has_produto") or not queryId.first()) {
      emit errorSignal("Erro buscando idCompra: " + queryId.lastError().text());
      return false;
    }

    const QString id = queryId.value("idCompra").toString();

    if (not modelProdutos.setData(row, "idCompra", id)) { return false; }
    if (not modelProdutos.setData(row, "ordemCompra", oc)) { return false; }
    if (not modelProdutos.setData(row, "dataRealCompra", dataCompra)) { return false; }
    if (not modelProdutos.setData(row, "dataPrevConf", dataPrevista)) { return false; }

    // salvar status na venda

    if (modelProdutos.data(row, "idVendaProduto").toInt() != 0) {
      queryVenda.bindValue(":idCompra", id);
      queryVenda.bindValue(":dataRealCompra", dataCompra);
      queryVenda.bindValue(":dataPrevConf", dataPrevista);
      queryVenda.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

      if (not queryVenda.exec()) {
        emit errorSignal("Erro atualizando status da venda: " + queryVenda.lastError().text());
        return false;
      }
    }
  }

  if (not modelProdutos.submitAll()) { return false; }

  return true;
}

void WidgetCompraGerar::on_pushButtonGerarCompra_clicked() {
  // TODO: 1refatorar essa funcao, dividir em funcoes menores etc

  const auto folderKey = UserSession::getSetting("User/ComprasFolder");

  if (not folderKey or folderKey.value().toString().isEmpty()) {
    emit errorSignal("Por favor selecione uma pasta para salvar os arquivos nas configurações do usuário!");
    return;
  }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QList<int> lista;
  QStringList ids;
  QStringList idVendas;

  for (const auto &index : list) {
    lista << index.row();
    ids << modelProdutos.data(index.row(), "idPedido").toString();
    idVendas << modelProdutos.data(index.row(), "idVenda").toString();
  }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::GerarCompra);
  if (not inputDlg.setFilter(ids)) { return; }

  if (inputDlg.exec() != InputDialogProduto::Accepted) { return; }

  const QDateTime dataCompra = inputDlg.getDate();
  const QDateTime dataPrevista = inputDlg.getNextDate();

  // oc

  QSqlQuery queryOC;

  if (not queryOC.exec("SELECT COALESCE(MAX(ordemCompra), 0) + 1 AS ordemCompra FROM pedido_fornecedor_has_produto") or not queryOC.first()) {
    emit errorSignal("Erro buscando próximo O.C.!");
    return;
  }

  bool ok;
  oc = QInputDialog::getInt(this, "OC", "Qual a OC?", queryOC.value("ordemCompra").toInt(), 0, 99999, 1, &ok);
  if (not ok) { return; }

  QSqlQuery query2;
  query2.prepare("SELECT ordemCompra FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra LIMIT 1");

  while (true) {
    query2.bindValue(":ordemCompra", oc);

    if (not query2.exec()) {
      emit errorSignal("Erro buscando O.C.!");
      return;
    }

    if (not query2.first()) { break; }

    if (query2.first()) {
      QMessageBox msgBox(QMessageBox::Question, "Atenção!", "OC já existe! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
      msgBox.setButtonText(QMessageBox::Yes, "Continuar");
      msgBox.setButtonText(QMessageBox::No, "Voltar");

      const int choice = msgBox.exec();

      if (choice == QMessageBox::Yes) { break; }

      if (choice != QMessageBox::Yes) {
        bool ok2;
        oc = QInputDialog::getInt(this, "OC", "Qual a OC?", query2.value("ordemCompra").toInt(), 0, 99999, 1, &ok2);
        if (not ok2) { return; }
      }
    }
  }

  // enviar email

  QSqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  query.bindValue(":razaoSocial", modelProdutos.data(0, "fornecedor").toString());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando dados do fornecedor: " + query.lastError().text());
    return;
  }

  QString anexo;

  const bool isRepresentacao = query.value("representacao").toBool();

  if (not gerarExcel(lista, anexo, isRepresentacao)) { return; }

  QMessageBox msgBox(QMessageBox::Question, "Enviar E-mail?", "Deseja enviar e-mail?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Enviar");
  msgBox.setButtonText(QMessageBox::No, "Pular");

  if (msgBox.exec() == QMessageBox::Yes) {
    const QString fornecedor = modelProdutos.data(0, "fornecedor").toString();

    auto *mail = new SendMail(SendMail::Tipo::GerarCompra, anexo, fornecedor, this);
    mail->setAttribute(Qt::WA_DeleteOnClose);

    mail->exec();
  }

  // -------------------------------------------------------------------------

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not gerarCompra(lista, dataCompra, dataPrevista)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) { return; }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Compra gerada com sucesso!");
}

bool WidgetCompraGerar::gerarExcel(const QList<int> &lista, QString &anexo, const bool isRepresentacao) {
  if (isRepresentacao) {
    if (modelProdutos.data(lista.first(), "idVenda").toString().isEmpty()) {
      emit errorSignal("'Venda' vazio!");
      return false;
    }

    Excel excel(modelProdutos.data(lista.at(0), "idVenda").toString());
    const QString representacao = "OC " + QString::number(oc) + " " + modelProdutos.data(lista.first(), "idVenda").toString() + " " + modelProdutos.data(lista.first(), "fornecedor").toString();
    if (not excel.gerarExcel(oc, true, representacao)) { return false; }
    anexo = excel.getFileName();
    return true;
  }

  const QString fornecedor = modelProdutos.data(lista.at(0), "fornecedor").toString();
  const QString idVenda = modelProdutos.data(lista.at(0), "idVenda").toString();

  const QString arquivoModelo = "modelo compras.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    emit errorSignal("Não encontrou o modelo do Excel!");
    return false;
  }

  const auto folderKey = UserSession::getSetting("User/ComprasFolder");

  if (not folderKey or folderKey.value().toString().isEmpty()) { return false; }

  const QString fileName = folderKey.value().toString() + "/" + QString::number(oc) + " " + idVenda + " " + fornecedor + ".xlsx";

  anexo = fileName;

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) {
    emit errorSignal("Não foi possível abrir o arquivo para escrita: " + fileName);
    emit errorSignal("Erro: " + file.errorString());
    return false;
  }

  file.close();

  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    emit errorSignal("Erro buscando contato do fornecedor: " + queryFornecedor.lastError().text());
    return false;
  }

  QXlsx::Document xlsx(arquivoModelo);

  //  xlsx.currentWorksheet()->setFitToPage(true);
  //  xlsx.currentWorksheet()->setFitToHeight(true);
  //  xlsx.currentWorksheet()->setOrientationVertical(false);

  xlsx.write("E4", oc);                                                                             // ordem compra
  xlsx.write("E5", idVenda);                                                                        // idVenda
  xlsx.write("E6", modelProdutos.data(lista.at(0), "fornecedor"));                                  // fornecedor
  xlsx.write("E7", queryFornecedor.first() ? queryFornecedor.value("contatoNome").toString() : ""); // representante
  xlsx.write("E8", QDateTime::currentDateTime().toString("dddd dd 'de' MMMM 'de' yyyy hh:mm"));     // Data: dd//mm/yyyy

  double total = 0;

  for (int row = 0; row < lista.size(); ++row) {
    xlsx.write("A" + QString::number(13 + row), QString::number(row + 1));                          // item n. 1,2,3...
    xlsx.write("B" + QString::number(13 + row), modelProdutos.data(lista.at(row), "codComercial")); // cod produto
    QString formato = modelProdutos.data(lista.at(row), "formComercial").toString();
    QString produto = modelProdutos.data(lista.at(row), "descricao").toString() + (formato.isEmpty() ? "" : " - " + formato);
    xlsx.write("C" + QString::number(13 + row), produto);                                          // descricao produto
    xlsx.write("E" + QString::number(13 + row), modelProdutos.data(lista.at(row), "prcUnitario")); // prc. unitario
    xlsx.write("F" + QString::number(13 + row), modelProdutos.data(lista.at(row), "un"));          // un.
    xlsx.write("G" + QString::number(13 + row), modelProdutos.data(lista.at(row), "quant"));       // qtd.
    xlsx.write("H" + QString::number(13 + row), modelProdutos.data(lista.at(row), "preco"));       // valor

    const QString st = modelProdutos.data(lista.at(row), "st").toString();

    if (st == "ST Fornecedor") {
      xlsx.write("I" + QString::number(13 + row), modelProdutos.data(lista.at(row), "idVenda"));
      total += modelProdutos.data(lista.at(row), "preco").toDouble();
    }
  }

  const QString st = modelProdutos.data(lista.first(), "st").toString();

  if (st == "ST Fornecedor") {
    xlsx.write("G200", "ST:");
    xlsx.write("H200", total * modelProdutos.data(lista.first(), "aliquotaSt").toDouble() / 100);
  }

  for (int row = lista.size() + 13; row < 200; ++row) { xlsx.setRowHidden(row, true); }

  if (not xlsx.saveAs(fileName)) {
    emit errorSignal("Ocorreu algum erro ao salvar o arquivo.");
    return false;
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  emit informationSignal("Arquivo salvo como " + fileName);

  return true;
}

void WidgetCompraGerar::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->tableProdutos->selectAll() : ui->tableProdutos->clearSelection(); }

void WidgetCompraGerar::on_tableResumo_activated(const QModelIndex &index) {
  const QString fornecedor = modelResumo.data(index.row(), "fornecedor").toString();
  const bool checked = ui->checkBoxMostrarSul->isChecked();

  modelProdutos.setFilter("fornecedor = '" + fornecedor + "' AND status = 'PENDENTE' AND " +
                          QString(checked ? "(idVenda LIKE 'CAMB%' OR idVenda IS NULL)" : "(idVenda NOT LIKE 'CAMB%' OR idVenda IS NULL)"));

  if (not modelProdutos.select()) { return; }

  ui->tableProdutos->resizeColumnsToContents();
}

void WidgetCompraGerar::on_tableProdutos_entered(const QModelIndex &) { ui->tableProdutos->resizeColumnsToContents(); }

bool WidgetCompraGerar::cancelar(const QModelIndexList &list) {
  QSqlQuery query;
  query.prepare("UPDATE venda_has_produto SET status = 'PENDENTE' WHERE idVendaProduto = :idVendaProduto AND status = 'INICIADO'");

  for (const auto &index : list) {
    if (not modelProdutos.setData(index.row(), "status", "CANCELADO")) { return false; }

    query.bindValue(":idVendaProduto", modelProdutos.data(index.row(), "idVendaProduto"));

    if (not query.exec()) {
      emit errorSignal("Erro voltando status do produto: " + query.lastError().text());
      return false;
    }
  }

  if (not modelProdutos.submitAll()) { return false; }

  return true;
}

void WidgetCompraGerar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  QStringList idVendas;

  for (const auto &index : list) { idVendas << modelProdutos.data(index.row(), "idVenda").toString(); }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) { return; }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not Sql::updateVendaStatus(idVendas.join(", "))) { return; }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  updateTables();
  emit informationSignal("Itens cancelados!");
}

// TODO: 2vincular compras geradas com loja selecionada em configuracoes
// TODO: 5colocar tamanho minimo da tabela da esquerda para mostrar todas as colunas

void WidgetCompraGerar::on_checkBoxMostrarSul_toggled(bool checked) {
  modelResumo.setFilter(checked ? "(idVenda LIKE '%CAMB%')" : "(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  if (not modelResumo.select()) { return; }
}

// TODO: avulso
// TODO: no caso da quartzobras se for mais de um pedido deixar o campo 'PEDIDO DE VENDA NR.' vazio
// TODO: no caso da quartzobras ordenar por cod. produto em vez de por pedido
