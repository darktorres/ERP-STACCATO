#include "widgetrelatorio.h"
#include "ui_widgetrelatorio.h"

#include "application.h"
#include "file.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "sql.h"
#include "user.h"
#include "xlsxdocument.h"

#include <QDebug>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QSqlError>

WidgetRelatorio::WidgetRelatorio(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetRelatorio) { ui->setupUi(this); }

WidgetRelatorio::~WidgetRelatorio() { delete ui; }

void WidgetRelatorio::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetRelatorio::dateEditMes_dateChanged, connectionType);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetRelatorio::on_pushButtonExcel_clicked, connectionType);
}

void WidgetRelatorio::setFilterTotaisVendedor() {
  const QString mes = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  const QString idUsuario = (User::isVendedor()) ? User::idUsuario : "";

  const QString idUsuarioConsultor = (User::isEspecial()) ? User::idUsuario : "";

  const QString loja = (User::isGerente()) ? User::fromLoja("descricao").toString() : "";

  modelViewRelatorioVendedor.setQuery(Sql::view_relatorio_vendedor(mes, idUsuario, idUsuarioConsultor, loja));

  modelViewRelatorioVendedor.select();

  ui->tableTotalVendedor->setModel(&modelViewRelatorioVendedor);

  ui->tableTotalVendedor->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("%", new PorcentagemDelegate(false, this));

  ui->tableTotalVendedor->hideColumn("Mês");
  ui->tableTotalVendedor->hideColumn("idUsuario");
  ui->tableTotalVendedor->hideColumn("idUsuarioConsultor");
}

void WidgetRelatorio::setFilterTotaisLoja() {
  const QString mes = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  const QString idUsuario = (User::isVendedor()) ? User::idUsuario : "";

  const QString idUsuarioConsultor = (User::isEspecial()) ? User::idUsuario : "";

  const QString loja = (User::isGerente()) ? User::fromLoja("descricao").toString() : "";

  modelViewRelatorioLoja.setQuery(Sql::view_relatorio_loja(mes, idUsuario, idUsuarioConsultor, loja));

  modelViewRelatorioLoja.select();

  ui->tableTotalLoja->setModel(&modelViewRelatorioLoja);

  ui->tableTotalLoja->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("%", new PorcentagemDelegate(false, this));
  ui->tableTotalLoja->setItemDelegateForColumn("Reposição", new ReaisDelegate(this));

  ui->tableTotalLoja->hideColumn("Mês");
}

void WidgetRelatorio::setupTables() {
  modelViewRelatorio.setTable("view_relatorio");

  modelViewRelatorio.setHeaderData("idVenda", "Venda");

  ui->tableRelatorio->setModel(&modelViewRelatorio);

  ui->tableRelatorio->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("%", new PorcentagemDelegate(false, this));

  ui->tableRelatorio->hideColumn("Mês");
  ui->tableRelatorio->hideColumn("idUsuario");
  ui->tableRelatorio->hideColumn("idUsuarioConsultor");
}

void WidgetRelatorio::calcularTotalGeral() {
  double totalGeral = 0;
  double comissao = 0;
  double porcentagem = 0;

  for (int row = 0; row < modelViewRelatorioLoja.rowCount(); ++row) {
    totalGeral += modelViewRelatorioLoja.data(row, "Faturamento").toDouble();
    comissao += modelViewRelatorioLoja.data(row, "Comissão").toDouble();
    porcentagem += modelViewRelatorioLoja.data(row, "%").toDouble();
  }

  ui->doubleSpinBoxGeral->setValue(totalGeral);
  ui->doubleSpinBoxValorComissao->setValue(comissao);
  if (modelViewRelatorioLoja.rowCount() > 0) { ui->doubleSpinBoxPorcentagemComissao->setValue(porcentagem / modelViewRelatorioLoja.rowCount()); }
}

void WidgetRelatorio::calcularTotalVendedor() {
  double comissao = 0;
  double porcentagem = 0;

  for (int row = 0; row < modelViewRelatorioVendedor.rowCount(); ++row) {
    comissao += modelViewRelatorioVendedor.data(row, "Comissão").toDouble();
    porcentagem += modelViewRelatorioVendedor.data(row, "%").toDouble();
  }

  ui->doubleSpinBoxValorComissao->setValue(comissao);
  if (modelViewRelatorioVendedor.rowCount() > 0) { ui->doubleSpinBoxPorcentagemComissao->setValue(porcentagem / modelViewRelatorioVendedor.rowCount()); }
}

void WidgetRelatorio::setFilterRelatorio() {
  const QString date = ui->dateEditMes->date().toString("yyyy-MM");
  QString filter = "Mês = '" + date + "'";

  if (User::isVendedor()) { filter += " AND idUsuario = " + User::idUsuario; }

  if (User::isEspecial()) { filter += " AND idUsuarioConsultor = " + User::idUsuario; }

  if (User::isGerente() and not User::fromLoja("descricao").toString().isEmpty()) { filter += " AND Loja = '" + User::fromLoja("descricao").toString() + "'"; }

  filter += " ORDER BY Loja, Vendedor, idVenda";

  modelViewRelatorio.setFilter(filter);

  qDebug() << "filter_Pedidos: " << modelViewRelatorio.filter();

  modelViewRelatorio.select();
}

void WidgetRelatorio::dateEditMes_dateChanged(const QDate) { updateTables(); }

void WidgetRelatorio::updateTables() {
  if (not isSet) {
    ui->dateEditMes->setDate(qApp->serverDate());

    if (not User::isAdministrativo()) { ui->frameLoja->hide(); }

    if (User::isVendedor()) {
      ui->frameLoja->hide();
      ui->frameVendedores->hide();
    }

    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  QElapsedTimer time;
  time.start();
  setFilterRelatorio();
  qDebug() << "setFilterRelatorio: " << time.restart();
  setFilterTotaisVendedor();
  qDebug() << "setFilterTotaisVendedor: " << time.restart();
  setFilterTotaisLoja();
  qDebug() << "setFilterTotaisLoja: " << time.restart();
  calcularTotalGeral();
  qDebug() << "calcularTotalGeral: " << time.restart();
}

void WidgetRelatorio::resetTables() { modelIsSet = false; }

void WidgetRelatorio::on_pushButtonExcel_clicked() {
  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = QDir::currentPath() + "/modelos/relatorio.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do Excel!"); }

  const QString fileName = dir + "/relatorio-" + ui->dateEditMes->date().toString("MM-yyyy") + ".xlsx";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  gerarExcel(arquivoModelo, fileName);

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, this);
}

void WidgetRelatorio::gerarExcel(const QString &arquivoModelo, const QString &fileName) {
  QXlsx::Document xlsx(arquivoModelo, this);

  //  xlsx.currentWorksheet()->setFitToPage(true);
  //  xlsx.currentWorksheet()->setFitToHeight(true);
  //  xlsx.currentWorksheet()->setOrientationVertical(false);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelViewRelatorio.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelViewRelatorio.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelViewRelatorio.rowCount(); ++row) {
    for (int col = 0; col < modelViewRelatorio.columnCount(); ++col, ++column) {
      if (col == 5) { // ajustar data
        xlsx.write(column + QString::number(row + 2), QDate::fromString(modelViewRelatorio.data(row, col).toString(), "yyyy-MM").toString("MM-yyyy"));
      } else if (col == 6) { // ajustar data
        xlsx.write(column + QString::number(row + 2), modelViewRelatorio.data(row, col).toDate().toString("dd-MM-yyyy"));
      } else if (col == 9) { // ajustar porcentagem
        xlsx.write(column + QString::number(row + 2), modelViewRelatorio.data(row, col).toDouble() / 100);
      } else {
        xlsx.write(column + QString::number(row + 2), modelViewRelatorio.data(row, col));
      }
    }

    column = 'A';
  }

  xlsx.selectSheet("Sheet2");

  for (int col = 0; col < modelViewRelatorioVendedor.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelViewRelatorioVendedor.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelViewRelatorioVendedor.rowCount(); ++row) {
    for (int col = 0; col < modelViewRelatorioVendedor.columnCount(); ++col, ++column) {
      if (col == 6) { // ajustar porcentagem
        xlsx.write(column + QString::number(row + 2), modelViewRelatorioVendedor.data(row, col).toDouble() / 100);
      } else if (col == 7) { // ajustar data
        xlsx.write(column + QString::number(row + 2), QDate::fromString(modelViewRelatorioVendedor.data(row, col).toString(), "yyyy-MM").toString("MM-yyyy"));
      } else {
        xlsx.write(column + QString::number(row + 2), modelViewRelatorioVendedor.data(row, col));
      }
    }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o arquivo!"); }
}

// TODO: vendedor especial recebe 0,5% nas vendas como consultor
// TODO: reimplement views as materialized views? https://www.fromdual.com/mysql-materialized-views
