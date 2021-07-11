#include "widgetestoques.h"
#include "ui_widgetestoques.h"

#include "doubledelegate.h"
#include "estoque.h"
#include "file.h"
#include "sql.h"
#include "user.h"
#include "xlsxdocument.h"

#include <QDesktopServices>
#include <QFileDialog>

WidgetEstoques::WidgetEstoques(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoques) { ui->setupUi(this); }

WidgetEstoques::~WidgetEstoques() { delete ui; }

void WidgetEstoques::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetEstoques::escolheFiltro, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetEstoques::delayFiltro, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetEstoques::on_pushButtonRelatorio_clicked, connectionType);
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
  model.setHeaderData("caixas", "Cx Rest.");
  model.setHeaderData("restante", "Quant. Rest.");
  model.setHeaderData("unEst", "Un.");
  if (ui->radioButtonEstoqueContabil->isChecked()) { model.setHeaderData("unProd", "Un. Prod."); }
  model.setHeaderData("lote", "Lote");
  model.setHeaderData("local", "Local");
  model.setHeaderData("bloco", "Bloco");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("nfe", "NFe");
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
    timer.setSingleShot(true);
    ui->dateEditMes->setDate(qApp->serverDate());
    setConnections();

    if (User::isVendedorOrEspecial()) {
      ui->groupBoxFiltros->hide();
      ui->groupBoxRelatorio->hide();
    }

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  model.select();
}

void WidgetEstoques::delayFiltro() { timer.start(500); }

void WidgetEstoques::resetTables() { modelIsSet = false; }

void WidgetEstoques::on_table_activated(const QModelIndex &index) {
  const QString idEstoque = model.data(index, "idEstoque").toString();

  auto *estoque = new Estoque(idEstoque, true, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
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

  const QString filtroBusca =
      " AND (e.idEstoque LIKE '%" + textoBusca + "%' OR e.descricao LIKE '%" + textoBusca + "%' OR e.codComercial LIKE '%" + textoBusca + "%' OR p.fornecedor LIKE '%" + textoBusca + "%')";

  return filtroBusca;
}

void WidgetEstoques::on_pushButtonRelatorio_clicked() {
  // 1. codigo produto
  // 2. descricao
  // 3. ncm
  // 4. unidade
  // 5. quantidade
  // 6. valor
  // 7. aliquota icms (se tiver)

  const QString data = ui->dateEditMes->date().toString("yyyy-MM-dd");

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

void WidgetEstoques::gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) {
  QXlsx::Document xlsx(arquivoModelo, this);

  xlsx.selectSheet("Sheet1");

  char column = 'A';

  for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(1), modelContabil.headerData(col, Qt::Horizontal).toString()); }

  column = 'A';

  for (int row = 0; row < modelContabil.rowCount(); ++row) {
    for (int col = 0; col < modelContabil.columnCount(); ++col, ++column) { xlsx.write(column + QString::number(row + 2), modelContabil.data(row, col)); }

    column = 'A';
  }

  if (not xlsx.saveAs(fileName)) { throw RuntimeException("Ocorreu algum erro ao salvar o arquivo!"); }
}
