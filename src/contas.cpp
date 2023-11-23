#include "contas.h"
#include "ui_contas.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "searchdialog.h"
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

Contas::~Contas() {
  delete ui;

  SearchDialog::clearCache();
}

void Contas::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxMostrarCancelados, &QCheckBox::toggled, this, &Contas::on_checkBoxMostrarCancelados_toggled, connectionType);
  connect(ui->pushButtonCriarLancamento, &QPushButton::clicked, this, &Contas::on_pushButtonCriarLancamento_clicked, connectionType);
  connect(ui->pushButtonDuplicarLancamento, &QPushButton::clicked, this, &Contas::on_pushButtonDuplicarLancamento_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &Contas::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->tablePendentes->model(), &QAbstractItemModel::dataChanged, this, &Contas::preencher, connectionType);
  connect(ui->tablePendentes->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Contas::somarPendentes, connectionType);
  connect(ui->tableProcessados->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Contas::somarProcessados, connectionType);
}

void Contas::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxMostrarCancelados, &QCheckBox::toggled, this, &Contas::on_checkBoxMostrarCancelados_toggled);
  disconnect(ui->pushButtonCriarLancamento, &QPushButton::clicked, this, &Contas::on_pushButtonCriarLancamento_clicked);
  disconnect(ui->pushButtonDuplicarLancamento, &QPushButton::clicked, this, &Contas::on_pushButtonDuplicarLancamento_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &Contas::on_pushButtonSalvar_clicked);
  disconnect(ui->tablePendentes->model(), &QAbstractItemModel::dataChanged, this, &Contas::preencher);
  disconnect(ui->tablePendentes->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Contas::somarPendentes);
  disconnect(ui->tableProcessados->selectionModel(), &QItemSelectionModel::selectionChanged, this, &Contas::somarProcessados);
}

void Contas::validarData(const QModelIndex &index) {
  if (index.column() == ui->tablePendentes->columnIndex("dataPagamento")) {
    const int row = index.row();
    const int idPagamento = modelPendentes.data(row, "idPagamento").toInt();

    SqlQuery query;
    query.prepare("SELECT dataPagamento FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
    query.bindValue(":idPagamento", idPagamento);

    if (not query.exec()) { throw RuntimeException("Erro buscando dataPagamento: " + query.lastError().text(), this); }

    if (not query.first()) { return; }

    const QDate oldDate = query.value("dataPagamento").toDate();
    const QDate newDate = modelPendentes.data(row, "dataPagamento").toDate();

    if (oldDate.isNull()) { return; }

    if (tipo == Tipo::Pagar and (newDate > oldDate.addDays(30) or newDate < oldDate.addDays(-30))) {
      // modelPendentes.setData(row, "dataPagamento", oldDate);
      // throw RuntimeError("Limite de alteração de data excedido! Use corrigir fluxo na tela de compras!", this);
      qApp->enqueueWarning("Alteração de data maior que 30 dias!");
    }

    if (tipo == Tipo::Receber and (newDate > oldDate.addDays(30) or newDate < oldDate.addDays(-30))) {
      // modelPendentes.setData(row, "dataPagamento", oldDate);
      // throw RuntimeError("Limite de alteração de data excedido! Use corrigir fluxo na tela de vendas!", this);
      qApp->enqueueWarning("Alteração de data maior que 30 dias!");
    }
  }
}

void Contas::preencher(const QModelIndex &index) {
  validarData(index);

  unsetConnections();

  try {
    [&] {
      const int row = index.row();

      //      if (index.column() == ui->tablePendentes->columnIndex("valor")) {
      //        SqlQuery query;
      //        query.prepare("SELECT valor FROM " + modelPendentes.tableName() + " WHERE idPagamento = :idPagamento");
      //        query.bindValue(":idPagamento", modelPendentes.data(row, "idPagamento"));

      //        if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando valor: " + query.lastError().text(), this); }

      //        const double oldValor = query.value("valor").toDouble();
      //        const double newValor = modelPendentes.data(row, "valor").toDouble();

      //        if ((oldValor / newValor < 0.99 or oldValor / newValor > 1.01) and qFabs(oldValor - newValor) > 5) {
      //          modelPendentes.setData(row, "valor", oldValor);
      //          throw RuntimeError("Limite de alteração de valor excedido! Use a função de corrigir fluxo!", this);
      //        }
      //      }

      const QString tipoPagamento = modelPendentes.data(row, "tipo").toString();
      const QString parcela = modelPendentes.data(row, "parcela").toString();

      if (index.column() == ui->tablePendentes->columnIndex("dataRealizado")) {
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
        modelPendentes.setData(row, "dataRealizado", qApp->ajustarDiaUtil(modelPendentes.data(row, "dataRealizado").toDate()));

        // -------------------------------------------------------------------------

        // TODO: substituir esse código por uma coluna 'idRelacionado' no banco de dados
        if (tipoPagamento.contains("DÉBITO") or tipoPagamento.contains("CRÉDITO")) {
          const auto match = modelPendentes.multiMatch({{"tipo", tipoPagamento.left(1) + ". TAXA CARTÃO"}, {"parcela", parcela}});

          for (const auto &rowMatch : match) {
            if (modelPendentes.data(rowMatch, "status").toString() == "CANCELADO") { continue; }

            if (queryConta.first()) {
              if (modelPendentes.data(rowMatch, "idConta").toInt() == 0) { modelPendentes.setData(rowMatch, "idConta", queryConta.value("idConta")); }
            }

            modelPendentes.setData(rowMatch, "dataRealizado", modelPendentes.data(row, "dataRealizado"));
            modelPendentes.setData(rowMatch, "status", (tipo == Tipo::Receber) ? "RECEBIDO" : "PAGO");
            modelPendentes.setData(rowMatch, "valorReal", modelPendentes.data(rowMatch, "valor"));
            modelPendentes.setData(rowMatch, "tipoReal", modelPendentes.data(rowMatch, "tipo"));
            modelPendentes.setData(rowMatch, "parcelaReal", modelPendentes.data(rowMatch, "parcela"));
            modelPendentes.setData(rowMatch, "centroCusto", modelPendentes.data(rowMatch, "idLoja"));
          }
        }
      }

      // buscar linha da taxa cartao e alterar a conta para ser igual
      if (index.column() == ui->tablePendentes->columnIndex("idConta")) {
        if (index.data().toInt() == 0) { return; } // for dealing with ItemBox editor emiting signal when mouseOver

        // TODO: substituir esse código por uma coluna 'idRelacionado' no banco de dados
        if (tipoPagamento.contains("DÉBITO") or tipoPagamento.contains("CRÉDITO")) {
          const auto match = modelPendentes.multiMatch({{"tipo", tipoPagamento.left(1) + ". TAXA CARTÃO"}, {"parcela", parcela}});

          for (const auto &rowMatch : match) { modelPendentes.setData(rowMatch, "idConta", modelPendentes.data(row, "idConta")); }
        }
      }

      if (index.column() == ui->tablePendentes->columnIndex("centroCusto")) {
        if (index.data().toInt() == 0) { return; }

        modelPendentes.setData(row, "idLoja", modelPendentes.data(row, "centroCusto"));
      }

      //      if (index.column() != ui->tablePendentes->columnIndex("dataRealizado")) {
      //        if (index.data().toString() == "PENDENTE") { return; }

      //        if (modelPendentes.data(row, "status").toString() == "PENDENTE") { modelPendentes.setData(row, "status", "CONFERIDO"); }
      //      }
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Contas::setupTables() {
  if (tipo == Tipo::Receber) { modelPendentes.setTable("conta_a_receber_has_pagamento"); }
  if (tipo == Tipo::Pagar) { modelPendentes.setTable("conta_a_pagar_has_pagamento"); }

  modelPendentes.setHeaderData("dataEmissao", "Data Emissão");
  modelPendentes.setHeaderData("idVenda", "Venda");
  modelPendentes.setHeaderData("contraParte", "Contraparte");
  modelPendentes.setHeaderData("idNFe", "NF-e cadastrada");
  modelPendentes.setHeaderData("nfe", "NF-e");
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
  //  ui->tablePendentes->setItemDelegateForColumn("contraParte", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  //  ui->tablePendentes->setItemDelegateForColumn("parcela", new NoEditDelegate(this));
  ui->tablePendentes->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelPendentes.fieldIndex("dataPagamento"), modelPendentes.fieldIndex("tipo"), (tipo == Tipo::Receber), this));

  if (tipo == Tipo::Receber) { ui->tablePendentes->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Receber, this)); }
  if (tipo == Tipo::Pagar) { ui->tablePendentes->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this)); }

  ui->tablePendentes->setItemDelegateForColumn("idNFe", new ItemBoxDelegate(ItemBoxDelegate::Tipo::NFe, false, this));
  ui->tablePendentes->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->tablePendentes->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->tablePendentes->setItemDelegateForColumn("grupo", new LineEditDelegate(LineEditDelegate::Tipo::Grupo, this));

  ui->tablePendentes->setPersistentColumns({"status", "idNFe"});

  if (tipo == Tipo::Receber) {
    ui->tablePendentes->hideColumn("representacao");
    ui->tablePendentes->hideColumn("idVenda");
    ui->tablePendentes->hideColumn("comissao");
    ui->tablePendentes->hideColumn("taxa");
  }

  if (tipo == Tipo::Pagar) {
    ui->tablePendentes->hideColumn("idCompra");
    ui->tablePendentes->hideColumn("idCnab");
  }

  ui->tablePendentes->hideColumn("idPagamento");
  ui->tablePendentes->hideColumn("idLoja");
  ui->tablePendentes->hideColumn("desativado");
  ui->tablePendentes->hideColumn("created");
  ui->tablePendentes->hideColumn("lastUpdated");

  // -------------------------------------------------------------------------

  if (tipo == Tipo::Receber) { modelProcessados.setTable("conta_a_receber_has_pagamento"); }
  if (tipo == Tipo::Pagar) { modelProcessados.setTable("conta_a_pagar_has_pagamento"); }

  modelProcessados.setHeaderData("dataEmissao", "Data Emissão");
  modelProcessados.setHeaderData("idVenda", "Venda");
  modelProcessados.setHeaderData("contraParte", "Contraparte");
  modelProcessados.setHeaderData("idNFe", "NF-e cadastrada");
  modelProcessados.setHeaderData("nfe", "NF-e");
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

  ui->tableProcessados->setItemDelegateForColumn("idNFe", new ItemBoxDelegate(ItemBoxDelegate::Tipo::NFe, true, this));
  ui->tableProcessados->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tableProcessados->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tableProcessados->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, true, this));
  ui->tableProcessados->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, true, this));

  ui->tableProcessados->setPersistentColumns({"idNFe"});

  if (tipo == Tipo::Receber) {
    ui->tableProcessados->hideColumn("representacao");
    ui->tableProcessados->hideColumn("idVenda");
    ui->tableProcessados->hideColumn("comissao");
    ui->tableProcessados->hideColumn("taxa");
  }

  if (tipo == Tipo::Pagar) {
    ui->tableProcessados->hideColumn("idCompra");
    ui->tableProcessados->hideColumn("idCnab");
  }

  ui->tableProcessados->hideColumn("idPagamento");
  ui->tableProcessados->hideColumn("idLoja");
  ui->tableProcessados->hideColumn("desativado");
  ui->tableProcessados->hideColumn("created");
  ui->tableProcessados->hideColumn("lastUpdated");
}

void Contas::verifyFields() {
  for (int row = 0; row < ui->tablePendentes->rowCount(); ++row) {
    if (modelPendentes.data(row, "valor") == 0) { throw RuntimeError("'R$' vazio na linha " + QString::number(row + 1) + "!", this); }

    const QString status = modelPendentes.data(row, "status").toString();

    if ((tipo == Tipo::Pagar and status == "PAGO") or (tipo == Tipo::Receber and status == "RECEBIDO")) {
      if (modelPendentes.data(row, "dataRealizado").toString().isEmpty()) { throw RuntimeError("'Data Realizado' vazio na linha " + QString::number(row + 1) + "!", this); }
      if (modelPendentes.data(row, "valorReal") == 0) { throw RuntimeError("'R$ Real' vazio na linha " + QString::number(row + 1) + "!", this); }
      if (modelPendentes.data(row, "tipoReal").toString().isEmpty()) { throw RuntimeError("'Tipo Real' vazio na linha " + QString::number(row + 1) + "!", this); }
      if (modelPendentes.data(row, "idConta") == 0) { throw RuntimeError("'Conta' vazio na linha " + QString::number(row + 1) + "!", this); }
      if (modelPendentes.data(row, "centroCusto") == 0) { throw RuntimeError("'Centro Custo' vazio na linha " + QString::number(row + 1) + "!", this); }
      if (modelPendentes.data(row, "grupo").toString().isEmpty()) { throw RuntimeError("'Grupo' vazio na linha " + QString::number(row + 1) + "!", this); }
    }
  }
}

void Contas::on_pushButtonSalvar_clicked() {
  verifyFields();

  for (int row = 0; row < modelPendentes.rowCount(); ++row) {
    QString contraParte = modelPendentes.data(row, "contraParte").toString();
    modelPendentes.setData(row, "contraParte", contraParte.remove("\r").remove("\n").trimmed());

    QString observacao = modelPendentes.data(row, "observacao").toString();
    modelPendentes.setData(row, "observacao", observacao.remove("\r").remove("\n").trimmed());
  }

  qApp->startTransaction("Contas::on_pushButtonSalvar_clicked");

  modelPendentes.submitAll();

  qApp->endTransaction();

  close();
}

void Contas::viewContaPagarPgt(const QString &idPagamento) {
  modelPendentes.setFilter("idPagamento = '" + idPagamento + "' AND status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO')");

  modelProcessados.setFilter("idPagamento = '" + idPagamento + "' AND status IN ('PAGO')");

  // -------------------------------------------------------------------------

  modelPendentes.select();

  modelProcessados.select();
}

void Contas::viewContaPagarContraparte(const QString &contraparte) {
  setWindowTitle(windowTitle() + " - Contraparte: " + contraparte);

  // -------------------------------------------------------------------------

  modelPendentes.setFilter("contraparte = '" + contraparte + "' AND status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO')");

  modelProcessados.setFilter("contraparte = '" + contraparte + "' AND status IN ('PAGO')");

         // -------------------------------------------------------------------------

  modelPendentes.select();

  modelProcessados.select();
}

void Contas::viewContaPagarData(const QString &dataPagamento) {
  setWindowTitle(windowTitle() + " - Data: " + QDate::fromString(dataPagamento, "yyyy-MM-dd").toString("dd-MM-yyyy"));

  // -------------------------------------------------------------------------

  modelPendentes.setFilter("dataPagamento = '" + dataPagamento + "' AND status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO')");

  modelProcessados.setFilter("dataPagamento = '" + dataPagamento + "' AND status IN ('PAGO')");

  // -------------------------------------------------------------------------

  modelPendentes.select();

  modelProcessados.select();
}

void Contas::viewContaPagarOrdemCompra(const QString &ordemCompra) {
  setWindowTitle(windowTitle() + " - O.C.: " + ordemCompra);

  // -------------------------------------------------------------------------

  SqlQuery query;

  if (not query.exec("SELECT GROUP_CONCAT(DISTINCT idCompra) AS idCompra FROM pedido_fornecedor_has_produto WHERE ordemCompra = " + ordemCompra)) {
    throw RuntimeException("Erro buscando O.C.: " + query.lastError().text());
  }

  if (not query.first()) { throw RuntimeException("Não encontrado idCompra da O.C.: '" + ordemCompra + "'"); }

  const QString idCompra = query.value("idCompra").toString();

  // -------------------------------------------------------------------------

  modelPendentes.setFilter("idCompra IN (" + idCompra + ") AND status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO')");

  modelProcessados.setFilter("idCompra IN (" + idCompra + ") AND status IN ('PAGO')");

  // -------------------------------------------------------------------------

  modelPendentes.select();

  modelProcessados.select();
}

void Contas::viewContaReceber(const QString &idPagamento, const QString &contraparte) {
  SqlQuery query;
  query.prepare("SELECT idVenda FROM conta_a_receber_has_pagamento WHERE idPagamento = :idPagamento");
  query.bindValue(":idPagamento", idPagamento);

  if (not query.exec()) { throw RuntimeException("Erro buscando dados: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Não encontrado Venda do pagamento com id: '" + idPagamento + "'"); }

    const QString idVenda = query.value("idVenda").toString().left(11);

    // -------------------------------------------------------------------------

    setWindowTitle("Contas A Receber - " + contraparte + " " + idVenda);

    // -------------------------------------------------------------------------

    modelPendentes.setFilter("status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO') AND representacao = FALSE AND " + (idVenda.isEmpty() ? "idPagamento = " + idPagamento : "idVenda LIKE '" + idVenda + "%'"));

    // -------------------------------------------------------------------------

    modelProcessados.setFilter("status IN ('RECEBIDO') AND representacao = FALSE AND " + (idVenda.isEmpty() ? "idPagamento = " + idPagamento : "idVenda LIKE '" + idVenda + "%'"));

    // -------------------------------------------------------------------------

    modelPendentes.select();

    modelProcessados.select();
  }

void Contas::viewContaReceberContraparte(const QString &contraparte) {
    setWindowTitle(windowTitle() + " - Contraparte: " + contraparte);

           // -------------------------------------------------------------------------

    modelPendentes.setFilter("status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO') AND representacao = FALSE AND contraParte = '" + contraparte + "'");

    modelProcessados.setFilter("status IN ('RECEBIDO') AND representacao = FALSE AND contraParte = '" + contraparte + "'");

           // -------------------------------------------------------------------------

    modelPendentes.select();

    modelProcessados.select();
  }

void Contas::viewContaReceberPgt(const QString &idPagamento) {
    modelPendentes.setFilter("status IN ('PENDENTE', 'CONFERIDO', 'AGENDADO') AND representacao = FALSE AND idPagamento = " + idPagamento);

           // -------------------------------------------------------------------------

    modelProcessados.setFilter("status IN ('RECEBIDO') AND representacao = FALSE AND idPagamento = " + idPagamento);

           // -------------------------------------------------------------------------

    modelPendentes.select();

    modelProcessados.select();
}

void Contas::on_pushButtonCriarLancamento_clicked() {
  unsetConnections();

  try {
    const int newRow = modelPendentes.insertRowAtEnd();

    modelPendentes.setData(newRow, "status", "PENDENTE");
    modelPendentes.setData(newRow, "dataEmissao", qApp->serverDate());
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void Contas::on_pushButtonDuplicarLancamento_clicked() {
  const auto selection = ui->tablePendentes->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Deve selecionar uma linha primeiro!", this); }

  for (const auto index : selection) {
    const int row = index.row();
    const int newRow = modelPendentes.insertRowAtEnd();

    for (int col = 0; col < modelPendentes.columnCount(); ++col) {
      if (modelPendentes.fieldIndex("idPagamento") == col) { continue; }
      if (modelPendentes.fieldIndex("nfe") == col) { continue; }
      if (modelPendentes.fieldIndex("valor") == col) { continue; }
      if (modelPendentes.fieldIndex("grupo") == col) { continue; }
      if (modelPendentes.fieldIndex("subGrupo") == col) { continue; }
      if (modelPendentes.fieldIndex("desativado") == col) { continue; }
      if (modelPendentes.fieldIndex("created") == col) { continue; }
      if (modelPendentes.fieldIndex("lastUpdated") == col) { continue; }

      const QVariant value = modelPendentes.data(row, col);

      if (value.isNull()) { continue; }

      modelPendentes.setData(newRow, col, value);
    }
  }
}

void Contas::on_checkBoxMostrarCancelados_toggled(const bool checked) {
  // FIXME: se o model tiver linhas com alterações elas serão perdidas

  QString filter = modelPendentes.filter();

  const QString a = "'AGENDADO')";
  const QString b = "'AGENDADO', 'CANCELADO', 'SUBSTITUIDO')";

  (checked) ? filter.replace(a, b) : filter.replace(b, a);

  modelPendentes.setFilter(filter);
}

void Contas::somarPendentes() {
  const auto selection = ui->tablePendentes->selectionModel()->selectedIndexes();

  QSet<int> rows;

  for (auto index : selection) { rows << index.row(); }

  double soma = 0.;

  for (auto row : rows) { soma += modelPendentes.data(row, "valor").toDouble(); }

  ui->doubleSpinBoxSomaPendentes->setValue(soma);
  ui->doubleSpinBoxSomaPendentes->setSuffix(" - " + QString::number(rows.size()) + " linha(s)");
}

void Contas::somarProcessados() {
  const auto selection = ui->tableProcessados->selectionModel()->selectedRows();

  double soma = 0.;

  for (auto index : selection) { soma += modelProcessados.data(index.row(), "valor").toDouble(); }

  ui->doubleSpinBoxSomaProcessados->setValue(soma);
  ui->doubleSpinBoxSomaProcessados->setSuffix(" - " + QString::number(selection.size()) + " linha(s)");
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
// TODO: avisar se a data do pagamento for no passado ou muito no futuro
