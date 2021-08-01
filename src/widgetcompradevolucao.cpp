#include "widgetcompradevolucao.h"
#include "ui_widgetcompradevolucao.h"

#include "application.h"
#include "followup.h"
#include "sql.h"

#include <QDate>
#include <QDebug>
#include <QSqlError>

WidgetCompraDevolucao::WidgetCompraDevolucao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraDevolucao) { ui->setupUi(this); }

WidgetCompraDevolucao::~WidgetCompraDevolucao() { delete ui; }

void WidgetCompraDevolucao::resetTables() { modelIsSet = false; }

void WidgetCompraDevolucao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonDevolucaoFornecedor, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonRetornarEstoque, &QPushButton::clicked, this, &WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked, connectionType);
  connect(ui->radioButtonFiltroDevolvido, &QRadioButton::clicked, this, &WidgetCompraDevolucao::on_radioButtonFiltroDevolvido_clicked, connectionType);
  connect(ui->radioButtonFiltroPendente, &QRadioButton::clicked, this, &WidgetCompraDevolucao::on_radioButtonFiltroPendente_clicked, connectionType);
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

  modelVendaProduto.select();
}

void WidgetCompraDevolucao::setupTables() {
  modelVendaProduto.setTable("venda_has_produto2");

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
  modelVendaProduto.setHeaderData("quantCaixa", "Quant./Cx.");
  modelVendaProduto.setHeaderData("codComercial", "Cód. Com.");
  modelVendaProduto.setHeaderData("formComercial", "Form. Com.");

  ui->table->setModel(&modelVendaProduto);

  ui->table->hideColumn("idNFeEntrada");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("idVendaProdutoFK");
  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("entregou");
  ui->table->hideColumn("recebeu");
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
  ui->table->hideColumn("mostrarDesconto");
  ui->table->hideColumn("estoque");
  ui->table->hideColumn("promocao");
  ui->table->hideColumn("reposicaoEntrega");
  ui->table->hideColumn("reposicaoReceb");
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

  connect(ui->table->selectionModel(), &QItemSelectionModel::selectionChanged, this, &WidgetCompraDevolucao::on_table_selectionChanged, Qt::ConnectionType(Qt::AutoConnection | Qt::UniqueConnection));
}

void WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor_clicked() {
  // NOTE: colocar produto no fluxo da logistica para devolver ao fornecedor?
  // TODO: 2criar nota de devolucao caso tenha recebido nota (troca cfop, inverte remetente/destinario, nat. 'devolucao
  // de mercadoria'

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Não selecionou nenhuma linha!", this); }

  const QString idVenda = modelVendaProduto.data(list.first().row(), "idVenda").toString();

  qApp->startTransaction("WidgetCompraDevolucao::on_pushButtonDevolucaoFornecedor");

  retornarFornecedor(list);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  updateTables();

  qApp->enqueueInformation("Retornado para fornecedor!", this);
}

void WidgetCompraDevolucao::retornarEstoque(const QModelIndexList &list) {
  // TODO: ao fazer a linha copia da devolucao limpar todas as datas e preencher 'dataRealEnt' com a data em que foi feita o retorno para o estoque

  // TODO: ao retornar para estoque criar linha livre no pf2 para consumos posteriores (ou agrupar com linha livre existente)

  for (const auto &index : list) {
    const int row = index.row();

    modelVendaProduto.setData(row, "status", "DEVOLVIDO ESTOQUE");

    const QString idRelacionado = modelVendaProduto.data(row, "idRelacionado").toString();

    // TODO: 0perguntar se quer cancelar o produto correspondente da compra/ ou a compra inteira (verificar pelo idVendaProduto)
    // TODO: 0colocar uma linha de pagamento negativa no fluxo da compra para quando corrigir fluxo ter o valor total alterado
    // TODO: 0criar uma tabelinha de coisas pendentes para o financeiro

    // NOTE: *quebralinha estoque_consumo
    SqlTableModel modelConsumo;
    modelConsumo.setTable("estoque_has_consumo");
    modelConsumo.setFilter("idVendaProduto2 = " + idRelacionado);

    modelConsumo.select();

    if (modelConsumo.rowCount() == 0) { throw RuntimeException("Não encontrou consumo!"); }

    const int newRow = modelConsumo.insertRowAtEnd();

    for (int column = 0; column < modelConsumo.columnCount(); ++column) {
      if (column == modelConsumo.fieldIndex("idConsumo")) { continue; }
      if (column == modelConsumo.fieldIndex("idVendaProduto2")) { continue; }
      if (column == modelConsumo.fieldIndex("created")) { continue; }
      if (column == modelConsumo.fieldIndex("lastUpdated")) { continue; }

      const QVariant value = modelConsumo.data(0, column);

      if (value.isNull()) { continue; }

      modelConsumo.setData(newRow, column, value);
    }

    modelConsumo.setData(newRow, "idVendaProduto2", modelVendaProduto.data(row, "idVendaProduto2"));
    modelConsumo.setData(newRow, "status", "DEVOLVIDO");
    modelConsumo.setData(newRow, "quant", modelVendaProduto.data(row, "quant").toDouble() * -1);
    modelConsumo.setData(newRow, "quantUpd", 5);
    modelConsumo.setData(newRow, "caixas", modelVendaProduto.data(row, "caixas").toDouble() * -1);

    modelConsumo.submitAll();
  }

  modelVendaProduto.submitAll();
}

void WidgetCompraDevolucao::retornarFornecedor(const QModelIndexList &list) {
  // se for devolucao fornecedor na logica nova nao havera linha de consumo, a quant. simplesmente mantem o consumo negativo na linha original de DEVOLVIDO

  for (const auto &index : list) { modelVendaProduto.setData(index.row(), "status", "DEVOLVIDO FORN."); }

  modelVendaProduto.submitAll();
}

void WidgetCompraDevolucao::on_pushButtonRetornarEstoque_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Não selecionou nenhuma linha!", this); }

  const QString idVenda = modelVendaProduto.data(list.first().row(), "idVenda").toString();

  qApp->startTransaction("WidgetCompraDevolucao::on_pushButtonRetornarEstoque");

  retornarEstoque(list);

  Sql::updateVendaStatus(idVenda);

  qApp->endTransaction();

  modelVendaProduto.select();

  qApp->enqueueInformation("Retornado para estoque!", this);
}

void WidgetCompraDevolucao::on_radioButtonFiltroPendente_clicked() { montaFiltro(); }

void WidgetCompraDevolucao::on_radioButtonFiltroDevolvido_clicked() { montaFiltro(); }

void WidgetCompraDevolucao::montaFiltro() {
  const bool isPendente = ui->radioButtonFiltroPendente->isChecked();

  ui->pushButtonDevolucaoFornecedor->setEnabled(isPendente);
  ui->pushButtonRetornarEstoque->setEnabled(isPendente);

  modelVendaProduto.setFilter("quant < 0 AND " + QString(isPendente ? "status = 'PENDENTE DEV.'" : "status != 'PENDENTE DEV.'"));
}

void WidgetCompraDevolucao::on_table_selectionChanged() {
  const auto list = ui->table->selectionModel()->selectedRows();

  QStringList status;

  for (auto index : list) { status << modelVendaProduto.data(index.row(), "statusOriginal").toString(); }

  status.removeDuplicates();

  if (status.contains("PENDENTE") or status.contains("INICIADO") or status.contains("EM COMPRA") or status.contains("EM FATURAMENTO")) {
    ui->pushButtonRetornarEstoque->setDisabled(true);
  } else {
    ui->pushButtonRetornarEstoque->setEnabled(true);
  }
}

void WidgetCompraDevolucao::on_pushButtonFollowup_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = modelVendaProduto.data(list.first().row(), "idVenda").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}
