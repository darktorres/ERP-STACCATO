#include "widgetestoque.h"
#include "ui_widgetestoque.h"

#include "application.h"
#include "doubledelegate.h"
#include "estoque.h"
#include "file.h"
#include "searchdialogproxymodel.h"
#include "sortfilterproxymodel.h"
#include "sql.h"
#include "user.h"
#include "xlsxdocument.h"

#include <QDebug>
#include <QDesktopServices>
#include <QFileDialog>
#include <QSqlError>

WidgetEstoque::WidgetEstoque(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoque) {
  ui->setupUi(this);
  timer.setSingleShot(true);
  timer2.setSingleShot(true);
}

WidgetEstoque::~WidgetEstoque() { delete ui; }

void WidgetEstoque::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetEstoque::escolheFiltro, connectionType);
  connect(&timer2, &QTimer::timeout, this, &WidgetEstoque::montaFiltro2, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetEstoque::delayFiltro, connectionType);
  connect(ui->lineEditBusca2, &QLineEdit::textChanged, this, &WidgetEstoque::delayFiltro2, connectionType);
  connect(ui->pushButtonRelatorio, &QPushButton::clicked, this, &WidgetEstoque::on_pushButtonRelatorio_clicked, connectionType);
  connect(ui->radioButtonEstoque, &QRadioButton::toggled, this, &WidgetEstoque::on_radioButtonEstoque_toggled, connectionType);
  connect(ui->radioButtonEstoqueContabil, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltroContabil, connectionType);
  connect(ui->radioButtonEstoqueZerado, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltro, connectionType);
  connect(ui->radioButtonMaior, &QRadioButton::clicked, this, &WidgetEstoque::montaFiltro, connectionType);
  connect(ui->radioButtonStaccatoOFF, &QRadioButton::toggled, this, &WidgetEstoque::on_radioButtonStaccatoOFF_toggled, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetEstoque::on_radioButtonTodos_toggled, connectionType);
  connect(ui->tableEstoque, &TableView::activated, this, &WidgetEstoque::on_table_activated, connectionType);
}

void WidgetEstoque::setupTables() {
  escolheFiltro();

  model.proxyModel = new SortFilterProxyModel(&model, this);

  ui->tableEstoque->setModel(&model);

  ui->tableEstoque->setItemDelegate(new DoubleDelegate(this));

  //----------------------------------------------

  modelProdutos.setTable("view_produto");

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.proxyModel = new SearchDialogProxyModel(&modelProdutos, this);

  modelProdutos.select();

  modelProdutos.setHeaderData("fornecedor", "Fornecedor");
  modelProdutos.setHeaderData("statusEstoque", "Estoque");
  modelProdutos.setHeaderData("descricao", "Descrição");
  modelProdutos.setHeaderData("estoqueRestante", "Estoque Disp.");
  modelProdutos.setHeaderData("estoqueCaixa", "Estoque Cx.");
  modelProdutos.setHeaderData("lote", "Lote");
  modelProdutos.setHeaderData("un", "Un.");
  modelProdutos.setHeaderData("un2", "Un.2");
  modelProdutos.setHeaderData("colecao", "Coleção");
  modelProdutos.setHeaderData("tipo", "Tipo");
  modelProdutos.setHeaderData("minimo", "Mínimo");
  modelProdutos.setHeaderData("multiplo", "Múltiplo");
  modelProdutos.setHeaderData("m2cx", "M/Cx.");
  modelProdutos.setHeaderData("pccx", "Pç./Cx.");
  modelProdutos.setHeaderData("kgcx", "Kg./Cx.");
  modelProdutos.setHeaderData("formComercial", "Form. Com.");
  modelProdutos.setHeaderData("codComercial", "Cód. Com.");
  modelProdutos.setHeaderData("precoVenda", "R$");
  modelProdutos.setHeaderData("validade", "Validade");
  modelProdutos.setHeaderData("ui", "UI");

  ui->tableProdutos->setModel(&modelProdutos);

  ui->tableProdutos->hideColumn("idProduto");
  ui->tableProdutos->hideColumn("estoque");
  ui->tableProdutos->hideColumn("promocao");
  ui->tableProdutos->hideColumn("descontinuado");
  ui->tableProdutos->hideColumn("desativado");
  ui->tableProdutos->hideColumn("representacao");
}

void WidgetEstoque::setHeaderData() {
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

void WidgetEstoque::updateTables() {
  if (not isSet) {
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

  const QString currentTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currentTab == "Estoques") { model.select(); }

  if (currentTab == "Produtos") { modelProdutos.select(); }
}

void WidgetEstoque::delayFiltro() { timer.start(500); }

void WidgetEstoque::delayFiltro2() { timer2.start(500); }

void WidgetEstoque::resetTables() { modelIsSet = false; }

void WidgetEstoque::on_table_activated(const QModelIndex &index) {
  const QString idEstoque = model.data(index, "idEstoque").toString();

  auto *estoque = new Estoque(idEstoque, true, this);
  estoque->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetEstoque::escolheFiltro() { ui->radioButtonEstoqueContabil->isChecked() ? montaFiltroContabil() : montaFiltro(); }

void WidgetEstoque::montaFiltro() {
  const QString having = (ui->radioButtonMaior->isChecked()) ? "restante > 0" : "restante <= 0";

  model.setQuery(Sql::queryEstoque(getMatch(), having));

  model.select();

  setHeaderData();
}

void WidgetEstoque::montaFiltro2() {
  const QString text = ui->lineEditBusca2->text();

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE AND (fornecedor LIKE '%" + text + "%' OR descricao LIKE '%" + text + "%' OR codComercial LIKE '%" + text +
                          "%')");
}

void WidgetEstoque::montaFiltroContabil() {
  model.setQuery(Sql::view_estoque_contabil(getMatch()));

  model.select();

  setHeaderData();
}

QString WidgetEstoque::getMatch() const {
  const QString text = qApp->sanitizeSQL(ui->lineEditBusca->text());

  if (text.isEmpty()) { return QString(); }

  QStringList strings = text.split(" ", Qt::SkipEmptyParts);

  for (auto &string : strings) { string.contains("-") ? string.prepend("\"").append("\"") : string.prepend("+").append("*"); }

  const QString text2 = strings.join(" ");

  const QString match =
      " AND (MATCH (e.descricao , e.codComercial) AGAINST ('" + text2 + "' IN BOOLEAN MODE) OR MATCH (p.fornecedor) AGAINST ('" + text2 + "' IN BOOLEAN MODE) OR e.idEstoque LIKE '" + text + "%')";

  return match;
}

void WidgetEstoque::on_pushButtonRelatorio_clicked() {
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

void WidgetEstoque::gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) {
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

void WidgetEstoque::on_radioButtonTodos_toggled(bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

void WidgetEstoque::on_radioButtonStaccatoOFF_toggled(bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND promocao = 2 AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

void WidgetEstoque::on_radioButtonEstoque_toggled(bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND promocao = 0 AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

// NOTE: gerenciar lugares de estoque (cadastro/permissoes)
// TODO: 3tem produto com unidade barra que na verdade significa ML

// TODO: 5colocar um filtro para mostrar os cancelados/quebrados?
// TODO: 2poder trocar bloco do estoque
// TODO: -1verificar se o custo do pedido_fornecedor bate com os valores do estoque/consumo
// TODO: terminar de arrumar relatorio estoque
// TODO: [Conrado] colocar filtro/tela para buscar por pedido e mostrar os estoques em que foi consumido
// TODO: fix fulltext indexes (put match against inside subquery)
// TODO: criar um segundo relatorio para os gerentes sem o custo
// TODO: dividir os 2 widgets em classes separadas
// TODO: ordenar estoque zerado por status congela o sistema
