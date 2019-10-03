#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "ui_widgetcompradevolucao.h"
#include "widgetcompradevolucao.h"

WidgetCompraDevolucao::WidgetCompraDevolucao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraDevolucao) { ui->setupUi(this); }

WidgetCompraDevolucao::~WidgetCompraDevolucao() { delete ui; }

void WidgetCompraDevolucao::resetTables() { modelIsSet = false; }

void WidgetCompraDevolucao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonDevolucaoFornecedor, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked, connectionType);
  connect(ui->pushButtonRetornarEstoque, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked, connectionType);
  connect(ui->radioButtonFiltroPendente, &QRadioButton::clicked, this, &WidgetCompraDevolucao::on_radioButtonFiltroPendente_clicked, connectionType);
  connect(ui->radioButtonFiltroDevolvido, &QRadioButton::clicked, this, &WidgetCompraDevolucao::on_radioButtonFiltroDevolvido_clicked, connectionType);
}

void WidgetCompraDevolucao::updateTables() {
  if (not isSet) {
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  if (not modelVendaProduto.select()) { return; }
}

void WidgetCompraDevolucao::setupTables() {
  modelVendaProduto.setTable("venda_has_produto");

  modelVendaProduto.setSort("idVenda");

  modelVendaProduto.setHeaderData("status", "Status");
  modelVendaProduto.setHeaderData("statusOriginal", "Status Original");
  modelVendaProduto.setHeaderData("fornecedor", "Fornecedor");
  modelVendaProduto.setHeaderData("idVenda", "Venda");
  modelVendaProduto.setHeaderData("produto", "Produto");
  modelVendaProduto.setHeaderData("obs", "Obs.");
  modelVendaProduto.setHeaderData("lote", "Lote");
  modelVendaProduto.setHeaderData("caixas", "Cx.");
  modelVendaProduto.setHeaderData("quant", "Quant.");
  modelVendaProduto.setHeaderData("un", "Un.");
  modelVendaProduto.setHeaderData("unCaixa", "Un./Cx.");
  modelVendaProduto.setHeaderData("codComercial", "Cód. Com.");
  modelVendaProduto.setHeaderData("formComercial", "Form. Com.");

  ui->table->setModel(&modelVendaProduto);

  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("recebeu");
  ui->table->hideColumn("entregou");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("idVendaProduto1");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idNFeSaida");
  ui->table->hideColumn("idNFeFutura");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("prcUnitario");
  ui->table->hideColumn("descUnitario");
  ui->table->hideColumn("parcial");
  ui->table->hideColumn("desconto");
  ui->table->hideColumn("parcialDesc");
  ui->table->hideColumn("descGlobal");
  ui->table->hideColumn("total");
  ui->table->hideColumn("reposicaoEntrega");
  ui->table->hideColumn("reposicaoReceb");
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
}

void WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked() {
  // NOTE: colocar produto no fluxo da logistica para devolver ao fornecedor?
  // TODO: 2criar nota de devolucao caso tenha recebido nota (troca cfop, inverte remetente/destinario, nat. 'devolucao
  // de mercadoria'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma linha!", this); }

  for (const auto &item : list) {
    if (not modelVendaProduto.setData(item.row(), "status", "DEVOLVIDO FORN.")) { return; }
  }

  if (not modelVendaProduto.submitAll()) { return; }

  qApp->enqueueInformation("Retornado para fornecedor!", this);
}

bool WidgetCompraDevolucao::retornarEstoque(const QModelIndexList &list) {
  // TODO: ao fazer a linha copia da devolucao limpar todas as datas e preencher 'dataRealEnt' com a data em que foi feita o retorno para o estoque

  QSqlQuery query;
  // TODO: add quant too?
  // TODO: e se tiver varios consumos?
  query.prepare("SELECT `idVendaProduto2` FROM venda_has_produto2 WHERE idVenda = :idVenda AND idProduto = :idProduto");

  for (const auto &item : list) {
    const QString status = modelVendaProduto.data(item.row(), "statusOriginal").toString();

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

      if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando idVendaProduto2: " + query.lastError().text(), this); }

      const QString idVendaProduto2 = query.value("idVendaProduto2").toString();

      SqlRelationalTableModel modelConsumo;
      modelConsumo.setTable("estoque_has_consumo");
      modelConsumo.setFilter("idVendaProduto2 = " + idVendaProduto2);

      if (not modelConsumo.select()) { return false; }

      if (modelConsumo.rowCount() == 0) { return qApp->enqueueError(false, "Não encontrou estoque!", this); }

      const int newRow = modelConsumo.insertRowAtEnd();

      for (int column = 0; column < modelConsumo.columnCount(); ++column) {
        if (modelConsumo.fieldIndex("idConsumo") == column) { continue; }
        if (modelConsumo.fieldIndex("idVendaProduto") == column) { continue; }
        if (modelConsumo.fieldIndex("created") == column) { continue; }
        if (modelConsumo.fieldIndex("lastUpdated") == column) { continue; }

        const QVariant value = modelConsumo.data(0, column);

        if (not modelConsumo.setData(newRow, column, value)) { return false; }
      }

      // TODO: update other fields
      if (not modelConsumo.setData(newRow, "idVendaProduto2", modelVendaProduto.data(item.row(), "idVendaProduto2"))) { return false; }
      if (not modelConsumo.setData(newRow, "status", "DEVOLVIDO")) { return false; }
      if (not modelConsumo.setData(newRow, "caixas", modelVendaProduto.data(item.row(), "caixas").toDouble() * -1)) { return false; }
      if (not modelConsumo.setData(newRow, "quant", modelVendaProduto.data(item.row(), "quant").toDouble() * -1)) { return false; }
      if (not modelConsumo.setData(newRow, "quantUpd", 5)) { return false; }

      if (not modelConsumo.submitAll()) { return false; }
    }
  }

  return modelVendaProduto.submitAll();
}

void WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhuma linha!", this); }

  if (not qApp->startTransaction()) { return; }

  if (not retornarEstoque(list)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  if (not modelVendaProduto.select()) { return; }

  qApp->enqueueInformation("Retornado para estoque!", this);
}

void WidgetCompraDevolucao::on_radioButtonFiltroPendente_clicked(const bool) { montaFiltro(); }

void WidgetCompraDevolucao::on_radioButtonFiltroDevolvido_clicked(const bool) { montaFiltro(); }

void WidgetCompraDevolucao::montaFiltro() {
  const bool isPendente = ui->radioButtonFiltroPendente->isChecked();

  ui->pushButtonDevolucaoFornecedor->setEnabled(isPendente);
  ui->pushButtonRetornarEstoque->setEnabled(isPendente);

  modelVendaProduto.setFilter("quant < 0 AND " + QString(isPendente ? "status = 'PENDENTE DEV.'" : "status != 'PENDENTE DEV.'"));
}
