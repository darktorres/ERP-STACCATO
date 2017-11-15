#include <QDateTime>
#include <QDebug>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QUrl>

#include "acbr.h"
#include "cadastrarnfe.h"
#include "doubledelegate.h"
#include "inputdialog.h"
#include "inputdialogconfirmacao.h"
#include "sqlquerymodel.h"
#include "ui_widgetlogisticaentregas.h"
#include "usersession.h"
#include "widgetlogisticaentregas.h"
#include "xlsxdocument.h"

WidgetLogisticaEntregas::WidgetLogisticaEntregas(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetLogisticaEntregas) {
  ui->setupUi(this);

  ui->splitter->setStretchFactor(0, 0);
  ui->splitter->setStretchFactor(1, 1);
}

WidgetLogisticaEntregas::~WidgetLogisticaEntregas() { delete ui; }

bool WidgetLogisticaEntregas::updateTables() {
  if (modelCarga.tableName().isEmpty()) setupTables();

  if (not modelCarga.select()) {
    emit errorSignal("Erro lendo tabela de entregas: " + modelCarga.lastError().text());
    return false;
  }

  ui->tableCarga->resizeColumnsToContents();

  if (not modelCalendario.select()) {
    emit errorSignal("Erro lendo tabela calendario: " + modelCalendario.lastError().text());
    return false;
  }

  ui->tableCalendario->resizeColumnsToContents();

  if (not modelCarga.select()) {
    emit errorSignal("Erro lendo tabela calendario: " + modelCalendario.lastError().text());
    return false;
  }

  ui->tableCarga->resizeColumnsToContents();

  if (modelCarga.rowCount() == 0) modelProdutos.setFilter("0");

  if (not modelProdutos.select()) {
    emit errorSignal("Erro lendo tabela calendario: " + modelCalendario.lastError().text());
    return false;
  }

  ui->tableProdutos->resizeColumnsToContents();

  return true;
}

void WidgetLogisticaEntregas::setupTables() {
  // REFAC: refactor this to not select in here

  modelCalendario.setTable("view_calendario_entrega");
  modelCalendario.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelCalendario.setHeaderData("data", "Agendado");
  modelCalendario.setHeaderData("modelo", "Modelo");
  modelCalendario.setHeaderData("razaoSocial", "Transp.");
  modelCalendario.setHeaderData("kg", "Kg.");
  modelCalendario.setHeaderData("idVenda", "Venda");

  if (not modelCalendario.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela calendario: " + modelCalendario.lastError().text());

  ui->tableCalendario->setModel(&modelCalendario);
  ui->tableCalendario->hideColumn("idVeiculo");
  ui->tableCalendario->setItemDelegateForColumn("kg", new DoubleDelegate(this));

  //

  modelCarga.setTable("view_calendario_carga");
  modelCarga.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelCarga.setHeaderData("dataPrevEnt", "Agendado");
  modelCarga.setHeaderData("numeroNFe", "NFe");
  modelCarga.setHeaderData("idVenda", "Venda");
  modelCarga.setHeaderData("status", "Status");
  modelCarga.setHeaderData("produtos", "Produtos");
  modelCarga.setHeaderData("kg", "Kg.");

  modelCarga.setFilter("0");

  if (not modelCarga.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela cargas: " + modelCarga.lastError().text());

  ui->tableCarga->setModel(&modelCarga);
  ui->tableCarga->hideColumn("idEvento");
  ui->tableCarga->hideColumn("idNFe");
  ui->tableCarga->hideColumn("idVeiculo");
  ui->tableCarga->setItemDelegateForColumn("kg", new DoubleDelegate(this));

  //

  modelProdutos.setTable("view_calendario_produto");
  modelProdutos.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("idVenda", "Venda");
  modelProdutos.setHeaderData("produto", "Produto");
  modelProdutos.setHeaderData("caixas", "Cx.");
  modelProdutos.setHeaderData("kg", "Kg.");
  modelProdutos.setHeaderData("quant", "Quant.");
  modelProdutos.setHeaderData("un", "Un.");

  modelProdutos.setFilter("0");

  if (not modelProdutos.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela produtos: " + modelProdutos.lastError().text());

  ui->tableProdutos->setModel(&modelProdutos);
  ui->tableProdutos->hideColumn("idEvento");
  ui->tableProdutos->hideColumn("idVendaProduto");
  ui->tableProdutos->setItemDelegateForColumn("kg", new DoubleDelegate(this));
  ui->tableProdutos->setItemDelegateForColumn("quant", new DoubleDelegate(this));
}

void WidgetLogisticaEntregas::on_pushButtonReagendar_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  InputDialog input(InputDialog::Tipo::AgendarEntrega);

  if (input.exec() != InputDialog::Accepted) return;

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not reagendar(list, input.getNextDate())) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  updateTables();
  QMessageBox::information(this, "Aviso!", "Reagendado com sucesso!");
}

bool WidgetLogisticaEntregas::reagendar(const QModelIndexList &list, const QDate &dataPrevEnt) {
  QSqlQuery query;

  for (const auto &item : list) {
    for (int row = 0; row < modelProdutos.rowCount(); ++row) {
      query.prepare("UPDATE venda_has_produto SET dataPrevEnt = :dataPrevEnt WHERE idVendaProduto = :idVendaProduto");
      query.bindValue(":dataPrevEnt", dataPrevEnt);
      query.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

      if (not query.exec()) {
        emit errorSignal("Erro atualizando data venda: " + query.lastError().text());
        return false;
      }

      query.prepare("UPDATE pedido_fornecedor_has_produto SET dataPrevEnt = :dataPrevEnt WHERE idVendaProduto = :idVendaProduto");
      query.bindValue(":dataPrevEnt", dataPrevEnt);
      query.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

      if (not query.exec()) {
        emit errorSignal("Erro atualizando data pedido_fornecedor: " + query.lastError().text());
        return false;
      }
    }

    query.prepare("UPDATE veiculo_has_produto SET data = :data WHERE idEvento = :idEvento");
    query.bindValue(":data", dataPrevEnt);
    query.bindValue(":idEvento", modelCarga.data(item.row(), "idEvento"));

    if (not query.exec()) {
      emit errorSignal("Erro atualizando data carga: " + query.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaEntregas::on_pushButtonGerarNFeEntregar_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  const QString idVenda = modelCarga.data(list.first().row(), "idVenda").toString();

  QList<int> lista;

  for (int row = 0; row < modelProdutos.rowCount(); ++row) lista.append(modelProdutos.data(row, "idVendaProduto").toInt());

  auto *nfe = new CadastrarNFe(idVenda, this);
  nfe->setAttribute(Qt::WA_DeleteOnClose);
  nfe->prepararNFe(lista);

  connect(nfe, &CadastrarNFe::errorSignal, this, &WidgetLogisticaEntregas::errorSignal);
  connect(nfe, &CadastrarNFe::transactionStarted, this, &WidgetLogisticaEntregas::transactionStarted);
  connect(nfe, &CadastrarNFe::transactionEnded, this, &WidgetLogisticaEntregas::transactionEnded);

  nfe->show();
}

void WidgetLogisticaEntregas::on_tableCalendario_clicked(const QModelIndex &index) {
  const QString data = modelCalendario.data(index.row(), "data").toString();
  const QString veiculo = modelCalendario.data(index.row(), "idVeiculo").toString();

  modelCarga.setFilter("DATE_FORMAT(dataPrevEnt, '%d-%m-%Y') = '" + data + "' AND idVeiculo = " + veiculo);

  if (not modelCarga.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela entregas: " + modelCarga.lastError().text());

  ui->tableCarga->resizeColumnsToContents();

  modelProdutos.setFilter("0");

  if (not modelProdutos.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela produtos: " + modelProdutos.lastError().text());

  ui->pushButtonReagendar->setDisabled(true);
  ui->pushButtonConfirmarEntrega->setDisabled(true);
  ui->pushButtonGerarNFeEntregar->setDisabled(true);
  ui->pushButtonImprimirDanfe->setDisabled(true);
  ui->pushButtonCancelarEntrega->setDisabled(true);
}

void WidgetLogisticaEntregas::on_tableCarga_clicked(const QModelIndex &index) {
  modelProdutos.setFilter("idVenda = '" + modelCarga.data(index.row(), "idVenda").toString() + "' AND idEvento = " + modelCarga.data(index.row(), "idEvento").toString());

  if (not modelProdutos.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela produtos: " + modelProdutos.lastError().text());

  ui->tableProdutos->resizeColumnsToContents();

  const QString status = modelCarga.data(index.row(), "status").toString();
  const QString nfeStatus = modelCarga.data(index.row(), "NFe Status").toString();

  ui->pushButtonReagendar->setEnabled(true);
  ui->pushButtonCancelarEntrega->setEnabled(true);

  if (status == "ENTREGA AGEND.") {
    ui->pushButtonGerarNFeEntregar->setEnabled(true);
    ui->pushButtonConfirmarEntrega->setEnabled(true);
    ui->pushButtonImprimirDanfe->setDisabled(true);
  }

  if (status == "EM ENTREGA") {
    ui->pushButtonGerarNFeEntregar->setDisabled(true);
    ui->pushButtonConfirmarEntrega->setEnabled(true);
    ui->pushButtonImprimirDanfe->setEnabled(true);
  }

  if (status == "NOTA PENDENTE") {
    ui->pushButtonGerarNFeEntregar->setDisabled(true);
    ui->pushButtonConfirmarEntrega->setDisabled(true);
    ui->pushButtonImprimirDanfe->setDisabled(true);
  }

  ui->pushButtonConsultarNFe->setEnabled(nfeStatus == "NOTA PENDENTE" or nfeStatus == "AUTORIZADO");
}

bool WidgetLogisticaEntregas::confirmarEntrega(const QDateTime &dataRealEnt, const QString &entregou, const QString &recebeu) {
  QSqlQuery query;

  for (int row = 0; row < modelProdutos.rowCount(); ++row) {
    const int idVendaProduto = modelProdutos.data(row, "idVendaProduto").toInt();

    query.prepare("UPDATE veiculo_has_produto SET status = 'ENTREGUE' WHERE status != 'QUEBRADO' AND idVendaProduto = :idVendaProduto");
    query.bindValue(":idVendaProduto", idVendaProduto);

    if (not query.exec()) {
      emit errorSignal("Erro salvando veiculo_has_produto: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'ENTREGUE', dataRealEnt = :dataRealEnt WHERE idVendaProduto = :idVendaProduto");
    query.bindValue(":dataRealEnt", dataRealEnt);
    query.bindValue(":idVendaProduto", idVendaProduto);

    if (not query.exec()) {
      emit errorSignal("Erro salvando pedido_fornecedor: " + query.lastError().text());
      return false;
    }

    query.prepare("UPDATE venda_has_produto SET status = 'ENTREGUE', entregou = :entregou, recebeu = :recebeu, dataRealEnt = :dataRealEnt WHERE idVendaProduto = :idVendaProduto");
    query.bindValue(":entregou", entregou);
    query.bindValue(":recebeu", recebeu);
    query.bindValue(":dataRealEnt", dataRealEnt);
    query.bindValue(":idVendaProduto", idVendaProduto);

    if (not query.exec()) {
      emit errorSignal("Erro salvando venda_produto: " + query.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaEntregas::on_pushButtonConfirmarEntrega_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  InputDialogConfirmacao inputDlg(InputDialogConfirmacao::Tipo::Entrega);
  inputDlg.setFilter(modelCarga.data(list.first().row(), "idVenda").toString(), modelCarga.data(list.first().row(), "idEvento").toString());

  if (inputDlg.exec() != InputDialogConfirmacao::Accepted) return;

  const QDateTime dataRealEnt = inputDlg.getDate();
  const QString entregou = inputDlg.getEntregou();
  const QString recebeu = inputDlg.getRecebeu();

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not confirmarEntrega(dataRealEnt, entregou, recebeu)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  QSqlQuery query;

  if (not query.exec("CALL update_venda_status()")) {
    QMessageBox::critical(this, "Erro!", "Erro atualizando status das vendas: " + query.lastError().text());
    return;
  }

  updateTables();
  QMessageBox::information(this, "Aviso!", "Entrega confirmada!");
}

void WidgetLogisticaEntregas::on_pushButtonImprimirDanfe_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  if (not ACBr::gerarDanfe(modelCarga.data(list.first().row(), "idNFe").toInt())) return;
}

void WidgetLogisticaEntregas::on_lineEditBuscar_textChanged(const QString &text) {
  modelCarga.setFilter(text.isEmpty() ? "0" : "idVenda LIKE '%" + text + "%'");

  if (not modelCarga.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela carga: " + modelCarga.lastError().text());
    return;
  }
}

void WidgetLogisticaEntregas::on_pushButtonCancelarEntrega_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not cancelarEntrega(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  QMessageBox::information(this, "Aviso!", "Operação realizada com sucesso!");
}

bool WidgetLogisticaEntregas::cancelarEntrega(const QModelIndexList &list) {
  const int idEvento = modelCarga.data(list.first().row(), "idEvento").toInt();

  QSqlQuery query;
  query.prepare("SELECT idVendaProduto FROM veiculo_has_produto WHERE idEvento = :idEvento");
  query.bindValue(":idEvento", idEvento);

  if (not query.exec()) {
    emit errorSignal("Erro buscando produtos: " + query.lastError().text());
    return false;
  }

  while (query.next()) {
    QSqlQuery query2;
    query2.prepare("UPDATE venda_has_produto SET status = 'ESTOQUE', dataPrevEnt = NULL WHERE idVendaProduto = :idVendaProduto");
    query2.bindValue(":idVendaProduto", query.value("idVendaProduto"));

    if (not query2.exec()) {
      emit errorSignal("Erro voltando status produto: " + query2.lastError().text());
      return false;
    }
  }

  query.prepare("DELETE FROM veiculo_has_produto WHERE idEvento = :idEvento");
  query.bindValue(":idEvento", idEvento);

  if (not query.exec()) {
    emit errorSignal("Erro deletando evento: " + query.lastError().text());
    return false;
  }

  return true;
}

// TODO: 2quando cancelar/devolver um produto cancelar/devolver na logistica/veiculo_has_produto
// TODO: 1refazer sistema para permitir multiplas notas para uma mesma carga/pedido (notas parciais)
// TODO: 0no filtro de 'parte estoque' nao considerar 'devolvido' e 'cancelado'

void WidgetLogisticaEntregas::on_tableCarga_entered(const QModelIndex &) { ui->tableCarga->resizeColumnsToContents(); }

void WidgetLogisticaEntregas::on_pushButtonConsultarNFe_clicked() {
  const auto selection = ui->tableCarga->selectionModel()->selectedRows();

  if (selection.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhuma linha selecionada!");
    return;
  }

  const int idNFe = modelCarga.data(selection.first().row(), "idNFe").toInt();

  if (auto tuple = ACBr::consultarNFe(idNFe); tuple) {
    auto [xml, resposta] = *tuple;

    emit transactionStarted();

    QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
    QSqlQuery("START TRANSACTION").exec();

    if (not consultarNFe(idNFe, xml)) {
      QSqlQuery("ROLLBACK").exec();
      emit transactionEnded();
      return;
    }

    QSqlQuery("COMMIT").exec();

    emit transactionEnded();

    QMessageBox::information(this, "Aviso!", resposta);
  }
}

bool WidgetLogisticaEntregas::consultarNFe(const int idNFe, const QString &xml) {
  QSqlQuery query;
  query.prepare("UPDATE nfe SET status = 'AUTORIZADO', xml = :xml WHERE idNFe = :idNFe");
  query.bindValue(":xml", xml);
  query.bindValue(":idNFe", idNFe);

  if (not query.exec()) {
    emit errorSignal("Erro marcando nota como 'AUTORIZADO': " + query.lastError().text());
    return false;
  }

  QSqlQuery query1;
  query1.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM ENTREGA' WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query2;
  query2.prepare("UPDATE venda_has_produto SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE idVendaProduto = :idVendaProduto");

  QSqlQuery query3;
  query3.prepare("UPDATE veiculo_has_produto SET status = 'EM ENTREGA', idNFeSaida = :idNFeSaida WHERE idVendaProduto = :idVendaProduto");

  for (int row = 0; row < modelProdutos.rowCount(); ++row) {
    query1.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

    if (not query1.exec()) {
      emit errorSignal("Erro atualizando status do pedido_fornecedor: " + query1.lastError().text());
      return false;
    }

    query2.bindValue(":idNFeSaida", idNFe);
    query2.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

    if (not query2.exec()) {
      emit errorSignal("Erro salvando NFe nos produtos: " + query2.lastError().text());
      return false;
    }

    query3.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));
    query3.bindValue(":idNFeSaida", idNFe);

    if (not query3.exec()) {
      emit errorSignal("Erro atualizando carga veiculo: " + query3.lastError().text());
      return false;
    }
  }

  return true;
}

void WidgetLogisticaEntregas::on_pushButtonProtocoloEntrega_clicked() {
  const auto list = ui->tableCarga->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  const QString idVenda = modelCarga.data(list.first().row(), "idVenda").toString();
  const QString idEvento = modelCarga.data(list.first().row(), "idEvento").toString();

  SqlQueryModel modelProdutosAgrupado;
  modelProdutosAgrupado.setQuery("SELECT idEvento, idVenda, fornecedor, produto, SUM(caixas) AS caixas, SUM(kg) AS kg, SUM(quant) AS quant, un FROM view_calendario_produto WHERE idVenda = '" +
                                 idVenda + "' AND idEvento = '" + idEvento + "' GROUP BY produto");

  //-------------------------------------------------------------------------

  if (modelProdutosAgrupado.rowCount() > 20) {
    QMessageBox::critical(this, "Erro!", "Mais produtos do que cabe no modelo do Excel!");
    return;
  }

  //-------------------------------------------------------------------------

  const auto folderKey = UserSession::getSetting("User/EntregasPdfFolder");

  if (not folderKey) {
    QMessageBox::critical(this, "Erro!", "Não há uma pasta definida para salvar PDF. Por favor escolha uma nas configurações do ERP!");
    return;
  }

  const QString path = folderKey.value().toString();

  QDir dir(path);

  if (not dir.exists() and not dir.mkdir(path)) {
    QMessageBox::critical(this, "Erro!", "Erro ao criar a pasta escolhida nas configurações!");
    return;
  }

  const QString arquivoModelo = "espelho_entrega.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    QMessageBox::critical(this, "Erro!", "Não encontrou o modelo do Excel!");
    return;
  }

  const QString fileName = path + "/teste_protocolo_entrega.xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) {
    QMessageBox::critical(this, "Erro!", "Não foi psossível abrir o arquivo para escrita:\n" + fileName);
    QMessageBox::critical(this, "Erro!", "Erro: " + file.errorString());
    return;
  }

  file.close();

  QXlsx::Document xlsx(arquivoModelo);

  xlsx.currentWorksheet()->setFitToPage(true);
  xlsx.currentWorksheet()->setFitToHeight(true);
  xlsx.currentWorksheet()->setOrientation(QXlsx::Worksheet::Orientation::Vertical);

  QSqlQuery queryCliente;
  queryCliente.prepare("SELECT nome_razao, tel, telCel FROM cliente c WHERE c.idCliente = (SELECT idCliente FROM venda WHERE idVenda = :idVenda)");
  queryCliente.bindValue(":idVenda", idVenda);

  if (not queryCliente.exec() or not queryCliente.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando dados cliente: " + queryCliente.lastError().text());
    return;
  }

  const QString tel = queryCliente.value("tel").toString();
  const QString telCel = queryCliente.value("telCel").toString();
  const QString tels = tel.isEmpty() ? "" : tel + (telCel.isEmpty() ? "" : " - " + telCel);

  xlsx.write("AA5", idVenda);
  xlsx.write("G11", queryCliente.value("nome_razao"));
  xlsx.write("Y11", tels);

  QSqlQuery queryEndereco;
  queryEndereco.prepare("SELECT logradouro, numero, complemento, bairro, cidade, cep FROM cliente_has_endereco WHERE idEndereco = (SELECT idEnderecoEntrega FROM venda WHERE idVenda = :idVenda)");
  queryEndereco.bindValue(":idVenda", idVenda);

  if (not queryEndereco.exec()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando endereço: " + queryEndereco.lastError().text());
    return;
  }

  if (queryEndereco.first()) {
    xlsx.write("J19", queryEndereco.value("logradouro").toString() + " " + queryEndereco.value("numero").toString() + " " + queryEndereco.value("complemento").toString() + " - " +
                          queryEndereco.value("bairro").toString() + ", " + queryEndereco.value("cidade").toString());
    xlsx.write("I17", queryEndereco.value("cep"));
  }

  for (int row = 27, index = 0; index < modelProdutosAgrupado.rowCount(); row = row + 2, ++index) {
    xlsx.write("D" + QString::number(row), modelProdutosAgrupado.data(index, "fornecedor"));
    xlsx.write("I" + QString::number(row), modelProdutosAgrupado.data(index, "produto"));                                                                     // produto
    xlsx.write("Y" + QString::number(row), modelProdutosAgrupado.data(index, "quant").toString() + " " + modelProdutosAgrupado.data(index, "un").toString()); // quant
    xlsx.write("AC" + QString::number(row), modelProdutosAgrupado.data(index, "caixas"));                                                                     // caixas
  }

  if (not xlsx.saveAs(fileName)) {
    QMessageBox::critical(this, "Erro!", "Ocorreu algum erro ao salvar o arquivo.");
    return;
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  QMessageBox::information(this, "Ok!", "Arquivo salvo como " + fileName);
}