#include <QDebug>
#include <QDesktopServices>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QSqlError>

#include "application.h"
#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "ui_widgetrelatorio.h"
#include "usersession.h"
#include "widgetrelatorio.h"
#include "xlsxdocument.h"

WidgetRelatorio::WidgetRelatorio(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetRelatorio) { ui->setupUi(this); }

WidgetRelatorio::~WidgetRelatorio() { delete ui; }

void WidgetRelatorio::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetRelatorio::dateEditMes_dateChanged, connectionType);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetRelatorio::on_pushButtonExcel_clicked, connectionType);
}

void WidgetRelatorio::setFilterTotaisVendedor() {
  QString filter = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") { filter += " AND idUsuario = " + QString::number(UserSession::idUsuario()); }

  if (tipoUsuario == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) { filter += " AND Loja = '" + descricaoLoja->toString() + "'"; }
  }

  filter += " ORDER BY Loja, Vendedor";

  modelViewRelatorioVendedor.setFilter(filter);

  qDebug() << "filter2: " << modelViewRelatorioVendedor.filter();

  if (not modelViewRelatorioVendedor.select()) { return; }
}

void WidgetRelatorio::setFilterTotaisLoja() {
  QString filter = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  if (UserSession::tipoUsuario() == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) { filter += " AND Loja = '" + descricaoLoja->toString() + "'"; }
  }

  filter += " ORDER BY Loja";

  modelViewRelatorioLoja.setFilter(filter);

  if (not modelViewRelatorioLoja.select()) { return; }
}

void WidgetRelatorio::setupTables() {
  modelViewRelatorio.setTable("view_relatorio");

  modelViewRelatorio.setHeaderData("idVenda", "Venda");

  ui->tableRelatorio->setModel(&modelViewRelatorio);

  ui->tableRelatorio->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("%", new PorcentagemDelegate(this));

  ui->tableRelatorio->hideColumn("Mês");
  ui->tableRelatorio->hideColumn("idUsuario");

  // -------------------------------------------------------------------------

  modelViewRelatorioVendedor.setTable("view_relatorio_vendedor");

  ui->tableTotalVendedor->setModel(&modelViewRelatorioVendedor);

  ui->tableTotalVendedor->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("%", new PorcentagemDelegate(this));

  ui->tableTotalVendedor->hideColumn("idUsuario");
  ui->tableTotalVendedor->hideColumn("Mês");

  // -------------------------------------------------------------------------

  modelViewRelatorioLoja.setTable("view_relatorio_loja");

  ui->tableTotalLoja->setModel(&modelViewRelatorioLoja);

  ui->tableTotalLoja->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Comissão", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("%", new PorcentagemDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Reposição", new ReaisDelegate(this));

  ui->tableTotalLoja->hideColumn("Mês");
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
  const QString tipoUsuario = UserSession::tipoUsuario();
  QString filter = "Mês = '" + date + "'";

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") { filter += " AND idUsuario = " + QString::number(UserSession::idUsuario()); }

  if (tipoUsuario == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) { filter += " AND Loja = '" + descricaoLoja->toString() + "'"; }
  }

  filter += " ORDER BY Loja, Vendedor, idVenda";

  modelViewRelatorio.setFilter(filter);

  qDebug() << "filter1: " << modelViewRelatorio.filter();

  if (not modelViewRelatorio.select()) { return; }
}

void WidgetRelatorio::dateEditMes_dateChanged(const QDate &) { updateTables(); }

void WidgetRelatorio::updateTables() {
  const auto tipo = UserSession::tipoUsuario();

  if (not isSet) {
    if (tipo == "VENDEDOR" or tipo == "VENDEDOR ESPECIAL") {
      ui->labelTotalLoja->hide();
      ui->tableTotalLoja->hide();
      ui->doubleSpinBoxGeral->hide();
      ui->groupBoxResumoOrcamento->hide();
    }

    ui->dateEditMes->setDate(qApp->serverDate());
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  if (tipo == "VENDEDOR" or tipo == "VENDEDOR ESPECIAL") {
    setFilterRelatorio();
    setFilterTotaisVendedor();

    calcularTotalVendedor();
  } else {
    QElapsedTimer time;
    time.start();
    setFilterRelatorio();
    qDebug() << "1: " << time.restart();
    setFilterTotaisVendedor();
    qDebug() << "2: " << time.restart();
    setFilterTotaisLoja();
    qDebug() << "3: " << time.restart();
    calcularTotalGeral();
    qDebug() << "4: " << time.restart();
    setResumoOrcamento();
    qDebug() << "5: " << time.restart();
  }
}

void WidgetRelatorio::setResumoOrcamento() {
  QSqlQuery query;

  if (not query.exec("SET @mydate = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'")) { return qApp->enqueueError("Erro comunicando com o banco de dados: " + query.lastError().text(), this); }

  modelOrcamento.setTable("view_resumo_relatorio");

  modelOrcamento.setFilter("");

  if (UserSession::tipoUsuario() == "GERENTE LOJA") {
    if (const auto descricaoLoja = UserSession::fromLoja("descricao"); descricaoLoja) { modelOrcamento.setFilter("Loja = '" + descricaoLoja->toString() + "' ORDER BY Loja, Vendedor"); }
  }

  if (not modelOrcamento.select()) { return; }

  ui->tableResumoOrcamento->setModel(&modelOrcamento);

  ui->tableResumoOrcamento->setItemDelegateForColumn("Validos Anteriores", new ReaisDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("Gerados Mes", new ReaisDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("Revalidados Mes", new ReaisDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("Fechados Mes", new ReaisDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("Perdidos Mes", new ReaisDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("Validos Mes", new ReaisDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("% Fechados / Gerados", new PorcentagemDelegate(this));
  ui->tableResumoOrcamento->setItemDelegateForColumn("% Fechados / Carteira", new PorcentagemDelegate(this));
}

void WidgetRelatorio::resetTables() { modelIsSet = false; }

void WidgetRelatorio::on_pushButtonExcel_clicked() {
  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = "relatorio.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) { return qApp->enqueueError("Não encontrou o modelo do Excel!", this); }

  const QString fileName = dir + "/relatorio-" + ui->dateEditMes->date().toString("MM-yyyy") + ".xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) { return qApp->enqueueError("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString(), this); }

  file.close();

  if (not gerarExcel(arquivoModelo, fileName)) { return; }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, this);
}

bool WidgetRelatorio::gerarExcel(const QString &arquivoModelo, const QString &fileName) {
  QXlsx::Document xlsx(arquivoModelo);

  //  xlsx.currentWorksheet()->setFitToPage(true);
  //  xlsx.currentWorksheet()->setFitToHeight(true);
  //  xlsx.currentWorksheet()->setOrientationVertical(false);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelViewRelatorio.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelViewRelatorio.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelViewRelatorio.rowCount(); ++row) {
    for (int col = 0; col < modelViewRelatorio.columnCount(); ++col, ++column) {
      if (col == 7) {
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
      if (col == 5) {
        xlsx.write(column + QString::number(row + 2), modelViewRelatorioVendedor.data(row, col).toDouble() / 100);
      } else {
        xlsx.write(column + QString::number(row + 2), modelViewRelatorioVendedor.data(row, col));
      }
    }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) { return qApp->enqueueError(false, "Ocorreu algum erro ao salvar o arquivo.", this); }

  return true;
}

// TODO: vendedor especial recebe 0,5% nas vendas como consultor
// TODO: reimplement views as materialized views? https://www.fromdual.com/mysql-materialized-views
