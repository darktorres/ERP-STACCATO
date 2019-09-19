#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "cancelaproduto.h"
#include "sql.h"
#include "ui_cancelaproduto.h"

CancelaProduto::CancelaProduto(const Tipo &tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::CancelaProduto) {
  ui->setupUi(this);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &CancelaProduto::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CancelaProduto::on_pushButtonSalvar_clicked);

  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  setupTables();

  show();
}

CancelaProduto::~CancelaProduto() { delete ui; }

void CancelaProduto::setFilter(const QString &ordemCompra) {
  if (tipo == Tipo::CompraConfirmar) { model.setFilter("ordemCompra = " + ordemCompra + " AND status = 'EM COMPRA'"); }
  if (tipo == Tipo::CompraFaturamento) { model.setFilter("ordemCompra = " + ordemCompra + " AND status = 'EM FATURAMENTO'"); }

  if (not model.select()) { return qApp->enqueueError("Erro carregando tabela: " + model.lastError().text(), this); }
}

void CancelaProduto::setupTables() {
  model.setTable("pedido_fornecedor_has_produto");

  model.setHeaderData("ordemRepresentacao", "Cód. Rep.");
  model.setHeaderData("idVenda", "Código");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("colecao", "Coleção");
  model.setHeaderData("caixas", "Caixas");
  model.setHeaderData("prcUnitario", "$ Unit.");
  model.setHeaderData("quant", "Quant.");
  model.setHeaderData("preco", "Total");
  model.setHeaderData("un", "Un.");
  model.setHeaderData("un2", "Un.2");
  model.setHeaderData("kgcx", "Kg./Cx.");
  model.setHeaderData("formComercial", "Formato");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("obs", "Obs.");

  ui->table->setModel(&model);

  ui->table->hideColumn("selecionado");
  ui->table->hideColumn("idVendaProduto");
  ui->table->hideColumn("statusFinanceiro");
  ui->table->hideColumn("ordemCompra");
  ui->table->hideColumn("quantConsumida");
  ui->table->hideColumn("idNfe");
  ui->table->hideColumn("idEstoque");
  ui->table->hideColumn("quantUpd");
  ui->table->hideColumn("idPedido");
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
}

void CancelaProduto::on_pushButtonSalvar_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Não selecionou nenhum produto!", this); }

  if (not qApp->startTransaction()) { return; }

  if (not cancelar(list)) { return qApp->rollbackTransaction(); }

  if (not qApp->endTransaction()) { return; }

  qApp->enqueueInformation("Itens cancelados!", this);

  close();
}

void CancelaProduto::on_pushButtonCancelar_clicked() { close(); }

bool CancelaProduto::cancelar(const QModelIndexList &list) {
  if (tipo != Tipo::CompraConfirmar and tipo != Tipo::CompraFaturamento) { return qApp->enqueueError(false, "Não implementado!", this); }

  QSqlQuery queryCompra;
  queryCompra.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'CANCELADO' WHERE idPedido = :idPedido");

  const QString status = (tipo == Tipo::CompraConfirmar) ? "EM COMPRA" : "EM FATURAMENTO";

  QSqlQuery queryVenda;
  queryVenda.prepare("UPDATE venda_has_produto SET status = CASE WHEN reposicaoEntrega THEN 'REPO. ENTREGA' WHEN reposicaoReceb THEN 'REPO. RECEB.' ELSE 'PENDENTE' END, idCompra = NULL, "
                     "dataPrevCompra = NULL, dataRealCompra = NULL, dataPrevConf = NULL, dataRealConf = NULL, dataPrevFat = NULL, dataRealFat = NULL, dataPrevColeta = NULL, dataRealColeta = NULL, "
                     "dataPrevReceb = NULL, dataRealReceb = NULL, dataPrevEnt = NULL, dataRealEnt = NULL WHERE status = '" +
                     status + "' AND idVendaProduto = :idVendaProduto");

  for (const auto &index : list) {
    const int row = index.row();

    queryCompra.bindValue(":idPedido", model.data(row, "idPedido"));

    if (not queryCompra.exec()) { return qApp->enqueueError(false, "Erro atualizando compra: " + queryCompra.lastError().text(), this); }

    queryVenda.bindValue(":idVendaProduto", model.data(row, "idVendaProduto"));

    if (not queryVenda.exec()) { return qApp->enqueueError(false, "Erro atualizando venda: " + queryVenda.lastError().text(), this); }
  }

  return true;
}

// TODO: 5verificar como tratar conta_a_pagar_has_pagamento
// TODO: alterar a funcao de cancelar por uma tela de SAC onde o usuario indica as operacoes necessarias (troca de nfe, produto nao disponivel etc) e
// realiza as mudanças necessarias, bem como alteracoes no fluxo de pagamento se necessario
