#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "doubledelegate.h"
#include "estoqueproxymodel.h"
#include "importarxml.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "singleeditdelegate.h"
#include "ui_importarxml.h"

ImportarXML::ImportarXML(const QStringList &idsCompra, const QDateTime &dataReal, QWidget *parent) : QDialog(parent), dataReal(dataReal), idsCompra(idsCompra), ui(new Ui::ImportarXML) {
  ui->setupUi(this);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonImportar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonImportar_clicked);
  connect(ui->pushButtonProcurar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonProcurar_clicked);
  connect(ui->checkBoxSemLote, &QCheckBox::toggled, this, &ImportarXML::on_checkBoxSemLote_toggled);

  setWindowFlags(Qt::Window);

  setupTables();
}

ImportarXML::~ImportarXML() { delete ui; }

void ImportarXML::setupTables() {
  modelEstoque.setTable("estoque");

  modelEstoque.setHeaderData("status", "Status");
  modelEstoque.setHeaderData("numeroNFe", "NFe");
  modelEstoque.setHeaderData("fornecedor", "Fornecedor");
  modelEstoque.setHeaderData("descricao", "Produto");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("bloco", "Bloco");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Cx.");
  modelEstoque.setHeaderData("codBarras", "Cód. Bar.");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");
  modelEstoque.setHeaderData("valorUnid", "R$ Unid.");
  modelEstoque.setHeaderData("valor", "R$");
  modelEstoque.setHeaderData("vICMSST", "ST");

  ui->tableEstoque->setModel(new EstoqueProxyModel(&modelEstoque, this));

  ui->tableEstoque->setItemDelegate(new NoEditDelegate(this));

  ui->tableEstoque->setItemDelegateForColumn("codComercial", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("lote", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("bloco", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("quant", new DoubleDelegate(this, 3));
  ui->tableEstoque->setItemDelegateForColumn("un", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("descricao", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("valorUnid", new ReaisDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("vICMSST", new ReaisDelegate(this));

  ui->tableEstoque->hideColumn("idEstoque");
  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("recebidoPor");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("observacao");
  ui->tableEstoque->hideColumn("quantUpd");
  ui->tableEstoque->hideColumn("ncm");
  ui->tableEstoque->hideColumn("cfop");
  ui->tableEstoque->hideColumn("codBarrasTrib");
  ui->tableEstoque->hideColumn("unTrib");
  ui->tableEstoque->hideColumn("quantTrib");
  ui->tableEstoque->hideColumn("valorTrib");
  ui->tableEstoque->hideColumn("desconto");
  ui->tableEstoque->hideColumn("compoeTotal");
  ui->tableEstoque->hideColumn("numeroPedido");
  ui->tableEstoque->hideColumn("itemPedido");
  ui->tableEstoque->hideColumn("tipoICMS");
  ui->tableEstoque->hideColumn("orig");
  ui->tableEstoque->hideColumn("cstICMS");
  ui->tableEstoque->hideColumn("modBC");
  ui->tableEstoque->hideColumn("vBC");
  ui->tableEstoque->hideColumn("pICMS");
  ui->tableEstoque->hideColumn("vICMS");
  ui->tableEstoque->hideColumn("modBCST");
  ui->tableEstoque->hideColumn("pMVAST");
  ui->tableEstoque->hideColumn("vBCST");
  ui->tableEstoque->hideColumn("pICMSST");
  ui->tableEstoque->hideColumn("cEnq");
  ui->tableEstoque->hideColumn("cstIPI");
  ui->tableEstoque->hideColumn("cstPIS");
  ui->tableEstoque->hideColumn("vBCPIS");
  ui->tableEstoque->hideColumn("pPIS");
  ui->tableEstoque->hideColumn("vPIS");
  ui->tableEstoque->hideColumn("cstCOFINS");
  ui->tableEstoque->hideColumn("vBCCOFINS");
  ui->tableEstoque->hideColumn("pCOFINS");
  ui->tableEstoque->hideColumn("vCOFINS");

  // -------------------------------------------------------------------------

  modelConsumo.setTable("estoque_has_consumo");

  modelConsumo.setHeaderData("status", "Status");
  modelConsumo.setHeaderData("numeroNFe", "NFe");
  modelConsumo.setHeaderData("local", "Local");
  modelConsumo.setHeaderData("fornecedor", "Fornecedor");
  modelConsumo.setHeaderData("descricao", "Produto");
  modelConsumo.setHeaderData("quant", "Quant.");
  modelConsumo.setHeaderData("un", "Un.");
  modelConsumo.setHeaderData("caixas", "Cx.");
  modelConsumo.setHeaderData("codBarras", "Cód. Bar.");
  modelConsumo.setHeaderData("codComercial", "Cód. Com.");
  modelConsumo.setHeaderData("valorUnid", "R$ Unid.");
  modelConsumo.setHeaderData("valor", "R$");

  ui->tableConsumo->setModel(new EstoqueProxyModel(&modelConsumo, this));

  ui->tableConsumo->setItemDelegate(new NoEditDelegate(this));

  ui->tableConsumo->setItemDelegateForColumn("valorUnid", new ReaisDelegate(this));
  ui->tableConsumo->setItemDelegateForColumn("valor", new ReaisDelegate(this));

  ui->tableConsumo->hideColumn("idPedido");
  ui->tableConsumo->hideColumn("idConsumo");
  ui->tableConsumo->hideColumn("quantUpd");
  ui->tableConsumo->hideColumn("idNFe");
  ui->tableConsumo->hideColumn("idEstoque");
  ui->tableConsumo->hideColumn("idCompra");
  ui->tableConsumo->hideColumn("idVendaProduto");
  ui->tableConsumo->hideColumn("idProduto");
  ui->tableConsumo->hideColumn("ncm");
  ui->tableConsumo->hideColumn("cfop");
  ui->tableConsumo->hideColumn("codBarrasTrib");
  ui->tableConsumo->hideColumn("unTrib");
  ui->tableConsumo->hideColumn("quantTrib");
  ui->tableConsumo->hideColumn("valorTrib");
  ui->tableConsumo->hideColumn("desconto");
  ui->tableConsumo->hideColumn("compoeTotal");
  ui->tableConsumo->hideColumn("numeroPedido");
  ui->tableConsumo->hideColumn("itemPedido");
  ui->tableConsumo->hideColumn("tipoICMS");
  ui->tableConsumo->hideColumn("orig");
  ui->tableConsumo->hideColumn("cstICMS");
  ui->tableConsumo->hideColumn("modBC");
  ui->tableConsumo->hideColumn("vBC");
  ui->tableConsumo->hideColumn("pICMS");
  ui->tableConsumo->hideColumn("vICMS");
  ui->tableConsumo->hideColumn("modBCST");
  ui->tableConsumo->hideColumn("pMVAST");
  ui->tableConsumo->hideColumn("vBCST");
  ui->tableConsumo->hideColumn("pICMSST");
  ui->tableConsumo->hideColumn("vICMSST");
  ui->tableConsumo->hideColumn("cEnq");
  ui->tableConsumo->hideColumn("cstIPI");
  ui->tableConsumo->hideColumn("cstPIS");
  ui->tableConsumo->hideColumn("vBCPIS");
  ui->tableConsumo->hideColumn("pPIS");
  ui->tableConsumo->hideColumn("vPIS");
  ui->tableConsumo->hideColumn("cstCOFINS");
  ui->tableConsumo->hideColumn("vBCCOFINS");
  ui->tableConsumo->hideColumn("pCOFINS");
  ui->tableConsumo->hideColumn("vCOFINS");

  // -------------------------------------------------------------------------

  modelCompra.setTable("pedido_fornecedor_has_produto");

  modelCompra.setHeaderData("status", "Status");
  modelCompra.setHeaderData("ordemCompra", "OC");
  modelCompra.setHeaderData("idVenda", "Venda");
  modelCompra.setHeaderData("fornecedor", "Fornecedor");
  modelCompra.setHeaderData("descricao", "Produto");
  modelCompra.setHeaderData("colecao", "Coleção");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("quantConsumida", "Consumido");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("un2", "Un. 2");
  modelCompra.setHeaderData("caixas", "Cx.");
  modelCompra.setHeaderData("prcUnitario", "R$ Unid.");
  modelCompra.setHeaderData("preco", "R$");
  modelCompra.setHeaderData("kgcx", "Kg./Cx.");
  modelCompra.setHeaderData("formComercial", "Form. Com.");
  modelCompra.setHeaderData("codComercial", "Cód. Com.");
  modelCompra.setHeaderData("codBarras", "Cód. Bar.");
  modelCompra.setHeaderData("obs", "Obs.");

  modelCompra.setFilter("idCompra = " + idsCompra.join(" OR idCompra = "));

  if (not modelCompra.select()) { return; }

  ui->tableCompra->setModel(new EstoqueProxyModel(&modelCompra, this));

  ui->tableCompra->setItemDelegate(new NoEditDelegate(this));

  ui->tableCompra->setItemDelegateForColumn("codComercial", new SingleEditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("descricao", new SingleEditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  ui->tableCompra->hideColumn("ordemRepresentacao");
  ui->tableCompra->hideColumn("idVendaProduto");
  ui->tableCompra->hideColumn("selecionado");
  ui->tableCompra->hideColumn("statusFinanceiro");
  ui->tableCompra->hideColumn("quantUpd");
  ui->tableCompra->hideColumn("idCompra");
  ui->tableCompra->hideColumn("idNFe");
  ui->tableCompra->hideColumn("idEstoque");
  ui->tableCompra->hideColumn("idPedido");
  ui->tableCompra->hideColumn("idProduto");
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
  ui->tableCompra->hideColumn("aliquotaSt");
  ui->tableCompra->hideColumn("st");

  // -------------------------------------------------------------------------

  modelNFe.setTable("nfe");

  // -------------------------------------------------------------------------

  modelEstoque_compra.setTable("estoque_has_compra");
}

bool ImportarXML::cadastrarProdutoEstoque(const QVector<std::tuple<int, int, double>> &tuples) {
  QSqlQuery query;
  // TODO: change hardcoded 'validade'
  query.prepare(
      "INSERT INTO produto SELECT NULL, p.idProdutoUpd, :idEstoque, p.idFornecedor, p.idFornecedorUpd, p.fornecedor, p.fornecedorUpd, CONCAT(p.descricao, ' (ESTOQUE)'), p.descricaoUpd, "
      ":estoqueRestante, p.estoqueRestanteUpd, p.un, p.unUpd, p.un2, p.un2Upd, p.colecao, p.colecaoUpd, p.tipo, p.tipoUpd, p.minimo, p.minimoUpd, p.multiplo, p.multiploUpd, p.m2cx, p.m2cxUpd, "
      "p.pccx, p.pccxUpd, p.kgcx, p.kgcxUpd, p.formComercial, p.formComercialUpd, p.codComercial, p.codComercialUpd, p.codBarras, p.codBarrasUpd, p.ncm, p.ncmUpd, p.ncmEx, p.ncmExUpd, p.cfop, "
      "p.cfopUpd, p.icms, p.icmsUpd, p.cst, p.cstUpd, p.qtdPallet, p.qtdPalletUpd, p.custo, p.custoUpd, p.ipi, p.ipiUpd, p.st, p.stUpd, p.precoVenda, p.precoVendaUpd, p.markup, p.markupUpd, "
      "p.comissao, p.comissaoUpd, p.observacoes, p.observacoesUpd, p.origem, p.origemUpd, p.temLote, p.temLoteUpd, p.ui, p.uiUpd, '2020-12-31', p.validadeUpd, :descontinuado, p.descontinuadoUpd, "
      "p.atualizarTabelaPreco, p.representacao, 1, 0, p.idProduto, 0, NULL, NULL FROM produto p WHERE p.idProduto = :idProduto");

  for (const auto &tuple : tuples) {
    const auto [idProduto, idEstoque, estoqueRestante] = tuple;

    query.bindValue(":idProduto", idProduto);
    query.bindValue(":idEstoque", idEstoque);
    query.bindValue(":estoqueRestante", estoqueRestante);
    query.bindValue(":descontinuado", qFuzzyIsNull(estoqueRestante) ? true : false);

    if (not query.exec()) { return qApp->enqueueError(false, "Erro criando produto_estoque: " + query.lastError().text(), this); }
  }

  return true;
}

QVector<std::tuple<int, int, double>> ImportarXML::mapTuples() {
  QVector<std::tuple<int, int, double>> tuples;

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    double consumo = 0;

    for (int row2 = 0; row2 < modelConsumo.rowCount(); ++row2) {
      if (modelConsumo.data(row2, "idEstoque").toInt() != modelEstoque.data(row, "idEstoque").toInt()) { continue; }

      consumo += modelConsumo.data(row2, "quant").toDouble();
    }

    const double estoqueRestante = modelEstoque.data(row, "quant").toDouble() + consumo;
    const int idProduto = modelEstoque.data(row, "idProduto").toInt();
    const int idEstoque = modelEstoque.data(row, "idEstoque").toInt();

    tuples << std::make_tuple(idProduto, idEstoque, estoqueRestante);
  }

  return tuples;
}

bool ImportarXML::salvarLoteNaVenda() {
  QSqlQuery queryLote;
  queryLote.prepare("UPDATE venda_has_produto SET lote = :lote WHERE idVendaProduto = :idVendaProduto");

  for (int row = 0; row < modelConsumo.rowCount(); ++row) {
    const int idEstoque = modelConsumo.data(row, "idEstoque").toInt();

    const auto match = modelEstoque.match("idEstoque", idEstoque, 1, Qt::MatchExactly);

    if (match.isEmpty()) { return qApp->enqueueError(false, "Error out!", this); }

    const int rowEstoque = match.first().row();
    const QString lote = modelEstoque.data(rowEstoque, "lote").toString();
    const int idVendaProduto = modelConsumo.data(row, "idVendaProduto").toInt();

    queryLote.bindValue(":lote", lote);
    queryLote.bindValue(":idVendaProduto", idVendaProduto);

    if (not queryLote.exec()) { return qApp->enqueueError(false, "Erro atualizando lote na venda: " + queryLote.lastError().text(), this); }
  }

  return true;
}

bool ImportarXML::importar() {
  for (int row = 0; row < modelCompra.rowCount(); ++row)
    if (not modelCompra.setData(row, "selecionado", false)) { return false; }

  if (not salvarLoteNaVenda()) { return false; }

  // mapear idProduto/idEstoque para cadastrarProdutoEstoque
  const auto tuples = mapTuples();

  if (not modelEstoque.submitAll()) { return false; }

  if (not modelCompra.submitAll()) { return false; }

  if (not modelConsumo.submitAll()) { return false; }

  if (not modelEstoque_compra.submitAll()) { return false; }

  if (not modelNFe.submitAll()) { return false; }

  if (not cadastrarProdutoEstoque(tuples)) { return false; }

  if (not atualizaDados()) { return false; }

  return true;
}

bool ImportarXML::atualizaDados() {
  QSqlQuery query;
  query.prepare("UPDATE venda_has_produto SET status = 'EM COLETA', dataRealFat = :dataRealFat WHERE idVendaProduto = :idVendaProduto AND status = 'EM FATURAMENTO'");

  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    const int idVendaProduto = modelCompra.data(row, "idVendaProduto").toInt();
    const FieldColors color = static_cast<FieldColors>(modelCompra.data(row, "quantUpd").toInt());
    const QString status = modelCompra.data(row, "status").toString();

    if (idVendaProduto == 0) { continue; }
    if (color == FieldColors::None) { continue; }
    if (status != "EM FATURAMENTO") { continue; }

    query.bindValue(":dataRealFat", dataReal);
    query.bindValue(":idVendaProduto", modelCompra.data(row, "idVendaProduto"));

    if (not query.exec()) { return qApp->enqueueError(false, "Erro atualizando status do produto da venda: " + query.lastError().text(), this); }
  }

  QSqlQuery query2;
  query2.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM COLETA', dataRealFat = :dataRealFat WHERE idCompra = :idCompra AND quantUpd = 1 AND status = 'EM FATURAMENTO'");

  for (const auto &idCompra : idsCompra) {
    query2.bindValue(":dataRealFat", dataReal);
    query2.bindValue(":idCompra", idCompra);

    if (not query2.exec()) { return qApp->enqueueError(false, "Erro atualizando status da compra: " + query2.lastError().text(), this); }
  }

  return true;
}

bool ImportarXML::verifyFields() {
  bool ok = false;

  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    if (modelCompra.data(row, "quantUpd") == static_cast<int>(FieldColors::Green)) {
      ok = true;
      break;
    }
  }

  if (not ok) { return qApp->enqueueError(false, "Nenhuma compra pareada!", this); }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    const FieldColors color = static_cast<FieldColors>(modelEstoque.data(row, "quantUpd").toInt());

    if (color == FieldColors::Red) { return qApp->enqueueError(false, "Nem todos os estoques estão ok!", this); }
  }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (modelEstoque.data(row, "lote").toString().isEmpty()) { return qApp->enqueueError(false, "Lote vazio! Se não há coloque 'N/D'", this); }
  }

  return true;
}

void ImportarXML::on_pushButtonImportar_clicked() {
  if (not verifyFields()) { return; }

  if (not qApp->startTransaction()) { return; }

  disconnect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::reparear);

  if (not importar()) { return qApp->rollbackTransaction(); }

  connect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::reparear);

  if (not qApp->endTransaction()) { return; }

  QDialog::accept();
  close();
}

bool ImportarXML::limparAssociacoes() {
  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    const QString status = modelCompra.data(row, "status").toString();
    const int color = modelCompra.data(row, "quantUpd").toInt();

    if (status != "EM FATURAMENTO" and color == static_cast<int>(FieldColors::Green)) { continue; }

    if (not modelCompra.setData(row, "quantUpd", static_cast<int>(FieldColors::None))) { return false; }
    if (not modelCompra.setData(row, "quantConsumida", 0)) { return false; }
  }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (not modelEstoque.setData(row, "quantUpd", static_cast<int>(FieldColors::None))) { return false; }
  }

  modelConsumo.revertAll();
  modelEstoque_compra.revertAll();

  return true;
}

void ImportarXML::on_pushButtonProcurar_clicked() {
  disconnect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::reparear);

  [&] {
    if (not lerXML()) { return; }

    if (not parear()) { return; }

    bool ok = true;

    for (int row = 0; row < modelCompra.rowCount(); ++row) {
      if (static_cast<FieldColors>(modelCompra.data(row, "quantUpd").toInt()) != FieldColors::Green) {
        ok = false;
        break;
      }
    }

    if (ok) { ui->pushButtonProcurar->setDisabled(true); }
  }();

  connect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::reparear);
}

std::optional<double> ImportarXML::buscarCaixas(const int rowEstoque) {
  QSqlQuery query;
  query.prepare("SELECT idProduto, m2cx, pccx, UPPER(un) AS un FROM produto WHERE codComercial = :codComercial");
  query.bindValue(":codComercial", modelEstoque.data(rowEstoque, "codComercial"));

  if (not query.exec()) {
    qApp->enqueueError(false, "Erro lendo tabela produto: " + query.lastError().text(), this);
    return {};
  }

  if (not query.first()) { return {}; }

  const QString un = modelEstoque.data(rowEstoque, "un").toString();

  const double quantCaixa = un == "M2" or un == "M²" or un == "ML" ? query.value("m2cx").toDouble() : query.value("pccx").toDouble();

  const double quant = modelEstoque.data(rowEstoque, "quant").toDouble();

  const double caixas = qRound(quant / quantCaixa * 100) / 100.;

  return caixas;
}

bool ImportarXML::associarItens(const int rowCompra, const int rowEstoque, double &estoqueConsumido) {
  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();
  const double quantConsumida = modelCompra.data(rowCompra, "quantConsumida").toDouble();

  const double espaco = quantCompra - quantConsumida;
  const double estoqueDisponivel = quantEstoque - estoqueConsumido;
  const double quantAdicionar = qMin(estoqueDisponivel, espaco);
  estoqueConsumido += quantAdicionar;

  if (not modelCompra.setData(rowCompra, "descricao", modelEstoque.data(rowEstoque, "descricao").toString())) { return false; }
  if (not modelCompra.setData(rowCompra, "quantConsumida", quantConsumida + quantAdicionar)) { return false; }
  if (not modelCompra.setData(rowCompra, "quantUpd", static_cast<int>(qFuzzyCompare((quantConsumida + quantAdicionar), quantCompra) ? FieldColors::Green : FieldColors::Yellow))) { return false; }

  const auto caixas = buscarCaixas(rowEstoque);

  if (caixas and not modelEstoque.setData(rowEstoque, "caixas", caixas.value())) { return false; }
  if (not modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(qFuzzyCompare(estoqueConsumido, quantEstoque) ? FieldColors::Green : FieldColors::Yellow))) { return false; }
  if (not modelEstoque.setData(rowEstoque, "idProduto", modelCompra.data(rowCompra, "idProduto"))) { return false; }

  const int rowEstoque_compra = modelEstoque_compra.rowCount();
  modelEstoque_compra.insertRow(rowEstoque_compra);

  if (not modelEstoque_compra.setData(rowEstoque_compra, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"))) { return false; }
  if (not modelEstoque_compra.setData(rowEstoque_compra, "idCompra", modelCompra.data(rowCompra, "idCompra"))) { return false; }
  if (not modelEstoque_compra.setData(rowEstoque_compra, "idPedido", modelCompra.data(rowCompra, "idPedido"))) { return false; }

  return criarConsumo(rowCompra, rowEstoque);
}

bool ImportarXML::verificaCNPJ(const XML &xml) {
  QSqlQuery queryLoja;

  // TODO: 5make this not hardcoded but still it shouldnt need the user to set a UserSession flag
  if (not queryLoja.exec("SELECT cnpj FROM loja WHERE descricao = 'CD'") or not queryLoja.first()) { return qApp->enqueueError(false, "Erro na query CNPJ: " + queryLoja.lastError().text(), this); }

  if (queryLoja.value("cnpj").toString().remove(".").remove("/").remove("-") != xml.cnpj) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "CNPJ da nota não é do galpão. Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    return msgBox.exec() == QMessageBox::Yes;
  }

  return true;
}

bool ImportarXML::verificaExiste(const XML &xml) {
  QSqlQuery query;
  query.prepare("SELECT idNFe FROM nfe WHERE chaveAcesso = :chaveAcesso");
  query.bindValue(":chaveAcesso", xml.chaveAcesso);

  if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se nota já cadastrada: " + query.lastError().text(), this); }

  const auto list = modelNFe.match("chaveAcesso", xml.chaveAcesso);

  if (query.first() or not list.isEmpty()) { return qApp->enqueueError(true, "Nota já cadastrada!", this); }

  return false;
}

bool ImportarXML::verificaValido(const XML &xml) {
  if (not xml.fileContent.contains("nProt")) { return qApp->enqueueError(false, "NFe não está autorizada pela SEFAZ!", this); }

  return true;
}

bool ImportarXML::cadastrarNFe(XML &xml) {
  if (not verificaCNPJ(xml) or verificaExiste(xml) or not verificaValido(xml)) { return false; }

  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idNFe), 1) AS idNFe FROM nfe") or not query.first()) { return qApp->enqueueError(false, "Erro buscando próximo id: " + query.lastError().text(), this); }

  xml.idNFe = query.value("idNFe").toInt();

  for (int row = 0; row < modelNFe.rowCount(); ++row) {
    const int id = modelNFe.data(row, "idNFe").toInt();
    if (id > xml.idNFe) { xml.idNFe = id; }
  }

  xml.idNFe++;

  const int row = modelNFe.rowCount();
  if (not modelNFe.insertRow(row)) { return false; }

  if (not modelNFe.setData(row, "idNFe", xml.idNFe)) { return false; }
  if (not modelNFe.setData(row, "tipo", "ENTRADA")) { return false; }
  if (not modelNFe.setData(row, "cnpjDest", xml.cnpj)) { return false; }
  if (not modelNFe.setData(row, "chaveAcesso", xml.chaveAcesso)) { return false; }
  if (not modelNFe.setData(row, "numeroNFe", xml.nNF)) { return false; }
  if (not modelNFe.setData(row, "xml", xml.fileContent)) { return false; }
  if (not modelNFe.setData(row, "transportadora", xml.xNomeTransp)) { return false; }

  return true;
}

bool ImportarXML::lerXML() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Arquivo XML", QDir::currentPath(), "XML (*.xml)");

  if (filePath.isEmpty()) { return false; }

  ui->lineEdit->setText(filePath);

  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) { return qApp->enqueueError(false, "Erro lendo arquivo: " + file.errorString(), this); }

  XML xml(file.readAll(), file.fileName());

  file.close();

  if (not cadastrarNFe(xml)) { return false; }
  if (not perguntarLocal(xml)) { return false; }
  if (not percorrerXml(xml, xml.model.item(0, 0))) { return false; }

  return true;
}

bool ImportarXML::perguntarLocal(XML &xml) {
  QSqlQuery query;

  if (not query.exec("SELECT descricao FROM loja WHERE descricao NOT IN ('', 'CD')")) { return qApp->enqueueError(false, "Erro buscando lojas: " + query.lastError().text(), this); }

  QStringList lojas{"CD"};

  while (query.next()) { lojas << query.value("descricao").toString(); }

  QInputDialog input;
  input.setInputMode(QInputDialog::TextInput);
  input.setCancelButtonText("Cancelar");
  input.setWindowTitle("Local");
  input.setLabelText("Local do depósito:");
  input.setComboBoxItems(lojas);
  input.setComboBoxEditable(false);

  if (input.exec() != QInputDialog::Accepted) { return false; }

  xml.local = input.textValue();

  return true;
}

bool ImportarXML::inserirItemModel(const XML &xml) {
  const auto idEstoque = buscarProximoIdEstoque();

  if (not idEstoque) { return false; }

  const int newRow = modelEstoque.rowCount();

  if (not modelEstoque.insertRow(newRow)) { return qApp->enqueueError(false, "Erro inserindo linha na tabela: " + modelEstoque.lastError().text(), this); }

  if (not modelEstoque.setData(newRow, "idEstoque", idEstoque.value())) { return false; }
  if (not modelEstoque.setData(newRow, "idNFe", xml.idNFe)) { return false; }
  if (not modelEstoque.setData(newRow, "fornecedor", xml.xNome)) { return false; }
  if (not modelEstoque.setData(newRow, "local", xml.local)) { return false; }
  if (not modelEstoque.setData(newRow, "descricao", xml.descricao)) { return false; }
  if (not modelEstoque.setData(newRow, "quant", xml.quant)) { return false; }
  if (not modelEstoque.setData(newRow, "un", xml.un)) { return false; }
  if (not modelEstoque.setData(newRow, "codBarras", xml.codBarras)) { return false; }
  if (not modelEstoque.setData(newRow, "codComercial", xml.codProd)) { return false; }
  if (not modelEstoque.setData(newRow, "ncm", xml.ncm)) { return false; }
  if (not modelEstoque.setData(newRow, "cfop", xml.cfop)) { return false; }
  if (not modelEstoque.setData(newRow, "valorUnid", xml.valorUnid)) { return false; }
  if (not modelEstoque.setData(newRow, "valor", xml.valor)) { return false; }
  if (not modelEstoque.setData(newRow, "codBarrasTrib", xml.codBarrasTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "unTrib", xml.unTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "quantTrib", xml.quantTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "valorTrib", xml.valorTrib)) { return false; }
  if (not modelEstoque.setData(newRow, "desconto", xml.desconto)) { return false; }
  if (not modelEstoque.setData(newRow, "compoeTotal", xml.compoeTotal)) { return false; }
  if (not modelEstoque.setData(newRow, "numeroPedido", xml.numeroPedido)) { return false; }
  if (not modelEstoque.setData(newRow, "itemPedido", xml.itemPedido)) { return false; }
  if (not modelEstoque.setData(newRow, "tipoICMS", xml.tipoICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "orig", xml.orig)) { return false; }
  if (not modelEstoque.setData(newRow, "cstICMS", xml.cstICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "modBC", xml.modBC)) { return false; }
  if (not modelEstoque.setData(newRow, "vBC", xml.vBC)) { return false; }
  if (not modelEstoque.setData(newRow, "pICMS", xml.pICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "vICMS", xml.vICMS)) { return false; }
  if (not modelEstoque.setData(newRow, "modBCST", xml.modBCST)) { return false; }
  if (not modelEstoque.setData(newRow, "pMVAST", xml.pMVAST)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCST", xml.vBCST)) { return false; }
  if (not modelEstoque.setData(newRow, "pICMSST", xml.pICMSST)) { return false; }
  if (not modelEstoque.setData(newRow, "vICMSST", xml.vICMSST)) { return false; }
  if (not modelEstoque.setData(newRow, "cEnq", xml.cEnq)) { return false; }
  if (not modelEstoque.setData(newRow, "cstIPI", xml.cstIPI)) { return false; }
  if (not modelEstoque.setData(newRow, "cstPIS", xml.cstPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCPIS", xml.vBCPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "pPIS", xml.pPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "vPIS", xml.vPIS)) { return false; }
  if (not modelEstoque.setData(newRow, "cstCOFINS", xml.cstCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "vBCCOFINS", xml.vBCCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "pCOFINS", xml.pCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "vCOFINS", xml.vCOFINS)) { return false; }
  if (not modelEstoque.setData(newRow, "status", "EM COLETA")) { return false; }

  return true;
}

bool ImportarXML::percorrerXml(XML &xml, const QStandardItem *item) {
  for (int row = 0; row < item->rowCount(); ++row) {
    for (int col = 0; col < item->columnCount(); ++col) {
      const QStandardItem *child = item->child(row, col);
      const QString text = child->text();

      if (text.mid(0, 10) == "det nItem=") {
        xml.lerValores(child);
        if (not inserirItemModel(xml)) { return false; }
      }

      if (child->hasChildren()) { percorrerXml(xml, child); }
    }
  }

  return true;
}

bool ImportarXML::criarConsumo(const int rowCompra, const int rowEstoque) {
  const int idVendaProduto = modelCompra.data(rowCompra, "idVendaProduto").toInt();
  const int idEstoque = modelEstoque.data(rowEstoque, "idEstoque").toInt();
  //  const QString codCompra = modelCompra.data(rowCompra, "codComercial").toString();
  //  const QString codEstoque = modelEstoque.data(rowEstoque, "codComercial").toString();

  if (idVendaProduto == 0) { return true; }

  const int rowConsumo = modelConsumo.rowCount();
  modelConsumo.insertRow(rowConsumo);

  for (int column = 0, columnCount = modelEstoque.columnCount(); column < columnCount; ++column) {
    const QString field = modelEstoque.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field, true);

    if (index == -1) { continue; }
    if (column == modelEstoque.fieldIndex("cfop")) { break; }

    const QVariant value = modelEstoque.data(rowEstoque, column);

    if (not modelConsumo.setData(rowConsumo, index, value)) { return false; }
  }

  QSqlQuery query;
  query.prepare("SELECT quant FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");
  query.bindValue(":idVendaProduto", idVendaProduto);

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + query.lastError().text(), this); }

  const double quantConsumo = query.value("quant").toDouble();
  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double quant = qMin(quantConsumo, quantEstoque);

  // TODO: implement this
  //  if (quantConsumo > quantEstoque) {
  // dividir venda/pedido_fornecedor
  //  }

  // -------------------------------------

  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelCompra.data(rowCompra, "idProduto"));

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando dados do produto: " + query.lastError().text(), this); }

  const QString un = query.value("un").toString();
  const double m2cx = query.value("m2cx").toDouble();
  const double pccx = query.value("pccx").toDouble();

  const double unCaixa = un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx;

  const double caixas = qRound(quant / unCaixa * 100) / 100.;

  if (not modelConsumo.setData(rowConsumo, "status", "PRÉ-CONSUMO")) { return false; }
  if (not modelConsumo.setData(rowConsumo, "quant", quant * -1)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "caixas", caixas)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen))) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idVendaProduto", idVendaProduto)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idEstoque", idEstoque)) { return false; }
  if (not modelConsumo.setData(rowConsumo, "idPedido", modelCompra.data(rowCompra, "idPedido"))) { return false; }

  return true;
}

void ImportarXML::on_pushButtonCancelar_clicked() { close(); }

bool ImportarXML::produtoCompativel(const int rowCompra, const QString &codComercialEstoque) {
  if (modelCompra.data(rowCompra, "status").toString() != "EM FATURAMENTO") { return false; }
  if (modelCompra.data(rowCompra, "quantUpd").toInt() == static_cast<int>(FieldColors::Green)) { return false; }
  const QString codComercialCompra = modelCompra.data(rowCompra, "codComercial").toString();
  if (codComercialCompra != codComercialEstoque) { return false; }

  return true;
}

bool ImportarXML::reparear(const QModelIndex &index) {
  if (index.column() != modelEstoque.fieldIndex("codComercial") and index.column() != modelEstoque.fieldIndex("quant")) { return true; }

  return parear();
}

bool ImportarXML::parear() {
  disconnect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::reparear);

  const auto parear2 = [&] {
    if (not limparAssociacoes()) { return false; }

    for (int rowEstoque = 0, totalEstoque = modelEstoque.rowCount(); rowEstoque < totalEstoque; ++rowEstoque) {
      // pintar inicialmente de vermelho, pintar diferente na associacao
      if (not modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(FieldColors::Red))) { return false; }

      const QString codComercialEstoque = modelEstoque.data(rowEstoque, "codComercial").toString();
      const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
      double quantConsumida = 0;
      bool pareado = false;

      // fazer busca por quantidades iguais
      for (int rowCompra = 0, totalCompra = modelCompra.rowCount(); rowCompra < totalCompra; ++rowCompra) {
        if (not produtoCompativel(rowCompra, codComercialEstoque)) { continue; }

        const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();

        if (qFuzzyCompare(quantEstoque, quantCompra)) {
          if (not associarItens(rowCompra, rowEstoque, quantConsumida)) { return false; }
          pareado = true;
          break;
        }
      }

      // se o estoque foi associado passar para o proximo produto
      if (pareado) { continue; }

      // fazer busca por quantidades diferentes
      for (int rowCompra = 0, totalCompra = modelCompra.rowCount(); rowCompra < totalCompra; ++rowCompra) {
        if (not produtoCompativel(rowCompra, codComercialEstoque)) { continue; }
        if (not associarItens(rowCompra, rowEstoque, quantConsumida)) { return false; }
      }
    }

    ui->tableEstoque->clearSelection();
    ui->tableCompra->clearSelection();

    ui->tableCompra->setFocus(); // for updating proxyModel

    return true;
  }();

  connect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::reparear);

  return parear2;
}

std::optional<int> ImportarXML::buscarProximoIdEstoque() {
  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idEstoque), 1) AS idEstoque FROM estoque") or not query.first()) {
    qApp->enqueueError("Erro buscando próximo id: " + query.lastError().text(), this);
    return {};
  }

  int idEstoque = query.value("idEstoque").toInt();

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    const int id = modelEstoque.data(row, "idEstoque").toInt();
    if (id > idEstoque) { idEstoque = id; }
  }

  idEstoque++;

  return idEstoque;
}

void ImportarXML::on_checkBoxSemLote_toggled(bool checked) {
  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (not modelEstoque.setData(row, "lote", checked ? "N/D" : "")) { return; }
  }
}

// NOTE: 5utilizar tabela em arvore (qtreeview) para agrupar consumos com seu estoque (para cada linha do model inserir
// items na arvore?)
// TODO: 5quando as unidades vierem diferente pedir para usuario converter
// TODO: 5substituir 'quantConsumida' por separacao de linha na associacao de nota/compra
// TODO: 5avisar se R$ da nota for diferente do R$ da compra
// TODO: 5bloquear a importacao de documentos que nao sejam NFe
// TODO: verificar se o valor total da nota bate com o valor total da compra (bater impostos/st)
// TODO: quando o usuario editar valorUnid recalcular o total
// TODO: poder fazer importacao parcial de nota (quando a linha fica amarela)

// TODO: verificar:
//          *1 linha vp/pf para 2 linhas estoque
//          *2 linhas vp/pf para 1 linha estoque

// TODO: salvar valor nota no SQL
