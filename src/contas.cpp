#include "contas.h"
#include "ui_contas.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QtMath>

Contas::Contas(const Tipo tipo, QWidget *parent) : QDialog(parent), tipo(tipo), ui(new Ui::Contas) {
  ui->setupUi(this);

  if (tipo == Tipo::Pagar) { setWindowTitle("Contas a pagar"); }
  if (tipo == Tipo::Receber) { setWindowTitle("Contas a receber"); }

  setWindowFlags(Qt::Window);

  setupTables();

  showMaximized();

  setConnections();
}

Contas::~Contas() { delete ui; }

void Contas::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &Contas::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->tablePendentes->model(), &QAbstractItemModel::dataChanged, this, &Contas::preencher, connectionType);
}

void Contas::unsetConnections() {
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &Contas::on_pushButtonSalvar_clicked);
  disconnect(ui->tablePendentes->model(), &QAbstractItemModel::dataChanged, this, &Contas::preencher);
}

bool Contas::validarData(const QModelIndex &index) {
  if (index.column() == ui->tablePendentes->columnIndex("dataPagamento")) {
    const int row = index.row();
    const int idPagamento = modelPendentes.data(row, "idPagamento").toInt();

    QSqlQuery query;
    query.prepare("SELECT dataPagamento FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", idPagamento);

    if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dataPagamento: " + query.lastError().text(), this); }

    const QDate oldDate = query.value("dataPagamento").toDate();
    const QDate newDate = modelPendentes.data(row, "dataPagamento").toDate();

    if (oldDate.isNull()) { return true; }

    if (tipo == Tipo::Pagar and (newDate > oldDate.addDays(92) or newDate < oldDate.addDays(-32))) {
      if (not modelPendentes.setData(row, "dataPagamento", oldDate)) { return false; }
      return qApp->enqueueError(false, "Limite de alteração de data excedido! Use corrigir fluxo na tela de compras!", this);
    }

    if (tipo == Tipo::Receber and (newDate > oldDate.addDays(32) or newDate < oldDate.addDays(-92))) {
      if (not modelPendentes.setData(row, "dataPagamento", oldDate)) { return false; }
      return qApp->enqueueError(false, "Limite de alteração de data excedido! Use corrigir fluxo na tela de vendas!", this);
    }
  }

  return true;
}

void Contas::preencher(const QModelIndex &index) {
  if (not validarData(index)) { return; }

  unsetConnections();

  [&] {
    const int row = index.row();

    if (index.column() == ui->tablePendentes->columnIndex("valor")) {
      QSqlQuery query;
      query.prepare("SELECT valor FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
      query.bindValue(":idPagamento", modelPendentes.data(row, "idPagamento"));

      if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando valor: " + query.lastError().text(), this); }

      const double oldValor = query.value("valor").toDouble();
      const double newValor = modelPendentes.data(row, "valor").toDouble();

      if ((oldValor / newValor < 0.99 or oldValor / newValor > 1.01) and qFabs(oldValor - newValor) > 5) {
        qApp->enqueueError("Limite de alteração de valor excedido! Use a função de corrigir fluxo!", this);
        if (not modelPendentes.setData(row, "valor", oldValor)) { return; }
      }
    }

    if (index.column() == ui->tablePendentes->columnIndex("dataRealizado")) {
      const int contaSantander = 3;
      const int contaItau = 33;

      const int idConta = (tipo == Tipo::Receber and modelPendentes.data(row, "tipo").toString().contains("BOLETO")) ? contaItau : contaSantander;

      if (not modelPendentes.setData(row, "status", (tipo == Tipo::Receber) ? "RECEBIDO" : "PAGO")) { return; }
      if (not modelPendentes.setData(row, "valorReal", modelPendentes.data(row, "valor"))) { return; }
      if (not modelPendentes.setData(row, "tipoReal", modelPendentes.data(row, "tipo"))) { return; }
      if (not modelPendentes.setData(row, "parcelaReal", modelPendentes.data(row, "parcela"))) { return; }
      if (not modelPendentes.setData(row, "idConta", idConta)) { return; }
      if (not modelPendentes.setData(row, "centroCusto", modelPendentes.data(row, "idLoja"))) { return; }

      // -------------------------------------------------------------------------

      const auto list = modelPendentes.multiMatch({{"tipo", modelPendentes.data(row, "tipo").toString().left(1) + ". TAXA CARTÃO"}, {"parcela", modelPendentes.data(row, "parcela")}});

      for (const auto &rowMatch : list) {
        if (not modelPendentes.setData(rowMatch, "dataRealizado", modelPendentes.data(row, "dataRealizado"))) { return; }
        if (not modelPendentes.setData(rowMatch, "status", (tipo == Tipo::Receber) ? "RECEBIDO" : "PAGO")) { return; }
        if (not modelPendentes.setData(rowMatch, "valorReal", modelPendentes.data(rowMatch, "valor"))) { return; }
        if (not modelPendentes.setData(rowMatch, "tipoReal", modelPendentes.data(rowMatch, "tipo"))) { return; }
        if (not modelPendentes.setData(rowMatch, "parcelaReal", modelPendentes.data(rowMatch, "parcela"))) { return; }
        if (not modelPendentes.setData(rowMatch, "idConta", idConta)) { return; }
        if (not modelPendentes.setData(rowMatch, "centroCusto", modelPendentes.data(rowMatch, "idLoja"))) { return; }
      }
    }

    // buscar linha da taxa cartao e alterar a conta para ser igual
    if (index.column() == ui->tablePendentes->columnIndex("idConta")) {
      if (index.data() == 0) { return; } // for dealing with ItemBox editor emiting signal when mouseOver

      const auto list = modelPendentes.multiMatch({{"tipo", modelPendentes.data(row, "tipo").toString().left(1) + ". TAXA CARTÃO"}, {"parcela", modelPendentes.data(row, "parcela")}});

      for (const auto &rowMatch : list) {
        if (not modelPendentes.setData(rowMatch, "idConta", modelPendentes.data(row, "idConta"))) { return; }
      }
    }

    if (index.column() != ui->tablePendentes->columnIndex("dataRealizado")) {
      if (index.data().toString() == "PENDENTE") { return; }

      if (modelPendentes.data(row, "status").toString() == "PENDENTE") {
        if (not modelPendentes.setData(row, "status", "CONFERIDO")) { return; }
      }
    }
  }();

  setConnections();
}

void Contas::setupTables() {
  if (tipo == Tipo::Receber) { modelPendentes.setTable("conta_a_receber_has_pagamento"); }
  if (tipo == Tipo::Pagar) { modelPendentes.setTable("conta_a_pagar_has_pagamento"); }

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
  modelPendentes.setHeaderData("idConta", "Conta");
  modelPendentes.setHeaderData("tipoDet", "Tipo Det");
  modelPendentes.setHeaderData("centroCusto", "Centro Custo");
  modelPendentes.setHeaderData("grupo", "Grupo");
  modelPendentes.setHeaderData("subGrupo", "SubGrupo");

  modelPendentes.setSort("dataPagamento");

  modelPendentes.proxyModel = new SortFilterProxyModel(&modelPendentes, this);

  ui->tablePendentes->setModel(&modelPendentes);

  ui->tablePendentes->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("dataEmissao", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("contraParte", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("tipo", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("parcela", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("dataPagamento", new DateFormatDelegate(modelPendentes.fieldIndex("dataPagamento"), this));
  ui->tablePendentes->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelPendentes.fieldIndex("dataPagamento"), this));

  if (tipo == Tipo::Receber) { ui->tablePendentes->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Receber, this)); }
  if (tipo == Tipo::Pagar) { ui->tablePendentes->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this)); }

  ui->tablePendentes->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->tablePendentes->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->tablePendentes->setItemDelegateForColumn("grupo", new LineEditDelegate(LineEditDelegate::Tipo::Grupo, this));

  ui->tablePendentes->setPersistentColumns({"status", "idConta", "centroCusto"});

  if (tipo == Tipo::Receber) {
    ui->tablePendentes->hideColumn("representacao");
    ui->tablePendentes->hideColumn("idVenda");
    ui->tablePendentes->hideColumn("comissao");
    ui->tablePendentes->hideColumn("taxa");
  }

  if (tipo == Tipo::Pagar) {
    ui->tablePendentes->hideColumn("idCompra");
    ui->tablePendentes->hideColumn("idNFe");
  }

  ui->tablePendentes->hideColumn("idPagamento");
  ui->tablePendentes->hideColumn("idLoja");
  ui->tablePendentes->hideColumn("created");
  ui->tablePendentes->hideColumn("lastUpdated");
  ui->tablePendentes->hideColumn("desativado");

  // -------------------------------------------------------------------------

  if (tipo == Tipo::Receber) { modelProcessados.setTable("conta_a_receber_has_pagamento"); }
  if (tipo == Tipo::Pagar) { modelProcessados.setTable("conta_a_pagar_has_pagamento"); }

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
  modelProcessados.setHeaderData("idConta", "Conta");
  modelProcessados.setHeaderData("tipoDet", "Tipo Det");
  modelProcessados.setHeaderData("centroCusto", "Centro Custo");
  modelProcessados.setHeaderData("grupo", "Grupo");
  modelProcessados.setHeaderData("subGrupo", "SubGrupo");

  modelProcessados.proxyModel = new SortFilterProxyModel(&modelProcessados, this);

  ui->tableProcessados->setModel(&modelProcessados);

  ui->tableProcessados->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tableProcessados->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tableProcessados->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, true, this));
  ui->tableProcessados->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, true, this));

  ui->tableProcessados->setPersistentColumns({"idConta", "centroCusto"});

  if (tipo == Tipo::Receber) {
    ui->tableProcessados->hideColumn("representacao");
    ui->tableProcessados->hideColumn("idVenda");
    ui->tableProcessados->hideColumn("comissao");
    ui->tableProcessados->hideColumn("taxa");
  }

  if (tipo == Tipo::Pagar) {
    ui->tableProcessados->hideColumn("idCompra");
    ui->tableProcessados->hideColumn("idNFe");
  }

  ui->tableProcessados->hideColumn("idPagamento");
  ui->tableProcessados->hideColumn("idLoja");
  ui->tableProcessados->hideColumn("created");
  ui->tableProcessados->hideColumn("lastUpdated");
  ui->tableProcessados->hideColumn("desativado");
}

bool Contas::verifyFields() {
  for (int row = 0; row < ui->tablePendentes->rowCount(); ++row) {
    const QString status = modelPendentes.data(row, "status").toString();

    if ((tipo == Tipo::Pagar and status == "PAGO") or (tipo == Tipo::Receber and status == "RECEBIDO")) {
      if (modelPendentes.data(row, "dataRealizado").toString().isEmpty()) { return qApp->enqueueError(false, "'Data Realizado' vazio!", this); }
      if (modelPendentes.data(row, "valorReal").toString().isEmpty()) { return qApp->enqueueError(false, "'R$ Real' vazio!", this); }
      if (modelPendentes.data(row, "tipoReal").toString().isEmpty()) { return qApp->enqueueError(false, "'Tipo Real' vazio!", this); }
      if (modelPendentes.data(row, "parcelaReal").toString().isEmpty()) { return qApp->enqueueError(false, "'Parcela Real' vazio!", this); }
      if (modelPendentes.data(row, "idConta").toString().isEmpty()) { return qApp->enqueueError(false, "'Conta Dest.' vazio!", this); }
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

void Contas::viewContaPagar(const QString &dataPagamento) {
  modelPendentes.setFilter("dataPagamento = '" + dataPagamento + "' AND status IN ('PENDENTE', 'CONFERIDO') AND desativado = FALSE");

  modelProcessados.setFilter("dataPagamento = '" + dataPagamento + "' AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO', 'SUBSTITUIDO') AND desativado = FALSE");

  if (not modelPendentes.select()) { return; }

  if (not modelProcessados.select()) { return; }
}

void Contas::viewContaReceber(const QString &idPagamento, const QString &contraparte) {
  QSqlQuery query;
  query.prepare("SELECT idVenda FROM conta_a_receber_has_pagamento WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", idPagamento);

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando dados: " + query.lastError().text(), this); }

  const QString idVenda = query.value("idVenda").toString();

  setWindowTitle("Contas A Receber - " + contraparte + " " + idVenda);

  modelPendentes.setFilter(idVenda.isEmpty() ? "idPagamento = " + idPagamento + " AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE"
                                             : "idVenda LIKE '" + idVenda + "%' AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE");

  // -------------------------------------------------------------------------

  modelProcessados.setFilter(idVenda.isEmpty() ? "idPagamento = " + idPagamento + " AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE"
                                               : "idVenda = '" + idVenda + "' AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE");

  // -------------------------------------------------------------------------

  if (not modelPendentes.select()) { return; }

  if (not modelProcessados.select()) { return; }
}

// TODO: 5adicionar coluna 'boleto' para dizer onde foi pago
// TODO: 5fazer somatoria dos valores
// TODO: 5mostrar nfe que é mostrada na tela do widgetpagamentos
// TODO: 5verificar centroCusto que usa dois campos (idLoja/centroCusto)
// TODO: 5a funcao de marcar 'conferido' nao deixa voltar para pendente
// TODO: 5funcao de marcar 'conferido' marca na linha de baixo

// FIXME: quando cancelar uma transferencia cancelar a outra ponta tambem
// TODO: pagamentos que são agendados para o final de semana devem pular para segunda
// TODO: parametrizar as regras de cada operadora de cartao e cadastrar uma forma de pagamento para cada
