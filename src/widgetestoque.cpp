#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "importarxml.h"
#include "ui_widgetestoque.h"
#include "widgetestoque.h"
#include "xlsxdocument.h"
#include "xml.h"

WidgetEstoque::WidgetEstoque(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoque) {
  ui->setupUi(this);
  // FIXME: fix query
  //  ui->pushButtonRelatorio->setDisabled(true);
}

WidgetEstoque::~WidgetEstoque() { delete ui; }

void WidgetEstoque::setConnections() {
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetEstoque::montaFiltro);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetEstoque::on_pushButtonRelatorio_clicked);
  connect(ui->radioButtonEstoqueContabil, &QRadioButton::toggled, this, &WidgetEstoque::montaFiltro);
  connect(ui->radioButtonEstoqueZerado, &QRadioButton::toggled, this, &WidgetEstoque::montaFiltro);
  connect(ui->radioButtonMaior, &QRadioButton::toggled, this, &WidgetEstoque::montaFiltro);
  connect(ui->table, &TableView::activated, this, &WidgetEstoque::on_table_activated);
  connect(ui->table, &TableView::entered, this, &WidgetEstoque::on_table_entered);
}

void WidgetEstoque::setupTables() {
  montaFiltro();

  model.setHeaderData("cnpjDest", "CNPJ");
  model.setHeaderData("status", "Status");
  model.setHeaderData("idEstoque", "Estoque");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  model.setHeaderData("restante", "Quant. Rest.");
  model.setHeaderData("restante est", "Quant. Depósito");
  model.setHeaderData("unEst", "Un. Est.");
  model.setHeaderData("unProd", "Un. Prod.");
  model.setHeaderData("unProd2", "Un. Prod. 2");
  model.setHeaderData("lote", "Lote");
  model.setHeaderData("local", "Local");
  model.setHeaderData("bloco", "Bloco");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("nfe", "NFe");
  model.setHeaderData("dataPrevColeta", "Prev. Coleta");
  model.setHeaderData("dataRealColeta", "Coleta");
  model.setHeaderData("dataPrevReceb", "Prev. Receb.");
  model.setHeaderData("dataRealReceb", "Receb.");
  model.setHeaderData("dataPrevEnt", "Prev. Ent.");
  model.setHeaderData("dataRealEnt", "Entrega");
  model.setHeaderData("ordemCompra", "OC");

  auto *proxyFilter = new QSortFilterProxyModel(this);
  proxyFilter->setDynamicSortFilter(true);
  proxyFilter->setSourceModel(&model);

  ui->table->setModel(proxyFilter);
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("restante est");
  ui->table->setItemDelegate(new DoubleDelegate(this));
}

void WidgetEstoque::updateTables() {
  if (not isSet) {
    ui->dateEditMes->setDate(QDate::currentDate());
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  model.setQuery(model.query().executedQuery());

  if (model.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela estoque: " + model.lastError().text()); }

  ui->table->resizeColumnsToContents();
}

void WidgetEstoque::resetTables() { modelIsSet = false; }

void WidgetEstoque::on_table_activated(const QModelIndex &index) {
  auto *estoque = new Estoque(ui->table->model()->data(ui->table->model()->index(index.row(), model.record().indexOf("idEstoque"))).toString(), true, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetEstoque::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetEstoque::montaFiltro() {
  // FIXME: digitar hifen causa erro na pesquisa
  // TODO: arrumar query (está nas gambiarras temporariamente)
  // TODO: this query is different from view_estoque2 string and some columns change

  ui->radioButtonEstoqueContabil->isChecked() ? ui->table->showColumn("restante est") : ui->table->hideColumn("restante est");

  const QString text = ui->lineEditBusca->text();

  const QString match1 = text.isEmpty() ? "" : " AND (MATCH (e.descricao , e.codComercial) AGAINST ('+" + text + "*' IN BOOLEAN MODE) OR e.idEstoque = '" + text + "')";

  // TODO: remove
  //  const QString match2 = text.isEmpty() ? ""
  //                                        : "(MATCH (e.descricao , e.codComercial) AGAINST ('+" + text + "*' IN BOOLEAN MODE) OR MATCH (p.fornecedor) AGAINST ('+" + text +
  //                                              "*' IN BOOLEAN MODE) OR e.idEstoque = '" + text + "')";

  const QString restante = ui->radioButtonEstoqueZerado->isChecked() ? "consumo <= 0" : "consumo > 0";

  model.setQuery(
      "SELECT `n`.`cnpjDest` AS `cnpjDest`, e.status, e.idEstoque, pf.fornecedor, e.descricao, e.consumo AS restante, e.un AS `unEst`, "
      "if((`p`.`un` = `p`.`un2`), `p`.`un`, concat(`p`.`un`, '/', `p`.`un2`)) AS `unProd`, if(((`p`.`un` = 'M²') OR (`p`.`un` = 'M2') OR (`p`.`un` = 'ML')), "
      "(consumo / `p`.`m2cx`), (consumo / `p`.`pccx`)) AS `Caixas`, e.lote, e.local, e.bloco, e.codComercial, `n`.`numeroNFe` AS `nfe`, pf.idCompra, pf.dataPrevColeta, pf.dataRealColeta, "
      "pf.dataPrevReceb, pf.dataRealReceb FROM (SELECT e.idProduto, e.status, e.idEstoque, e.descricao, e.codComercial, e.un, e.lote, e.local, e.bloco, e.quant, e.quant + "
      "coalesce(sum(consumo.quant), 0) AS consumo FROM estoque e LEFT JOIN estoque_has_consumo consumo ON e.idEstoque = consumo.idEstoque WHERE e.status != 'CANCELADO' AND e.status != 'QUEBRADO' " +
      match1 + " GROUP BY e.idEstoque HAVING " + restante +
      ") e LEFT JOIN estoque_has_compra ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN pedido_fornecedor_has_produto pf ON pf.idCompra = ehc.idCompra AND e.codComercial = pf.codComercial "
      "LEFT JOIN estoque_has_nfe ehn ON e.idEstoque = ehn.idEstoque LEFT JOIN nfe n ON ehn.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto GROUP BY e.idEstoque , n.cnpjDest , "
      "pf.fornecedor , pf.idCompra , pf.dataPrevColeta , pf.dataRealColeta , pf.dataPrevReceb , pf.dataRealReceb , n.numeroNFe");

  //  model.setQuery(view_estoque2 + " " + match + " GROUP BY e.idEstoque HAVING " + restante);

  //  qDebug() << "query: " << view_estoque2 + " " + match + " GROUP BY e.idEstoque HAVING " + restante;

  if (model.lastError().isValid()) { qApp->enqueueError("Erro lendo tabela estoque: " + model.lastError().text()); }
}

void WidgetEstoque::on_pushButtonRelatorio_clicked() {
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
      "SELECT e.idEstoque, group_concat(DISTINCT `n`.`cnpjDest` SEPARATOR ',') AS `cnpjDest`, e.status, p.fornecedor, e.descricao, e.quant + coalesce(e2.consumoEst, 0) + ajuste AS contabil, "
      "e.restante AS disponivel, e.un AS `unEst`, p.un AS `unProd`, if(((`p`.`un` = 'M²') OR (`p`.`un` = 'M2') OR (`p`.`un` = 'ML')), "
      "((e.quant + coalesce(e2.consumoEst, 0) + ajuste) / `p`.`m2cx`), ((e.quant + coalesce(e2.consumoEst, 0) + ajuste) / `p`.`pccx`)) AS `Caixas`, e.lote, e.local, e.bloco, e.codComercial, "
      "group_concat(DISTINCT `n`.`numeroNFe` SEPARATOR ', ') AS `nfe`, p.custo AS custoUnit, p.precoVenda AS precoVendaUnit, p.custo * (e.quant + coalesce(e2.consumoEst, 0) + ajuste) AS custo, "
      "p.precoVenda * (e.quant + coalesce(e2.consumoEst, 0) + ajuste) AS precoVenda, pf.idCompra, pf.dataPrevColeta, pf.dataRealColeta, pf.dataPrevReceb, pf.dataRealReceb FROM (SELECT e.idProduto, "
      "e.status, e.idEstoque, e.descricao, e.codComercial, e.valorTrib, e.un, e.lote, e.local, e.bloco, e.quant, e.quant + coalesce(sum(consumo.quant), 0) AS restante, sum(CASE WHEN consumo.status = "
      "'AJUSTE' THEN consumo.quant ELSE 0 END) AS ajuste, e.created FROM estoque e LEFT JOIN estoque_has_consumo consumo ON e.idEstoque = consumo.idEstoque WHERE e.status = 'ESTOQUE' "
      "AND e.created < '" +
      data +
      "' GROUP BY e.idEstoque) e LEFT JOIN (SELECT consumo.idEstoque, SUM(consumo.quant) AS consumoEst, SUM(IF(vp.status != 'DEVOLVIDO ESTOQUE', vp.quant, 0)) AS consumoVenda, vp.dataRealEnt FROM "
      "estoque_has_consumo consumo LEFT JOIN venda_has_produto vp ON consumo.idVendaProduto = vp.idVendaProduto WHERE (vp.dataRealEnt < '" +
      data +
      "') AND consumo.status != 'CANCELADO' GROUP BY consumo.idEstoque) e2 ON e.idEstoque = e2.idEstoque LEFT JOIN estoque_has_compra ehc ON e.idEstoque = ehc.idEstoque LEFT JOIN "
      "pedido_fornecedor_has_produto pf ON pf.idCompra = ehc.idCompra AND e.codComercial = pf.codComercial LEFT JOIN estoque_has_nfe ehn ON e.idEstoque = ehn.idEstoque LEFT JOIN "
      "nfe n ON ehn.idNFe = n.idNFe LEFT JOIN produto p ON e.idProduto = p.idProduto "
      " GROUP BY e.idEstoque HAVING contabil > 0");

  if (modelContabil.lastError().isValid()) { return qApp->enqueueError("Erro lendo tabela: " + modelContabil.lastError().text()); }

  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = "modelo relatorio contabil.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) { return qApp->enqueueError("Não encontrou o modelo do Excel!"); }

  const QString fileName = dir + "/relatorio_contabil.xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  if (not gerarExcel(arquivoModelo, fileName, modelContabil)) { return; }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName);
}

bool WidgetEstoque::gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) {
  QXlsx::Document xlsx(arquivoModelo);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelContabil.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelContabil.rowCount(); ++row) {
    for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(row + 2), modelContabil.data(modelContabil.index(row, col))); }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) { return qApp->enqueueError(false, "Ocorreu algum erro ao salvar o arquivo."); }

  return true;
}

// NOTE: gerenciar lugares de estoque (cadastro/permissoes)
// TODO: 3tem produto com unidade barra que na verdade significa ML

// TODO: 5colocar um filtro para mostrar os cancelados/quebrados?
// TODO: 2poder trocar bloco do estoque
// TODO: -1verificar se o custo do pedido_fornecedor bate com os valores do estoque/consumo
// TODO: reimplementar estoque_contabil
// TODO: terminar de arrumar relatorio estoque
// TODO: [Conrado] colocar filtro/tela para buscar por pedido e mostrar os estoques em que foi consumido
// TODO: arrumar 'estoque contabil'
// TODO: fix fulltext indexes (put match against inside subquery)
