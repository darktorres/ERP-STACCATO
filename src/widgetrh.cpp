#include "widgetrh.h"
#include "ui_widgetrh.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"
#include "xlsxdocument.h"

#include <QMessageBox>
#include <QSqlError>

WidgetRh::WidgetRh(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetRh) { ui->setupUi(this); }

WidgetRh::~WidgetRh() { delete ui; }

void WidgetRh::resetTables() { setupTables(); }

void WidgetRh::updateTables() {
  if (not isSet) {
    ui->dateEdit->setDate(qApp->serverDate());
    setupTables();
    setConnections();
    isSet = true;
  }

  modelFolhaPag.select();
}

void WidgetRh::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetRh::on_groupBoxData_toggled, connectionType);
  connect(ui->pushButtonDarBaixa, &QPushButton::clicked, this, &WidgetRh::on_pushButtonDarBaixa_clicked, connectionType);
  connect(ui->pushButtonImportarFolhaPag, &QPushButton::clicked, this, &WidgetRh::on_pushButtonImportarFolhaPag_clicked, connectionType);
  connect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetRh::on_pushButtonRemessaItau_clicked, connectionType);
  connect(ui->radioButtonAgendado, &QRadioButton::clicked, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->radioButtonCancelado, &QRadioButton::clicked, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->radioButtonConferido, &QRadioButton::clicked, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->radioButtonPago, &QRadioButton::clicked, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->radioButtonPendente, &QRadioButton::clicked, this, &WidgetRh::montaFiltro, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::clicked, this, &WidgetRh::montaFiltro, connectionType);
  // NOTE: QSqlTableModel::OnFieldChange sets the flags to not editable during dataChanged so the function below wont work unless flags() is overriden
  // connect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &WidgetRh::preencher, connectionType);
}

void WidgetRh::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->dateEdit, &QDateEdit::dateChanged, this, &WidgetRh::montaFiltro);
  disconnect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetRh::montaFiltro);
  disconnect(ui->groupBoxData, &QGroupBox::toggled, this, &WidgetRh::on_groupBoxData_toggled);
  disconnect(ui->pushButtonDarBaixa, &QPushButton::clicked, this, &WidgetRh::on_pushButtonDarBaixa_clicked);
  disconnect(ui->pushButtonImportarFolhaPag, &QPushButton::clicked, this, &WidgetRh::on_pushButtonImportarFolhaPag_clicked);
  disconnect(ui->pushButtonRemessaItau, &QPushButton::clicked, this, &WidgetRh::on_pushButtonRemessaItau_clicked);
  disconnect(ui->table->model(), &QAbstractItemModel::dataChanged, this, &WidgetRh::preencher);
}

void WidgetRh::setupTables() {
  modelFolhaPag.setTable("folha_pagamento");

  modelFolhaPag.setEditStrategy(QSqlTableModel::OnFieldChange);

  modelFolhaPag.setFilter("");

  modelFolhaPag.setHeaderData("dataEmissao", "Data Emissão");
  modelFolhaPag.setHeaderData("idVenda", "Venda");
  modelFolhaPag.setHeaderData("contraParte", "Contraparte");
  modelFolhaPag.setHeaderData("idNFe", "NF-e cadastrada");
  modelFolhaPag.setHeaderData("nfe", "NF-e");
  modelFolhaPag.setHeaderData("valor", "R$");
  modelFolhaPag.setHeaderData("tipo", "Tipo");
  modelFolhaPag.setHeaderData("parcela", "Parcela");
  modelFolhaPag.setHeaderData("dataPagamento", "Vencimento");
  modelFolhaPag.setHeaderData("observacao", "Obs.");
  modelFolhaPag.setHeaderData("status", "Status");
  modelFolhaPag.setHeaderData("dataRealizado", "Data Realizado");
  modelFolhaPag.setHeaderData("valorReal", "R$ Real");
  modelFolhaPag.setHeaderData("tipoReal", "Tipo Real");
  modelFolhaPag.setHeaderData("parcelaReal", "Parcela Real");
  modelFolhaPag.setHeaderData("idConta", "Conta");
  modelFolhaPag.setHeaderData("tipoDet", "Tipo Det");
  modelFolhaPag.setHeaderData("centroCusto", "Centro Custo");
  modelFolhaPag.setHeaderData("grupo", "Grupo");
  modelFolhaPag.setHeaderData("subGrupo", "SubGrupo");

  modelFolhaPag.setSort("dataPagamento");

  modelFolhaPag.proxyModel = new SortFilterProxyModel(&modelFolhaPag, this);

  ui->table->setModel(&modelFolhaPag);

  ui->table->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("dataEmissao", new NoEditDelegate(this));
  //  ui->table->setItemDelegateForColumn("contraParte", new NoEditDelegate(this));
  ui->table->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->table->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  //  ui->table->setItemDelegateForColumn("parcela", new NoEditDelegate(this));
  ui->table->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelFolhaPag.fieldIndex("dataPagamento"), modelFolhaPag.fieldIndex("tipo"), false, this));

  ui->table->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this));

  ui->table->setItemDelegateForColumn("idNFe", new ItemBoxDelegate(ItemBoxDelegate::Tipo::NFe, false, this));
  ui->table->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->table->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->table->setItemDelegateForColumn("grupo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Grupo, this));

  // NOTE: to avoid this making a query to the database for each row needs to change SearchDialog::nfe to not select() on the ctor
  // select() only if the initial value is not empty or zero, set by the model in setEditorData()
  ui->table->setPersistentColumns({"status", "idNFe"});

  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("idCnab");

  ui->table->hideColumn("idPagamento");
  ui->table->hideColumn("idLoja");
  ui->table->hideColumn("desativado");
  ui->table->hideColumn("created");
  ui->table->hideColumn("lastUpdated");
}

void WidgetRh::on_pushButtonImportarFolhaPag_clicked() {
  // TODO: cadastrar as linhas do Excel na tabela folha_pagamento e na tabela de contas_a_pagar lançar o total de cada loja

  const QString file = QFileDialog::getOpenFileName(this, "Importar arquivo do Excel", "", "Excel (*.xlsx)");

  if (file.isEmpty()) { return; }

  SqlTableModel modelImportar;
  modelImportar.setTable("folha_pagamento");

  QXlsx::Document xlsx(file, this);

  if (not xlsx.selectSheet("Planilha1")) { throw RuntimeException("Não encontrou 'Planilha1' na tabela!", this); }

  verificaCabecalho(xlsx);

  const int rows = xlsx.dimension().rowCount();

  QMap<QList<QVariant>, double> totalLoja;

  for (int rowExcel = 2; rowExcel <= rows; ++rowExcel) {
    if (xlsx.readValue(rowExcel, 1).toString().isEmpty() or xlsx.readValue(rowExcel, 1).toString() == "00:00:00.000") { continue; }

    SqlQuery queryLoja;

    if (not queryLoja.exec("SELECT idLoja FROM loja WHERE nomeFantasia = '" + xlsx.readValue(rowExcel, 2).toString() + "'")) {
      throw RuntimeException("Erro buscando idLoja: " + queryLoja.lastError().text());
    }

    if (not queryLoja.first()) { throw RuntimeError("Linha " + QString::number(rowExcel) + ", Loja '" + xlsx.readValue(rowExcel, 2).toString() + "' não encontrada no banco de dados!"); }

    SqlQuery queryConta;

    if (not queryConta.exec("SELECT idConta FROM loja_has_conta WHERE banco = '" + xlsx.readValue(rowExcel, 7).toString() + "'")) {
      throw RuntimeException("Erro buscando idConta: " + queryConta.lastError().text());
    }

    if (not queryConta.first()) { throw RuntimeError("Linha " + QString::number(rowExcel) + ", Conta '" + xlsx.readValue(rowExcel, 7).toString() + "' não encontrada no banco de dados!"); }

    const int rowModel = modelImportar.insertRowAtEnd();

    modelImportar.setData(rowModel, "dataEmissao", xlsx.readValue(rowExcel, 1));
    modelImportar.setData(rowModel, "idLoja", queryLoja.value("idLoja"));
    modelImportar.setData(rowModel, "contraParte", xlsx.readValue(rowExcel, 3));
    modelImportar.setData(rowModel, "valor", xlsx.readValue(rowExcel, 4));
    modelImportar.setData(rowModel, "tipo", xlsx.readValue(rowExcel, 5));
    modelImportar.setData(rowModel, "dataPagamento", xlsx.readValue(rowExcel, 6));
    modelImportar.setData(rowModel, "observacao", xlsx.readValue(rowExcel, 8));
    modelImportar.setData(rowModel, "idConta", queryConta.value("idConta"));
    modelImportar.setData(rowModel, "centroCusto", queryLoja.value("idLoja"));
    modelImportar.setData(rowModel, "grupo", xlsx.readValue(rowExcel, 9));

    //-------------------------------------

    QList<QVariant> keys;
    keys << queryLoja.value("idLoja").toInt() << xlsx.readValue(rowExcel, 5).toString() << xlsx.readValue(rowExcel, 7).toString();
    totalLoja[keys] += xlsx.readValue(rowExcel, 4).toDouble();
  }

  SqlTableModel modelPagar;
  modelPagar.setTable("conta_a_pagar_has_pagamento");

  for (QList<QVariant> idLojaTipoBanco : totalLoja.keys()) {
    SqlQuery queryLoja;

    if (not queryLoja.exec("SELECT nomeFantasia FROM loja WHERE idLoja = " + idLojaTipoBanco.at(0).toString())) {
      throw RuntimeException("Erro buscando idLoja: " + queryLoja.lastError().text());
    }

    if (not queryLoja.first()) { throw RuntimeError("Loja '" + idLojaTipoBanco.at(0).toString() + "' não encontrada no banco de dados!"); }

    SqlQuery queryConta;

    if (not queryConta.exec("SELECT idConta FROM loja_has_conta WHERE banco = '" + idLojaTipoBanco.at(2).toString() + "'")) {
      throw RuntimeException("Erro buscando idConta: " + queryConta.lastError().text());
    }

    if (not queryConta.first()) { throw RuntimeError("Conta '" + idLojaTipoBanco.at(2).toString() + "' não encontrada no banco de dados!"); }

    const int rowModel = modelPagar.insertRowAtEnd();

    modelPagar.setData(rowModel, "dataEmissao", xlsx.readValue(2, 1));
    modelPagar.setData(rowModel, "idLoja", idLojaTipoBanco.at(0));
    modelPagar.setData(rowModel, "contraParte", queryLoja.value("nomeFantasia"));
    modelPagar.setData(rowModel, "valor", totalLoja.value(idLojaTipoBanco));
    modelPagar.setData(rowModel, "tipo", idLojaTipoBanco.at(1));
    modelPagar.setData(rowModel, "dataPagamento", xlsx.readValue(2, 6));
    modelPagar.setData(rowModel, "observacao", xlsx.readValue(2, 8));
    modelPagar.setData(rowModel, "idConta", queryConta.value("idConta"));
    modelPagar.setData(rowModel, "centroCusto", idLojaTipoBanco.at(0));
    modelPagar.setData(rowModel, "grupo", xlsx.readValue(2, 9));
  }

  qApp->startTransaction("WidgetRh::pushButtonImportarFolhaPag");

  modelImportar.submitAll();
  modelPagar.submitAll();

  qApp->endTransaction();

  qApp->enqueueInformation("Tabela importada com sucesso!", this);
  updateTables();
}

void WidgetRh::verificaCabecalho(QXlsx::Document &xlsx) {
  if (xlsx.readValue(1, 1).toString() != "Data Emissão") { throw RuntimeError("Cabeçalho errado na coluna 1!"); }
  if (xlsx.readValue(1, 2).toString() != "Centro de Custo") { throw RuntimeError("Cabeçalho errado na coluna 2!"); }
  if (xlsx.readValue(1, 3).toString() != "Contraparte") { throw RuntimeError("Cabeçalho errado na coluna 3!"); }
  if (xlsx.readValue(1, 4).toString() != "") { throw RuntimeError("Cabeçalho errado na coluna 4!"); }
  if (xlsx.readValue(1, 5).toString() != "Tipo") { throw RuntimeError("Cabeçalho errado na coluna 5!"); }
  if (xlsx.readValue(1, 6).toString() != "Vencimento") { throw RuntimeError("Cabeçalho errado na coluna 6!"); }
  if (xlsx.readValue(1, 7).toString() != "Banco") { throw RuntimeError("Cabeçalho errado na coluna 7!"); }
  if (xlsx.readValue(1, 8).toString() != "Obs") { throw RuntimeError("Cabeçalho errado na coluna 8!"); }
  if (xlsx.readValue(1, 9).toString() != "Grupo") { throw RuntimeError("Cabeçalho errado na coluna 9!"); }
}

void WidgetRh::on_pushButtonRemessaItau_clicked()
{
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  for (const auto index : selection) {
    if (modelFolhaPag.data(index.row(), "status").toString() == "PAGO") { throw RuntimeError("Linha selecionada já paga!", this); }
    if (not modelFolhaPag.data(index.row(), "tipo").toString().contains("TRANSF. ITAÚ")) { throw RuntimeError("Pagamento selecionado não é transferência ITAÚ!", this); }
  }

  CNAB cnab(this);
  const QString idCnab = cnab.remessaPagamentoItau240(montarPagamento(selection));

  QStringList ids;

  for (const auto &index : selection) { ids << modelFolhaPag.data(index.row(), "idPagamento").toString(); }

  SqlQuery query;

  if (not query.exec("UPDATE folha_pagamento SET status = 'AGENDADO', idCnab = " + idCnab + " WHERE idPagamento IN (" + ids.join(",") + ")")) {
    throw RuntimeException("Erro alterando pagamento: " + query.lastError().text(), this);
  }

  updateTables();
}

QVector<CNAB::Pagamento> WidgetRh::montarPagamento(const QModelIndexList &selection) {
  QVector<CNAB::Pagamento> pagamentos;

  for (const auto index : selection) {
    const QString grupo = modelFolhaPag.data(index.row(), "grupo").toString();
    const QString contraParte = modelFolhaPag.data(index.row(), "contraParte").toString();
    const QString observacao = modelFolhaPag.data(index.row(), "observacao").toString();
    const QString data = modelFolhaPag.data(index.row(), "dataPagamento").toDate().toString("ddMMyyyy");

    CNAB::Pagamento pagamento;

    if (grupo == "RH - SALÁRIOS") {
      SqlQuery queryFuncionario;

      if (not queryFuncionario.exec("SELECT banco, agencia, cc, nomeBanco, cpfBanco FROM usuario WHERE nomeBanco = '" + contraParte + "'")) {
        throw RuntimeException("Erro buscando dados báncarios do funcionário: " + queryFuncionario.lastError().text());
      }

      if (not queryFuncionario.first()) { throw RuntimeException("Não encontrou o funcionário: '" + contraParte + "'"); }

      const int codBanco = queryFuncionario.value("banco").toString().left(3).toInt();
      const QString cpfDest = queryFuncionario.value("cpfBanco").toString().remove(".").remove("/").remove("-");
      const QString agencia = queryFuncionario.value("agencia").toString().remove("-");
      const QStringList contaDac = queryFuncionario.value("cc").toString().split("-");
      const QString nome = queryFuncionario.value("nomeBanco").toString();

      if (codBanco == 0 or cpfDest.isEmpty() or agencia.isEmpty() or nome.isEmpty()) { throw RuntimeError("Dados bancários incompletos do funcionário: " + contraParte); }

      if (contaDac.size() != 2) { throw RuntimeError("Conta corrente formatada errada! Deve seguir o formato XXXXX-X!\nFuncionário: " + contraParte); }

      const QString &conta = contaDac.at(0);
      const QString &dac = contaDac.at(1);

      pagamento.tipo = CNAB::Pagamento::Tipo::Salario;
      pagamento.codBanco = codBanco;
      pagamento.valor = QString::number(modelFolhaPag.data(index.row(), "valor").toDouble(), 'f', 2).remove('.').toULong();
      pagamento.observacao = observacao;
      pagamento.data = data;
      pagamento.cpfDest = cpfDest;
      pagamento.agencia = agencia.toULong();
      pagamento.conta = conta.toULong();
      pagamento.dac = dac.toULong();
      pagamento.nome = nome;

      pagamentos << pagamento;

    } else if (grupo == "PRODUTOS - VENDA") {
      SqlQuery queryFornecedor;

      if (not queryFornecedor.exec("SELECT banco, agencia, cc, nomeBanco, cnpjBanco FROM fornecedor WHERE razaoSocial = '" + contraParte + "'")) {
        throw RuntimeException("Erro buscando dados báncarios do fornecedor: " + queryFornecedor.lastError().text());
      }

      if (not queryFornecedor.first()) { throw RuntimeException("Não encontrou o fornecedor: '" + contraParte + "'"); }

      const int codBanco = queryFornecedor.value("banco").toString().left(3).toInt();
      const QString cnpjDest = queryFornecedor.value("cnpjBanco").toString().remove(".").remove("/").remove("-");
      const QString agencia = queryFornecedor.value("agencia").toString().remove("-");
      const QStringList contaDac = queryFornecedor.value("cc").toString().split("-");
      const QString nome = queryFornecedor.value("nomeBanco").toString();

      if (codBanco == 0 or cnpjDest.isEmpty() or agencia.isEmpty() or nome.isEmpty()) { throw RuntimeError("Dados bancários incompletos do fornecedor: " + contraParte); }

      if (contaDac.size() != 2) { throw RuntimeError("Conta corrente formatada errada! Deve seguir o formato XXXXX-X!\nFornecedor: " + contraParte); }

      const QString &conta = contaDac.at(0);
      const QString &dac = contaDac.at(1);

      pagamento.tipo = CNAB::Pagamento::Tipo::Fornecedor;
      pagamento.codBanco = codBanco;
      pagamento.valor = QString::number(modelFolhaPag.data(index.row(), "valor").toDouble(), 'f', 2).remove('.').toULong();
      pagamento.observacao = observacao;
      pagamento.data = data;
      pagamento.cnpjDest = cnpjDest;
      pagamento.agencia = agencia.toULong();
      pagamento.conta = conta.toULong();
      pagamento.dac = dac.toULong();
      pagamento.nome = nome;
      pagamento.codFornecedor = modelFolhaPag.data(index.row(), "codFornecedor").toString() + " " + modelFolhaPag.data(index.row(), "pf2_idVenda").toString();

      pagamentos << pagamento;

    } else {
      throw RuntimeError("Grupo não permitido: " + grupo);
    }
  }

  return pagamentos;
}

void WidgetRh::on_groupBoxData_toggled(const bool enabled) {
  const auto children = ui->groupBoxData->findChildren<QDateEdit *>(QRegularExpression("dateEdit"));

  for (const auto &child : children) { child->setEnabled(enabled); }
}

void WidgetRh::montaFiltro() {
  QStringList filtros;
  QString status;

  const auto children = ui->groupBoxStatus->findChildren<QRadioButton *>(QRegularExpression("radioButton"));

  for (const auto &child : children) {
    if (child->isChecked()) {
      if (child->text() == "Todos") { break; }

      status = child->text();
      break;
    }
  }

  if (not status.isEmpty()) { filtros << "status = '" + status + "'"; }

  //-------------------------------------

  const QString dataPagamento = ui->groupBoxData->isChecked() ? "MONTH(dataPagamento) = '" + ui->dateEdit->date().toString("MM") + "' AND YEAR(dataPagamento) = '" + ui->dateEdit->date().toString("yyyy") + "'"
                                                              : "";
  if (not dataPagamento.isEmpty()) { filtros << dataPagamento; }

  modelFolhaPag.setFilter(filtros.join(" AND "));
}

void WidgetRh::preencher(const QModelIndex &index) {
  unsetConnections();

  try {
    [&] {
      const int row = index.row();

      const QString tipoPagamento = modelFolhaPag.data(row, "tipo").toString();
      const QString parcela = modelFolhaPag.data(row, "parcela").toString();

      if (index.column() == ui->table->columnIndex("dataRealizado")) {
        const int idContaExistente = modelFolhaPag.data(row, "idConta").toInt();

        SqlQuery queryConta;

        if (not queryConta.exec("SELECT idConta FROM forma_pagamento WHERE pagamento = '" + tipoPagamento + "'")) {
          throw RuntimeException("Erro buscando conta do pagamento: " + queryConta.lastError().text(), this);
        }

        if (queryConta.first()) {
          const int idConta = queryConta.value("idConta").toInt();

          if (idContaExistente == 0 and idConta != 0) { modelFolhaPag.setData(row, "idConta", idConta); }
        }

        modelFolhaPag.setData(row, "status", "PAGO");
        modelFolhaPag.setData(row, "valorReal", modelFolhaPag.data(row, "valor"));
        modelFolhaPag.setData(row, "tipoReal", modelFolhaPag.data(row, "tipo"));
        modelFolhaPag.setData(row, "parcelaReal", modelFolhaPag.data(row, "parcela"));
        modelFolhaPag.setData(row, "centroCusto", modelFolhaPag.data(row, "idLoja"));
        modelFolhaPag.setData(row, "dataRealizado", qApp->ajustarDiaUtil(modelFolhaPag.data(row, "dataRealizado").toDate()));
      }

      if (index.column() == ui->table->columnIndex("centroCusto")) {
        if (index.data().toInt() == 0) { return; }

        modelFolhaPag.setData(row, "idLoja", modelFolhaPag.data(row, "centroCusto"));
      }

      // if (index.column() != ui->tablePendentes->columnIndex("dataRealizado")) {
      //   if (index.data().toString() == "PENDENTE") { return; }

      //   if (modelFolhaPag.data(row, "status").toString() == "PENDENTE") { modelFolhaPag.setData(row, "status", "CONFERIDO"); }
      // }
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetRh::on_pushButtonDarBaixa_clicked()
{
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  qApp->startTransaction("WidgetRh::on_pushButtonDarBaixa_clicked");

  for (const auto index : selection) {
    const int row = index.row();

    modelFolhaPag.setData(row, "status", "PAGO");
    modelFolhaPag.setData(row, "valorReal", modelFolhaPag.data(row, "valor"));
    modelFolhaPag.setData(row, "tipoReal", modelFolhaPag.data(row, "tipo"));
    modelFolhaPag.setData(row, "parcelaReal", modelFolhaPag.data(row, "parcela"));
    modelFolhaPag.setData(row, "centroCusto", modelFolhaPag.data(row, "idLoja"));
    modelFolhaPag.setData(row, "dataRealizado", qApp->ajustarDiaUtil(modelFolhaPag.data(row, "dataRealizado").toDate()));
  }

  qApp->endTransaction();
}

