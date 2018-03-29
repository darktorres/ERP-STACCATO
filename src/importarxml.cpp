#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "estoqueproxymodel.h"
#include "importarxml.h"
#include "noeditdelegate.h"
#include "reaisdelegate.h"
#include "singleeditdelegate.h"
#include "ui_importarxml.h"

ImportarXML::ImportarXML(const QStringList &idsCompra, const QDateTime &dataReal, QWidget *parent) : Dialog(parent), dataReal(dataReal), idsCompra(idsCompra), ui(new Ui::ImportarXML) {
  ui->setupUi(this);

  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonImportar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonImportar_clicked);
  connect(ui->pushButtonProcurar, &QPushButton::clicked, this, &ImportarXML::on_pushButtonProcurar_clicked);
  connect(ui->tableCompra, &TableView::entered, this, &ImportarXML::on_tableCompra_entered);
  connect(ui->tableConsumo, &TableView::entered, this, &ImportarXML::on_tableConsumo_entered);
  connect(ui->tableEstoque, &TableView::entered, this, &ImportarXML::on_tableEstoque_entered);

  setWindowFlags(Qt::Window);

  setupTables(idsCompra);
}

ImportarXML::~ImportarXML() { delete ui; }

void ImportarXML::setupTables(const QStringList &idsCompra) {
  modelEstoque.setTable("estoque");
  modelEstoque.setEditStrategy(QSqlTableModel::OnManualSubmit);

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

  modelEstoque.setFilter("status = 'TEMP'");

  if (not modelEstoque.select()) emit errorSignal("Erro lendo tabela estoque: " + modelEstoque.lastError().text());

  ui->tableEstoque->setModel(new EstoqueProxyModel(&modelEstoque, this));
  ui->tableEstoque->setItemDelegate(new NoEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("codComercial", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("lote", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("bloco", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("quant", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("un", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("descricao", new SingleEditDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("valorUnid", new ReaisDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("valor", new ReaisDelegate(this));
  ui->tableEstoque->setItemDelegateForColumn("vICMSST", new ReaisDelegate(this));
  ui->tableEstoque->hideColumn("recebidoPor");
  ui->tableEstoque->hideColumn("quantUpd");
  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("idEstoque");
  ui->tableEstoque->hideColumn("idCompra");
  ui->tableEstoque->hideColumn("idProduto");
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

  //

  modelConsumo.setTable("estoque_has_consumo");
  modelConsumo.setEditStrategy(QSqlTableModel::OnManualSubmit);

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

  modelConsumo.setFilter("status = 'TEMP'");

  if (not modelConsumo.select()) emit errorSignal("Erro lendo tabela estoque_has_consumo: " + modelConsumo.lastError().text());

  ui->tableConsumo->setModel(new EstoqueProxyModel(&modelConsumo, this));
  ui->tableConsumo->setItemDelegate(new NoEditDelegate(this));
  ui->tableConsumo->setItemDelegateForColumn("valorUnid", new ReaisDelegate(this));
  ui->tableConsumo->setItemDelegateForColumn("valor", new ReaisDelegate(this));
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

  //

  modelCompra.setTable("pedido_fornecedor_has_produto");
  modelCompra.setEditStrategy(QSqlTableModel::OnManualSubmit);

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

  if (not modelCompra.select()) emit errorSignal("Erro lendo tabela pedido_fornecedor_has_produto: " + modelCompra.lastError().text());

  ui->tableCompra->setModel(new EstoqueProxyModel(&modelCompra, this));
  ui->tableCompra->setItemDelegate(new NoEditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("codComercial", new SingleEditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("descricao", new SingleEditDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));
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

  ui->tableCompra->resizeColumnsToContents();

  //

  modelNFe.setTable("nfe");
  modelNFe.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelNFe.setFilter("0");

  if (not modelNFe.select()) emit errorSignal("Erro lendo tabela nfe: " + modelNFe.lastError().text());

  modelEstoque_nfe.setTable("estoque_has_nfe");
  modelEstoque_nfe.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelEstoque_nfe.setFilter("0");

  if (not modelEstoque_nfe.select()) emit errorSignal("Erro lendo tabela estoque_has_nfe: " + modelEstoque_nfe.lastError().text());

  modelEstoque_compra.setTable("estoque_has_compra");
  modelEstoque_compra.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelEstoque_compra.setFilter("0");

  if (not modelEstoque_compra.select()) emit errorSignal("Erro lendo tabela estoque_has_compra: " + modelEstoque_compra.lastError().text());
}

bool ImportarXML::cadastrarProdutoEstoque() {
  QSqlQuery query;
  query.prepare(
      "INSERT INTO produto SELECT NULL, p.idProdutoUpd, :idEstoque, p.idFornecedor, p.idFornecedorUpd, p.fornecedor, p.fornecedorUpd, CONCAT(p.descricao, ' (ESTOQUE)'), p.descricaoUpd, "
      ":estoqueRestante, p.estoqueRestanteUpd, p.un, p.unUpd, p.un2, p.un2Upd, p.colecao, p.colecaoUpd, p.tipo, p.tipoUpd, p.minimo, p.minimoUpd, p.multiplo, p.multiploUpd, p.m2cx, p.m2cxUpd, "
      "p.pccx, p.pccxUpd, p.kgcx, p.kgcxUpd, p.formComercial, p.formComercialUpd, p.codComercial, p.codComercialUpd, p.codBarras, p.codBarrasUpd, p.ncm, p.ncmUpd, p.ncmEx, p.ncmExUpd, p.cfop, "
      "p.cfopUpd, p.icms, p.icmsUpd, p.cst, p.cstUpd, p.qtdPallet, p.qtdPalletUpd, p.custo, p.custoUpd, p.ipi, p.ipiUpd, p.st, p.stUpd, p.precoVenda, p.precoVendaUpd, p.markup, p.markupUpd, "
      "p.comissao, p.comissaoUpd, p.observacoes, p.observacoesUpd, p.origem, p.origemUpd, p.temLote, p.temLoteUpd, p.ui, p.uiUpd, '2020-12-31', p.validadeUpd, 0, p.descontinuadoUpd, "
      "p.atualizarTabelaPreco, p.representacao, 1, 0, p.idProduto, 0, NULL, NULL FROM produto p WHERE p.idProduto = :idProduto");

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    double consumo = 0;

    for (int row2 = 0; row2 < modelConsumo.rowCount(); ++row2) {
      if (modelConsumo.data(row2, "idEstoque").toInt() != modelEstoque.data(row, "idEstoque").toInt()) continue;

      consumo += modelConsumo.data(row2, "quant").toDouble();
    }

    const double estoqueRestante = modelEstoque.data(row, "quant").toDouble() + consumo;

    if (qFuzzyIsNull(estoqueRestante)) continue;

    query.bindValue(":idProduto", modelEstoque.data(row, "idProduto"));
    query.bindValue(":idEstoque", modelEstoque.data(row, "idEstoque"));
    query.bindValue(":estoqueRestante", estoqueRestante);

    if (not query.exec()) {
      emit errorSignal("Erro criando produto_estoque: " + query.lastError().text());
      return false;
    }
  }

  return true;
}

bool ImportarXML::importar() {
  if (not cadastrarProdutoEstoque()) return false;

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (not modelEstoque.setData(row, "status", "EM COLETA")) return false;
  }

  for (int row = 0; row < modelConsumo.rowCount(); ++row) {
    if (not modelConsumo.setData(row, "status", "PRÉ-CONSUMO")) return false;
  }

  for (int row = 0; row < modelCompra.rowCount(); ++row)
    if (not modelCompra.setData(row, "selecionado", false)) return false;

  //--------------

  if (not modelEstoque.submitAll()) {
    emit errorSignal("Erro salvando dados da tabela estoque: " + modelEstoque.lastError().text());
    return false;
  }

  if (not modelCompra.submitAll()) {
    emit errorSignal("Erro salvando dados da tabela compra: " + modelCompra.lastError().text());
    return false;
  }

  if (not modelConsumo.submitAll()) {
    emit errorSignal("Erro salvando dados do consumo: " + modelConsumo.lastError().text());
    return false;
  }

  if (not modelEstoque_compra.submitAll()) {
    emit errorSignal("Erro salvando modelEstoque_compra: " + modelEstoque_compra.lastError().text());
    return false;
  }

  if (not modelNFe.submitAll()) {
    emit errorSignal("Erro salvando modelNFe: " + modelNFe.lastError().text());
    return false;
  }

  if (not modelEstoque_nfe.submitAll()) {
    emit errorSignal("Erro salvando modelEstoque_nfe: " + modelEstoque_nfe.lastError().text());
    return false;
  }

  //------------------------------
  QSqlQuery query;
  query.prepare("UPDATE venda_has_produto SET status = 'EM COLETA', dataRealFat = :dataRealFat WHERE idVendaProduto = :idVendaProduto");

  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    const int idVendaProduto = modelCompra.data(row, "idVendaProduto").toInt();
    const FieldColors color = static_cast<FieldColors>(modelCompra.data(row, "quantUpd").toInt());
    const QString status = modelCompra.data(row, "status").toString();

    if (idVendaProduto == 0) continue;
    if (color == FieldColors::White) continue;
    if (status != "EM FATURAMENTO") continue;

    query.bindValue(":dataRealFat", dataReal);
    query.bindValue(":idVendaProduto", modelCompra.data(row, "idVendaProduto"));

    if (not query.exec()) {
      emit errorSignal("Erro atualizando status do produto da venda: " + query.lastError().text());
      return false;
    }
  }

  query.prepare("UPDATE pedido_fornecedor_has_produto SET status = 'EM COLETA', dataRealFat = :dataRealFat WHERE idCompra = :idCompra AND quantUpd = 1 AND status = 'EM FATURAMENTO'");

  for (const auto &idCompra : idsCompra) {
    query.bindValue(":dataRealFat", dataReal);
    query.bindValue(":idCompra", idCompra);

    if (not query.exec()) {
      emit errorSignal("Erro atualizando status da compra: " + query.lastError().text());
      return false;
    }
  }
  //------------------------------

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

  if (not ok) {
    emit errorSignal("Nenhuma compra pareada!");
    return false;
  }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    const FieldColors color = static_cast<FieldColors>(modelEstoque.data(row, "quantUpd").toInt());

    if (color == FieldColors::Red) {
      emit errorSignal("Nem todos os estoques estão ok!");
      return false;
    }
  }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (modelEstoque.data(row, "lote").toString().isEmpty()) {
      emit errorSignal("Lote vazio! Se não há coloque 'N/D'");
      return false;
    }
  }

  return true;
}

void ImportarXML::on_pushButtonImportar_clicked() {
  if (not verifyFields()) return;

  emit transactionStarted();

  disconnect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::wrapParear);

  if (not QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec()) return;
  if (not QSqlQuery("START TRANSACTION").exec()) return;

  if (not importar()) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return;
  }

  if (not QSqlQuery("COMMIT").exec()) return;

  connect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::wrapParear);

  emit transactionEnded();

  QDialog::accept();
  close();
}

bool ImportarXML::limparAssociacoes() {
  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    const QString status = modelCompra.data(row, "status").toString();
    const int color = modelCompra.data(row, "quantUpd").toInt();

    if (status != "EM FATURAMENTO" and color == static_cast<int>(FieldColors::Green)) continue;

    if (not modelCompra.setData(row, "quantUpd", static_cast<int>(FieldColors::White))) return false;
    if (not modelCompra.setData(row, "quantConsumida", 0)) return false;
  }

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    if (not modelEstoque.setData(row, "quantUpd", static_cast<int>(FieldColors::White))) return false;
  }

  while (modelConsumo.rowCount() > 0) {
    if (not modelConsumo.removeRow(0)) return false;
  }

  if (not modelConsumo.submitAll()) return false;

  return true;
}

void ImportarXML::on_pushButtonProcurar_clicked() {
  disconnect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::wrapParear);

  procurar();

  connect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::wrapParear);
}

void ImportarXML::procurar() {
  const QString filePath = QFileDialog::getOpenFileName(this, "Arquivo XML", QDir::currentPath(), "XML (*.xml)");

  if (filePath.isEmpty()) {
    ui->lineEdit->setText(QString());
    return;
  }

  QFile file(filePath);

  if (not file.open(QFile::ReadOnly)) {
    emit errorSignal("Erro lendo arquivo: " + file.errorString());
    return;
  }

  if (not lerXML(file)) return;

  file.close();

  if (not parear()) return;

  bool ok = true;

  for (int row = 0; row < modelCompra.rowCount(); ++row) {
    if (static_cast<FieldColors>(modelCompra.data(row, "quantUpd").toInt()) != FieldColors::Green) {
      ok = false;
      break;
    }
  }

  if (ok) ui->pushButtonProcurar->setDisabled(true);

  ui->tableEstoque->resizeColumnsToContents();
  ui->tableConsumo->resizeColumnsToContents();
  ui->tableCompra->resizeColumnsToContents();
}

std::optional<double> ImportarXML::buscarCaixas(const int rowEstoque) {
  QSqlQuery query;
  query.prepare("SELECT idProduto, m2cx, pccx, UPPER(un) AS un FROM produto WHERE codComercial = :codComercial");
  query.bindValue(":codComercial", modelEstoque.data(rowEstoque, "codComercial"));

  if (not query.exec()) {
    emit errorSignal("Erro lendo tabela produto: " + query.lastError().text());
    return false;
  }

  if (not query.first()) {
    emit errorSignal("Produto não cadastrado: " + modelEstoque.data(rowEstoque, "codComercial").toString());
    return false;
  }

  const QString un = modelEstoque.data(rowEstoque, "un").toString();

  const double quantCaixa = un == "M2" or un == "M²" or un == "ML" ? query.value("m2cx").toDouble() : query.value("pccx").toDouble();

  const double quant = modelEstoque.data(rowEstoque, "quant").toDouble();

  const double caixas = qRound(quant / quantCaixa * 100) / 100.;

  return caixas;
}

bool ImportarXML::associarItens(const int rowCompra, const int rowEstoque, double &estoqueConsumido) {
  if (static_cast<FieldColors>(modelEstoque.data(rowEstoque, "quantUpd").toInt()) == FieldColors::Green) return true;
  if (static_cast<FieldColors>(modelCompra.data(rowCompra, "quantUpd").toInt()) == FieldColors::Green) return true;

  //-------------------------------

  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();
  const double quantConsumida = modelCompra.data(rowCompra, "quantConsumida").toDouble();

  const double espaco = quantCompra - quantConsumida;
  const double estoqueDisponivel = quantEstoque - estoqueConsumido;
  const double quantAdicionar = estoqueDisponivel > espaco ? espaco : estoqueDisponivel;
  estoqueConsumido += quantAdicionar;

  if (not modelCompra.setData(rowCompra, "descricao", modelEstoque.data(rowEstoque, "descricao").toString())) return false;
  if (not modelCompra.setData(rowCompra, "quantConsumida", quantConsumida + quantAdicionar)) return false;
  if (not modelCompra.setData(rowCompra, "quantUpd", static_cast<int>(qFuzzyCompare((quantConsumida + quantAdicionar), quantCompra) ? FieldColors::Green : FieldColors::Yellow))) return false;
  if (not modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(qFuzzyCompare(estoqueConsumido, quantEstoque) ? FieldColors::Green : FieldColors::Yellow))) return false;

  const auto caixas = buscarCaixas(rowEstoque);

  if (not caixas) return false;

  if (not modelEstoque.setData(rowEstoque, "caixas", *caixas)) return false;
  if (not modelEstoque.setData(rowEstoque, "idProduto", modelCompra.data(rowCompra, "idProduto"))) return false;

  const int idCompra = modelCompra.data(rowCompra, "idCompra").toInt();
  const int idEstoque = modelEstoque.data(rowEstoque, "idEstoque").toInt();

  bool exists = false;

  for (int row = 0; row < modelEstoque_compra.rowCount(); ++row) {
    const int idEstoque_temp = modelEstoque_compra.data(row, "idEstoque").toInt();
    const int idCompra_temp = modelEstoque_compra.data(row, "idCompra").toInt();

    if (idEstoque_temp == idEstoque and idCompra_temp == idCompra) exists = true;
  }

  if (not exists) {
    const int rowEstoque_compra = modelEstoque_compra.rowCount();
    modelEstoque_compra.insertRow(rowEstoque_compra);

    if (not modelEstoque_compra.setData(rowEstoque_compra, "idEstoque", idEstoque)) return false;
    if (not modelEstoque_compra.setData(rowEstoque_compra, "idCompra", idCompra)) return false;
  }

  if (not criarConsumo(rowCompra, rowEstoque)) return false;

  return true;
}

bool ImportarXML::verificaCNPJ(const XML &xml) {
  QSqlQuery queryLoja;

  // TODO: 5make this not hardcoded but still it shouldnt need the user to set a UserSession flag
  if (not queryLoja.exec("SELECT cnpj FROM loja WHERE descricao = 'CD'") or not queryLoja.first()) {
    emit errorSignal("Erro na query CNPJ: " + queryLoja.lastError().text());
    return false;
  }

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

  if (not query.exec()) {
    emit errorSignal("Erro verificando se nota já cadastrada: " + query.lastError().text());
    return false;
  }

  if (query.first()) {
    emit errorSignal("Nota já cadastrada!");
    return true;
  }

  const auto list = modelNFe.match(modelNFe.index(0, modelNFe.fieldIndex("chaveAcesso")), Qt::DisplayRole, xml.chaveAcesso);

  if (list.size() > 0) {
    emit errorSignal("Nota já cadastrada!");
    return true;
  }

  return false;
}

bool ImportarXML::cadastrarNFe(XML &xml) {
  if (not verificaCNPJ(xml) or verificaExiste(xml)) return false;

  QFile file(xml.fileName);

  if (not file.open(QFile::ReadOnly)) {
    emit errorSignal("Erro lendo arquivo: " + file.errorString());
    return false;
  }

  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idNFe), 1) AS idNFe FROM nfe") or not query.first()) {
    emit errorSignal("Erro buscando próximo id: " + query.lastError().text());
    return false;
  }

  xml.idNFe = query.value("idNFe").toInt();

  for (int row = 0; row < modelNFe.rowCount(); ++row) {
    const int id = modelNFe.data(row, "idNFe").toInt();
    if (id > xml.idNFe) xml.idNFe = id;
  }

  xml.idNFe++;

  const int row = modelNFe.rowCount();
  if (not modelNFe.insertRow(row)) return false;

  if (not modelNFe.setData(row, "idNFe", xml.idNFe)) return false;
  if (not modelNFe.setData(row, "tipo", "ENTRADA")) return false;
  if (not modelNFe.setData(row, "cnpjDest", xml.cnpj)) return false;
  if (not modelNFe.setData(row, "chaveAcesso", xml.chaveAcesso)) return false;
  if (not modelNFe.setData(row, "numeroNFe", xml.nNF)) return false;
  if (not modelNFe.setData(row, "xml", file.readAll())) return false;
  if (not modelNFe.setData(row, "transportadora", xml.xNomeTransp)) return false;

  return true;
}

bool ImportarXML::lerXML(QFile &file) {
  XML xml(file.readAll(), file.fileName());

  if (not cadastrarNFe(xml)) return false;
  if (not perguntarLocal(xml)) return false;
  if (not inserirNoSqlModel(xml, xml.model.item(0, 0))) return false;

  return true;
}

bool ImportarXML::perguntarLocal(XML &xml) {
  QSqlQuery query;

  if (not query.exec("SELECT descricao FROM loja WHERE descricao != '' and descricao != 'CD'")) {
    emit errorSignal("Erro buscando lojas: " + query.lastError().text());
    return false;
  }

  QStringList lojas{"CD"};

  while (query.next()) lojas << query.value("descricao").toString();

  QInputDialog input;
  input.setInputMode(QInputDialog::TextInput);
  input.setCancelButtonText("Cancelar");
  input.setWindowTitle("Local");
  input.setLabelText("Local do depósito:");
  input.setComboBoxItems(lojas);
  input.setComboBoxEditable(false);

  if (input.exec() != QInputDialog::Accepted) return false;

  xml.local = input.textValue();

  return true;
}

bool ImportarXML::inserirItemSql(XML &xml) { // REFAC: extract functions, too big/complex
  const auto list = modelEstoque.match(modelEstoque.index(0, modelEstoque.fieldIndex("codComercial")), Qt::DisplayRole, xml.codProd, -1, Qt::MatchFlags(Qt::MatchFixedString | Qt::MatchWrap));

  for (const auto &item : list) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Produto é do mesmo lote da linha " + QString::number(item.row() + 1) + "?", QMessageBox::Yes | QMessageBox::No, nullptr);
    msgBox.setButtonText(QMessageBox::Yes, "Sim");
    msgBox.setButtonText(QMessageBox::No, "Não");

    if (msgBox.exec() == QMessageBox::Yes) {
      const int row = item.row();

      const double newQuant = xml.quant + modelEstoque.data(row, "quant").toDouble();

      if (not modelEstoque.setData(row, "quant", newQuant)) return false;

      //

      const int rowNFe = modelEstoque_nfe.rowCount();
      modelEstoque_nfe.insertRow(rowNFe);

      if (not modelEstoque_nfe.setData(rowNFe, "idEstoque", modelEstoque.data(row, "idEstoque"))) return false;
      if (not modelEstoque_nfe.setData(rowNFe, "idNFe", xml.idNFe)) return false;

      //

      return true;
    }
  }

  const int newRow = modelEstoque.rowCount();

  if (not modelEstoque.insertRow(newRow)) {
    emit errorSignal("Erro inserindo linha na tabela: " + modelEstoque.lastError().text());
    return false;
  }

  // remove 'A'
  if (xml.xNome == "CECRISA REVEST. CERAMICOS S.A." and xml.codProd.endsWith("A")) xml.codProd = xml.codProd.left(xml.codProd.size() - 1);

  QSqlQuery query;

  if (not query.exec("SELECT COALESCE(MAX(idEstoque), 1) AS idEstoque FROM estoque") or not query.first()) {
    emit errorSignal("Erro buscando próximo id: " + query.lastError().text());
    return false;
  }

  int idEstoque = query.value("idEstoque").toInt();

  for (int row = 0; row < modelEstoque.rowCount(); ++row) {
    const int id = modelEstoque.data(row, "idEstoque").toInt();
    if (id > idEstoque) idEstoque = id;
  }

  idEstoque++;

  if (not modelEstoque.setData(newRow, "idEstoque", idEstoque)) return false;
  if (not modelEstoque.setData(newRow, "fornecedor", xml.xNome)) return false;
  if (not modelEstoque.setData(newRow, "local", xml.local)) return false;
  if (not modelEstoque.setData(newRow, "descricao", xml.descricao)) return false;
  if (not modelEstoque.setData(newRow, "quant", xml.quant)) return false;
  if (not modelEstoque.setData(newRow, "un", xml.un)) return false;
  if (not modelEstoque.setData(newRow, "codBarras", xml.codBarras)) return false;
  if (not modelEstoque.setData(newRow, "codComercial", xml.codProd)) return false;
  if (not modelEstoque.setData(newRow, "ncm", xml.ncm)) return false;
  if (not modelEstoque.setData(newRow, "cfop", xml.cfop)) return false;
  if (not modelEstoque.setData(newRow, "valorUnid", xml.valorUnid)) return false;
  if (not modelEstoque.setData(newRow, "valor", xml.valor)) return false;
  if (not modelEstoque.setData(newRow, "codBarrasTrib", xml.codBarrasTrib)) return false;
  if (not modelEstoque.setData(newRow, "unTrib", xml.unTrib)) return false;
  if (not modelEstoque.setData(newRow, "quantTrib", xml.quantTrib)) return false;
  if (not modelEstoque.setData(newRow, "valorTrib", xml.valorTrib)) return false;
  if (not modelEstoque.setData(newRow, "desconto", xml.desconto)) return false;
  if (not modelEstoque.setData(newRow, "compoeTotal", xml.compoeTotal)) return false;
  if (not modelEstoque.setData(newRow, "numeroPedido", xml.numeroPedido)) return false;
  if (not modelEstoque.setData(newRow, "itemPedido", xml.itemPedido)) return false;
  if (not modelEstoque.setData(newRow, "tipoICMS", xml.tipoICMS)) return false;
  if (not modelEstoque.setData(newRow, "orig", xml.orig)) return false;
  if (not modelEstoque.setData(newRow, "cstICMS", xml.cstICMS)) return false;
  if (not modelEstoque.setData(newRow, "modBC", xml.modBC)) return false;
  if (not modelEstoque.setData(newRow, "vBC", xml.vBC)) return false;
  if (not modelEstoque.setData(newRow, "pICMS", xml.pICMS)) return false;
  if (not modelEstoque.setData(newRow, "vICMS", xml.vICMS)) return false;
  if (not modelEstoque.setData(newRow, "modBCST", xml.modBCST)) return false;
  if (not modelEstoque.setData(newRow, "pMVAST", xml.pMVAST)) return false;
  if (not modelEstoque.setData(newRow, "vBCST", xml.vBCST)) return false;
  if (not modelEstoque.setData(newRow, "pICMSST", xml.pICMSST)) return false;
  if (not modelEstoque.setData(newRow, "vICMSST", xml.vICMSST)) return false;
  if (not modelEstoque.setData(newRow, "cEnq", xml.cEnq)) return false;
  if (not modelEstoque.setData(newRow, "cstIPI", xml.cstIPI)) return false;
  if (not modelEstoque.setData(newRow, "cstPIS", xml.cstPIS)) return false;
  if (not modelEstoque.setData(newRow, "vBCPIS", xml.vBCPIS)) return false;
  if (not modelEstoque.setData(newRow, "pPIS", xml.pPIS)) return false;
  if (not modelEstoque.setData(newRow, "vPIS", xml.vPIS)) return false;
  if (not modelEstoque.setData(newRow, "cstCOFINS", xml.cstCOFINS)) return false;
  if (not modelEstoque.setData(newRow, "vBCCOFINS", xml.vBCCOFINS)) return false;
  if (not modelEstoque.setData(newRow, "pCOFINS", xml.pCOFINS)) return false;
  if (not modelEstoque.setData(newRow, "vCOFINS", xml.vCOFINS)) return false;
  if (not modelEstoque.setData(newRow, "status", "TEMP")) return false;

  const int rowNFe = modelEstoque_nfe.rowCount();
  modelEstoque_nfe.insertRow(rowNFe);

  if (not modelEstoque_nfe.setData(rowNFe, "idEstoque", idEstoque)) return false;
  if (not modelEstoque_nfe.setData(rowNFe, "idNFe", xml.idNFe)) return false;

  return true;
}

bool ImportarXML::inserirNoSqlModel(XML &xml, const QStandardItem *item) {
  for (int row = 0; row < item->rowCount(); ++row) {
    for (int col = 0; col < item->columnCount(); ++col) {
      const QStandardItem *child = item->child(row, col);
      const QString text = child->text();

      if (text.mid(0, 10) == "det nItem=") {
        xml.lerValores(child);
        if (not inserirItemSql(xml)) return false;
      }

      if (child->hasChildren()) inserirNoSqlModel(xml, child);
    }
  }

  return true;
}

bool ImportarXML::criarConsumo(const int rowCompra, const int rowEstoque) {
  const int idVendaProduto = modelCompra.data(rowCompra, "idVendaProduto").toInt();
  const int idEstoque = modelEstoque.data(rowEstoque, "idEstoque").toInt();
  const QString codCompra = modelCompra.data(rowCompra, "codComercial").toString();
  const QString codEstoque = modelEstoque.data(rowEstoque, "codComercial").toString();

  if (idVendaProduto == 0) return true;
  if (codCompra != codEstoque) return true;

  const int rowConsumo = modelConsumo.rowCount();
  modelConsumo.insertRow(rowConsumo);

  for (int column = 0; column < modelEstoque.columnCount(); ++column) {
    const QString field = modelEstoque.record().fieldName(column);
    const int index = modelConsumo.fieldIndex(field);

    if (index == -1) continue;

    if (modelEstoque.fieldIndex("created") == column) continue;
    if (modelEstoque.fieldIndex("lastUpdated") == column) continue;

    const QVariant value = modelEstoque.data(rowEstoque, column);

    if (not modelConsumo.setData(rowConsumo, index, value)) return false;
  }

  QSqlQuery query;
  query.prepare("SELECT quant FROM venda_has_produto WHERE idVendaProduto = :idVendaProduto");
  query.bindValue(":idVendaProduto", idVendaProduto);

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando dados do produto: " + query.lastError().text());
    return false;
  }

  const double quant = qMin(query.value("quant").toDouble(), modelEstoque.data(rowEstoque, "quant").toDouble());

  // -------------------------------------

  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", modelCompra.data(rowCompra, "idProduto"));

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando dados do produto: " + query.lastError().text());
    return false;
  }

  const QString un = query.value("un").toString();
  const double m2cx = query.value("m2cx").toDouble();
  const double pccx = query.value("pccx").toDouble();

  const double unCaixa = un == "M2" or un == "M²" or un == "ML" ? m2cx : pccx;

  // REFAC: ??? const double caixas = qRound(proporcao / unCaixa * 100) / 100.;
  const double caixas = qRound(quant / unCaixa * 100) / 100.;

  const double proporcao = quant / modelEstoque.data(rowEstoque, "quant").toDouble();

  const double desconto = modelEstoque.data(rowEstoque, "desconto").toDouble() * proporcao;
  const double valor = modelEstoque.data(rowEstoque, "valor").toDouble() * proporcao;
  const double vBC = modelEstoque.data(rowEstoque, "vBC").toDouble() * proporcao;
  const double vICMS = modelEstoque.data(rowEstoque, "vICMS").toDouble() * proporcao;
  const double vBCST = modelEstoque.data(rowEstoque, "vBCST").toDouble() * proporcao;
  const double vICMSST = modelEstoque.data(rowEstoque, "vICMSST").toDouble() * proporcao;
  const double vBCPIS = modelEstoque.data(rowEstoque, "vBCPIS").toDouble() * proporcao;
  const double vPIS = modelEstoque.data(rowEstoque, "vPIS").toDouble() * proporcao;
  const double vBCCOFINS = modelEstoque.data(rowEstoque, "vBCCOFINS").toDouble() * proporcao;
  const double vCOFINS = modelEstoque.data(rowEstoque, "vCOFINS").toDouble() * proporcao;

  // -------------------------------------

  if (not modelConsumo.setData(rowConsumo, "quant", quant * -1)) return false;
  if (not modelConsumo.setData(rowConsumo, "caixas", caixas)) return false;
  if (not modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen))) return false;
  if (not modelConsumo.setData(rowConsumo, "idVendaProduto", idVendaProduto)) return false;
  if (not modelConsumo.setData(rowConsumo, "idEstoque", idEstoque)) return false;
  if (not modelConsumo.setData(rowConsumo, "desconto", desconto)) return false;
  if (not modelConsumo.setData(rowConsumo, "valor", valor)) return false;
  if (not modelConsumo.setData(rowConsumo, "vBC", vBC)) return false;
  if (not modelConsumo.setData(rowConsumo, "vICMS", vICMS)) return false;
  if (not modelConsumo.setData(rowConsumo, "vBCST", vBCST)) return false;
  if (not modelConsumo.setData(rowConsumo, "vICMSST", vICMSST)) return false;
  if (not modelConsumo.setData(rowConsumo, "vBCPIS", vBCPIS)) return false;
  if (not modelConsumo.setData(rowConsumo, "vPIS", vPIS)) return false;
  if (not modelConsumo.setData(rowConsumo, "vBCCOFINS", vBCCOFINS)) return false;
  if (not modelConsumo.setData(rowConsumo, "vCOFINS", vCOFINS)) return false;

  return true;
}

void ImportarXML::on_pushButtonCancelar_clicked() { close(); }

void ImportarXML::wrapParear() {
  // TODO: colocar um parametro para refazer o parear apenas quando a coluna alterada for a 'codComercial'
  // TODO: 0terminar de refatorar e homologar
  disconnect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::wrapParear);

  parear();

  connect(&modelEstoque, &QSqlTableModel::dataChanged, this, &ImportarXML::wrapParear);
}

bool ImportarXML::produtoCompativel(const int rowCompra, const QString &codComercialEstoque) {
  if (modelCompra.data(rowCompra, "status").toString() != "EM FATURAMENTO") return false;
  const QString codComercialCompra = modelCompra.data(rowCompra, "codComercial").toString();
  if (codComercialCompra != codComercialEstoque) return false;
  if (modelCompra.data(rowCompra, "quantUpd").toInt() == static_cast<int>(FieldColors::Green)) return false;

  return true;
}

bool ImportarXML::parear() {
  if (not limparAssociacoes()) return false;

  for (int rowEstoque = 0, totalEstoque = modelEstoque.rowCount(); rowEstoque < totalEstoque; ++rowEstoque) {
    // pintar inicialmente de vermelho, pintar diferente na associacao
    if (not modelEstoque.setData(rowEstoque, "quantUpd", static_cast<int>(FieldColors::Red))) return false;

    const QString codComercialEstoque = modelEstoque.data(rowEstoque, "codComercial").toString();
    const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
    double quantConsumida = 0;

    // fazer busca por quantidades iguais
    for (int rowCompra = 0, totalCompra = modelCompra.rowCount(); rowCompra < totalCompra; ++rowCompra) {
      if (not produtoCompativel(rowCompra, codComercialEstoque)) continue;

      const double quantCompra = modelCompra.data(rowCompra, "quant").toDouble();

      if (qFuzzyCompare(quantEstoque, quantCompra)) {
        associarItens(rowCompra, rowEstoque, quantConsumida);
        break;
      }
    }

    // se o estoque foi associado passar para o proximo produto
    if (qFuzzyCompare(quantConsumida, quantEstoque)) continue;

    for (int rowCompra = 0, totalCompra = modelCompra.rowCount(); rowCompra < totalCompra; ++rowCompra) {
      if (not produtoCompativel(rowCompra, codComercialEstoque)) continue;

      associarItens(rowCompra, rowEstoque, quantConsumida);
    }
  }

  ui->tableEstoque->clearSelection();
  ui->tableCompra->clearSelection();

  ui->tableCompra->setFocus(); // for updating proxyModel

  return true;
}

void ImportarXML::on_tableEstoque_entered(const QModelIndex &) { ui->tableEstoque->resizeColumnsToContents(); }

void ImportarXML::on_tableCompra_entered(const QModelIndex &) { ui->tableCompra->resizeColumnsToContents(); }

void ImportarXML::on_tableConsumo_entered(const QModelIndex &) { ui->tableConsumo->resizeColumnsToContents(); }

// NOTE: 5utilizar tabela em arvore (qtreeview) para agrupar consumos com seu estoque (para cada linha do model inserir
// items na arvore?)
// TODO: 5quando as unidades vierem diferente pedir para usuario converter
// TODO: 5substituir 'quantConsumida' por separacao de linha na associacao de nota/compra
// TODO: 5avisar se R$ da nota for diferente do R$ da compra
// TODO: 5bloquear a importacao de documentos que nao sejam NFe
// TODO: verificar se o valor total da nota bate com o valor total da compra (bater impostos/st)
// TODO: quando o usuario editar valorUnid recalcular o total
// TODO: poder fazer importacao parcial de nota (quando a linha fica amarela)
// TODO: antes de salvar verificar se o lote foi preenchido (se nao houver lote pedir para preencher com N/D)

// TODO: erro na criacao dos consumos: item que foi consumido em 2 estoques fez o consumo total nos dois (vide ALPH-180154)
