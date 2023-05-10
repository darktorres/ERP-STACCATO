#include "estoque.h"
#include "ui_estoque.h"

#include "acbrlib.h"
#include "application.h"
#include "doubledelegate.h"
#include "estoqueproxymodel.h"
#include "sql.h"
#include "user.h"

#include <QDebug>
#include <QScrollBar>
#include <QSqlError>
#include <QSqlRecord>

Estoque::Estoque(const QVariant &idEstoque_, QWidget *parent) : QDialog(parent), idEstoque(idEstoque_.toString()), ui(new Ui::Estoque) {
  ui->setupUi(this);

  setWindowFlags(Qt::Window);

  setupTables();

  preencherRestante();

  limitarAlturaTabela();

  if (not User::isAdministrativo()) {
    ui->pushButtonExibirNfe->hide();
    ui->pushButtonAjustarQuant->hide();
  }

  if (idEstoque.isEmpty()) { throw RuntimeException("Estoque não encontrado!", this); }

  setConnections();
}

Estoque::~Estoque() { delete ui; }

void Estoque::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->pushButtonAjustarQuant, &QPushButton::clicked, this, &Estoque::on_pushButtonAjustarQuant_clicked, connectionType);
  connect(ui->pushButtonExibirNfe, &QPushButton::clicked, this, &Estoque::on_pushButtonExibirNfe_clicked, connectionType);
  connect(ui->tableConsumo, &TableView::doubleClicked, this, &Estoque::on_tableConsumo_doubleClicked, connectionType);
}

void Estoque::setupTables() {
  modelEstoque.setQuery(Sql::view_estoque(idEstoque));

  modelEstoque.select();

  modelEstoque.setHeaderData("idEstoque", "Estoque");
  modelEstoque.setHeaderData("recebidoPor", "Recebido Por");
  modelEstoque.setHeaderData("status", "Status");
  modelEstoque.setHeaderData("fornecedor", "Fornecedor");
  modelEstoque.setHeaderData("descricao", "Produto");
  modelEstoque.setHeaderData("observacao", "Obs.");
  modelEstoque.setHeaderData("lote", "Lote");
  modelEstoque.setHeaderData("local", "Local");
  modelEstoque.setHeaderData("label", "Bloco");
  modelEstoque.setHeaderData("quant", "Quant.");
  modelEstoque.setHeaderData("un", "Un.");
  modelEstoque.setHeaderData("caixas", "Caixas");
  modelEstoque.setHeaderData("codComercial", "Cód. Com.");

  modelEstoque.proxyModel = new EstoqueProxyModel(&modelEstoque, this);

  ui->tableEstoque->setModel(&modelEstoque);

  ui->tableEstoque->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));

  ui->tableEstoque->hideColumn("idNFe");
  ui->tableEstoque->hideColumn("idProduto");
  ui->tableEstoque->hideColumn("idBloco");
  ui->tableEstoque->hideColumn("quantUpd");
  ui->tableEstoque->hideColumn("restante");
  ui->tableEstoque->hideColumn("codBarras");
  ui->tableEstoque->hideColumn("ncm");
  ui->tableEstoque->hideColumn("cfop");
  ui->tableEstoque->hideColumn("valorUnid");
  ui->tableEstoque->hideColumn("quantCaixa");
  ui->tableEstoque->hideColumn("codBarrasTrib");
  ui->tableEstoque->hideColumn("unTrib");
  ui->tableEstoque->hideColumn("quantTrib");
  ui->tableEstoque->hideColumn("valorUnidTrib");
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
  ui->tableEstoque->hideColumn("vICMSST");
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

  //--------------------------------------------------------------------

  modelConsumo.setTable("view_estoque_consumo");

  modelConsumo.setFilter("idEstoque = " + idEstoque);

  modelConsumo.select();

  modelConsumo.setHeaderData("statusProduto", "Status Pedido");
  modelConsumo.setHeaderData("status", "Status Consumo");
  modelConsumo.setHeaderData("bloco", "Bloco");
  modelConsumo.setHeaderData("fornecedor", "Fornecedor");
  modelConsumo.setHeaderData("descricao", "Produto");
  modelConsumo.setHeaderData("quant", "Quant.");
  modelConsumo.setHeaderData("un", "Un.");
  modelConsumo.setHeaderData("caixas", "Caixas");
  modelConsumo.setHeaderData("codComercial", "Cód. Com.");
  modelConsumo.setHeaderData("dataRealEnt", "Entrega");
  modelConsumo.setHeaderData("created", "Criado");

  modelConsumo.proxyModel = new EstoqueProxyModel(&modelConsumo, this);

  ui->tableConsumo->setModel(&modelConsumo);

  ui->tableConsumo->setItemDelegateForColumn("quant", new DoubleDelegate(4, this));

  ui->tableConsumo->showColumn("created");
  ui->tableConsumo->hideColumn("idEstoque");
  ui->tableConsumo->hideColumn("quantUpd");
}

void Estoque::preencherRestante() {
  const double quantRestante = modelEstoque.data(0, "restante").toDouble();
  const QString un = modelEstoque.data(0, "un").toString();

  ui->doubleSpinBoxQuantRestante->setValue(quantRestante);
  ui->doubleSpinBoxQuantRestante->setSuffix(" " + un);

  //--------------------------------------

  const double quantCaixa = modelEstoque.data(0, "quantCaixa").toDouble();
  const double caixasRestante = quantRestante / quantCaixa;

  ui->doubleSpinBoxCaixasRestante->setValue(caixasRestante);
  ui->doubleSpinBoxCaixasRestante->setSuffix(" cx.");
}

void Estoque::limitarAlturaTabela() {
  int rowTotalHeight = ui->tableEstoque->verticalHeader()->sectionSize(0) * 2; // add extra height

  int count = ui->tableEstoque->verticalHeader()->count();

  for (int i = 0; i < count; ++i) {
    if (not ui->tableEstoque->verticalHeader()->isSectionHidden(i)) { rowTotalHeight += ui->tableEstoque->verticalHeader()->sectionSize(i); }
  }

  ui->tableEstoque->setMaximumHeight(rowTotalHeight);
}

void Estoque::on_pushButtonExibirNfe_clicked() { exibirNota(); }

void Estoque::exibirNota() {
  SqlQuery query;
  query.prepare("SELECT xml FROM nfe WHERE idNFe = :idNFe");
  query.bindValue(":idNFe", modelEstoque.data(0, "idNFe"));

  if (not query.exec()) { throw RuntimeException("Erro buscando NF-e: " + query.lastError().text(), this); }

  if (not query.first()) { return qApp->enqueueWarning("Não encontrou NF-e associada!", this); }

  ACBrLib::gerarDanfe(query.value("xml").toString(), true);
}

void Estoque::criarConsumo(const int idVendaProduto2, const double quant) {
  // TODO: verificar se as divisões de linha batem com a outra função criarConsumo

  if (modelEstoque.rowCount() == 0) { throw RuntimeException("Não setou idEstoque!"); }

  // TODO: o valor restante no spinbox não é atualizado, se essa função for chamada mais de uma vez pode ocasionar em um consumo maior que o estoque
  if (quant > ui->doubleSpinBoxQuantRestante->value()) { throw RuntimeException("Quantidade insuficiente do estoque " + idEstoque + "!"); }

  // -------------------------------------------------------------------------

  dividirCompra(idVendaProduto2, quant);

  // -------------------------------------------------------------------------

  SqlTableModel modelConsumo;

  modelConsumo.setTable("estoque_has_consumo");

  const int rowEstoque = 0;
  const int rowConsumo = modelConsumo.insertRowAtEnd();

  const double quantCaixa = modelEstoque.data(rowEstoque, "quantCaixa").toDouble();
  const double caixas = quant / quantCaixa;
  const double quantEstoque = modelEstoque.data(rowEstoque, "quant").toDouble();
  const double proporcao = quant / quantEstoque;

  modelConsumo.setData(rowConsumo, "idEstoque", modelEstoque.data(rowEstoque, "idEstoque"));
  modelConsumo.setData(rowConsumo, "idVendaProduto2", idVendaProduto2);
  modelConsumo.setData(rowConsumo, "status", "CONSUMO");
  modelConsumo.setData(rowConsumo, "idBloco", modelEstoque.data(rowEstoque, "idBloco"));
  modelConsumo.setData(rowConsumo, "idProduto", modelEstoque.data(rowEstoque, "idProduto"));
  modelConsumo.setData(rowConsumo, "fornecedor", modelEstoque.data(rowEstoque, "fornecedor"));
  modelConsumo.setData(rowConsumo, "descricao", modelEstoque.data(rowEstoque, "descricao"));
  modelConsumo.setData(rowConsumo, "quant", quant * -1);
  modelConsumo.setData(rowConsumo, "quantUpd", static_cast<int>(FieldColors::DarkGreen));
  modelConsumo.setData(rowConsumo, "un", modelEstoque.data(rowEstoque, "un"));
  modelConsumo.setData(rowConsumo, "caixas", caixas);
  modelConsumo.setData(rowConsumo, "codBarras", modelEstoque.data(rowEstoque, "codBarras"));
  modelConsumo.setData(rowConsumo, "codComercial", modelEstoque.data(rowEstoque, "codComercial"));
  modelConsumo.setData(rowConsumo, "ncm", modelEstoque.data(rowEstoque, "ncm"));
  modelConsumo.setData(rowConsumo, "cfop", modelEstoque.data(rowEstoque, "cfop"));
  modelConsumo.setData(rowConsumo, "valorUnid", modelEstoque.data(rowEstoque, "valorUnid"));

  const double valorUnid = modelConsumo.data(rowConsumo, "valorUnid").toDouble();
  modelConsumo.setData(rowConsumo, "valor", quant * valorUnid);

  modelConsumo.setData(rowConsumo, "codBarrasTrib", modelEstoque.data(rowEstoque, "codBarrasTrib"));
  modelConsumo.setData(rowConsumo, "unTrib", modelEstoque.data(rowEstoque, "unTrib"));
  modelConsumo.setData(rowConsumo, "quantTrib", modelEstoque.data(rowEstoque, "quantTrib").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "valorUnidTrib", modelEstoque.data(rowEstoque, "valorUnidTrib"));
  modelConsumo.setData(rowConsumo, "desconto", modelEstoque.data(rowEstoque, "desconto").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "compoeTotal", modelEstoque.data(rowEstoque, "compoeTotal"));
  modelConsumo.setData(rowConsumo, "numeroPedido", modelEstoque.data(rowEstoque, "numeroPedido"));
  modelConsumo.setData(rowConsumo, "itemPedido", modelEstoque.data(rowEstoque, "itemPedido"));
  modelConsumo.setData(rowConsumo, "tipoICMS", modelEstoque.data(rowEstoque, "tipoICMS"));
  modelConsumo.setData(rowConsumo, "orig", modelEstoque.data(rowEstoque, "orig"));
  modelConsumo.setData(rowConsumo, "cstICMS", modelEstoque.data(rowEstoque, "cstICMS"));
  modelConsumo.setData(rowConsumo, "modBC", modelEstoque.data(rowEstoque, "modBC"));
  modelConsumo.setData(rowConsumo, "vBC", modelEstoque.data(rowEstoque, "vBC").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pICMS", modelEstoque.data(rowEstoque, "pICMS"));
  modelConsumo.setData(rowConsumo, "vICMS", modelEstoque.data(rowEstoque, "vICMS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "modBCST", modelEstoque.data(rowEstoque, "modBCST"));
  modelConsumo.setData(rowConsumo, "pMVAST", modelEstoque.data(rowEstoque, "pMVAST"));
  modelConsumo.setData(rowConsumo, "vBCST", modelEstoque.data(rowEstoque, "vBCST").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pICMSST", modelEstoque.data(rowEstoque, "pICMSST"));
  modelConsumo.setData(rowConsumo, "vICMSST", modelEstoque.data(rowEstoque, "vICMSST").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "cEnq", modelEstoque.data(rowEstoque, "cEnq"));
  modelConsumo.setData(rowConsumo, "cstIPI", modelEstoque.data(rowEstoque, "cstIPI"));
  modelConsumo.setData(rowConsumo, "cstPIS", modelEstoque.data(rowEstoque, "cstPIS"));
  modelConsumo.setData(rowConsumo, "vBCPIS", modelEstoque.data(rowEstoque, "vBCPIS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pPIS", modelEstoque.data(rowEstoque, "pPIS"));
  modelConsumo.setData(rowConsumo, "vPIS", modelEstoque.data(rowEstoque, "vPIS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "cstCOFINS", modelEstoque.data(rowEstoque, "cstCOFINS"));
  modelConsumo.setData(rowConsumo, "vBCCOFINS", modelEstoque.data(rowEstoque, "vBCCOFINS").toDouble() * proporcao);
  modelConsumo.setData(rowConsumo, "pCOFINS", modelEstoque.data(rowEstoque, "pCOFINS"));
  modelConsumo.setData(rowConsumo, "vCOFINS", modelEstoque.data(rowEstoque, "vCOFINS").toDouble() * proporcao);

  modelConsumo.submitAll();

  // -------------------------------------------------------------------------
  // copy lote to venda_has_produto

  const QString lote = modelEstoque.data(rowEstoque, "lote").toString();

  if (not lote.isEmpty() and lote != "N/D") {
    SqlQuery queryProduto;
    queryProduto.prepare("UPDATE venda_has_produto2 SET lote = :lote WHERE idVendaProduto2 = :idVendaProduto2");
    queryProduto.bindValue(":lote", modelEstoque.data(rowEstoque, "lote"));
    queryProduto.bindValue(":idVendaProduto2", idVendaProduto2);

    if (not queryProduto.exec()) { throw RuntimeException("Erro salvando lote: " + queryProduto.lastError().text()); }
  }
}

void Estoque::dividirCompra(const int idVendaProduto2, const double quant) {
  // se quant a consumir for igual a quant da compra apenas alterar idVenda/produto
  // senao fazer a quebra

  SqlTableModel modelCompra;
  modelCompra.setTable("pedido_fornecedor_has_produto2");

  const QString subQuery = "SELECT idPedido2 FROM estoque_has_compra WHERE idEstoque = " + idEstoque;
  modelCompra.setFilter("(idPedido2 IN (" + subQuery + ") OR idRelacionado IN (" + subQuery + ")) AND idVenda IS NULL AND idVendaProduto2 IS NULL AND quant >= " + QString::number(quant));

  modelCompra.select();

  if (modelCompra.rowCount() == 0) { return; }

  //--------------------------------------------------------------------

  SqlQuery query;
  query.prepare("SELECT idVenda FROM venda_has_produto2 WHERE idVendaProduto2 = :idVendaProduto2");
  query.bindValue(":idVendaProduto2", idVendaProduto2);

  if (not query.exec()) { throw RuntimeException("Erro buscando idVenda: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para id: '" + QString::number(idVendaProduto2) + "'"); }

  const int row = 0;
  const double quantCompra = modelCompra.data(row, "quant").toDouble();

  if (quant > quantCompra) { throw RuntimeException("Erro quant > quantCompra"); }

  if (qFuzzyCompare(quant, quantCompra)) {
    modelCompra.setData(row, "idVenda", query.value("idVenda"));
    modelCompra.setData(row, "idVendaProduto2", idVendaProduto2);
  }

  const bool dividir = (quant < quantCompra);

  if (dividir) {
    // NOTE: *quebralinha pedido_fornecedor2

    const double caixas = modelCompra.data(row, "caixas").toDouble();
    const double prcUnitario = modelCompra.data(row, "prcUnitario").toDouble();
    const double quantOriginal = modelCompra.data(row, "quant").toDouble();
    const double proporcaoNovo = quant / quantOriginal;
    const double proporcaoAntigo = (quantOriginal - quant) / quantOriginal;

    modelCompra.setData(row, "quant", quantOriginal - quant);
    modelCompra.setData(row, "caixas", caixas * proporcaoAntigo);
    modelCompra.setData(row, "preco", prcUnitario * (quantOriginal - quant));

    // -------------------------------------------------------------------------

    const int newRow = modelCompra.insertRowAtEnd();

    for (int column = 0, columnCount = modelCompra.columnCount(); column < columnCount; ++column) {
      if (column == modelCompra.fieldIndex("idPedido2")) { continue; }
      if (column == modelCompra.fieldIndex("created")) { continue; }
      if (column == modelCompra.fieldIndex("lastUpdated")) { continue; }

      const QVariant value = modelCompra.data(row, column);

      if (value.isNull()) { continue; }

      modelCompra.setData(newRow, column, value);
    }

    // -------------------------------------------------------------------------

    modelCompra.setData(newRow, "idRelacionado", modelCompra.data(row, "idPedido2"));
    modelCompra.setData(newRow, "idVenda", query.value("idVenda"));
    modelCompra.setData(newRow, "idVendaProduto2", idVendaProduto2);
    modelCompra.setData(newRow, "quant", quant);
    modelCompra.setData(newRow, "caixas", caixas * proporcaoNovo);
    modelCompra.setData(newRow, "preco", prcUnitario * quant);
  }

  modelCompra.submitAll();
}

void Estoque::desfazerConsumo(const int idVendaProduto2) {
  // there is one implementation in InputDialogConfirmacao
  // TODO: juntar as lógicas
  // TODO: se houver agendamento de estoque remover

  // NOTE: estoque_has_consumo may have the same idVendaProduto2 in more than one row (only until the field is made UNIQUE)
  SqlQuery queryDelete;

  if (not queryDelete.exec("DELETE FROM estoque_has_consumo WHERE idVendaProduto2 = " + QString::number(idVendaProduto2))) {
    throw RuntimeException("Erro removendo consumo estoque: " + queryDelete.lastError().text());
  }

  // TODO: juntar linhas sem consumo do mesmo tipo? (usar idRelacionado)
  SqlQuery queryCompra;

  if (not queryCompra.exec("UPDATE pedido_fornecedor_has_produto2 SET idVenda = NULL, idVendaProduto2 = NULL WHERE idVendaProduto2 = " + QString::number(idVendaProduto2) +
                           " AND status NOT IN ('CANCELADO', 'DEVOLVIDO')")) {
    throw RuntimeException("Erro atualizando pedido compra: " + queryCompra.lastError().text());
  }

  SqlQuery queryVenda;

  if (not queryVenda.exec(" UPDATE venda_has_produto2 SET "
                          "    status = CASE "
                          "    WHEN reposicaoEntrega THEN 'REPO. ENTREGA' "
                          "    WHEN reposicaoReceb THEN 'REPO. RECEB.' "
                          "    ELSE 'PENDENTE' END, "
                          "    idCompra = NULL, "
                          "    lote = NULL, "
                          "    dataPrevCompra = NULL, "
                          "    dataRealCompra = NULL, "
                          "    dataPrevConf = NULL, "
                          "    dataRealConf = NULL, "
                          "    dataPrevFat = NULL, "
                          "    dataRealFat = NULL, "
                          "    dataPrevColeta = NULL, "
                          "    dataRealColeta = NULL, "
                          "    dataPrevReceb = NULL, "
                          "    dataRealReceb = NULL, "
                          "    dataPrevEnt = NULL, "
                          "    dataRealEnt = NULL "
                          " WHERE "
                          "    `idVendaProduto2` = " +
                          QString::number(idVendaProduto2) + " AND status NOT IN ('CANCELADO', 'DEVOLVIDO', 'QUEBRADO')")) {
    throw RuntimeException("Erro atualizando pedido venda: " + queryVenda.lastError().text());
  }
}

void Estoque::on_tableConsumo_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = modelConsumo.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelConsumo.data(index.row(), "Venda")); }
}

void Estoque::on_pushButtonAjustarQuant_clicked() {
  // criar linha de consumo 'ajuste'
  // TODO: salvar foto dos produtos quebrados
  // TODO: se a quantidade quebrada for maior que o estoque loja, pedir para o usuario escolher qual consumo remover e fazer um followup no pedido dizendo que o consumo
  // foi removido devido a quebra

  const double quantCx = modelEstoque.data(0, "quantCaixa").toDouble();

  bool ok = false;
  const double ajusteCx = QInputDialog::getDouble(this, "Ajuste", "Ajustar caixas: ", 0, ui->doubleSpinBoxCaixasRestante->value() * -1, INT_MAX, 4, &ok);

  if (not ok or qFuzzyIsNull(ajusteCx)) { return; }

  QString observacao = QInputDialog::getText(this, "Observação", "Digite a observação: ", QLineEdit::Normal, QString(), &ok);

  if (not ok) { return; }

  if (observacao.isEmpty()) { throw RuntimeError("É necessário preencher a observação!", this); }

  observacao.prepend("Ajuste feito por " + User::usuario + ". Obs: ");

  SqlQuery queryConsumo;

  QString quant = QString::number(ajusteCx * quantCx);
  QString caixas = QString::number(ajusteCx);

  if (not queryConsumo.exec("INSERT INTO estoque_has_consumo (idEstoque, status, descricao, quant, caixas) VALUES (" + idEstoque + ", 'AJUSTE', '" + observacao.toUpper() + "', " + quant + ", " +
                            caixas + ")")) {
    throw RuntimeException("Erro criando consumo de ajuste: " + queryConsumo.lastError().text());
  }

  setupTables();
  preencherRestante();

  qApp->enqueueInformation("Ajuste feito com sucesso!");
}

// TODO: 1colocar o botao de desvincular consumo nesta tela
// TODO: no view_widget_estoque deixar apenas o status do consumo
