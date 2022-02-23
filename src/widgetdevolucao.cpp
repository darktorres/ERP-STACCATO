#include "widgetdevolucao.h"
#include "ui_widgetdevolucao.h"

#include "application.h"
#include "cadastrarnfe.h"

WidgetDevolucao::WidgetDevolucao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetDevolucao) { ui->setupUi(this); }

WidgetDevolucao::~WidgetDevolucao() { delete ui; }

void WidgetDevolucao::resetTables() { setupTables(); }

void WidgetDevolucao::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    ui->dateEditMes->setDate(qApp->serverDate());

    setupTables();

    setConnections();
    isSet = true;
  }

  model.select();
}

void WidgetDevolucao::montaFiltro() {
  ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

  const QString filtroMes = ui->groupBoxMes->isChecked() ? "DATE_FORMAT(Data, '%Y-%m') = '" + ui->dateEditMes->date().toString("yyyy-MM") + "'" : "";
  if (not filtroMes.isEmpty()) { filtros << filtroMes; }

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(idVenda LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  model.setFilter(filtros.join(" AND "));
}

void WidgetDevolucao::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetDevolucao::montaFiltro, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetDevolucao::montaFiltro, connectionType);
  connect(ui->pushButtonGerarNFe, &QPushButton::clicked, this, &WidgetDevolucao::on_pushButtonGerarNFe_clicked, connectionType);
  connect(ui->table, &QTableView::doubleClicked, this, &WidgetDevolucao::on_table_doubleClicked, connectionType);
}

void WidgetDevolucao::unsetConnections() {
  disconnect(ui->groupBoxMes, &QGroupBox::toggled, this, &WidgetDevolucao::montaFiltro);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetDevolucao::montaFiltro);
  disconnect(ui->pushButtonGerarNFe, &QPushButton::clicked, this, &WidgetDevolucao::on_pushButtonGerarNFe_clicked);
  disconnect(ui->table, &QTableView::doubleClicked, this, &WidgetDevolucao::on_table_doubleClicked);
}

void WidgetDevolucao::setupTables() {
  model.setTable("view_devolucao");

  model.setHeaderData("status", "Status");
  model.setHeaderData("statusOriginal", "Status Original");
  model.setHeaderData("idNFeSaida", "NF-e Saida");
  model.setHeaderData("idNFeEntrada", "NF-e Entrada");
  model.setHeaderData("idNFeFutura", "NF-e Futura");
  model.setHeaderData("idVenda", "Venda");
  model.setHeaderData("produto", "Produto");
  model.setHeaderData("obs", "Obs.");
  model.setHeaderData("caixas", "Caixas");
  model.setHeaderData("quant", "Quant");
  model.setHeaderData("un", "Un.");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("formComercial", "Form. Com.");

  model.setFilter("");

  ui->table->setModel(&model);

  ui->table->hideColumn("data");
  ui->table->hideColumn("idVendaProduto2");
  ui->table->hideColumn("idProduto");
}

void WidgetDevolucao::on_pushButtonGerarNFe_clicked() {
  // TODO: não deixar misturar idVendas diferentes na mesma nfe?

  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // -------------------------------------------------------------------------

  const int row = selection.first().row();

  const int idNFeSaida = model.data(row, "idNFeSaida").toInt();

  if (idNFeSaida == 0) { throw RuntimeError("Linha não possui NF-e de saída!", this); }

  // -------------------------------------------------------------------------

  const int idNFeEntrada = model.data(row, "idNFeEntrada").toInt();

  if (idNFeEntrada != 0) { throw RuntimeError("Linha já possui NF-e de entrada!", this); }

  // -------------------------------------------------------------------------

  QStringList nfes;

  for (const auto &index : selection) { nfes << model.data(index.row(), "idNFeSaida").toString(); }

  nfes.removeDuplicates();

  if (nfes.size() > 1) { throw RuntimeError("Selecionado mais de uma NF-e de saída!"); }

  // -------------------------------------------------------------------------

  const QString idVenda = model.data(row, "idVenda").toString();

  QStringList lista;

  for (const auto &index : selection) { lista << model.data(index.row(), "idVendaProduto2").toString(); }

  lista.removeDuplicates();

  // -------------------------------------------------------------------------

  auto *nfe = new CadastrarNFe(idVenda, lista, CadastrarNFe::Tipo::Entrada, this);
  nfe->setAttribute(Qt::WA_DeleteOnClose);

  nfe->show();
}

void WidgetDevolucao::ajustarGroupBoxStatus() {
  bool empty = true;
  auto filtrosStatus = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (auto *checkBox : filtrosStatus) {
    if (checkBox->isChecked()) { empty = false; }
  }

  unsetConnections();

  ui->groupBoxStatus->setChecked(not empty);

  for (auto *checkBox : filtrosStatus) { checkBox->setEnabled(true); }

  setConnections();
}

void WidgetDevolucao::on_table_doubleClicked(const QModelIndex &index) {
  if (not index.isValid()) { return; }

  const QString header = model.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(model.data(index.row(), "idVenda")); }
}

// TODO: renomear classe para WidgetLogisticaDevolucao
