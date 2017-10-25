#include <QDate>
#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

#include "checkboxdelegate.h"
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

  for (const auto &item : list) preco += modelProdutos.data(item.row(), "preco").toDouble();

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

bool WidgetCompraGerar::updateTables() {
  if (modelResumo.tableName().isEmpty()) setupTables();

  const auto selection = ui->tableResumo->selectionModel()->selectedRows();

  const auto index = selection.size() > 0 ? selection.first() : QModelIndex();

  const QString filter = modelResumo.filter();

  if (not modelResumo.select()) {
    emit errorSignal("Erro lendo tabela fornecedores: " + modelResumo.lastError().text());
    return false;
  }

  modelResumo.setFilter(filter);

  ui->tableResumo->resizeColumnsToContents();

  const QString filter2 = modelProdutos.filter();

  if (not modelProdutos.select()) {
    emit errorSignal("Erro lendo tabela pedido_fornecedor_has_produto: " + modelProdutos.lastError().text());
    return false;
  }

  modelProdutos.setFilter(filter2);

  if (selection.size() > 0) {
    on_tableResumo_activated(index);
    ui->tableResumo->selectRow(index.row());
  }

  return true;
}

bool WidgetCompraGerar::gerarCompra(const QList<int> &lista, const QDateTime &dataCompra, const QDateTime &dataPrevista) {
  QStringList produtos;

  for (const auto &row : lista) {
    produtos.append(modelProdutos.data(row, "descricao").toString() + ", Quant: " + modelProdutos.data(row, "quant").toString() + ", R$ " +
                    modelProdutos.data(row, "preco").toString().replace(".", ","));
  }

  for (const auto &row : lista) {
    if (not modelProdutos.setData(row, "status", "EM COMPRA")) return false;

    QSqlQuery queryId;

    if (not queryId.exec("SELECT COALESCE(MAX(idCompra), 0) + 1 AS idCompra FROM pedido_fornecedor_has_produto") or not queryId.first()) {
      error = "Erro buscando idCompra: " + queryId.lastError().text();
      return false;
    }

    const QString id = queryId.value("idCompra").toString();

    if (not modelProdutos.setData(row, "idCompra", id)) return false;
    if (not modelProdutos.setData(row, "ordemCompra", oc)) return false;
    if (not modelProdutos.setData(row, "dataRealCompra", dataCompra)) return false;
    if (not modelProdutos.setData(row, "dataPrevConf", dataPrevista)) return false;

    // salvar status na venda

    if (modelProdutos.data(row, "idVendaProduto").toInt() != 0) {
      QSqlQuery queryVenda;
      queryVenda.prepare("UPDATE venda_has_produto SET status = 'EM COMPRA', idCompra = :idCompra, dataRealCompra = "
                         ":dataRealCompra, dataPrevConf = :dataPrevConf WHERE idVendaProduto = :idVendaProduto");
      queryVenda.bindValue(":idCompra", id);
      queryVenda.bindValue(":dataRealCompra", dataCompra);
      queryVenda.bindValue(":dataPrevConf", dataPrevista);
      queryVenda.bindValue(":idVendaProduto", modelProdutos.data(row, "idVendaProduto"));

      if (not queryVenda.exec()) {
        error = "Erro atualizando status da venda: " + queryVenda.lastError().text();
        return false;
      }
    }
  }

  if (not modelProdutos.submitAll()) {
    error = "Erro salvando dados da tabela pedido_fornecedor_has_produto: " + modelProdutos.lastError().text();
    return false;
  }

  return true;
}

void WidgetCompraGerar::on_pushButtonGerarCompra_clicked() {
  // TODO: 1refatorar essa funcao, dividir em funcoes menores etc

  if (UserSession::settings("User/ComprasFolder").toString().isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Por favor selecione uma pasta para salvar os arquivos nas configurações do usuário!");
    return;
  }

  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Aviso!", "Nenhum item selecionado!");
    return;
  }

  QList<int> lista;
  QStringList ids;

  for (const auto &index : list) {
    lista.append(index.row());
    ids.append(modelProdutos.data(index.row(), "idPedido").toString());
  }

  InputDialogProduto inputDlg(InputDialogProduto::Tipo::GerarCompra);
  if (not inputDlg.setFilter(ids)) return;

  if (inputDlg.exec() != InputDialogProduto::Accepted) return;

  const QDateTime dataCompra = inputDlg.getDate();
  const QDateTime dataPrevista = inputDlg.getNextDate();

  // oc

  QSqlQuery queryOC;

  if (not queryOC.exec("SELECT COALESCE(MAX(ordemCompra), 0) + 1 AS ordemCompra FROM pedido_fornecedor_has_produto") or not queryOC.first()) {
    error = "Erro buscando próximo O.C.!";
    return;
  }

  oc = queryOC.value("ordemCompra").toInt();

  QInputDialog input;
  input.setInputMode(QInputDialog::IntInput);
  input.setCancelButtonText("Cancelar");
  input.setIntMinimum(0);
  input.setIntMaximum(99999);
  input.setIntValue(oc);
  input.setLabelText("Qual a OC?");

  if (input.exec() != QInputDialog::Accepted) return;

  oc = input.intValue();

  bool ok = false;

  while (not ok) {
    queryOC.prepare("SELECT ordemCompra FROM pedido_fornecedor_has_produto WHERE ordemCompra = :ordemCompra LIMIT 1");
    queryOC.bindValue(":ordemCompra", oc);

    if (not queryOC.exec()) {
      error = "Erro buscando O.C.!";
      return;
    }

    if (not queryOC.first()) {
      ok = true;
    } else {
      QMessageBox msgBox(QMessageBox::Question, "Atenção!", "OC já existe! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
      msgBox.setButtonText(QMessageBox::Yes, "Continuar");
      msgBox.setButtonText(QMessageBox::No, "Voltar");

      if (msgBox.exec() == QMessageBox::Yes) {
        ok = true;
      } else {
        if (input.exec() != QInputDialog::Accepted) return;
        oc = input.intValue();
      }
    }
  }

  // enviar email

  QSqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE razaoSocial = :razaoSocial");
  query.bindValue(":razaoSocial", modelProdutos.data(0, "fornecedor").toString());

  if (not query.exec() or not query.first()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando dados do fornecedor: " + query.lastError().text());
    return;
  }

  QString anexo;

  const bool isRepresentacao = query.value("representacao").toBool();

  if (not gerarExcel(lista, anexo, isRepresentacao)) return;

  QMessageBox msgBox(QMessageBox::Question, "Enviar E-mail?", "Deseja enviar e-mail?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Enviar");
  msgBox.setButtonText(QMessageBox::No, "Pular");

  if (msgBox.exec() == QMessageBox::Yes) {
    const int row = ui->tableResumo->selectionModel()->selectedRows().first().row();
    const QString fornecedor = modelResumo.data(row, "fornecedor").toString();

    auto *mail = new SendMail(SendMail::Tipo::GerarCompra, anexo, fornecedor, this);
    mail->setAttribute(Qt::WA_DeleteOnClose);

    mail->exec();
  }

  //

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not gerarCompra(lista, dataCompra, dataPrevista)) {
    QSqlQuery("ROLLBACK").exec();
    if (not error.isEmpty()) QMessageBox::critical(this, "Erro!", error);
    return;
  }

  QSqlQuery("COMMIT").exec();

  if (not query.exec("CALL update_venda_status()")) {
    QMessageBox::critical(this, "Erro!", "Erro atualizando status das vendas: " + query.lastError().text());
    return;
  }

  updateTables();
  QMessageBox::information(this, "Aviso!", "Compra gerada com sucesso!");
}

bool WidgetCompraGerar::gerarExcel(const QList<int> &lista, QString &anexo, const bool isRepresentacao) {
  if (isRepresentacao) {
    if (modelProdutos.data(lista.first(), "idVenda").toString().isEmpty()) {
      QMessageBox::critical(this, "Erro!", "'Venda' vazio!");
      return false;
    }

    Excel excel(modelProdutos.data(lista.at(0), "idVenda").toString());
    const QString representacao = "OC " + QString::number(oc) + " " + modelProdutos.data(lista.first(), "idVenda").toString() + " " + modelProdutos.data(lista.first(), "fornecedor").toString();
    if (not excel.gerarExcel(oc, true, representacao)) return false;
    anexo = excel.getFileName();
    return true;
  }

  const QString fornecedor = modelProdutos.data(lista.at(0), "fornecedor").toString();
  const QString idVenda = modelProdutos.data(lista.at(0), "idVenda").toString();

  const QString arquivoModelo = "modelo compras.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    QMessageBox::critical(this, "Erro!", "Não encontrou o modelo do Excel!");
    return false;
  }

  const QString fileName = UserSession::settings("User/ComprasFolder").toString() + "/" + QString::number(oc) + " " + idVenda + " " + fornecedor + ".xlsx";

  anexo = fileName;

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) {
    QMessageBox::critical(this, "Erro!", "Não foi possível abrir o arquivo para escrita: " + fileName);
    QMessageBox::critical(this, "Erro!", "Erro: " + file.errorString());
    return false;
  }

  file.close();

  QSqlQuery queryFornecedor;
  queryFornecedor.prepare("SELECT contatoNome FROM fornecedor WHERE razaoSocial = :razaoSocial");
  queryFornecedor.bindValue(":razaoSocial", fornecedor);

  if (not queryFornecedor.exec()) {
    QMessageBox::critical(this, "Erro!", "Erro buscando contato do fornecedor: " + queryFornecedor.lastError().text());
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

  for (int row = lista.size() + 13; row < 200; ++row) xlsx.setRowHeight(row, 0);

  if (not xlsx.saveAs(fileName)) {
    QMessageBox::critical(this, "Erro!", "Ocorreu algum erro ao salvar o arquivo.");
    return false;
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  QMessageBox::information(this, "Ok!", "Arquivo salvo como " + fileName);

  return true;
}

void WidgetCompraGerar::on_checkBoxMarcarTodos_clicked(const bool checked) { checked ? ui->tableProdutos->selectAll() : ui->tableProdutos->clearSelection(); }

void WidgetCompraGerar::on_tableResumo_activated(const QModelIndex &index) {
  const QString fornecedor = modelResumo.data(index.row(), "fornecedor").toString();
  const bool checked = ui->checkBoxMostrarSul->isChecked();

  modelProdutos.setFilter("fornecedor = '" + fornecedor + "' AND status = 'PENDENTE' AND " +
                          QString(checked ? "(idVenda LIKE 'CAMB%' OR idVenda IS NULL)" : "(idVenda NOT LIKE 'CAMB%' OR idVenda IS NULL)"));

  if (not modelProdutos.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela pedido_fornecedor_has_produto: " + modelProdutos.lastError().text());
    return;
  }

  ui->tableProdutos->resizeColumnsToContents();
}

void WidgetCompraGerar::on_tableProdutos_entered(const QModelIndex &) { ui->tableProdutos->resizeColumnsToContents(); }

bool WidgetCompraGerar::cancelar(const QModelIndexList &list) {
  for (const auto &index : list) {
    if (not modelProdutos.setData(index.row(), "status", "CANCELADO")) return false;

    QSqlQuery query;
    query.prepare("UPDATE venda_has_produto SET status = 'PENDENTE' WHERE idVendaProduto = :idVendaProduto AND status = 'INICIADO'");
    query.bindValue(":idVendaProduto", modelProdutos.data(index.row(), "idVendaProduto"));

    if (not query.exec()) {
      error = "Erro voltando status do produto: " + query.lastError().text();
      return false;
    }
  }

  if (not modelProdutos.submitAll()) {
    error = "Erro salvando dados: " + modelProdutos.lastError().text();
    return false;
  }

  return true;
}

void WidgetCompraGerar::on_pushButtonCancelarCompra_clicked() {
  const auto list = ui->tableProdutos->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    QMessageBox::critical(this, "Erro!", "Nenhum item selecionado!");
    return;
  }

  QMessageBox msgBox(QMessageBox::Question, "Cancelar?", "Tem certeza que deseja cancelar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cancelar");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::No) return;

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not cancelar(list)) {
    QSqlQuery("ROLLBACK").exec();
    if (not error.isEmpty()) QMessageBox::critical(this, "Erro!", error);
    return;
  }

  QSqlQuery("COMMIT").exec();

  updateTables();
  QMessageBox::information(this, "Aviso!", "Itens cancelados!");
}

// TODO: 2vincular compras geradas com loja selecionada em configuracoes
// TODO: 5colocar tamanho minimo da tabela da esquerda para mostrar todas as colunas

void WidgetCompraGerar::on_checkBoxMostrarSul_toggled(bool checked) {
  modelResumo.setFilter(checked ? "(idVenda LIKE '%CAMB%')" : "(idVenda NOT LIKE '%CAMB%' OR idVenda IS NULL)");

  if (not modelResumo.select()) QMessageBox::critical(this, "Erro!", "Erro ao ler tabela: " + modelResumo.lastError().text());
}

// TODO: avulso
