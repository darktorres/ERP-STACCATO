#include "compraavulsa.h"
#include "ui_compraavulsa.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "itemboxdelegate.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"
#include "user.h"
#include "xml.h"

CompraAvulsa::CompraAvulsa(QWidget *parent) : QDialog(parent), ui(new Ui::CompraAvulsa) {
  ui->setupUi(this);
  ui->itemBoxNFe->setSearchDialog(SearchDialog::nfe(false, false, this));

  setWindowFlags(Qt::Window);
  setupTables();
  setConnections();
  show();
}

CompraAvulsa::~CompraAvulsa() { delete ui; }

void CompraAvulsa::setupTables() {
  modelCompra.setTable("compra_avulsa");

  modelCompra.setHeaderData("status", "Status");
  modelCompra.setHeaderData("fornecedor", "Fornecedor");
  modelCompra.setHeaderData("descricao", "Produto");
  modelCompra.setHeaderData("obs", "Obs.");
  modelCompra.setHeaderData("codComercial", "Cód. Com.");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("prcUnitario", "R$ Unit.");
  modelCompra.setHeaderData("preco", "R$");

  modelCompra.proxyModel = new SortFilterProxyModel(&modelCompra, this);

  ui->tableCompra->setModel(&modelCompra);

  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("dataRealCompra", new DateFormatDelegate(this));

  if (User::isAdmin()) {
    ui->tableCompra->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::CompraAvulsa, this));
    ui->tableCompra->setPersistentColumns({"status"});
  } else {
    ui->tableCompra->setItemDelegateForColumn("status", new NoEditDelegate(this));
  }

  ui->tableCompra->hideColumn("idPedido1");
  ui->tableCompra->hideColumn("idRelacionado");
  ui->tableCompra->hideColumn("idFollowup");
  ui->tableCompra->hideColumn("selecionado");
  ui->tableCompra->hideColumn("aliquotaSt");
  ui->tableCompra->hideColumn("st");
  ui->tableCompra->hideColumn("statusFinanceiro");
  ui->tableCompra->hideColumn("ordemCompra");
  ui->tableCompra->hideColumn("ordemRepresentacao");
  ui->tableCompra->hideColumn("codFornecedor");
  ui->tableCompra->hideColumn("idVenda");
  ui->tableCompra->hideColumn("idVendaProduto1");
  ui->tableCompra->hideColumn("idVendaProduto2");
  ui->tableCompra->hideColumn("idCompra");
  ui->tableCompra->hideColumn("idProduto");
  ui->tableCompra->hideColumn("colecao");
  ui->tableCompra->hideColumn("quantUpd");
  ui->tableCompra->hideColumn("un2");
  ui->tableCompra->hideColumn("caixas");
  ui->tableCompra->hideColumn("desconto");
  ui->tableCompra->hideColumn("kgcx");
  ui->tableCompra->hideColumn("formComercial");
  ui->tableCompra->hideColumn("codBarras");
  ui->tableCompra->hideColumn("dataPrevCompra");
  ui->tableCompra->hideColumn("dataRealCompra");
  ui->tableCompra->hideColumn("dataPrevConf");
  ui->tableCompra->hideColumn("dataRealConf");
  ui->tableCompra->hideColumn("dataPrevFat");
  ui->tableCompra->hideColumn("dataRealFat");
  ui->tableCompra->hideColumn("dataPrevColeta");
  ui->tableCompra->hideColumn("dataRealColeta");
  ui->tableCompra->hideColumn("dataPrevReceb");
  ui->tableCompra->hideColumn("dataRealReceb");
  ui->tableCompra->hideColumn("dataPrevEnt");
  ui->tableCompra->hideColumn("dataRealEnt");
  ui->tableCompra->hideColumn("created");
  ui->tableCompra->hideColumn("lastUpdated");

  // -------------------------------------------------------------------------

  modelPagar.setTable("conta_a_pagar_has_pagamento");

  modelPagar.setHeaderData("dataEmissao", "Data Emissão");
  modelPagar.setHeaderData("contraParte", "Contraparte");
  modelPagar.setHeaderData("idNFe", "NF-e cadastrada");
  modelPagar.setHeaderData("nfe", "NF-e");
  modelPagar.setHeaderData("valor", "R$");
  modelPagar.setHeaderData("tipo", "Tipo");
  modelPagar.setHeaderData("parcela", "Parcela");
  modelPagar.setHeaderData("dataPagamento", "Vencimento");
  modelPagar.setHeaderData("observacao", "Obs.");
  modelPagar.setHeaderData("status", "Status");
  modelPagar.setHeaderData("dataRealizado", "Data Realizado");
  modelPagar.setHeaderData("valorReal", "R$ Real");
  modelPagar.setHeaderData("tipoReal", "Tipo Real");
  modelPagar.setHeaderData("parcelaReal", "Parcela Real");
  modelPagar.setHeaderData("idConta", "Conta");
  modelPagar.setHeaderData("centroCusto", "Centro Custo");
  modelPagar.setHeaderData("grupo", "Grupo");
  modelPagar.setHeaderData("subGrupo", "SubGrupo");

  modelPagar.setSort("dataPagamento");

  modelPagar.proxyModel = new SortFilterProxyModel(&modelPagar, this);

  ui->tablePagar->setModel(&modelPagar);

  ui->tablePagar->setItemDelegateForColumn("dataEmissao", new DateFormatDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("idNFe", new ItemBoxDelegate(ItemBoxDelegate::Tipo::NFe, false, this));
  ui->tablePagar->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("tipo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagamento, this));
  ui->tablePagar->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->tablePagar->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->tablePagar->setItemDelegateForColumn("grupo", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Grupo, this));
  ui->tablePagar->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelPagar.fieldIndex("dataPagamento"), modelPagar.fieldIndex("tipo"), false, this));

  ui->tablePagar->setPersistentColumns({"idNFe", "tipo", "grupo"});

  if (User::isAdmin()) {
    ui->tablePagar->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::PagarAvulso, this));
    ui->tablePagar->setPersistentColumns({"idNFe", "tipo", "grupo", "status"});
  } else {
    ui->tablePagar->setItemDelegateForColumn("status", new NoEditDelegate(this));
  }

  ui->tablePagar->hideColumn("idPagamento");
  ui->tablePagar->hideColumn("idVenda");
  ui->tablePagar->hideColumn("idLoja");
  ui->tablePagar->hideColumn("idCnab");
  ui->tablePagar->hideColumn("tipoDet");
  ui->tablePagar->hideColumn("compraAvulsa");
  ui->tablePagar->hideColumn("desativado");
  ui->tablePagar->hideColumn("created");
  ui->tablePagar->hideColumn("lastUpdated");

  // -------------------------------------------------------------------------

  modelContaIdCompra.setTable("conta_a_pagar_has_idcompra");
}

void CompraAvulsa::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->itemBoxNFe, &ItemBox::textChanged, this, &CompraAvulsa::on_itemBoxNFe_textChanged, connectionType);
  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarPagamento_clicked, connectionType);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarProduto_clicked, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->tableCompra->model(), &QAbstractItemModel::dataChanged, this, &CompraAvulsa::alterarPagamentos, connectionType);
  connect(ui->tablePagar->model(), &QAbstractItemModel::dataChanged, this, &CompraAvulsa::on_tablePagar_dataChanged, connectionType);
}

void CompraAvulsa::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->itemBoxNFe, &ItemBox::textChanged, this, &CompraAvulsa::on_itemBoxNFe_textChanged);
  disconnect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarPagamento_clicked);
  disconnect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarProduto_clicked);
  disconnect(ui->pushButtonCancelar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonCancelar_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonSalvar_clicked);
  disconnect(ui->tableCompra->model(), &QAbstractItemModel::dataChanged, this, &CompraAvulsa::alterarPagamentos);
  disconnect(ui->tablePagar->model(), &QAbstractItemModel::dataChanged, this, &CompraAvulsa::on_tablePagar_dataChanged);
}

void CompraAvulsa::on_tablePagar_dataChanged(const QModelIndex &index) {
  unsetConnections();

  try {
    [&] {
      const int row = index.row();

      if (index.column() == ui->tablePagar->columnIndex("dataRealizado")) {
        const int idContaExistente = modelPagar.data(row, "idConta").toInt();
        const QString tipoPagamento = modelPagar.data(row, "tipo").toString();

        SqlQuery queryConta;

        if (not queryConta.exec("SELECT idConta FROM forma_pagamento WHERE pagamento = '" + tipoPagamento + "'")) {
          throw RuntimeException("Erro buscando conta do pagamento: " + queryConta.lastError().text(), this);
        }

        if (queryConta.first()) {
          const int idConta = queryConta.value("idConta").toInt();

          if (idContaExistente == 0 and idConta != 0) { modelPagar.setData(row, "idConta", idConta); }
        }

        modelPagar.setData(row, "status", "PAGO");
        modelPagar.setData(row, "valorReal", modelPagar.data(row, "valor"));
        modelPagar.setData(row, "tipoReal", modelPagar.data(row, "tipo"));
        modelPagar.setData(row, "parcelaReal", modelPagar.data(row, "parcela"));
        modelPagar.setData(row, "centroCusto", modelPagar.data(row, "idLoja"));
        modelPagar.setData(row, "dataRealizado", qApp->ajustarDiaUtil(modelPagar.data(row, "dataRealizado").toDate()));
      }

      if (index.column() == ui->tablePagar->columnIndex("centroCusto")) {
        if (index.data().toInt() == 0) { return; }

        modelPagar.setData(row, "idLoja", modelPagar.data(row, "centroCusto"));
      }
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void CompraAvulsa::on_pushButtonAdicionarProduto_clicked() {
  const int row = modelCompra.insertRowAtEnd();

  modelCompra.setData(row, "status", "PEND. APROV.");

  ui->tableCompra->redoView();
}

void CompraAvulsa::on_pushButtonAdicionarPagamento_clicked() {
  const int row = modelPagar.insertRowAtEnd();

  modelPagar.setData(row, "dataEmissao", qApp->serverDate());
  modelPagar.setData(row, "status", "PEND. APROV.");
  modelPagar.setData(row, "compraAvulsa", true);

  ui->tablePagar->redoView();
}

void CompraAvulsa::on_pushButtonSalvar_clicked() {
  try {
    verifyFields();
  } catch (std::exception &) { throw; }

  qApp->startTransaction("CompraAvulsa::on_pushButtonSalvar");

  // -------------------------------------------------------------------------

  QStringList idNFes;

  for (int row = 0; row < modelPagar.rowCount(); ++row) {
    idNFes << modelPagar.data(row, "idNFe").toString();
  }

  idNFes.removeDuplicates();

  SqlQuery query;

  if (not query.exec("UPDATE nfe SET utilizada = TRUE WHERE idNFe IN (" + idNFes.join(", ") + ")")) { throw RuntimeException("Erro marcando NFes como utilizadas: " + query.lastError().text()); }

  // -------------------------------------------------------------------------

  if (not query.exec("SELECT MAX(idCompra) AS idCompra FROM compra_avulsa")) { throw RuntimeException("Erro gerando O.C.: " + query.lastError().text()); }

  int last = 0;

  if (query.first()) { last = query.value("idCompra").toString().remove("C.A.").toInt(); }

  QString idCompra = "C.A." + QString::number(last + 1).rightJustified(5, '0');

  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    modelCompra.setData(row, "idCompra", idCompra);
  }

  for (int row = 0; row < modelPagar.rowCount(); ++row) {
    modelPagar.setData(row, "idCompra", idCompra);
  }

  modelCompra.submitAll();
  modelPagar.submitAll();

  // -------------------------------------------------------------------------

  qApp->endTransaction();

  QDialog::accept();
  close();
}

void CompraAvulsa::verifyFields() {
  double totalCompra = 0;

  for (auto row = 0; row < modelCompra.rowCount(); ++row) {
    if (modelCompra.data(row, "fornecedor").isNull()) { throw RuntimeError("'Fornecedor' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelCompra.data(row, "descricao").isNull()) { throw RuntimeError("'Produto' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelCompra.data(row, "quant").isNull()) { throw RuntimeError("'Quant.' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelCompra.data(row, "un").isNull()) { throw RuntimeError("'Un.' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelCompra.data(row, "prcUnitario").isNull()) { throw RuntimeError("'R$ Unit.' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelCompra.data(row, "preco").isNull()) { throw RuntimeError("'R$' não preenchido na linha " + QString::number(row + 1) + "!"); }

    totalCompra += modelCompra.data(row, "preco").toDouble();
  }

  double totalPagar = 0;

  for (auto row = 0; row < modelPagar.rowCount(); ++row) {
    if (modelPagar.data(row, "contraParte").isNull()) { throw RuntimeError("'Contraparte' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelPagar.data(row, "valor").isNull()) { throw RuntimeError("'R$' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelPagar.data(row, "tipo").isNull()) { throw RuntimeError("'Tipo' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelPagar.data(row, "centroCusto").isNull()) { throw RuntimeError("'Centro Custo' não preenchido na linha " + QString::number(row + 1) + "!"); }

    totalPagar += modelPagar.data(row, "valor").toDouble();
  }

  if (not qFuzzyCompare(totalCompra, totalPagar)) {
    throw RuntimeError("Total dos produtos diferente do total dos pagamentos!");
  }
}

void CompraAvulsa::on_pushButtonCancelar_clicked() {
  close();
}

void CompraAvulsa::on_itemBoxNFe_textChanged(const QString &text) {
  if (text.isEmpty()) { return; }

  SqlQuery query;

  if (not query.exec("SELECT n.idNFe, n.xml, n.status, n.utilizada, l.idLoja FROM nfe n LEFT JOIN loja l ON n.cnpjDest = REGEXP_REPLACE(l.cnpj, '[^0-9]', '') WHERE n.idNFe = " + ui->itemBoxNFe->getId().toString())) {
    throw RuntimeException("Erro buscando XML: " + query.lastError().text(), this);
  }

  if (not query.first()) { throw RuntimeException("XML não encontrado para NF-e com id: '" + ui->itemBoxNFe->getId().toString() + "'"); }

  if (query.value("status").toString() != "AUTORIZADA") { throw RuntimeError("NF-e não está autorizada!", this); }

  const auto fileContent = query.value("xml").toString();

  // ----------------------------------------------------------------

  XML xml(fileContent, XML::Tipo::Entrada, this);
  xml.idNFe = query.value("idNFe").toInt();

  for (auto produto : xml.produtos) {
    const int row = modelCompra.insertRowAtEnd();

    modelCompra.setData(row, "status", "PEND. APROV.");
    modelCompra.setData(row, "fornecedor", xml.xNome);
    modelCompra.setData(row, "descricao", produto.descricao);
    modelCompra.setData(row, "obs", "NFe: " + xml.chaveAcesso);
    modelCompra.setData(row, "codComercial", produto.codProd);
    modelCompra.setData(row, "quant", produto.quant);
    modelCompra.setData(row, "un", produto.un);
    modelCompra.setData(row, "prcUnitario", produto.valorUnidTrib);
    modelCompra.setData(row, "preco", produto.valor);
    modelCompra.setData(row, "dataRealCompra", xml.dataHoraEmissao);
  }

  int parcela = 1;

  for (auto duplicata : xml.duplicatas) {
    const int row = modelPagar.insertRowAtEnd();

    modelPagar.setData(row, "status", "PEND. APROV.");
    modelPagar.setData(row, "dataEmissao", xml.dataHoraEmissao);
    modelPagar.setData(row, "idLoja", query.value("idLoja"));
    modelPagar.setData(row, "contraParte", xml.xNome);
    modelPagar.setData(row, "idNFe", xml.idNFe);
    modelPagar.setData(row, "nfe", xml.chaveAcesso);
    modelPagar.setData(row, "valor", duplicata.vDup);
    modelPagar.setData(row, "tipo", "BOLETO");
    modelPagar.setData(row, "parcela", parcela++);
    modelPagar.setData(row, "dataPagamento", duplicata.dVenc);
    modelPagar.setData(row, "observacao", "Duplicata: " + duplicata.nDup);
    modelPagar.setData(row, "centroCusto", query.value("idLoja"));
    // modelPagar.setData(row, "grupo", "???");
    // modelPagar.setData(row, "subgrupo", "???");
  }
}

void CompraAvulsa::viewRegisterById(const QString &idCompra) {
  modelCompra.setFilter("idCompra = '" + idCompra + "'");
  modelCompra.select();

  modelPagar.setFilter("idPagamento IN (SELECT idPagamento FROM conta_a_pagar_has_idcompra WHERE idCompra = '" + idCompra + "') AND desativado = FALSE");
  modelPagar.select();

  modelContaIdCompra.setFilter("idCompra = '" + idCompra + "'");

  if (not User::isAdmin()) {
    ui->tableCompra->closePersistentEditors();
    ui->tablePagar->closePersistentEditors();

    ui->tableCompra->setPersistentColumns({});
    ui->tablePagar->setPersistentColumns({});

    ui->tableCompra->setEditTriggers(QTableView::NoEditTriggers);
    ui->tablePagar->setEditTriggers(QTableView::NoEditTriggers);

    ui->pushButtonAdicionarProduto->hide();
    ui->pushButtonAdicionarPagamento->hide();
  }
}

void CompraAvulsa::alterarPagamentos(const QModelIndex &index) {
  unsetConnections();

  try {
    [&] {
      const int row = index.row();

      if (index.column() == ui->tableCompra->columnIndex("status")) {
        const QString status = modelCompra.data(row, "status").toString();

        if (status == "PEND. APROV." or status == "CONFERIDO" or status == "CANCELADO") {
          for (int row = 0; row < modelPagar.rowCount(); ++row) {
            // TODO: verificar quais status podem ser alterados, por exemplo, se o pagamento já estiver como pago não deve ser alterado para cancelado
            modelPagar.setData(row, "status", status);
          }
        }
      }
    }();
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

// TODO: colocar um autopreenchimento no prcUnitario/preco
