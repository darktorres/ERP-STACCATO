#include "compraavulsa.h"
#include "ui_compraavulsa.h"

#include "application.h"
#include "comboboxdelegate.h"
#include "dateformatdelegate.h"
#include "itemboxdelegate.h"
#include "lineeditdelegate.h"
#include "reaisdelegate.h"
#include "sortfilterproxymodel.h"
#include "sqlquery.h"
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

  modelCompra.setHeaderData("fornecedor", "Fornecedor");
  modelCompra.setHeaderData("descricao", "Produto");
  modelCompra.setHeaderData("obs", "Obs.");
  modelCompra.setHeaderData("codComercial", "Cód. Com.");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("prcUnitario", "R$ Unit.");
  modelCompra.setHeaderData("preco", "R$");
  modelCompra.setHeaderData("dataRealCompra", "Data");

  ui->tableCompra->setModel(&modelCompra);

  ui->tableCompra->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this));
  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("dataRealCompra", new DateFormatDelegate(this));

  // ui->tableCompra->setPersistentColumns({"status"});

  ui->tableCompra->hideColumn("idPedido1");
  ui->tableCompra->hideColumn("status");
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
  modelPagar.setHeaderData("centroCusto", "Centro Custo");
  modelPagar.setHeaderData("grupo", "Grupo");
  modelPagar.setHeaderData("subGrupo", "SubGrupo");

  modelPagar.setSort("dataPagamento");

  modelPagar.proxyModel = new SortFilterProxyModel(&modelPagar, this);

  ui->tablePagar->setModel(&modelPagar);

  ui->tablePagar->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("dataEmissao", new DateFormatDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("valorReal", new ReaisDelegate(this));
  ui->tablePagar->setItemDelegateForColumn("dataRealizado", new DateFormatDelegate(modelPagar.fieldIndex("dataPagamento"), modelPagar.fieldIndex("tipo"), false, this));

  ui->tablePagar->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::Pagar, this));

  ui->tablePagar->setItemDelegateForColumn("idNFe", new ItemBoxDelegate(ItemBoxDelegate::Tipo::NFe, false, this));
  ui->tablePagar->setItemDelegateForColumn("idConta", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Conta, false, this));
  ui->tablePagar->setItemDelegateForColumn("centroCusto", new ItemBoxDelegate(ItemBoxDelegate::Tipo::Loja, false, this));
  ui->tablePagar->setItemDelegateForColumn("grupo", new LineEditDelegate(LineEditDelegate::Tipo::Grupo, this));

  ui->tablePagar->setPersistentColumns({"status", "idNFe"});

  ui->tablePagar->hideColumn("idPagamento");
  ui->tablePagar->hideColumn("idCompra");
  ui->tablePagar->hideColumn("idVenda");
  ui->tablePagar->hideColumn("idLoja");
  ui->tablePagar->hideColumn("idCnab");
  ui->tablePagar->hideColumn("dataRealizado");
  ui->tablePagar->hideColumn("valorReal");
  ui->tablePagar->hideColumn("tipoReal");
  ui->tablePagar->hideColumn("parcelaReal");
  ui->tablePagar->hideColumn("idConta");
  ui->tablePagar->hideColumn("tipoDet");
  ui->tablePagar->hideColumn("compraAvulsa");
  ui->tablePagar->hideColumn("desativado");
  ui->tablePagar->hideColumn("created");
  ui->tablePagar->hideColumn("lastUpdated");
}

void CompraAvulsa::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarPagamento_clicked, connectionType);
  connect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarProduto_clicked, connectionType);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonCancelar_clicked, connectionType);
  connect(ui->pushButtonRemoverLinhasPagamento, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonRemoverLinhasPagamento_clicked, connectionType);
  connect(ui->pushButtonRemoverLinhasProdutos, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonRemoverLinhasProdutos_clicked, connectionType);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonSalvar_clicked, connectionType);
  connect(ui->itemBoxNFe, &ItemBox::textChanged, this, &CompraAvulsa::on_itemBoxNFe_textChanged, connectionType);
  connect(ui->tablePagar->model(), &QAbstractItemModel::dataChanged, this, &CompraAvulsa::on_tablePagar_dataChanged, connectionType);
}

void CompraAvulsa::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->pushButtonAdicionarPagamento, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarPagamento_clicked);
  disconnect(ui->pushButtonAdicionarProduto, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonAdicionarProduto_clicked);
  disconnect(ui->pushButtonCancelar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonCancelar_clicked);
  disconnect(ui->pushButtonRemoverLinhasPagamento, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonRemoverLinhasPagamento_clicked);
  disconnect(ui->pushButtonRemoverLinhasProdutos, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonRemoverLinhasProdutos_clicked);
  disconnect(ui->pushButtonSalvar, &QPushButton::clicked, this, &CompraAvulsa::on_pushButtonSalvar_clicked);
  disconnect(ui->itemBoxNFe, &ItemBox::textChanged, this, &CompraAvulsa::on_itemBoxNFe_textChanged);
  disconnect(ui->tablePagar->model(), &QAbstractItemModel::dataChanged, this, &CompraAvulsa::on_tablePagar_dataChanged);
}

void CompraAvulsa::on_tablePagar_dataChanged(const QModelIndex &index) {
  unsetConnections();

  try {
    [&] {
      const int row = index.row();

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
  modelCompra.insertRowAtEnd();

  ui->tableCompra->redoView();
}

void CompraAvulsa::on_pushButtonAdicionarPagamento_clicked() {
  modelPagar.insertRowAtEnd();

  ui->tablePagar->redoView();
}

void CompraAvulsa::on_pushButtonRemoverLinhasProdutos_clicked() {
  const auto selection = ui->tableCompra->selectionModel()->selectedIndexes();

  if (selection.isEmpty()) { throw RuntimeError("Não selecionou nenhuma linha!", this); }

  modelCompra.removeSelection(selection);
}

void CompraAvulsa::on_pushButtonRemoverLinhasPagamento_clicked() {
  const auto selection = ui->tablePagar->selectionModel()->selectedIndexes();

  if (selection.isEmpty()) { throw RuntimeError("Não selecionou nenhuma linha!", this); }

  modelPagar.removeSelection(selection);
}

void CompraAvulsa::on_pushButtonSalvar_clicked() {
  try {
    verifyFields();
  } catch (std::exception &) { throw; }

  qApp->startTransaction("CompraAvulsa::on_pushButtonSalvar");

  QStringList idNFes;

  for (int row = 0; row < modelPagar.rowCount(); ++row) {
    idNFes << modelPagar.data(row, "idNFe").toString();
  }

  idNFes.removeDuplicates();

  SqlQuery query;

  if (not query.exec("UPDATE nfe SET utilizada = TRUE WHERE idNFe IN (" + idNFes.join(", ") + ")")) { throw RuntimeException("Erro marcando NFes como utilizadas: " + query.lastError().text()); }

  modelCompra.submitAll();
  modelPagar.submitAll();

  qApp->endTransaction();

  QDialog::accept();
  close();
}

void CompraAvulsa::verifyFields() {
  for (auto row = 0; row < modelPagar.rowCount(); ++row) {
    if (modelPagar.data(row, "idLoja").isNull()) { throw RuntimeError("'Centro Custo' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelPagar.data(row, "valor").isNull()) { throw RuntimeError("'R$' não preenchido na linha " + QString::number(row + 1) + "!"); }
    if (modelPagar.data(row, "tipo").isNull()) { throw RuntimeError("'Tipo' não preenchido na linha " + QString::number(row + 1) + "!"); }
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
    // modelPagar.setData(row, "grupo", duplicata.dVenc); // ???
    // modelPagar.setData(row, "subgrupo", duplicata.dVenc); // ???
    modelPagar.setData(row, "compraAvulsa", true);
  }
}

// TODO: fazer mensagem de erro para 'No Fields to update'
// TODO: colocar um autopreenchimento no prcUnitario/preco
