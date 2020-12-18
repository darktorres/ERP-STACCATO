#include "widgetdevolucao.h"
#include "ui_widgetdevolucao.h"

#include "application.h"
#include "cadastrarnfe.h"

#include <QDebug>

WidgetDevolucao::WidgetDevolucao(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetDevolucao) { ui->setupUi(this); }

WidgetDevolucao::~WidgetDevolucao() { delete ui; }

void WidgetDevolucao::resetTables() { modelIsSet = false; }

void WidgetDevolucao::updateTables() {
  if (not isSet) {
    setConnections();
    ui->dateEditMes->setDate(qApp->serverDate());
    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    modelIsSet = true;
  }

  model.select();
}

void WidgetDevolucao::montaFiltro() {
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
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetDevolucao::montaFiltro, connectionType);
  connect(ui->pushButtonGerarNFe, &QPushButton::clicked, this, &WidgetDevolucao::on_pushButtonGerarNFe_clicked, connectionType);
}

void WidgetDevolucao::setupTables() {
  model.setTable("view_devolucao");

  model.setHeaderData("status", "Status");
  model.setHeaderData("statusOriginal", "Status Original");
  model.setHeaderData("idNFeSaida", "NFe Saida");
  model.setHeaderData("idNFeEntrada", "NFe Entrada");
  model.setHeaderData("idNFeFutura", "NFe Futura");
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

  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  // -------------------------------------------------------------------------

  const int row = list.first().row();

  const int idNFeSaida = model.data(row, "idNFeSaida").toInt();

  if (idNFeSaida == 0) { throw RuntimeError("Linha não possui NFe de saída!", this); }

  // -------------------------------------------------------------------------

  const int idNFeEntrada = model.data(row, "idNFeEntrada").toInt();

  if (idNFeEntrada != 0) { throw RuntimeError("Linha já possui NFe de entrada!", this); }

  // -------------------------------------------------------------------------

  QStringList nfes;

  for (auto &index : list) { nfes << model.data(index.row(), "idNFeSaida").toString(); }

  nfes.removeDuplicates();

  if (nfes.size() > 1) { throw RuntimeError("Selecionado mais de uma NFe de saída!"); }

  // -------------------------------------------------------------------------

  const QString idVenda = model.data(row, "idVenda").toString();

  QStringList lista;

  for (auto &index : list) { lista << model.data(index.row(), "idVendaProduto2").toString(); }

  lista.removeDuplicates();

  // -------------------------------------------------------------------------

  auto *nfe = new CadastrarNFe(idVenda, lista, CadastrarNFe::Tipo::Entrada, this);
  nfe->setAttribute(Qt::WA_DeleteOnClose);

  nfe->show();
}
