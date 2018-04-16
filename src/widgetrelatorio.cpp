#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QSqlError>

#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "ui_widgetrelatorio.h"
#include "usersession.h"
#include "widgetrelatorio.h"
#include "xlsxdocument.h"

WidgetRelatorio::WidgetRelatorio(QWidget *parent) : Widget(parent), ui(new Ui::WidgetRelatorio) { ui->setupUi(this); }

WidgetRelatorio::~WidgetRelatorio() { delete ui; }

void WidgetRelatorio::setConnections() {
  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetRelatorio::dateEditMes_dateChanged);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetRelatorio::on_pushButtonExcel_clicked);
  connect(ui->tableRelatorio, &TableView::entered, this, &WidgetRelatorio::on_tableRelatorio_entered);
  connect(ui->tableTotalLoja, &TableView::entered, this, &WidgetRelatorio::on_tableTotalLoja_entered);
  connect(ui->tableTotalVendedor, &TableView::entered, this, &WidgetRelatorio::on_tableTotalVendedor_entered);
}

void WidgetRelatorio::setFilterTotaisVendedor() {
  QString filter = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") { filter += " AND idUsuario = " + QString::number(UserSession::idUsuario()); }

  if (tipoUsuario == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) { filter += " AND Loja = '" + descricaoLoja.value().toString() + "'"; }
  }

  filter += " ORDER BY Loja, Vendedor";

  modelViewRelatorioVendedor.setFilter(filter);

  if (not modelViewRelatorioVendedor.select()) { return; }

  ui->tableTotalVendedor->resizeColumnsToContents();
}

void WidgetRelatorio::setFilterTotaisLoja() {
  QString filter = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  if (UserSession::tipoUsuario() == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) { filter += " AND Loja = '" + descricaoLoja.value().toString() + "'"; }
  }

  filter += " ORDER BY Loja";

  modelViewRelatorioLoja.setFilter(filter);

  if (not modelViewRelatorioLoja.select()) { return; }

  ui->tableTotalLoja->resizeColumnsToContents();
}

void WidgetRelatorio::setupTables() {
  modelViewRelatorio.setTable("view_relatorio");
  modelViewRelatorio.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelViewRelatorio.setHeaderData("idVenda", "Venda");

  ui->tableRelatorio->setModel(&modelViewRelatorio);
  ui->tableRelatorio->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("Valor Comissão", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("% Comissão", new PorcentagemDelegate(this));
  ui->tableRelatorio->hideColumn("Mês");
  ui->tableRelatorio->hideColumn("idUsuario");
  ui->tableRelatorio->resizeColumnsToContents();

  // -------------------------------------------------------------------------

  modelViewRelatorioVendedor.setTable("view_relatorio_vendedor");
  modelViewRelatorioVendedor.setEditStrategy(QSqlTableModel::OnManualSubmit);

  ui->tableTotalVendedor->setModel(&modelViewRelatorioVendedor);
  ui->tableTotalVendedor->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("Valor Comissão", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("% Comissão", new PorcentagemDelegate(this));
  ui->tableTotalVendedor->hideColumn("idUsuario");
  ui->tableTotalVendedor->hideColumn("Mês");
  ui->tableTotalVendedor->resizeColumnsToContents();

  // -------------------------------------------------------------------------

  modelViewRelatorioLoja.setTable("view_relatorio_loja");
  modelViewRelatorioLoja.setEditStrategy(QSqlTableModel::OnManualSubmit);

  ui->tableTotalLoja->setModel(&modelViewRelatorioLoja);
  ui->tableTotalLoja->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Valor Comissão", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("% Comissão", new PorcentagemDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Reposição", new ReaisDelegate(this));
  ui->tableTotalLoja->hideColumn("Mês");
  ui->tableTotalLoja->resizeColumnsToContents();
}

void WidgetRelatorio::calcularTotalGeral() {
  double totalGeral = 0;
  double comissao = 0;
  double porcentagem = 0;

  for (int row = 0; row < modelViewRelatorioLoja.rowCount(); ++row) {
    totalGeral += modelViewRelatorioLoja.data(row, "Faturamento").toDouble();
    comissao += modelViewRelatorioLoja.data(row, "Valor Comissão").toDouble();
    porcentagem += modelViewRelatorioLoja.data(row, "% Comissão").toDouble();
  }

  ui->doubleSpinBoxGeral->setValue(totalGeral);
  ui->doubleSpinBoxValorComissao->setValue(comissao);
  ui->doubleSpinBoxPorcentagemComissao->setValue(porcentagem / modelViewRelatorioLoja.rowCount());
}

void WidgetRelatorio::setFilterRelatorio() {
  const QString date = ui->dateEditMes->date().toString("yyyy-MM");
  const QString tipoUsuario = UserSession::tipoUsuario();
  QString filter = "Mês = '" + date + "'";

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") { filter += " AND idUsuario = " + QString::number(UserSession::idUsuario()); }

  if (tipoUsuario == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) { filter += " AND Loja = '" + descricaoLoja.value().toString() + "'"; }
  }

  filter += " ORDER BY Loja, Vendedor, idVenda";

  modelViewRelatorio.setFilter(filter);

  if (not modelViewRelatorio.select()) { return; }

  ui->tableRelatorio->resizeColumnsToContents();
}

void WidgetRelatorio::dateEditMes_dateChanged(const QDate &) { updateTables(); }

void WidgetRelatorio::on_tableRelatorio_entered(const QModelIndex &) { ui->tableRelatorio->resizeColumnsToContents(); }

void WidgetRelatorio::updateTables() {
  if (not isSet) {
    if (UserSession::tipoUsuario() == "VENDEDOR" or UserSession::tipoUsuario() == "VENDEDOR ESPECIAL") {
      ui->labelTotalLoja->hide();
      ui->tableTotalLoja->hide();
      ui->labelGeral->hide();
      ui->doubleSpinBoxGeral->hide();
      ui->groupBoxResumoOrcamento->hide();
    }

    ui->dateEditMes->setDate(QDate::currentDate());
    setConnections();
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  setFilterRelatorio();
  setFilterTotaisVendedor();
  setFilterTotaisLoja();

  calcularTotalGeral();

  setResumoOrcamento();
}

void WidgetRelatorio::setResumoOrcamento() {
  QSqlQuery query;

  if (not query.exec("SET @mydate = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'")) {
    emit errorSignal("Erro comunicando com o banco de dados: " + query.lastError().text());
    return;
  }

  modelOrcamento.setTable("view_resumo_relatorio");
  modelOrcamento.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (UserSession::tipoUsuario() == "GERENTE LOJA") {
    if (const auto descricaoLoja = UserSession::fromLoja("descricao"); descricaoLoja) { modelOrcamento.setFilter("Loja = '" + descricaoLoja.value().toString() + "' ORDER BY Loja, Vendedor"); }
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
  ui->tableResumoOrcamento->resizeColumnsToContents();
}

void WidgetRelatorio::resetTables() { modelIsSet = false; }

void WidgetRelatorio::on_tableTotalLoja_entered(const QModelIndex &) { ui->tableTotalLoja->resizeColumnsToContents(); }

void WidgetRelatorio::on_tableTotalVendedor_entered(const QModelIndex &) { ui->tableTotalVendedor->resizeColumnsToContents(); }

void WidgetRelatorio::on_pushButtonExcel_clicked() {
  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = "relatorio.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    emit errorSignal("Não encontrou o modelo do Excel!");
    return;
  }

  const QString fileName = dir + "/relatorio-" + ui->dateEditMes->date().toString("MM-yyyy") + ".xlsx";

  QFile file(fileName);

  if (not file.open(QFile::WriteOnly)) {
    emit errorSignal("Não foi possível abrir o arquivo para escrita: " + fileName);
    emit errorSignal("Erro: " + file.errorString());
    return;
  }

  file.close();

  if (not gerarExcel(arquivoModelo, fileName)) { return; }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  emit informationSignal("Arquivo salvo como " + fileName);
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

  if (not xlsx.saveAs(fileName)) {
    emit errorSignal("Ocorreu algum erro ao salvar o arquivo.");
    return false;
  }

  return true;
}
