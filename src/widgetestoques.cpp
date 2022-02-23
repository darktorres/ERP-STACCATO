#include "widgetestoques.h"
#include "ui_widgetestoques.h"

#include "doubledelegate.h"
#include "estoque.h"
#include "file.h"
#include "followup.h"
#include "sql.h"
#include "user.h"
#include "xlsxdocument.h"

#include <QDesktopServices>
#include <QFileDialog>

WidgetEstoques::WidgetEstoques(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoques) { ui->setupUi(this); }

WidgetEstoques::~WidgetEstoques() { delete ui; }

void WidgetEstoques::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetEstoques::escolheFiltro, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetEstoques::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetEstoques::on_pushButtonRelatorio_clicked, connectionType);
  connect(ui->pushButtonRelatorioContabil, &QPushButton::clicked, this, &WidgetEstoques::on_pushButtonRelatorioContabil_clicked, connectionType);
  connect(ui->radioButtonEstoqueContabil, &QRadioButton::clicked, this, &WidgetEstoques::montaFiltroContabil, connectionType);
  connect(ui->radioButtonEstoqueZerado, &QRadioButton::clicked, this, &WidgetEstoques::montaFiltro, connectionType);
  connect(ui->radioButtonMaior, &QRadioButton::clicked, this, &WidgetEstoques::montaFiltro, connectionType);
  connect(ui->tableEstoque, &TableView::activated, this, &WidgetEstoques::on_table_activated, connectionType);
}

void WidgetEstoques::setupTables() {
  escolheFiltro();

  ui->tableEstoque->setModel(&model);

  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));
}

void WidgetEstoques::setHeaderData() {
  model.setHeaderData("cnpjDest", "CNPJ");
  model.setHeaderData("status", "Status");
  model.setHeaderData("idEstoque", "Estoque");
  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Produto");
  if (ui->radioButtonEstoqueContabil->isChecked()) { model.setHeaderData("caixasContabil", "Cx. Contábil"); }
  if (ui->radioButtonEstoqueContabil->isChecked()) { model.setHeaderData("contabil", "Contábil"); }
  model.setHeaderData("caixas", "Cx. Rest.");
  model.setHeaderData("restante", "Quant. Rest.");
  model.setHeaderData("unEst", "Un.");
  if (ui->radioButtonEstoqueContabil->isChecked()) { model.setHeaderData("unProd", "Un. Prod."); }
  model.setHeaderData("lote", "Lote");
  model.setHeaderData("local", "Local");
  model.setHeaderData("label", "Bloco");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("formComercial", "Form. Com.");
  model.setHeaderData("nfe", "NF-e");
  model.setHeaderData("dataPrevColeta", "Prev. Coleta");
  model.setHeaderData("dataRealColeta", "Coleta");
  model.setHeaderData("dataPrevReceb", "Prev. Receb.");
  model.setHeaderData("dataRealReceb", "Receb.");

  if (ui->radioButtonEstoqueContabil->isChecked()) {
    ui->tableEstoque->hideColumn("ncm");
    ui->tableEstoque->hideColumn("cstICMS");
    ui->tableEstoque->hideColumn("pICMS");
    ui->tableEstoque->hideColumn("cstIPI");
    ui->tableEstoque->hideColumn("cstPIS");
    ui->tableEstoque->hideColumn("cstCOFINS");
    ui->tableEstoque->hideColumn("custoUnit");
    ui->tableEstoque->hideColumn("precoVendaUnit");
    ui->tableEstoque->hideColumn("custo");
    ui->tableEstoque->hideColumn("precoVenda");
    ui->tableEstoque->hideColumn("idVenda");
  }
}

void WidgetEstoques::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEditMes->setDate(qApp->serverDate());

    if (User::isVendedorOrEspecial()) {
      ui->groupBoxFiltros->hide();
      ui->groupBoxRelatorio->hide();
    }

    setupTables();

    setConnections();
    isSet = true;
    return; // to avoid double selecting table
  }

  model.select();
}

void WidgetEstoques::resetTables() { setupTables(); }

void WidgetEstoques::on_table_activated(const QModelIndex &index) {
  const QString idEstoque = model.data(index, "idEstoque").toString();

  auto *estoque = new Estoque(idEstoque, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
  estoque->show();
}

void WidgetEstoques::escolheFiltro() { ui->radioButtonEstoqueContabil->isChecked() ? montaFiltroContabil() : montaFiltro(); }

void WidgetEstoques::montaFiltro() {
  const QString having = (ui->radioButtonMaior->isChecked()) ? "restante > 0" : "restante <= 0";

  model.setQuery(Sql::queryEstoque(getMatch(), having));

  model.select();

  setHeaderData();
}

void WidgetEstoques::montaFiltroContabil() {
  model.setQuery(Sql::view_estoque_contabil(getMatch()));

  model.select();

  setHeaderData();
}

QString WidgetEstoques::getMatch() const {
  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());

  if (textoBusca.isEmpty()) { return QString(); }

  QString filtroBusca =
      " AND (e.idEstoque LIKE '%" + textoBusca + "%' OR e.descricao LIKE '%" + textoBusca + "%' OR e.codComercial LIKE '%" + textoBusca + "%' OR p.fornecedor LIKE '%" + textoBusca + "%')";

  return filtroBusca;
}

void WidgetEstoques::on_pushButtonRelatorioContabil_clicked() {
  // 1. codigo produto
  // 2. descricao
  // 3. ncm
  // 4. unidade
  // 5. quantidade
  // 6. valor
  // 7. aliquota icms (se tiver)

  const QString data = ui->dateEditMes->date().toString("yyyy-MM-dd");

  gerarRelatorio(data);
}

void WidgetEstoques::gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) {
  QXlsx::Document xlsx(arquivoModelo, this);

  xlsx.selectSheet("Sheet1");

  const int columnCount = (User::isAdministrativo()) ? modelContabil.columnCount() : modelContabil.fieldIndex("codComercial") + 1;

  // write header
  for (int cell = 'A', column = 0; column < columnCount; ++cell, ++column) { xlsx.write(QString(cell) + QString::number(1), modelContabil.headerData(column, Qt::Horizontal).toString()); }

  // write data
  for (int row = 0, rowCount = modelContabil.rowCount(); row < rowCount; ++row) {
    for (int cell = 'A', column = 0; column < columnCount; ++cell, ++column) { xlsx.write(QString(cell) + QString::number(row + 2), modelContabil.data(row, column)); }
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o arquivo!"); }
}

void WidgetEstoques::on_pushButtonFollowup_clicked() {
  const auto selection = ui->tableEstoque->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!"); }

  const QString idEstoque = model.data(selection.first().row(), "idEstoque").toString();

  auto *followup = new FollowUp(idEstoque, FollowUp::Tipo::Estoque, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetEstoques::on_pushButtonRelatorio_clicked() {
  const QString data = QDate::currentDate().toString("yyyy-MM-dd");

  gerarRelatorio(data);
}

void WidgetEstoques::gerarRelatorio(const QString &data) {
  SqlQueryModel modelContabil;

  modelContabil.setQuery(Sql::view_estoque_contabil("", data));

  modelContabil.select();

  const QString dir = QFileDialog::getExistingDirectory(this, "Pasta para salvar relatório");

  if (dir.isEmpty()) { return; }

  const QString arquivoModelo = QDir::currentPath() + "/modelos/relatorio_contabil.xlsx";

  File modelo(arquivoModelo);

  if (not modelo.exists()) { throw RuntimeException("Não encontrou o modelo do Excel!"); }

  const QString fileName = dir + "/relatorio_contabil_" + data + ".xlsx";

  File file(fileName);

  if (not file.open(QFile::WriteOnly)) { throw RuntimeException("Não foi possível abrir o arquivo '" + fileName + "' para escrita: " + file.errorString()); }

  file.close();

  gerarExcel(arquivoModelo, fileName, modelContabil);

  QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
  qApp->enqueueInformation("Arquivo salvo como " + fileName, this);
}

// TODO: colocar botao para marcar quebra e remover consumos (colocar um followup em cada venda/consumo para ficar o historico de que foi removido o consumo devido uma quebra)
