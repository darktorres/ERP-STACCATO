#include "widgetcompraavulsa.h"
#include "ui_widgetcompraavulsa.h"

#include "comboboxdelegate.h"
#include "compraavulsa.h"
#include "reaisdelegate.h"

WidgetCompraAvulsa::WidgetCompraAvulsa(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraAvulsa) { ui->setupUi(this); }

WidgetCompraAvulsa::~WidgetCompraAvulsa() { delete ui; }

void WidgetCompraAvulsa::setupTables() {
  modelCompra.setTable("compra_avulsa");

  modelCompra.setEditStrategy(QSqlTableModel::OnFieldChange);

  modelCompra.setFilter("");

  modelCompra.setHeaderData("status", "Status");
  modelCompra.setHeaderData("idCompra", "O.C.");
  modelCompra.setHeaderData("fornecedor", "Fornecedor");
  modelCompra.setHeaderData("descricao", "Produto");
  modelCompra.setHeaderData("obs", "Obs.");
  modelCompra.setHeaderData("codComercial", "Cód. Com.");
  modelCompra.setHeaderData("quant", "Quant.");
  modelCompra.setHeaderData("un", "Un.");
  modelCompra.setHeaderData("prcUnitario", "R$ Unit.");
  modelCompra.setHeaderData("preco", "R$");
  modelCompra.setHeaderData("dataRealCompra", "Data");

  ui->tableCompra->setModel(&modelCompra);

  ui->tableCompra->setItemDelegateForColumn("status", new ComboBoxDelegate(ComboBoxDelegate::Tipo::CompraAvulsa, this));
  ui->tableCompra->setItemDelegateForColumn("prcUnitario", new ReaisDelegate(this));
  ui->tableCompra->setItemDelegateForColumn("preco", new ReaisDelegate(this));

  ui->tableCompra->hideColumn("idPedido1");
  ui->tableCompra->hideColumn("idRelacionado");
  ui->tableCompra->hideColumn("idFollowup");
  ui->tableCompra->hideColumn("selecionado");
  ui->tableCompra->hideColumn("aliquotaSt");
  ui->tableCompra->hideColumn("st");
  ui->tableCompra->hideColumn("statusFinanceiro");
  ui->tableCompra->hideColumn("ordemCompra");
  ui->tableCompra->hideColumn("ordemRepresentacao");
  ui->tableCompra->hideColumn("codFornecedor");
  ui->tableCompra->hideColumn("idVenda");
  ui->tableCompra->hideColumn("idVendaProduto1");
  ui->tableCompra->hideColumn("idVendaProduto2");
  ui->tableCompra->hideColumn("idProduto");
  ui->tableCompra->hideColumn("colecao");
  ui->tableCompra->hideColumn("quantUpd");
  ui->tableCompra->hideColumn("un2");
  ui->tableCompra->hideColumn("caixas");
  ui->tableCompra->hideColumn("desconto");
  ui->tableCompra->hideColumn("kgcx");
  ui->tableCompra->hideColumn("formComercial");
  ui->tableCompra->hideColumn("codBarras");
  ui->tableCompra->hideColumn("dataPrevCompra");
  ui->tableCompra->hideColumn("dataPrevConf");
  ui->tableCompra->hideColumn("dataRealConf");
  ui->tableCompra->hideColumn("dataPrevFat");
  ui->tableCompra->hideColumn("dataRealFat");
  ui->tableCompra->hideColumn("dataPrevColeta");
  ui->tableCompra->hideColumn("dataRealColeta");
  ui->tableCompra->hideColumn("dataPrevReceb");
  ui->tableCompra->hideColumn("dataRealReceb");
  ui->tableCompra->hideColumn("dataPrevEnt");
  ui->tableCompra->hideColumn("dataRealEnt");
  ui->tableCompra->hideColumn("created");
  ui->tableCompra->hideColumn("lastUpdated");
}

void WidgetCompraAvulsa::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxProdutoCancelado, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxProdutoComprado, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxProdutoConferido, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->checkBoxProdutoPendente, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro, connectionType);
  connect(ui->groupBoxProdutos, &QGroupBox::toggled, this, &WidgetCompraAvulsa::on_groupBoxProdutos_toggled, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &WidgetCompraAvulsa::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->tableCompra, &QTableView::doubleClicked, this, &WidgetCompraAvulsa::on_tableCompra_doubleClicked, connectionType);
}

void WidgetCompraAvulsa::unsetConnections() {
  disconnect(ui->checkBoxProdutoCancelado, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
  disconnect(ui->checkBoxProdutoComprado, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
  disconnect(ui->checkBoxProdutoConferido, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
  disconnect(ui->checkBoxProdutoPendente, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
  disconnect(ui->groupBoxProdutos, &QGroupBox::toggled, this, &WidgetCompraAvulsa::on_groupBoxProdutos_toggled);
  disconnect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &WidgetCompraAvulsa::on_pushButtonCadastrar_clicked);
  disconnect(ui->tableCompra, &QTableView::doubleClicked, this, &WidgetCompraAvulsa::on_tableCompra_doubleClicked);
}

void WidgetCompraAvulsa::on_groupBoxProdutos_toggled(const bool enabled) {
  const auto container = ui->groupBoxProdutos->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : container) {
    disconnect(child, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
    child->setEnabled(true);
    child->setChecked(enabled);
    connect(child, &QCheckBox::toggled, this, &WidgetCompraAvulsa::montaFiltro);
  }

  montaFiltro();
}

void WidgetCompraAvulsa::montaFiltro() {
  QStringList filtros;
  QStringList filtroCheck;

  const auto children = ui->groupBoxProdutos->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  modelCompra.setFilter(filtros.join(" AND "));
}

void WidgetCompraAvulsa::updateTables() {
  if (not isSet) {
    setupTables();
    setConnections();
    montaFiltro();
    isSet = true;
  }

  modelCompra.select();
}

void WidgetCompraAvulsa::resetTables() {
  setupTables();
}

void WidgetCompraAvulsa::on_tableCompra_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  auto *compraAvulsa = new CompraAvulsa(this);
  compraAvulsa->setAttribute(Qt::WA_DeleteOnClose);
  compraAvulsa->viewRegisterById(modelCompra.data(index.row(), "idCompra").toString());

  compraAvulsa->show();
}

void WidgetCompraAvulsa::on_pushButtonCadastrar_clicked() {
  auto *compraAvulsa = new CompraAvulsa(this);
  compraAvulsa->setAttribute(Qt::WA_DeleteOnClose);

  compraAvulsa->show();
}
