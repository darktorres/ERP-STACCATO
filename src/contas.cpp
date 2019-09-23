#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QtMath>

#include "application.h"
#include "checkboxdelegate.h"
#include "comboboxdelegate.h"
#include "contas.h"
#include "dateformatdelegate.h"
#include "doubledelegate.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "ui_contas.h"

Contas::Contas(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::Contas) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  showMaximized();

  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &Contas::on_pushButtonSalvar_clicked);
  connect(&modelPendentes, &QSqlTableModel::dataChanged, this, &Contas::preencher);
  connect(&modelPendentes, &QSqlTableModel::dataChanged, this, &Contas::validarData);
}

Contas::~Contas() { delete ui; }

void Contas::validarData(const QModelIndex &index) {
  if (index.column() == modelPendentes.fieldIndex("dataPagamento")) {
    const int idPagamento = modelPendentes.data(index.row(), "idPagamento").toInt();

    QSqlQuery query;
    query.prepare("SELECT dataPagamento FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", idPagamento);

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dataPagamento: " + query.lastError().text(), this); }

    const QDate oldDate = query.value("dataPagamento").toDate();
    const QDate newDate = modelPendentes.data(index.row(), "dataPagamento").toDate();

    if (oldDate.isNull()) { return; }

    // TODO: verificar se essa funcao nao precisa retornar um bool e porque ela nao retorna nos erros
    if (tipo == Tipo::Pagar and (newDate > oldDate.addDays(92) or newDate < oldDate.addDays(-32))) {
      qApp->enqueueError("Limite de alteração de data excedido! Use corrigir fluxo na tela de compras!", this);
      if (not modelPendentes.setData(index.row(), "dataPagamento", oldDate)) { return; }
    }

    if (tipo == Tipo::Receber and (newDate > oldDate.addDays(32) or newDate < oldDate.addDays(-92))) {
      qApp->enqueueError("Limite de alteração de data excedido! Use corrigir fluxo na tela de vendas!", this);
      if (not modelPendentes.setData(index.row(), "dataPagamento", oldDate)) { return; }
    }
  }
}

void Contas::preencher(const QModelIndex &index) {
  if (index.column() == modelPendentes.fieldIndex("valor")) {
    QSqlQuery query;
    query.prepare("SELECT valor FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", modelPendentes.data(index.row(), "idPagamento"));

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando valor: " + query.lastError().text(), this); }

    const double oldValor = query.value("valor").toDouble();
    const double newValor = modelPendentes.data(index.row(), "valor").toDouble();

    if ((oldValor / newValor < 0.99 or oldValor / newValor > 1.01) and qFabs(oldValor - newValor) > 5) {
      qApp->enqueueError("Limite de alteração de valor excedido! Use a função de corrigir fluxo!", this);
      if (not modelPendentes.setData(index.row(), "valor", oldValor)) { return; }
    }
  }

  if (index.column() == modelPendentes.fieldIndex("dataRealizado")) {
    if (not modelPendentes.setData(index.row(), "status", tipo == Tipo::Receber ? "RECEBIDO" : "PAGO")) { return; }
    if (not modelPendentes.setData(index.row(), "valorReal", modelPendentes.data(index.row(), "valor"))) { return; }
    if (not modelPendentes.setData(index.row(), "tipoReal", modelPendentes.data(index.row(), "tipo"))) { return; }
    if (not modelPendentes.setData(index.row(), "parcelaReal", modelPendentes.data(index.row(), "parcela"))) { return; }
    if (not modelPendentes.setData(index.row(), "contaDestino", 3)) { return; }
    if (not modelPendentes.setData(index.row(), "centroCusto", modelPendentes.data(index.row(), "idLoja"))) { return; }

    // -------------------------------------------------------------------------

    const QModelIndexList list = modelPendentes.match("tipo", modelPendentes.data(index.row(), "tipo").toString().left(1) + ". Taxa Cartão", -1);

    for (const auto &indexMatch : list) {
      if (modelPendentes.data(indexMatch.row(), "parcela") != modelPendentes.data(index.row(), "parcela")) { continue; }

      if (not modelPendentes.setData(indexMatch.row(), "dataRealizado", modelPendentes.data(index.row(), "dataRealizado"))) { return; }
      if (not modelPendentes.setData(indexMatch.row(), "status", tipo == Tipo::Receber ? "RECEBIDO" : "PAGO")) { return; }
      if (not modelPendentes.setData(indexMatch.row(), "valorReal", modelPendentes.data(indexMatch.row(), "valor"))) { return; }
      if (not modelPendentes.setData(indexMatch.row(), "tipoReal", modelPendentes.data(indexMatch.row(), "tipo"))) { return; }
      if (not modelPendentes.setData(indexMatch.row(), "parcelaReal", modelPendentes.data(indexMatch.row(), "parcela"))) { return; }
      if (not modelPendentes.setData(indexMatch.row(), "contaDestino", 3)) { return; }
      if (not modelPendentes.setData(indexMatch.row(), "centroCusto", modelPendentes.data(indexMatch.row(), "idLoja"))) { return; }
    }
  }

  if (index.column() != modelPendentes.fieldIndex("dataRealizado")) {
    if (modelPendentes.data(index.row(), "status").toString() == "PENDENTE") {
      if (not modelPendentes.setData(index.row(), "status", "CONFERIDO")) { return; }
    }
  }
}

void Contas::setupTables() {
  modelPendentes.setTable(tipo == Tipo::Receber ? "conta_a_receber_has_pagamento" : "conta_a_pagar_has_pagamento");

  modelPendentes.setHeaderData("dataEmissao", "Data Emissão");
  modelPendentes.setHeaderData("contraParte", "ContraParte");
  modelPendentes.setHeaderData("nfe", "NFe");
  modelPendentes.setHeaderData("valor", "R$");
  modelPendentes.setHeaderData("tipo", "Tipo");
  modelPendentes.setHeaderData("parcela", "Parcela");
  modelPendentes.setHeaderData("dataPagamento", "Vencimento");
  modelPendentes.setHeaderData("observacao", "Obs.");
  modelPendentes.setHeaderData("status", "Status");
  modelPendentes.setHeaderData("dataRealizado", "Data Realizado");
  modelPendentes.setHeaderData("valorReal", "R$ Real");
  modelPendentes.setHeaderData("tipoReal", "Tipo Real");
  modelPendentes.setHeaderData("parcelaReal", "Parcela Real");
  modelPendentes.setHeaderData("contaDestino", "Conta");
  modelPendentes.setHeaderData("tipoDet", "Tipo Det");
  modelPendentes.setHeaderData("centroCusto", "Centro Custo");
  modelPendentes.setHeaderData("grupo", "Grupo");
  modelPendentes.setHeaderData("subGrupo", "SubGrupo");

  modelPendentes.setSort("dataPagamento", Qt::AscendingOrder);

  modelPendentes.proxyModel = new SortFilterProxyModel(&modelPendentes, this);

  ui->tablePendentes->setModel(&modelPendentes);

  ui->tablePendentes->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("dataEmissao", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("contraParte", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("tipo", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("parcela", new NoEditDelegate(this));
  // TODO: 3dateEditDelegate para vencimento
  ui->tablePendentes->setItemDelegateForColumn("dataPagamento", new DateFormatDelegate("dataPagamento", this));
  ui->tablePendentes->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate("dataPagamento", this));

  if (tipo == Tipo::Receber) {
    //    ui->tablePendentes->setItemDelegateForColumn("contraParte", new LineEditDelegate(LineEditDelegate::ContraParteReceber, this));
    ui->tablePendentes->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::StatusReceber, this));
  }

  if (tipo == Tipo::Pagar) {
    //    ui->tablePendentes->setItemDelegateForColumn("contraParte", new LineEditDelegate(LineEditDelegate::ContraPartePagar, this));
    ui->tablePendentes->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::StatusPagar, this));
  }

  ui->tablePendentes->setItemDelegateForColumn("contaDestino", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->tablePendentes->setItemDelegateForColumn("representacao", new CheckBoxDelegate(this, true));
  ui->tablePendentes->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->tablePendentes->setItemDelegateForColumn("grupo", new LineEditDelegate(LineEditDelegate::Tipo::Grupo, this));

  ui->tablePendentes->setPersistentColumns({"status", "contaDestino", "centroCusto"});

  ui->tablePendentes->hideColumn("idCompra");
  ui->tablePendentes->hideColumn("representacao");
  ui->tablePendentes->hideColumn("idPagamento");
  ui->tablePendentes->hideColumn("idVenda");
  ui->tablePendentes->hideColumn("idLoja");
  ui->tablePendentes->hideColumn("created");
  ui->tablePendentes->hideColumn("lastUpdated");
  ui->tablePendentes->hideColumn("comissao");
  ui->tablePendentes->hideColumn("taxa");
  ui->tablePendentes->hideColumn("desativado");

  // -------------------------------------------------------------------------

  modelProcessados.setTable(tipo == Tipo::Receber ? "conta_a_receber_has_pagamento" : "conta_a_pagar_has_pagamento");

  modelProcessados.setHeaderData("dataEmissao", "Data Emissão");
  modelProcessados.setHeaderData("contraParte", "ContraParte");
  modelProcessados.setHeaderData("nfe", "NFe");
  modelProcessados.setHeaderData("valor", "R$");
  modelProcessados.setHeaderData("tipo", "Tipo");
  modelProcessados.setHeaderData("parcela", "Parcela");
  modelProcessados.setHeaderData("dataPagamento", "Vencimento");
  modelProcessados.setHeaderData("observacao", "Obs.");
  modelProcessados.setHeaderData("status", "Status");
  modelProcessados.setHeaderData("dataRealizado", "Data Realizado");
  modelProcessados.setHeaderData("valorReal", "R$ Real");
  modelProcessados.setHeaderData("tipoReal", "Tipo Real");
  modelProcessados.setHeaderData("parcelaReal", "Parcela Real");
  modelProcessados.setHeaderData("contaDestino", "Conta");
  modelProcessados.setHeaderData("tipoDet", "Tipo Det");
  modelProcessados.setHeaderData("centroCusto", "Centro Custo");
  modelProcessados.setHeaderData("grupo", "Grupo");
  modelProcessados.setHeaderData("subGrupo", "SubGrupo");

  modelProcessados.proxyModel = new SortFilterProxyModel(&modelProcessados, this);

  ui->tableProcessados->setModel(&modelProcessados);

  ui->tableProcessados->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tableProcessados->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));

  ui->tableProcessados->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::StatusReceber, this));
  ui->tableProcessados->setItemDelegateForColumn("contaDestino", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, true, this));
  ui->tableProcessados->setItemDelegateForColumn("representacao", new CheckBoxDelegate(this, true));
  ui->tableProcessados->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, true, this));

  ui->tableProcessados->setPersistentColumns({"contaDestino", "centroCusto"});

  ui->tableProcessados->hideColumn("representacao");
  ui->tableProcessados->hideColumn("idPagamento");
  ui->tableProcessados->hideColumn("idVenda");
  ui->tableProcessados->hideColumn("idCompra");
  ui->tableProcessados->hideColumn("idLoja");
  ui->tableProcessados->hideColumn("created");
  ui->tableProcessados->hideColumn("lastUpdated");
  ui->tableProcessados->hideColumn("comissao");
  ui->tableProcessados->hideColumn("taxa");
  ui->tableProcessados->hideColumn("desativado");
}

bool Contas::verifyFields() {
  for (int row = 0; row < modelPendentes.rowCount(); ++row) {
    if ((tipo == Tipo::Pagar and modelPendentes.data(row, "status").toString() == "PAGO") or (tipo == Tipo::Receber and modelPendentes.data(row, "status").toString() == "RECEBIDO")) {
      if (modelPendentes.data(row, "dataRealizado").toString().isEmpty()) { return qApp->enqueueError(false, "'Data Realizado' vazio!", this); }
      if (modelPendentes.data(row, "valorReal").toString().isEmpty()) { return qApp->enqueueError(false, "'R$ Real' vazio!", this); }
      if (modelPendentes.data(row, "tipoReal").toString().isEmpty()) { return qApp->enqueueError(false, "'Tipo Real' vazio!", this); }
      if (modelPendentes.data(row, "parcelaReal").toString().isEmpty()) { return qApp->enqueueError(false, "'Parcela Real' vazio!", this); }
      if (modelPendentes.data(row, "contaDestino").toString().isEmpty()) { return qApp->enqueueError(false, "'Conta Dest.' vazio!", this); }
      if (modelPendentes.data(row, "centroCusto").toString().isEmpty()) { return qApp->enqueueError(false, "'Centro Custo' vazio!", this); }
      if (modelPendentes.data(row, "grupo").toString().isEmpty()) { return qApp->enqueueError(false, "'Grupo' vazio!", this); }
    }
  }

  return true;
}

void Contas::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  if (not modelPendentes.submitAll()) { return; }

  close();
}

void Contas::viewConta(const QString &idPagamento, const QString &contraparte) {
  if (tipo == Tipo::Receber) {
    QSqlQuery query;
    query.prepare("SELECT idVenda FROM conta_a_receber_has_pagamento WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", idPagamento);

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados: " + query.lastError().text(), this); }

    const QString idVenda = query.value("idVenda").toString();

    setWindowTitle("Contas A Receber - " + contraparte + " " + idVenda);

    modelPendentes.setFilter(idVenda.isEmpty() ? "idPagamento = " + idPagamento + " AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE"
                                               : "idVenda LIKE '" + idVenda + "%' AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE");

    // -------------------------------------------------------------------------

    modelProcessados.setFilter(idVenda.isEmpty() ? "idPagamento = " + idPagamento + " AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO') AND representacao = FALSE"
                                                 : "idVenda = '" + idVenda + "' AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO') AND representacao = FALSE");
  }

  if (tipo == Tipo::Pagar) {
    QSqlQuery query;
    query.prepare("SELECT cp.idCompra, pf.ordemCompra FROM conta_a_pagar_has_pagamento cp LEFT JOIN pedido_fornecedor_has_produto pf ON cp.idCompra = pf.idCompra WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", idPagamento);

    if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados: " + query.lastError().text(), this); }

    const QString idCompra = query.value("idCompra").toString();
    const QString ordemCompra = query.value("ordemCompra").toString();

    setWindowTitle("Contas A Pagar - " + contraparte + (ordemCompra == "0" ? "" : " OC " + ordemCompra));

    modelPendentes.setFilter(idCompra == "0" ? "idPagamento = " + idPagamento + " AND status IN ('PENDENTE', 'CONFERIDO')" : "idCompra = '" + idCompra + "' AND status IN ('PENDENTE', 'CONFERIDO')");

    // -------------------------------------------------------------------------

    modelProcessados.setFilter(idCompra == "0" ? "idPagamento = " + idPagamento + " AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO')"
                                               : "idCompra = " + idCompra + " AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO')");
  }

  if (not modelPendentes.select()) { return; }

  // -------------------------------------------------------------------------

  if (not modelProcessados.select()) { return; }
}

// TODO: 5adicionar coluna 'boleto' para dizer onde foi pago
// TODO: 5fazer somatoria dos valores
// TODO: 5mostrar nfe que é mostrada na tela do widgetpagamentos
// TODO: 5verificar centroCusto que usa dois campos (idLoja/centroCusto)
// TODO: 5a funcao de marcar 'conferido' nao deixa voltar para pendente
// TODO: 5funcao de marcar 'conferido' marca na linha de baixo

// FIXME: quando cancelar uma transferencia cancelar a outra ponta tambem
