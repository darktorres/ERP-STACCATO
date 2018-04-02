#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "ui_widgetcompradevolucao.h"
#include "widgetcompradevolucao.h"

WidgetCompraDevolucao::WidgetCompraDevolucao(QWidget *parent) : Widget(parent), ui(new Ui::WidgetCompraDevolucao) {
  ui->setupUi(this);

  connect(ui->pushButtonDevolucaoFornecedor, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked);
  connect(ui->pushButtonRetornarEstoque, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked);
  connect(ui->radioButtonFiltroPendente, &QRadioButton::toggled, this, &WidgetCompraDevolucao::on_radioButtonFiltroPendente_toggled);
  connect(ui->table, &TableView::entered, this, &WidgetCompraDevolucao::on_table_entered);
}

WidgetCompraDevolucao::~WidgetCompraDevolucao() { delete ui; }

bool WidgetCompraDevolucao::updateTables() {
  if (modelVendaProduto.tableName().isEmpty()) {
    setupTables();
    ui->radioButtonFiltroPendente->setChecked(true);
  }

  if (not modelVendaProduto.select()) {
    emit errorSignal("Erro lendo tabela faturamento: " + modelVendaProduto.lastError().text());
    return false;
  }

  ui->table->resizeColumnsToContents();

  return true;
}

void WidgetCompraDevolucao::setupTables() {
  // REFAC: refactor this to not select in here

  modelVendaProduto.setTable("venda_has_produto");
  modelVendaProduto.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelVendaProduto.setHeaderData("status", "Status");
  modelVendaProduto.setHeaderData("statusOriginal", "Status Original");
  modelVendaProduto.setHeaderData("fornecedor", "Fornecedor");
  modelVendaProduto.setHeaderData("idVenda", "Venda");
  modelVendaProduto.setHeaderData("produto", "Produto");
  modelVendaProduto.setHeaderData("obs", "Obs.");
  modelVendaProduto.setHeaderData("caixas", "Cx.");
  modelVendaProduto.setHeaderData("quant", "Quant.");
  modelVendaProduto.setHeaderData("un", "Un.");
  modelVendaProduto.setHeaderData("unCaixa", "Un./Cx.");
  modelVendaProduto.setHeaderData("codComercial", "Cód. Com.");
  modelVendaProduto.setHeaderData("formComercial", "Form. Com.");

  modelVendaProduto.setFilter("0");

  if (not modelVendaProduto.select()) emit errorSignal("Erro lendo tabela produtos pendentes: " + modelVendaProduto.lastError().text());

  ui->table->setModel(&modelVendaProduto);

  ui->table->sortByColumn("idVenda");
  ui->table->hideColumn("recebeu");
  ui->table->hideColumn("entregou");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("idVendaProduto");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idNFeSaida");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("prcUnitario");
  ui->table->hideColumn("descUnitario");
  ui->table->hideColumn("parcial");
  ui->table->hideColumn("desconto");
  ui->table->hideColumn("parcialDesc");
  ui->table->hideColumn("descGlobal");
  ui->table->hideColumn("total");
  ui->table->hideColumn("reposicao");
  ui->table->hideColumn("estoque");
  ui->table->hideColumn("promocao");
  ui->table->hideColumn("mostrarDesconto");
  ui->table->hideColumn("dataPrevCompra");
  ui->table->hideColumn("dataRealCompra");
  ui->table->hideColumn("dataPrevConf");
  ui->table->hideColumn("dataRealConf");
  ui->table->hideColumn("dataPrevFat");
  ui->table->hideColumn("dataRealFat");
  ui->table->hideColumn("dataPrevColeta");
  ui->table->hideColumn("dataRealColeta");
  ui->table->hideColumn("dataPrevReceb");
  ui->table->hideColumn("dataRealReceb");
  ui->table->hideColumn("dataPrevEnt");
  ui->table->hideColumn("dataRealEnt");
  ui->table->resizeColumnsToContents();
}

void WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked() {
  // NOTE: colocar produto no fluxo da logistica para devolver ao fornecedor?
  // TODO: 2criar nota de devolucao caso tenha recebido nota (troca cfop, inverte remetente/destinario, nat. 'devolucao
  // de mercadoria'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Não selecionou nenhuma linha!");
    return;
  }

  for (const auto &item : list) {
    if (not modelVendaProduto.setData(item.row(), "status", "DEVOLVIDO FORN.")) {
      emit errorSignal("Erro marcando status: " + modelVendaProduto.lastError().text());
      return;
    }
  }

  if (not modelVendaProduto.submitAll()) {
    emit errorSignal("Erro salvando status processado: " + modelVendaProduto.lastError().text());
    return;
  }

  emit informationSignal("Retornado para fornecedor!");
}

bool WidgetCompraDevolucao::retornarEstoque(const QModelIndexList &list) {
  // TODO: ao fazer a linha copia da devolucao limpar todas as datas e preencher 'dataRealEnt' com a data em que foi feita o retorno para o estoque

  QSqlQuery query;
  // TODO: add quant too?
  // TODO: e se tiver varios consumos?
  query.prepare("SELECT idVendaProduto FROM venda_has_produto WHERE idVenda = :idVenda AND idProduto = :idProduto");

  for (const auto &item : list) {
    const QString status = modelVendaProduto.data(item.row(), "status").toString();

    if (not modelVendaProduto.setData(item.row(), "status", "DEVOLVIDO ESTOQUE")) { return false; }

    // TODO: 5refazer isso para bloquear o botao
    if (status == "PENDENTE" or status == "INICIADO" or status == "EM COMPRA" or status == "EM FATURAMENTO") {
      // se nao faturado nao faz nada

      // TODO: 0perguntar se quer cancelar o produto correspondente da compra/ ou a compra inteira (verificar pelo
      // idVendaProduto)
      // TODO: 0colocar uma linha de pagamento negativa no fluxo da compra para quando corrigir fluxo ter o valor total
      // alterado
      // TODO: 0criar uma tabelinha de coisas pendentes para o financeiro

    } else {
      //    else if (status == "EM COLETA" or status == "EM RECEBIMENTO" or status == "ESTOQUE") {
      // se faturado criar devolucao estoque_has_consumo
      // 1.procurar em estoque pelo idVendaProduto
      // 2.copiar linha consumo mudando quant, status para devolucao e idCompra 0

      query.bindValue(":idVenda", modelVendaProduto.data(item.row(), "idVenda").toString().left(11));
      query.bindValue(":idProduto", modelVendaProduto.data(item.row(), "idProduto"));

      if (not query.exec() or not query.first()) {
        emit errorSignal("Erro buscando idVendaProduto: " + query.lastError().text());
        return false;
      }

      const QString idVendaProduto = query.value("idVendaProduto").toString();

      SqlRelationalTableModel modelConsumo;
      modelConsumo.setTable("estoque_has_consumo");
      modelConsumo.setFilter("idVendaProduto = " + idVendaProduto);

      if (not modelConsumo.select()) {
        emit errorSignal("Erro buscando consumo estoque: " + modelConsumo.lastError().text());
        return false;
      }

      if (modelConsumo.rowCount() == 0) {
        emit errorSignal("Não encontrou estoque!");
        return false;
      }

      const int newRow = modelConsumo.rowCount();
      modelConsumo.insertRow(newRow);

      for (int column = 0; column < modelConsumo.columnCount(); ++column) {
        if (modelConsumo.fieldIndex("idConsumo") == column) { continue; }
        if (modelConsumo.fieldIndex("idVendaProduto") == column) { continue; }
        if (modelConsumo.fieldIndex("created") == column) { continue; }
        if (modelConsumo.fieldIndex("lastUpdated") == column) { continue; }

        if (not modelConsumo.setData(newRow, column, modelConsumo.data(0, column))) {
          emit errorSignal("Erro copiando dados do consumo: " + modelConsumo.lastError().text());
          return false;
        }
      }

      // TODO: update other fields
      if (not modelConsumo.setData(newRow, "idVendaProduto", modelVendaProduto.data(item.row(), "idVendaProduto"))) { return false; }
      if (not modelConsumo.setData(newRow, "status", "DEVOLVIDO")) { return false; }
      if (not modelConsumo.setData(newRow, "caixas", modelVendaProduto.data(item.row(), "caixas").toDouble() * -1)) { return false; }
      if (not modelConsumo.setData(newRow, "quant", modelVendaProduto.data(item.row(), "quant").toDouble() * -1)) { return false; }
      if (not modelConsumo.setData(newRow, "quantUpd", 5)) { return false; }

      if (not modelConsumo.submitAll()) {
        emit errorSignal("Erro salvando devolução de estoque: " + modelConsumo.lastError().text());
        return false;
      }
    }
  }

  if (not modelVendaProduto.submitAll()) {
    emit errorSignal("Erro salvando status processado: " + modelVendaProduto.lastError().text());
    return false;
  }

  return true;
}

void WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Não selecionou nenhuma linha!");
    return;
  }

  emit transactionStarted();

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) { return; }
  if (not QSqlQuery("START TRANSACTION").exec()) { return; }

  if (not retornarEstoque(list)) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) { return; }

  emit transactionEnded();

  if (not modelVendaProduto.select()) emit errorSignal("Erro lendo tabela: " + modelVendaProduto.lastError().text());

  emit informationSignal("Retornado para estoque!");
}

void WidgetCompraDevolucao::on_radioButtonFiltroPendente_toggled(bool checked) {
  ui->pushButtonDevolucaoFornecedor->setEnabled(checked);
  ui->pushButtonRetornarEstoque->setEnabled(checked);

  modelVendaProduto.setFilter("quant < 0 AND " + QString(checked ? "status != 'DEVOLVIDO ESTOQUE' AND status != 'DEVOLVIDO FORN.'" : "(status = 'DEVOLVIDO ESTOQUE' OR status = 'DEVOLVIDO FORN.')"));

  if (not modelVendaProduto.select()) emit errorSignal("Erro lendo tabela: " + modelVendaProduto.lastError().text());
}

void WidgetCompraDevolucao::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }
