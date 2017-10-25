#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>

#include "doubledelegate.h"
#include "estoque.h"
#include "importarxml.h"
#include "ui_widgetestoque.h"
#include "widgetestoque.h"
#include "xlsxdocument.h"
#include "xml.h"

WidgetEstoque::WidgetEstoque(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoque) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetEstoque::montaFiltro);
  connect(ui->radioButtonEstoqueZerado, &QRadioButton::toggled, this, &WidgetEstoque::montaFiltro);
  connect(ui->radioButtonMaior, &QRadioButton::toggled, this, &WidgetEstoque::montaFiltro);
  connect(ui->radioButtonEstoqueContabil, &QRadioButton::toggled, this, &WidgetEstoque::montaFiltro);

  ui->dateEditMes->setDate(QDate::currentDate());
}

WidgetEstoque::~WidgetEstoque() { delete ui; }

void WidgetEstoque::setupTables() {
  // TODO: [Conrado] arrumar ordenacao que nao esta funcionando (tem outra tela que usei um proxy para ordenar)

  model.setQuery(
      "SELECT n.cnpjDest, e.status, e.idEstoque, p.fornecedor, e.descricao, e.quant + COALESCE(consumo, 0) AS restante, e.un AS unEst, IF((`p`.`un` = `p`.`un2`), `p`.`un`, CONCAT(`p`.`un`, '/', "
      "`p`.`un2`)) AS `unProd`, IF(((`p`.`un` = 'M²') OR (`p`.`un` = 'M2') OR (`p`.`un` = 'ML')), ((`e`.`quant` + COALESCE(consumo, 0)) / `p`.`m2cx`), ((`e`.`quant` + COALESCE(consumo, "
      "0)) / `p`.`pccx`)) AS `Caixas`, `e`.`lote` AS `lote`, `e`.`local` AS `local`, `e`.`bloco` AS `bloco`, `e`.`codComercial` AS `codComercial`, GROUP_CONCAT(DISTINCT `n`.`numeroNFe` "
      "SEPARATOR ', ') AS `nfe`, `vp`.`idCompra` AS `idCompra`, `vp`.`dataPrevColeta` AS `dataPrevColeta`, `vp`.`dataRealColeta` AS `dataRealColeta`, `vp`.`dataPrevReceb` AS "
      "`dataPrevReceb`, `vp`.`dataRealReceb` AS `datRealReceb` FROM (SELECT * FROM estoque e WHERE status != 'QUEBRADO' AND status != 'CANCELADO') e LEFT JOIN (SELECT SUM(quant) AS "
      "consumo, ec.idEstoque, ec.idVendaProduto FROM estoque_has_consumo ec GROUP BY idEstoque) ec ON e.idEstoque = ec.idEstoque LEFT JOIN (SELECT * FROM produto p) p ON p.idProduto = "
      "e.idProduto LEFT JOIN (SELECT * FROM venda_has_produto vp) vp ON vp.idVendaProduto = ec.idVendaProduto LEFT JOIN (SELECT * FROM estoque_has_nfe ehn) ehn ON e.idEstoque = "
      "ehn.idEstoque LEFT JOIN (SELECT * FROM nfe n) n ON ehn.idNFe = n.idNFe GROUP BY e.idEstoque HAVING restante > 0");

  if (model.lastError().isValid()) emit errorSignal("Erro lendo tabela estoque: " + model.lastError().text());

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

  auto *proxyFilter = new QSortFilterProxyModel(this);
  proxyFilter->setDynamicSortFilter(true);
  proxyFilter->setSourceModel(&model);

  ui->table->setModel(proxyFilter);
  ui->table->hideColumn("idCompra");
  ui->table->hideColumn("restante est");
  ui->table->setItemDelegate(new DoubleDelegate(this));
}

bool WidgetEstoque::updateTables() {
  if (model.query().executedQuery().isEmpty()) setupTables();

  model.setQuery(model.query().executedQuery());

  if (model.lastError().isValid()) emit errorSignal("Erro lendo tabela estoque: " + model.lastError().text());

  ui->table->resizeColumnsToContents();

  return true;
}

void WidgetEstoque::on_table_activated(const QModelIndex &index) {
  auto *estoque = new Estoque(model.data(index.row(), "idEstoque").toString(), true, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetEstoque::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetEstoque::montaFiltro() {
  ui->radioButtonEstoqueContabil->isChecked() ? ui->table->showColumn("restante est") : ui->table->hideColumn("restante est");

  const QString text = ui->lineEditBusca->text();

  const QString match = text.isEmpty() ? ""
                                       : "WHERE (MATCH (e.descricao , e.codComercial) AGAINST ('+" + text + "*' IN BOOLEAN MODE) OR MATCH (p.fornecedor) AGAINST ('+" + text +
                                             "*' IN BOOLEAN MODE) OR e.idEstoque = '" + text + "')";

  const QString restanteDeposito = ui->radioButtonEstoqueContabil->isChecked() ? "`restante deposito` > 0 AND status = 'ESTOQUE'" : "";
  const QString restante = ui->radioButtonEstoqueZerado->isChecked() ? "restante <= 0" : "restante > 0";

  model.setQuery(
      "SELECT n.cnpjDest, e.status, e.idEstoque, p.fornecedor, e.descricao, e.quant + COALESCE(consumo, 0) AS restante, e.un AS unEst, IF((`p`.`un` = `p`.`un2`), `p`.`un`, CONCAT(`p`.`un`, '/', "
      "`p`.`un2`)) AS `unProd`, IF(((`p`.`un` = 'M²') OR (`p`.`un` = 'M2') OR (`p`.`un` = 'ML')), ((`e`.`quant` + COALESCE(consumo, 0)) / `p`.`m2cx`), ((`e`.`quant` + COALESCE(consumo, "
      "0)) / `p`.`pccx`)) AS `Caixas`, `e`.`lote` AS `lote`, `e`.`local` AS `local`, `e`.`bloco` AS `bloco`, `e`.`codComercial` AS `codComercial`, GROUP_CONCAT(DISTINCT `n`.`numeroNFe` "
      "SEPARATOR ', ') AS `nfe`, `vp`.`idCompra` AS `idCompra`, `vp`.`dataPrevColeta` AS `dataPrevColeta`, `vp`.`dataRealColeta` AS `dataRealColeta`, `vp`.`dataPrevReceb` AS "
      "`dataPrevReceb`, `vp`.`dataRealReceb` AS `dataRealReceb` FROM (SELECT * FROM estoque e WHERE status != 'QUEBRADO' AND status != 'CANCELADO') e LEFT JOIN (SELECT SUM(quant) AS "
      "consumo, ec.idEstoque, ec.idVendaProduto FROM estoque_has_consumo ec GROUP BY idEstoque) ec ON e.idEstoque = ec.idEstoque LEFT JOIN (SELECT * FROM produto p) p ON p.idProduto = "
      "e.idProduto LEFT JOIN (SELECT * FROM venda_has_produto vp) vp ON vp.idVendaProduto = ec.idVendaProduto LEFT JOIN (SELECT * FROM estoque_has_nfe ehn) ehn ON e.idEstoque = "
      "ehn.idEstoque LEFT JOIN (SELECT * FROM nfe n) n ON ehn.idNFe = n.idNFe " +
      match + " GROUP BY e.idEstoque HAVING " + restante);

  if (model.lastError().isValid()) emit errorSignal("Erro lendo tabela estoque: " + model.lastError().text());
}

void WidgetEstoque::on_pushButtonRelatorio_clicked() {
  // TODO: 0para filtrar corretamente pela data refazer a view para considerar o `restante est` na data desejada e nao na
  // atual
  // TODO: 0adicionar caixas

  // 1. codigo produto
  // 2. descricao
  // 3. ncm
  // 4. unidade
  // 5. quantidade
  // 6. valor
  // 7. aliquota icms (se tiver)

  SqlTableModel modelContabil;
  modelContabil.setTable("view_estoque_contabil");
  modelContabil.setEditStrategy(SqlTableModel::OnManualSubmit);

  modelContabil.setFilter("dataRealReceb <= '" + ui->dateEditMes->date().toString("yyyy-MM-dd") + "'");

  if (not modelContabil.select()) {
    QMessageBox::critical(this, "Erro!", "Erro lendo tabela: " + modelContabil.lastError().text());
    return;
  }

  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) return;

  const QString arquivoModelo = "relatorio.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    QMessageBox::critical(this, "Erro!", "Não encontrou o modelo do Excel!");
    return;
  }

  //  const QString fileName = dir + "/relatorio-" + ui->dateEditMes->date().toString("MM-yyyy") + ".xlsx";
  const QString fileName = dir + "/relatorio_contabil.xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) {
    QMessageBox::critical(this, "Erro!", "Não foi possível abrir o arquivo para escrita: " + fileName);
    QMessageBox::critical(this, "Erro!", "Erro: " + file.errorString());
    return;
  }

  file.close();

  QXlsx::Document xlsx(arquivoModelo);

  //  xlsx.currentWorksheet()->setFitToPage(true);
  //  xlsx.currentWorksheet()->setFitToHeight(true);
  //  xlsx.currentWorksheet()->setOrientationVertical(false);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) xlsx.write(column + QString::number(1), modelContabil.headerData(col, Qt::Horizontal).toString());

  column = 'A';

  for (int row = 0; row < modelContabil.rowCount(); ++row) {
    for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) xlsx.write(column + QString::number(row + 2), modelContabil.data(row, col));

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) {
    QMessageBox::critical(this, "Erro!", "Ocorreu algum erro ao salvar o arquivo.");
    return;
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  QMessageBox::information(this, "Ok!", "Arquivo salvo como " + fileName);
}

// NOTE: gerenciar lugares de estoque (cadastro/permissoes)
// TODO: 3tem produto com unidade barra que na verdade significa ML

// TODO: 5colocar um filtro para mostrar os cancelados/quebrados?
// TODO: 2poder trocar bloco do estoque
// TODO: -1verificar se o custo do pedido_fornecedor bate com os valores do estoque/consumo
// TODO: reimplementar estoque_contabil
// TODO: terminar de arrumar relatorio estoque
// TODO: [Conrado] colocar filtro/tela para buscar por pedido e mostrar os estoques em que foi consumido
