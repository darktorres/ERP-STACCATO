#include "widgetcomprapendentes.h"
#include "ui_widgetcomprapendentes.h"

#include "application.h"
#include "doubledelegate.h"
#include "excel.h"
#include "financeiroproxymodel.h"
#include "inputdialog.h"
#include "pdf.h"
#include "produtospendentes.h"
#include "reaisdelegate.h"

#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QtMath>

WidgetCompraPendentes::WidgetCompraPendentes(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraPendentes) {
  ui->setupUi(this);
  timer.setSingleShot(true);
}

WidgetCompraPendentes::~WidgetCompraPendentes() { delete ui; }

void WidgetCompraPendentes::setarDadosAvulso() {
  if (ui->itemBoxProduto->getId().isNull()) {
    ui->doubleSpinBoxAvulsoQuant->setValue(0);
    ui->doubleSpinBoxAvulsoCaixas->setValue(0);
    ui->doubleSpinBoxAvulsoQuant->setSuffix("");

    return;
  }

  SqlQuery query;
  query.prepare("SELECT UPPER(un) AS un, quantCaixa FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando produto: " + query.lastError().text(), this); }

  const QString un = query.value("un").toString();
  const double quantCaixa = query.value("quantCaixa").toDouble();

  ui->doubleSpinBoxAvulsoQuant->setSingleStep(quantCaixa);

  ui->doubleSpinBoxAvulsoQuant->setValue(0);
  ui->doubleSpinBoxAvulsoCaixas->setValue(0);

  ui->doubleSpinBoxAvulsoQuant->setSuffix(" " + un);
}

void WidgetCompraPendentes::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(&timer, &QTimer::timeout, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxAtelier, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxServicos, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroColeta, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroCompra, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroEmEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroEntregaAgend, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroEntregue, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroEstoque, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroFaturamento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroIniciados, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroPendentes, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroRecebimento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroRepoEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->checkBoxFiltroRepoReceb, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->doubleSpinBoxAvulsoQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoQuant_valueChanged, connectionType);
  connect(ui->doubleSpinBoxAvulsoCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoCaixas_valueChanged, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraPendentes::delayFiltro, connectionType);
  connect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked, connectionType);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked, connectionType);
  connect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated, connectionType);
}

void WidgetCompraPendentes::unsetConnections() {
  disconnect(&timer, &QTimer::timeout, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxAtelier, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxServicos, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroColeta, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroCompra, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroEmEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroEntregaAgend, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroEntregue, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroEstoque, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroFaturamento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroIniciados, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroPendentes, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroRecebimento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroRepoEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->checkBoxFiltroRepoReceb, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->doubleSpinBoxAvulsoQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoQuant_valueChanged);
  disconnect(ui->doubleSpinBoxAvulsoCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoCaixas_valueChanged);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled);
  disconnect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraPendentes::delayFiltro);
  disconnect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked);
  disconnect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked);
  disconnect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated);
}

void WidgetCompraPendentes::updateTables() {
  if (not isSet) {
    setConnections();

    ui->itemBoxProduto->setSearchDialog(SearchDialog::produto(false, true, true, true, this));
    ui->itemBoxProduto->setRepresentacao(false);

    connect(ui->itemBoxProduto, &QLineEdit::textChanged, this, &WidgetCompraPendentes::setarDadosAvulso);

    isSet = true;
  }

  if (not modelIsSet) {
    setupTables();
    montaFiltro();
    modelIsSet = true;
  }

  modelViewVendaProduto.select();
}

void WidgetCompraPendentes::delayFiltro() { timer.start(500); }

void WidgetCompraPendentes::resetTables() { modelIsSet = false; }

void WidgetCompraPendentes::setupTables() {
  modelViewVendaProduto.setTable("view_venda_produto");

  modelViewVendaProduto.setSort("idVenda");

  modelViewVendaProduto.setHeaderData("prazoEntrega", "Prazo Limite");
  modelViewVendaProduto.setHeaderData("novoPrazoEntrega", "Novo Prazo");
  modelViewVendaProduto.setHeaderData("dataFinanceiro", "Financeiro");
  modelViewVendaProduto.setHeaderData("idVenda", "Venda");
  modelViewVendaProduto.setHeaderData("fornecedor", "Fornecedor");
  modelViewVendaProduto.setHeaderData("produto", "Produto");
  modelViewVendaProduto.setHeaderData("caixas", "Caixas");
  modelViewVendaProduto.setHeaderData("quant", "Quant.");
  modelViewVendaProduto.setHeaderData("un", "Un.");
  modelViewVendaProduto.setHeaderData("codComercial", "Cód. Com.");
  modelViewVendaProduto.setHeaderData("formComercial", "Form. Com.");
  modelViewVendaProduto.setHeaderData("status", "Status");
  modelViewVendaProduto.setHeaderData("statusFinanceiro", "Financeiro");
  modelViewVendaProduto.setHeaderData("obs", "Obs.");

  modelViewVendaProduto.proxyModel = new FinanceiroProxyModel(&modelViewVendaProduto, this);

  ui->table->setModel(&modelViewVendaProduto);

  ui->table->setItemDelegateForColumn("quant", new DoubleDelegate(3, this));
}

void WidgetCompraPendentes::on_table_activated(const QModelIndex &index) {
  const int row = index.row();

  const QString status = modelViewVendaProduto.data(row, "status").toString();

  if (status != "PENDENTE" and status != "REPO. ENTREGA" and status != "REPO. RECEB.") { throw RuntimeError("Produto não está 'PENDENTE/REPO. ENTREGA/REPO. RECEB.'!", this); }

  const QString financeiro = modelViewVendaProduto.data(row, "statusFinanceiro").toString();
  const QString fornecedor = modelViewVendaProduto.data(row, "fornecedor").toString();
  const QString codComercial = modelViewVendaProduto.data(row, "codComercial").toString();
  const QString idVenda = modelViewVendaProduto.data(row, "idVenda").toString();

  if (financeiro == "PENDENTE") {
    QMessageBox msgBox(QMessageBox::Question, "Pendente!", "Financeiro não liberou! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  auto *produtos = new ProdutosPendentes(this);
  produtos->setAttribute(Qt::WA_DeleteOnClose);
  produtos->viewProduto(fornecedor, codComercial, idVenda);
  produtos->show();
}

void WidgetCompraPendentes::on_groupBoxStatus_toggled(bool enabled) {
  const auto container = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : container) {
    disconnect(child, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
    child->setEnabled(true);
    child->setChecked(enabled);
    connect(child, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  }

  montaFiltro();
}

void WidgetCompraPendentes::montaFiltro() {
  QStringList filtros;
  QStringList filtroCheck;

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  const QString textoBusca = qApp->sanitizeSQL(ui->lineEditBusca->text());
  const QString filtroBusca = "(idVenda LIKE '%" + textoBusca + "%' OR fornecedor LIKE '%" + textoBusca + "%' OR produto LIKE '%" + textoBusca + "%' OR `codComercial` LIKE '%" + textoBusca + "%')";

  if (not textoBusca.isEmpty()) { filtros << filtroBusca; }

  //-------------------------------------

  const bool atelier = ui->checkBoxAtelier->isChecked();

  filtros << (atelier ? "fornecedor = 'ATELIER STACCATO'" : "fornecedor <> 'ATELIER STACCATO'");

  //-------------------------------------

  const bool servicos = ui->checkBoxServicos->isChecked();

  filtros << (servicos ? "fornecedor = 'STACCATO SERVIÇOS ESPECIAIS (SSE)'" : "fornecedor <> 'STACCATO SERVIÇOS ESPECIAIS (SSE)'");

  //-------------------------------------

  filtros << "quant > 0";

  modelViewVendaProduto.setFilter(filtros.join(" AND "));
}

void WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked() {
  if (ui->itemBoxProduto->text().isEmpty()) { throw RuntimeError("Nenhum produto selecionado!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxAvulsoQuant->value())) { throw RuntimeError("Deve escolher uma quantidade!", this); }

  InputDialog inputDlg(InputDialog::Tipo::Carrinho, this);

  if (inputDlg.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = inputDlg.getNextDate();

  insere(dataPrevista);

  qApp->enqueueInformation("Produto enviado para compras com sucesso!", this);

  ui->itemBoxProduto->clear();
}

void WidgetCompraPendentes::insere(const QDate &dataPrevista) {
  SqlTableModel model;
  model.setTable("pedido_fornecedor_has_produto");

  const int newRow = model.insertRowAtEnd();

  SqlQuery query;
  query.prepare("SELECT fornecedor, idProduto, descricao, colecao, un, un2, custo, kgcx, formComercial, codComercial, codBarras FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro buscando produto: " + query.lastError().text()); }

  model.setData(newRow, "fornecedor", query.value("fornecedor"));
  model.setData(newRow, "idProduto", query.value("idProduto"));
  model.setData(newRow, "descricao", query.value("descricao"));
  model.setData(newRow, "colecao", query.value("colecao"));
  model.setData(newRow, "quant", ui->doubleSpinBoxAvulsoQuant->value());
  model.setData(newRow, "un", query.value("un"));
  model.setData(newRow, "un2", query.value("un2"));
  model.setData(newRow, "caixas", ui->doubleSpinBoxAvulsoCaixas->value());
  model.setData(newRow, "prcUnitario", query.value("custo").toDouble());
  model.setData(newRow, "preco", query.value("custo").toDouble() * ui->doubleSpinBoxAvulsoQuant->value());
  model.setData(newRow, "kgcx", query.value("kgcx"));
  model.setData(newRow, "formComercial", query.value("formComercial"));
  model.setData(newRow, "codComercial", query.value("codComercial"));
  model.setData(newRow, "codBarras", query.value("codBarras"));
  model.setData(newRow, "dataPrevCompra", dataPrevista);

  model.submitAll();
}

void WidgetCompraPendentes::on_doubleSpinBoxAvulsoCaixas_valueChanged(const double caixas) {
  const double stepQt = ui->doubleSpinBoxAvulsoQuant->singleStep();
  const double stepCx = ui->doubleSpinBoxAvulsoCaixas->singleStep();

  unsetConnections();

  try {
    const double resto = fmod(caixas, stepCx);
    const double caixas2 = not qFuzzyIsNull(resto) ? ceil(caixas) : caixas;
    ui->doubleSpinBoxAvulsoCaixas->setValue(caixas2);

    const double quant2 = caixas2 * stepQt;
    ui->doubleSpinBoxAvulsoQuant->setValue(quant2);
  } catch (std::exception &) {}

  setConnections();
}

void WidgetCompraPendentes::on_doubleSpinBoxAvulsoQuant_valueChanged(const double quant) {
  const double stepQt = ui->doubleSpinBoxAvulsoQuant->singleStep();

  unsetConnections();

  try {
    const double resto = fmod(quant, stepQt);
    const double quant2 = not qFuzzyIsNull(resto) ? ceil(quant / stepQt) * stepQt : quant;
    ui->doubleSpinBoxAvulsoQuant->setValue(quant2);

    const double caixas2 = quant2 / stepQt;
    ui->doubleSpinBoxAvulsoCaixas->setValue(caixas2);
  } catch (std::exception &) {}

  setConnections();
}

void WidgetCompraPendentes::on_pushButtonExcel_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const QString idVenda = modelViewVendaProduto.data(list.first().row(), "idVenda").toString();

  Excel excel(idVenda, Excel::Tipo::Venda, this);
  excel.mostrarRT = true;
  excel.gerarExcel();
}

void WidgetCompraPendentes::on_pushButtonPDF_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const QString idVenda = modelViewVendaProduto.data(list.first().row(), "idVenda").toString();

  PDF impressao(idVenda, PDF::Tipo::Venda, this);
  impressao.mostrarRT = true;
  impressao.gerarPdf();
}

// TODO: [Conrado] quando for vendido produto_estoque marcar status como 'PRÉ-ESTOQUE' ou algo do tipo para
// o Conrado confirmar, apenas com um botão de ok ou cancelar.
// TODO: pegar a tableResumo e colocar em uma aba separada no widgetCompra
// TODO: não listar produto_estoque no SearchDialog da compra avulsa
