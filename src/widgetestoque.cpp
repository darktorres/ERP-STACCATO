#include "widgetestoque.h"
#include "ui_widgetestoque.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "sortfilterproxymodel.h"
#include "usersession.h"
#include "xlsxdocument.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QSqlError>

WidgetEstoque::WidgetEstoque(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoque) { ui->setupUi(this); }

WidgetEstoque::~WidgetEstoque() { delete ui; }

void WidgetEstoque::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetEstoque::escolheFiltro, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetEstoque::on_pushButtonRelatorio_clicked, connectionType);
  connect(ui->radioButtonEstoqueContabil, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltroContabil, connectionType);
  connect(ui->radioButtonEstoqueZerado, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltro, connectionType);
  connect(ui->radioButtonMaior, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltro, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetEstoque::on_table_activated, connectionType);
}

void WidgetEstoque::setupTables() {
  escolheFiltro();

  model.proxyModel = new SortFilterProxyModel(&model, this);

  ui->table->setModel(&model);

  ui->table->setItemDelegate(new DoubleDelegate(this));
}

void WidgetEstoque::setHeaderData() {
  model.setHeaderData("cnpjDest", "CNPJ");
  model.setHeaderData("status", "Status");
  model.setHeaderData("idEstoque", "Estoque");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("restante", "Quant. Rest.");

  if (ui->radioButtonEstoqueContabil->isChecked()) { model.setHeaderData("contabil", "Quant. Depósito"); }

  model.setHeaderData("unEst", "Un.");
  model.setHeaderData("lote", "Lote");
  model.setHeaderData("local", "Local");
  model.setHeaderData("bloco", "Bloco");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("nfe", "NFe");
  model.setHeaderData("dataPrevColeta", "Prev. Coleta");
  model.setHeaderData("dataRealColeta", "Coleta");
  model.setHeaderData("dataPrevReceb", "Prev. Receb.");
  model.setHeaderData("dataRealReceb", "Receb.");
}

void WidgetEstoque::updateTables() {
  if (not isSet) {
    ui->dateEditMes->setDate(qApp->serverDate());
    setConnections();

    const QString tipoUsuario = UserSession::tipoUsuario();

    if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") {
      ui->groupBoxFiltros->hide();
      ui->groupBoxRelatorio->hide();
    }

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  model.setQuery(model.query().executedQuery());

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela estoque: " + model.lastError().text(), this); }
}

void WidgetEstoque::resetTables() { modelIsSet = false; }

void WidgetEstoque::on_table_activated(const QModelIndex &index) {
  const QString idEstoque = model.data(index, "idEstoque").toString();

  auto *estoque = new Estoque(idEstoque, true, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetEstoque::escolheFiltro() { ui->radioButtonEstoqueContabil->isChecked() ? montaFiltroContabil() : montaFiltro(); }

void WidgetEstoque::montaFiltro() {
  const QString having = (ui->radioButtonMaior->isChecked()) ? "restante > 0" : "restante <= 0";

  model.setQuery(
      "SELECT n.cnpjDest AS cnpjDest, e.status, e.idEstoque, p.fornecedor, e.descricao, e.restante AS restante, e.un AS unEst, e.restante / p.quantCaixa AS Caixas, e.lote, e.local, e.bloco, "
      "e.codComercial, ANY_VALUE(n.numeroNFe) AS nfe, ANY_VALUE(pf.dataPrevColeta) AS dataPrevColeta, ANY_VALUE(pf.dataRealColeta) AS dataRealColeta, ANY_VALUE(pf.dataPrevReceb) AS dataPrevReceb, "
      "ANY_VALUE(pf.dataRealReceb) AS dataRealReceb FROM estoque e LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque LEFT JOIN pedido_fornecedor_has_produto2 pf ON pf.idPedido2 = "
      "ehc2.idPedido2 LEFT JOIN nfe n ON e.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto WHERE e.status NOT IN ('CANCELADO', 'QUEBRADO')" +
      getMatch() + " GROUP BY e.idEstoque HAVING " + having);

  if (model.lastError().isValid()) { qApp->enqueueError("Erro lendo tabela estoque: " + model.lastError().text(), this); }

  setHeaderData();
}

void WidgetEstoque::montaFiltroContabil() {
  // TODO: trocar o NOW() por uma data escolhida pelo usuario

  model.setQuery(
      "SELECT n.cnpjDest AS cnpjDest, e.status, e.idEstoque, p.fornecedor, e.descricao, e.quant + COALESCE(ehc.contabil, 0) + e.ajuste AS contabil, e.restante AS restante, e.un AS unEst, e.restante "
      "/ p.quantCaixa AS Caixas, e.lote, e.local, e.bloco, e.codComercial, ANY_VALUE(n.numeroNFe) AS nfe, ANY_VALUE(pf.dataPrevColeta) AS dataPrevColeta, ANY_VALUE(pf.dataRealColeta) AS "
      "dataRealColeta, ANY_VALUE(pf.dataPrevReceb) AS dataPrevReceb, ANY_VALUE(pf.dataRealReceb) AS dataRealReceb FROM estoque e LEFT JOIN (SELECT ehc.idEstoque, SUM(ehc.quant) AS contabil FROM "
      "estoque_has_consumo ehc LEFT JOIN venda_has_produto2 vp ON ehc.idVendaProduto2 = vp.idVendaProduto2 WHERE (vp.dataRealEnt < NOW()) AND ehc.status != 'CANCELADO' GROUP BY ehc.idEstoque) ehc ON "
      "e.idEstoque = ehc.idEstoque LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque LEFT JOIN pedido_fornecedor_has_produto2 pf ON pf.idPedido2 = ehc2.idPedido2 LEFT JOIN nfe n ON "
      "e.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto WHERE e.status NOT IN ('CANCELADO', 'QUEBRADO')" +
      getMatch() + " GROUP BY e.idEstoque HAVING contabil > 0");

  if (model.lastError().isValid()) { qApp->enqueueError("Erro lendo tabela estoque: " + model.lastError().text(), this); }

  setHeaderData();
}

QString WidgetEstoque::getMatch() const {
  const QString text = ui->lineEditBusca->text();

  if (text.isEmpty()) { return QString(); }

  QStringList strings = text.split(" ", QString::SkipEmptyParts);

  for (auto &string : strings) {
    if (string.contains("-")) {
      string.prepend("\"").append("\"");
    } else {
      string.replace("+", "").replace("-", "").replace("@", "").replace(">", "").replace("<", "").replace("(", "").replace(")", "").replace("~", "").replace("*", "");
      string.prepend("+").append("*");
    }
  }

  const QString text2 = strings.join(" ");

  const QString match =
      " AND (MATCH (e.descricao , e.codComercial) AGAINST ('" + text2 + "' IN BOOLEAN MODE) OR MATCH (p.fornecedor) AGAINST ('" + text2 + "' IN BOOLEAN MODE) OR e.idEstoque = '" + text + "')";

  return match;
}

void WidgetEstoque::on_pushButtonRelatorio_clicked() {
  // NOTE: verificar se deve considerar os estoques não recebidos (em coleta/recebimento)

  // 1. codigo produto
  // 2. descricao
  // 3. ncm
  // 4. unidade
  // 5. quantidade
  // 6. valor
  // 7. aliquota icms (se tiver)

  const QString data = ui->dateEditMes->date().addDays(1).toString("yyyy-MM-dd");

  SqlQueryModel modelContabil;
  modelContabil.setQuery(
      "SELECT e.idEstoque, GROUP_CONCAT(DISTINCT n.cnpjDest SEPARATOR ',') AS cnpjDest, e.status, p.fornecedor, e.descricao, e.quant + COALESCE(ehc.contabil, 0) + ajuste AS contabil, "
      "e.restante AS disponivel, e.un AS unEst, p.un AS unProd, (e.quant + COALESCE(ehc.contabil, 0) + ajuste) / p.quantCaixa AS Caixas, e.lote, e.local, e.bloco, e.codComercial, "
      "e.ncm, e.cstICMS, e.pICMS, e.cstIPI, e.cstPIS, e.cstCOFINS, GROUP_CONCAT(DISTINCT n.numeroNFe SEPARATOR ', ') AS nfe, p.custo AS custoUnit, p.precoVenda AS precoVendaUnit, p.custo * (e.quant "
      "+ COALESCE(ehc.contabil, 0) + ajuste) AS custo, p.precoVenda * (e.quant + COALESCE(ehc.contabil, 0) + ajuste) AS precoVenda FROM estoque e LEFT JOIN (SELECT ehc.idEstoque, SUM(ehc.quant) AS "
      "contabil, SUM(IF(vp.status != 'DEVOLVIDO ESTOQUE', vp.quant, 0)) AS consumoVenda FROM estoque_has_consumo ehc LEFT JOIN venda_has_produto2 vp ON ehc.idVendaProduto2 = vp.idVendaProduto2 WHERE "
      "(vp.dataRealEnt < '" +
      data +
      "') AND ehc.status != 'CANCELADO' GROUP BY ehc.idEstoque) ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN estoque_has_compra ehc2 ON e.idEstoque = ehc2.idEstoque LEFT JOIN "
      "pedido_fornecedor_has_produto2 pf ON pf.idPedido2 = ehc2.idPedido2 LEFT JOIN nfe n ON e.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto WHERE e.status = 'ESTOQUE' AND "
      "e.created < '" +
      data + "' GROUP BY e.idEstoque HAVING contabil > 0");

  if (modelContabil.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela: " + modelContabil.lastError().text(), this); }

  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = "modelo relatorio contabil.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) { return qApp->enqueueError("Não encontrou o modelo do Excel!", this); }

  const QString fileName = dir + "/relatorio_contabil.xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), this); }

  file.close();

  if (not gerarExcel(arquivoModelo, fileName, modelContabil)) { return; }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, this);
}

bool WidgetEstoque::gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) {
  QXlsx::Document xlsx(arquivoModelo);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelContabil.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelContabil.rowCount(); ++row) {
    for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(row + 2), modelContabil.data(row, col)); }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) { return qApp->enqueueError(false, "Ocorreu algum erro ao salvar o arquivo!", this); }

  return true;
}

// NOTE: gerenciar lugares de estoque (cadastro/permissoes)
// TODO: 3tem produto com unidade barra que na verdade significa ML

// TODO: 5colocar um filtro para mostrar os cancelados/quebrados?
// TODO: 2poder trocar bloco do estoque
// TODO: -1verificar se o custo do pedido_fornecedor bate com os valores do estoque/consumo
// TODO: terminar de arrumar relatorio estoque
// TODO: [Conrado] colocar filtro/tela para buscar por pedido e mostrar os estoques em que foi consumido
// TODO: fix fulltext indexes (put match against inside subquery)
