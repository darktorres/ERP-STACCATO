#include "cancelaproduto.h"
#include "ui_cancelaproduto.h"

#include "application.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "sqlquery.h"

#include <QSqlError>

CancelaProduto::CancelaProduto(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::CancelaProduto) {
  ui->setupUi(this);

  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  setupTables();

  setConnections();

  show();
}

CancelaProduto::~CancelaProduto() { delete ui; }

void CancelaProduto::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonVoltar, &QPushButton::clicked, this, &CancelaProduto::on_pushButtonVoltar_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CancelaProduto::on_pushButtonSalvar_clicked, connectionType);
}

void CancelaProduto::setFilter(const QString &ordemCompra) {
  if (tipo == Tipo::CompraConfirmar) { model.setFilter("ordemCompra = " + ordemCompra + " AND status = 'EM COMPRA'"); }
  if (tipo == Tipo::CompraFaturamento) { model.setFilter("ordemCompra = " + ordemCompra + " AND status = 'EM FATURAMENTO'"); }

  model.select();
}

void CancelaProduto::setupTables() {
  model.setTable("pedido_fornecedor_has_produto2");

  model.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  model.setHeaderData("idVenda", "Código");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("colecao", "Coleção");
  model.setHeaderData("caixas", "Caixas");
  model.setHeaderData("prcUnitario", "R$ Unit.");
  model.setHeaderData("quant", "Quant.");
  model.setHeaderData("preco", "Total");
  model.setHeaderData("un", "Un.");
  model.setHeaderData("un2", "Un.2");
  model.setHeaderData("kgcx", "Kg./Cx.");
  model.setHeaderData("formComercial", "Formato");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("obs", "Obs.");

  ui->table->setModel(&model);

  ui->table->hideColumn("idPedidoFK");
  ui->table->hideColumn("idRelacionado");
  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("codFornecedor");
  ui->table->hideColumn("idVendaProduto1");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("idPedido2");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("codBarras");
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("status");
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
  ui->table->hideColumn("aliquotaSt");
  ui->table->hideColumn("st");

  ui->table->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("preco", new ReaisDelegate(this));
}

void CancelaProduto::on_pushButtonSalvar_clicked() {
  if (tipo != Tipo::CompraConfirmar and tipo != Tipo::CompraFaturamento) { throw RuntimeException("Não implementado!", this); }

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Não selecionou nenhum produto!", this); }

  qApp->startTransaction("CancelaProduto::on_pushButtonSalvar");

  cancelar(selection);

  qApp->endTransaction();

  qApp->enqueueInformation("Itens cancelados!", this);

  close();
}

void CancelaProduto::on_pushButtonVoltar_clicked() { close(); }

void CancelaProduto::cancelar(const QModelIndexList &list) {
  SqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto2 SET status = 'CANCELADO', idVenda = NULL, idVendaProduto2 = NULL WHERE `idPedido2` = :idPedido2");

  const QString status = (tipo == Tipo::CompraConfirmar) ? "EM COMPRA" : "EM FATURAMENTO";

  SqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto2 SET status = CASE WHEN reposicaoEntrega THEN 'REPO. ENTREGA' WHEN reposicaoReceb THEN 'REPO. RECEB.' ELSE 'PENDENTE' END, idCompra = NULL, "
                     "dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
                     "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE status = '" +
                     status + "' AND `idVendaProduto2` = :idVendaProduto2");

  QStringList idVendas;

  for (const auto &index : list) {
    const int row = index.row();

    queryCompra.bindValue(":idPedido2", model.data(row, "idPedido2"));

    if (not queryCompra.exec()) { throw RuntimeException("Erro atualizando compra: " + queryCompra.lastError().text()); }

    queryVenda.bindValue(":idVendaProduto2", model.data(row, "idVendaProduto2"));

    if (not queryVenda.exec()) { throw RuntimeException("Erro atualizando venda: " + queryVenda.lastError().text()); }

    idVendas << model.data(row, "idVenda").toString();

    SqlQuery query;

    if (not query.exec("CALL update_pedido_fornecedor_status(" + model.data(row, "idPedidoFK").toString() + ")")) {
      throw RuntimeException("Erro atualizando status compra: " + query.lastError().text());
    }
  }

  Sql::updateVendaStatus(idVendas);
}

// TODO: 5verificar como tratar conta_a_pagar_has_pagamento
// TODO: alterar a funcao de cancelar por uma tela de SAC onde o usuario indica as operacoes necessarias (troca de nfe, produto nao disponivel etc) e
// realiza as mudanças necessarias, bem como alteracoes no fluxo de pagamento se necessario
