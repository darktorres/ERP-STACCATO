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
#include "sqlquery.h"

#include <QDebug>
#include <QSqlError>
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

    SqlQuery query;
    query.prepare("SELECT dataPagamento FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", idPagamento);

    if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando dataPagamento: " + query.lastError().text(), this); }

    const QDate oldDate = query.value("dataPagamento").toDate();
    const QDate newDate = modelPendentes.data(row, "dataPagamento").toDate();

    if (oldDate.isNull()) { return true; }

    if (tipo == Tipo::Pagar and (newDate > oldDate.addDays(92) or newDate < oldDate.addDays(-32))) {
      modelPendentes.setData(row, "dataPagamento", oldDate);
      throw RuntimeError("Limite de alteração de data excedido! Use corrigir fluxo na tela de compras!", this);
    }

    if (tipo == Tipo::Receber and (newDate > oldDate.addDays(32) or newDate < oldDate.addDays(-92))) {
      modelPendentes.setData(row, "dataPagamento", oldDate);
      throw RuntimeError("Limite de alteração de data excedido! Use corrigir fluxo na tela de vendas!", this);
    }
  }

  return true;
}

void Contas::preencher(const QModelIndex &index) {
  if (not validarData(index)) { return; }

  unsetConnections();

  try {
    const int row = index.row();

    if (index.column() == ui->tablePendentes->columnIndex("valor")) {
      SqlQuery query;
      query.prepare("SELECT valor FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
      query.bindValue(":idPagamento", modelPendentes.data(row, "idPagamento"));

      if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando valor: " + query.lastError().text(), this); }

      const double oldValor = query.value("valor").toDouble();
      const double newValor = modelPendentes.data(row, "valor").toDouble();

      if ((oldValor / newValor < 0.99 or oldValor / newValor > 1.01) and qFabs(oldValor - newValor) > 5) {
        throw RuntimeError("Limite de alteração de valor excedido! Use a função de corrigir fluxo!", this);
        modelPendentes.setData(row, "valor", oldValor);
      }
    }

    if (index.column() == ui->tablePendentes->columnIndex("dataRealizado")) {
      const QString tipoPagamento = modelPendentes.data(row, "tipo").toString();
      const int idContaExistente = modelPendentes.data(row, "idConta").toInt();

      SqlQuery queryConta;

      if (not queryConta.exec("SELECT idConta FROM forma_pagamento WHERE pagamento = '" + tipoPagamento + "'")) {
        throw RuntimeException("Erro buscando conta do pagamento: " + queryConta.lastError().text(), this);
      }

      if (queryConta.first()) {
        const int idConta = queryConta.value("idConta").toInt();

        if (idContaExistente == 0 and idConta != 0) { modelPendentes.setData(row, "idConta", idConta); }
      }

      modelPendentes.setData(row, "status", (tipo == Tipo::Receber) ? "RECEBIDO" : "PAGO");
      modelPendentes.setData(row, "valorReal", modelPendentes.data(row, "valor"));
      modelPendentes.setData(row, "tipoReal", modelPendentes.data(row, "tipo"));
      modelPendentes.setData(row, "parcelaReal", modelPendentes.data(row, "parcela"));
      modelPendentes.setData(row, "centroCusto", modelPendentes.data(row, "idLoja"));

      // -------------------------------------------------------------------------

      const auto list = modelPendentes.multiMatch({{"tipo", modelPendentes.data(row, "tipo").toString().left(1) + ". TAXA CARTÃO"}, {"parcela", modelPendentes.data(row, "parcela")}});

      for (const auto &rowMatch : list) {
        if (queryConta.first()) {
          const int idConta = queryConta.value("idConta").toInt();

          if (modelPendentes.data(rowMatch, "idConta").toInt() == 0) { modelPendentes.setData(rowMatch, "idConta", idConta); }
        }

        modelPendentes.setData(rowMatch, "dataRealizado", modelPendentes.data(row, "dataRealizado"));
        modelPendentes.setData(rowMatch, "status", (tipo == Tipo::Receber) ? "RECEBIDO" : "PAGO");
        modelPendentes.setData(rowMatch, "valorReal", modelPendentes.data(rowMatch, "valor"));
        modelPendentes.setData(rowMatch, "tipoReal", modelPendentes.data(rowMatch, "tipo"));
        modelPendentes.setData(rowMatch, "parcelaReal", modelPendentes.data(rowMatch, "parcela"));
        modelPendentes.setData(rowMatch, "centroCusto", modelPendentes.data(rowMatch, "idLoja"));
      }
    }

    // buscar linha da taxa cartao e alterar a conta para ser igual
    if (index.column() == ui->tablePendentes->columnIndex("idConta")) {
      if (index.data() == 0) { return; } // for dealing with ItemBox editor emiting signal when mouseOver

      const auto list = modelPendentes.multiMatch({{"tipo", modelPendentes.data(row, "tipo").toString().left(1) + ". TAXA CARTÃO"}, {"parcela", modelPendentes.data(row, "parcela")}});

      for (const auto &rowMatch : list) { modelPendentes.setData(rowMatch, "idConta", modelPendentes.data(row, "idConta")); }
    }

    if (index.column() == ui->tablePendentes->columnIndex("centroCusto")) {
      if (index.data() == 0) { return; }

      modelPendentes.setData(row, "idLoja", modelPendentes.data(row, "centroCusto"));
    }

    if (index.column() != ui->tablePendentes->columnIndex("dataRealizado")) {
      if (index.data().toString() == "PENDENTE") { return; }

      if (modelPendentes.data(row, "status").toString() == "PENDENTE") { modelPendentes.setData(row, "status", "CONFERIDO"); }
    }
  } catch (std::exception &e) {}

  setConnections();
}

void Contas::setupTables() {
  if (tipo == Tipo::Receber) { modelPendentes.setTable("conta_a_receber_has_pagamento"); }
  if (tipo == Tipo::Pagar) { modelPendentes.setTable("conta_a_pagar_has_pagamento"); }

  modelPendentes.setHeaderData("dataEmissao", "Data Emissão");
  modelPendentes.setHeaderData("idVenda", "Venda");
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
  ui->tablePendentes->setItemDelegateForColumn("parcela", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelPendentes.fieldIndex("dataPagamento"), modelPendentes.fieldIndex("tipo"), (tipo == Tipo::Receber), this));

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
    ui->tablePendentes->hideColumn("idCnab");
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
  modelProcessados.setHeaderData("idVenda", "Venda");
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
    ui->tableProcessados->hideColumn("idCnab");
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
      if (modelPendentes.data(row, "dataRealizado").toString().isEmpty()) { throw RuntimeError("'Data Realizado' vazio!", this); }
      if (modelPendentes.data(row, "valorReal") == 0) { throw RuntimeError("'R$ Real' vazio!", this); }
      if (modelPendentes.data(row, "tipoReal").toString().isEmpty()) { throw RuntimeError("'Tipo Real' vazio!", this); }
      if (modelPendentes.data(row, "idConta") == 0) { throw RuntimeError("'Conta' vazio!", this); }
      if (modelPendentes.data(row, "centroCusto") == 0) { throw RuntimeError("'Centro Custo' vazio!", this); }
      if (modelPendentes.data(row, "grupo").toString().isEmpty()) { throw RuntimeError("'Grupo' vazio!", this); }
    }
  }

  return true;
}

void Contas::on_pushButtonSalvar_clicked() {
  if (not verifyFields()) { return; }

  modelPendentes.submitAll();

  close();
}

void Contas::viewContaPagar(const QString &dataPagamento) {
  modelPendentes.setFilter("dataPagamento = '" + dataPagamento + "' AND status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO') AND desativado = FALSE");

  modelProcessados.setFilter("dataPagamento = '" + dataPagamento + "' AND status NOT IN ('PENDENTE', 'CONFERIDO', 'AGENDADO', 'CANCELADO', 'SUBSTITUIDO') AND desativado = FALSE");

  modelPendentes.select();

  modelProcessados.select();
}

void Contas::viewContaReceber(const QString &idPagamento, const QString &contraparte) {
  SqlQuery query;
  query.prepare("SELECT idVenda FROM conta_a_receber_has_pagamento WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", idPagamento);

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando dados: " + query.lastError().text(), this); }

  const QString idVenda = query.value("idVenda").toString();

  setWindowTitle("Contas A Receber - " + contraparte + " " + idVenda);

  modelPendentes.setFilter(idVenda.isEmpty() ? "idPagamento = " + idPagamento + " AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE"
                                             : "idVenda LIKE '" + idVenda + "%' AND status IN ('PENDENTE', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE");

  // -------------------------------------------------------------------------

  modelProcessados.setFilter(idVenda.isEmpty() ? "idPagamento = " + idPagamento + " AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE"
                                               : "idVenda = '" + idVenda + "' AND status NOT IN ('PENDENTE', 'CANCELADO', 'CONFERIDO') AND representacao = FALSE AND desativado = FALSE");

  // -------------------------------------------------------------------------

  modelPendentes.select();

  modelProcessados.select();
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
