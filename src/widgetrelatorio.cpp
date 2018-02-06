#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>

#include "porcentagemdelegate.h"
#include "reaisdelegate.h"
#include "ui_widgetrelatorio.h"
#include "usersession.h"
#include "widgetrelatorio.h"
#include "xlsxdocument.h"

WidgetRelatorio::WidgetRelatorio(QWidget *parent) : Widget(parent), ui(new Ui::WidgetRelatorio) {
  ui->setupUi(this);

  if (UserSession::tipoUsuario() == "VENDEDOR" or UserSession::tipoUsuario() == "VENDEDOR ESPECIAL") {
    ui->labelTotalLoja->hide();
    ui->tableTotalLoja->hide();
    ui->labelGeral->hide();
    ui->doubleSpinBoxGeral->hide();
    ui->groupBoxResumoOrcamento->hide();
  }

  ui->dateEditMes->setDate(QDate::currentDate());

  connect(ui->dateEditMes, &QDateEdit::dateChanged, this, &WidgetRelatorio::dateEditMes_dateChanged);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetRelatorio::on_pushButtonExcel_clicked);
  connect(ui->tableRelatorio, &TableView::entered, this, &WidgetRelatorio::on_tableRelatorio_entered);
  connect(ui->tableTotalLoja, &TableView::entered, this, &WidgetRelatorio::on_tableTotalLoja_entered);
  connect(ui->tableTotalVendedor, &TableView::entered, this, &WidgetRelatorio::on_tableTotalVendedor_entered);
}

WidgetRelatorio::~WidgetRelatorio() { delete ui; }

void WidgetRelatorio::setFilterTotaisVendedor() {
  QString filter = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  const QString tipoUsuario = UserSession::tipoUsuario();

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") filter += " AND idUsuario = " + QString::number(UserSession::idUsuario());

  if (tipoUsuario == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) filter += " AND Loja = '" + descricaoLoja.value().toString() + "'";
  }

  filter += " ORDER BY Loja, Vendedor";

  modelTotalVendedor.setFilter(filter);

  if (not modelTotalVendedor.select()) emit errorSignal("Erro lendo tabela relatorio_vendedor: " + modelTotalVendedor.lastError().text());
}

void WidgetRelatorio::setFilterTotaisLoja() {
  QString filter = "Mês = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'";

  if (UserSession::tipoUsuario() == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) filter += " AND Loja = '" + descricaoLoja.value().toString() + "'";
  }

  filter += " ORDER BY Loja";

  modelTotalLoja.setFilter(filter);

  if (not modelTotalLoja.select()) emit errorSignal("Erro lendo tabela relatorio_loja: " + modelTotalLoja.lastError().text());
}

bool WidgetRelatorio::setupTables() {
  // REFAC: refactor this to not select in here

  modelRelatorio.setTable("view_relatorio");
  modelRelatorio.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelRelatorio.setHeaderData("idVenda", "Venda");

  modelRelatorio.setFilter("0");

  if (not modelRelatorio.select()) {
    emit errorSignal("Erro lendo tabela relatorio: " + modelRelatorio.lastError().text());
    return false;
  }

  ui->tableRelatorio->setModel(&modelRelatorio);
  ui->tableRelatorio->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("Valor Comissão", new ReaisDelegate(this));
  ui->tableRelatorio->setItemDelegateForColumn("% Comissão", new PorcentagemDelegate(this));
  ui->tableRelatorio->hideColumn("Mês");
  ui->tableRelatorio->hideColumn("idUsuario");
  ui->tableRelatorio->resizeColumnsToContents();

  //------------------------------------------------------------------------------

  modelTotalVendedor.setTable("view_relatorio_vendedor");
  modelTotalVendedor.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTotalVendedor.setFilter("0");

  if (not modelTotalVendedor.select()) {
    emit errorSignal("Erro lendo view_relatorio_vendedor: " + modelTotalVendedor.lastError().text());
    return false;
  }

  ui->tableTotalVendedor->setModel(&modelTotalVendedor);
  ui->tableTotalVendedor->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("Valor Comissão", new ReaisDelegate(this));
  ui->tableTotalVendedor->setItemDelegateForColumn("% Comissão", new PorcentagemDelegate(this));
  ui->tableTotalVendedor->hideColumn("idUsuario");
  ui->tableTotalVendedor->hideColumn("Mês");
  ui->tableTotalVendedor->resizeColumnsToContents();

  //--------------------------------------------------

  modelTotalLoja.setTable("view_relatorio_loja");
  modelTotalLoja.setEditStrategy(QSqlTableModel::OnManualSubmit);

  modelTotalLoja.setFilter("0");

  if (not modelTotalLoja.select()) {
    emit errorSignal("Erro lendo view_relatorio_vendedor: " + modelTotalLoja.lastError().text());
    return false;
  }

  ui->tableTotalLoja->setModel(&modelTotalLoja);
  ui->tableTotalLoja->setItemDelegateForColumn("Faturamento", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Valor Comissão", new ReaisDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("% Comissão", new PorcentagemDelegate(this));
  ui->tableTotalLoja->setItemDelegateForColumn("Reposição", new ReaisDelegate(this));
  ui->tableTotalLoja->hideColumn("Mês");
  ui->tableTotalLoja->resizeColumnsToContents();

  return true;
}

void WidgetRelatorio::calcularTotalGeral() {
  double totalGeral = 0;
  double comissao = 0;
  double porcentagem = 0;

  for (int row = 0; row < modelTotalLoja.rowCount(); ++row) {
    totalGeral += modelTotalLoja.data(row, "Faturamento").toDouble();
    comissao += modelTotalLoja.data(row, "Valor Comissão").toDouble();
    porcentagem += modelTotalLoja.data(row, "% Comissão").toDouble();
  }

  ui->doubleSpinBoxGeral->setValue(totalGeral);
  ui->doubleSpinBoxValorComissao->setValue(comissao);
  ui->doubleSpinBoxPorcentagemComissao->setValue(porcentagem / modelTotalLoja.rowCount());
}

void WidgetRelatorio::setFilterRelatorio() {
  const QString date = ui->dateEditMes->date().toString("yyyy-MM");
  const QString tipoUsuario = UserSession::tipoUsuario();
  QString filter = "Mês = '" + date + "'";

  if (tipoUsuario == "VENDEDOR" or tipoUsuario == "VENDEDOR ESPECIAL") filter += " AND idUsuario = " + QString::number(UserSession::idUsuario());

  if (tipoUsuario == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) filter += " AND Loja = '" + descricaoLoja.value().toString() + "'";
  }

  filter += " ORDER BY Loja, Vendedor, idVenda";

  modelRelatorio.setFilter(filter);

  if (not modelRelatorio.select()) emit errorSignal("Erro lendo tabela relatorio: " + modelRelatorio.lastError().text());
}

void WidgetRelatorio::dateEditMes_dateChanged(const QDate &) { updateTables(); }

void WidgetRelatorio::on_tableRelatorio_entered(const QModelIndex &) { ui->tableRelatorio->resizeColumnsToContents(); }

bool WidgetRelatorio::updateTables() {
  if (modelRelatorio.tableName().isEmpty() and not setupTables()) return false;

  setFilterRelatorio();
  setFilterTotaisVendedor();
  setFilterTotaisLoja();

  ui->tableRelatorio->resizeColumnsToContents();
  ui->tableTotalVendedor->resizeColumnsToContents();
  ui->tableTotalLoja->resizeColumnsToContents();

  calcularTotalGeral();

  QSqlQuery query;
  // REFAC: use a join in the view so as to not need setting it manually (look into widgetfinanceirocontas)
  query.prepare("SET @mydate = :mydate");
  query.bindValue(":mydate", ui->dateEditMes->date().toString("yyyy-MM"));

  if (not query.exec()) {
    emit errorSignal("Erro setando mydate: " + query.lastError().text());
    return false;
  }

  if (not query.exec("SELECT @mydate") or not query.first()) {
    emit errorSignal("Erro comunicando com o banco de dados: " + query.lastError().text());
    return false;
  }

  modelOrcamento.setTable("view_resumo_relatorio");
  modelOrcamento.setEditStrategy(QSqlTableModel::OnManualSubmit);

  if (UserSession::tipoUsuario() == "GERENTE LOJA") {
    const auto descricaoLoja = UserSession::fromLoja("descricao");

    if (descricaoLoja) modelOrcamento.setFilter("Loja = '" + descricaoLoja.value().toString() + "' ORDER BY Loja, Vendedor");
  }

  if (not modelOrcamento.select()) {
    emit errorSignal("Erro lendo view_resumo_relatorio: " + modelOrcamento.lastError().text());
    return false;
  }

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

  ui->tableRelatorio->resizeColumnsToContents();
  ui->tableTotalVendedor->resizeColumnsToContents();
  ui->tableTotalLoja->resizeColumnsToContents();

  return true;
}

void WidgetRelatorio::on_tableTotalLoja_entered(const QModelIndex &) { ui->tableTotalLoja->resizeColumnsToContents(); }

void WidgetRelatorio::on_tableTotalVendedor_entered(const QModelIndex &) { ui->tableTotalVendedor->resizeColumnsToContents(); }

void WidgetRelatorio::on_pushButtonExcel_clicked() {
  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) return;

  const QString arquivoModelo = "relatorio.xlsx";

  QFile modelo(QDir::currentPath() + "/" + arquivoModelo);

  if (not modelo.exists()) {
    QMessageBox::critical(this, "Erro!", "Não encontrou o modelo do Excel!");
    return;
  }

  const QString fileName = dir + "/relatorio-" + ui->dateEditMes->date().toString("MM-yyyy") + ".xlsx";

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

  for (int col = 0; col < modelRelatorio.columnCount(); ++col, ++column) {
    xlsx.write(column + QString::number(1), modelRelatorio.headerData(col, Qt::Horizontal).toString());
  }

  column = 'A';

  for (int row = 0; row < modelRelatorio.rowCount(); ++row) {
    for (int col = 0; col < modelRelatorio.columnCount(); ++col, ++column) {
      if (col == 7) {
        xlsx.write(column + QString::number(row + 2), modelRelatorio.data(row, col).toDouble() / 100);
      } else {
        xlsx.write(column + QString::number(row + 2), modelRelatorio.data(row, col));
      }
    }

    column = 'A';
  }

  xlsx.selectSheet("Sheet2");

  for (int col = 0; col < modelTotalVendedor.columnCount(); ++col, ++column) {
    xlsx.write(column + QString::number(1), modelTotalVendedor.headerData(col, Qt::Horizontal).toString());
  }

  column = 'A';

  for (int row = 0; row < modelTotalVendedor.rowCount(); ++row) {
    for (int col = 0; col < modelTotalVendedor.columnCount(); ++col, ++column) {
      if (col == 5) {
        xlsx.write(column + QString::number(row + 2), modelTotalVendedor.data(row, col).toDouble() / 100);
      } else {
        xlsx.write(column + QString::number(row + 2), modelTotalVendedor.data(row, col));
      }
    }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) {
    QMessageBox::critical(this, "Erro!", "Ocorreu algum erro ao salvar o arquivo.");
    return;
  }

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  QMessageBox::information(this, "Ok!", "Arquivo salvo como " + fileName);
}
