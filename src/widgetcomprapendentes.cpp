#include "widgetcomprapendentes.h"
#include "ui_widgetcomprapendentes.h"

#include "application.h"
#include "doubledelegate.h"
#include "excel.h"
#include "financeiroproxymodel.h"
#include "followup.h"
#include "inputdialog.h"
#include "pdf.h"
#include "produtospendentes.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QtMath>

WidgetCompraPendentes::WidgetCompraPendentes(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraPendentes) { ui->setupUi(this); }

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

  if (not query.exec()) { throw RuntimeException("Erro buscando produto: " + query.lastError().text(), this); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para o produto com id: '" + ui->itemBoxProduto->getId().toString() + "'"); }

  const QString un = query.value("un").toString();
  const double quantCaixa = query.value("quantCaixa").toDouble();

  ui->doubleSpinBoxAvulsoQuant->setSingleStep(quantCaixa);

  ui->doubleSpinBoxAvulsoQuant->setValue(0);
  ui->doubleSpinBoxAvulsoCaixas->setValue(0);

  ui->doubleSpinBoxAvulsoQuant->setSuffix(" " + un);
}

void WidgetCompraPendentes::setConnections() {
  if (not blockingSignals.isEmpty()) { blockingSignals.pop(); } // avoid crashing on first setConnections

  if (not blockingSignals.isEmpty()) { return; } // delay setting connections until last unset/set block

  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxAtelier, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
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
  connect(ui->checkBoxServicos, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->doubleSpinBoxAvulsoCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoCaixas_valueChanged, connectionType);
  connect(ui->doubleSpinBoxAvulsoQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoQuant_valueChanged, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled, connectionType);
  connect(ui->itemBoxProduto, &QLineEdit::textChanged, this, &WidgetCompraPendentes::setarDadosAvulso, connectionType);
  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked, connectionType);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked, connectionType);
  connect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonFollowup_clicked, connectionType);
  connect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated, connectionType);
}

void WidgetCompraPendentes::unsetConnections() {
  blockingSignals.push(0);

  disconnect(ui->checkBoxAtelier, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
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
  disconnect(ui->checkBoxServicos, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->doubleSpinBoxAvulsoCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoCaixas_valueChanged);
  disconnect(ui->doubleSpinBoxAvulsoQuant, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxAvulsoQuant_valueChanged);
  disconnect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled);
  disconnect(ui->itemBoxProduto, &QLineEdit::textChanged, this, &WidgetCompraPendentes::setarDadosAvulso);
  disconnect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetCompraPendentes::montaFiltro);
  disconnect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked);
  disconnect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked);
  disconnect(ui->pushButtonFollowup, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonFollowup_clicked);
  disconnect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked);
  disconnect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated);
}

void WidgetCompraPendentes::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();

    ui->itemBoxProduto->setSearchDialog(SearchDialog::produto(false, true, true, true, this));
    ui->itemBoxProduto->setRepresentacao(false); // TODO: move this to ctor?

    setupTables();
    montaFiltro();

    setConnections();
    isSet = true;
  }

  modelProduto.select();
}

void WidgetCompraPendentes::resetTables() {
  setupTables();
  montaFiltro();
}

void WidgetCompraPendentes::setupTables() {
  modelProduto.setTable("view_venda_produto");

  modelProduto.setSort("idVenda");

  modelProduto.setHeaderData("status", "Status");
  modelProduto.setHeaderData("statusFinanceiro", "Financeiro");
  modelProduto.setHeaderData("dataFinanceiro", "Financeiro");
  modelProduto.setHeaderData("idVenda", "Venda");
  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("produto", "Produto");
  modelProduto.setHeaderData("obs", "Obs.");
  modelProduto.setHeaderData("caixas", "Caixas");
  modelProduto.setHeaderData("quant", "Quant.");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("prazoEntrega", "Prazo Limite");
  modelProduto.setHeaderData("novoPrazoEntrega", "Novo Prazo");
  modelProduto.setHeaderData("dataFollowup", "Data Followup");
  modelProduto.setHeaderData("observacao", "Observação");

  modelProduto.proxyModel = new FinanceiroProxyModel(&modelProduto, this);

  ui->table->setModel(&modelProduto);

  ui->table->setItemDelegateForColumn("quant", new DoubleDelegate(3, this));
}

void WidgetCompraPendentes::on_table_activated(const QModelIndex &index) {
  const QString header = modelProduto.headerData(index.column(), Qt::Horizontal).toString();

  if (header == "Venda") { return qApp->abrirVenda(modelProduto.data(index.row(), "idVenda")); }

  // -------------------------------------------------------------------------

  const int row = index.row();

  const QString status = modelProduto.data(row, "status").toString();

  if (status != "PENDENTE" and status != "REPO. ENTREGA" and status != "REPO. RECEB.") { throw RuntimeError("Produto não está 'PENDENTE/REPO. ENTREGA/REPO. RECEB.'!", this); }

  const QString financeiro = modelProduto.data(row, "statusFinanceiro").toString();
  const QString fornecedor = modelProduto.data(row, "fornecedor").toString();
  const QString codComercial = modelProduto.data(row, "codComercial").toString();
  const QString idVenda = modelProduto.data(row, "idVenda").toString();

  if (financeiro == "PENDENTE") {
    QMessageBox msgBox(QMessageBox::Question, "Pendente!", "Financeiro não liberou! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.button(QMessageBox::Yes)->setText("Continuar");
    msgBox.button(QMessageBox::No)->setText("Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  auto *produtos = new ProdutosPendentes(this);
  produtos->setAttribute(Qt::WA_DeleteOnClose);
  produtos->viewProduto(fornecedor, codComercial, idVenda);
  produtos->show();
}

void WidgetCompraPendentes::on_groupBoxStatus_toggled(const bool enabled) {
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
  ajustarGroupBoxStatus();

  //-------------------------------------

  QStringList filtros;

  //-------------------------------------

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

  modelProduto.setFilter(filtros.join(" AND "));
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

void WidgetCompraPendentes::insere(const QDate dataPrevista) {
  SqlTableModel model;
  model.setTable("pedido_fornecedor_has_produto");

  const int newRow = model.insertRowAtEnd();

  SqlQuery query;
  query.prepare("SELECT fornecedor, idProduto, descricao, colecao, un, un2, custo, kgcx, formComercial, codComercial, codBarras FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec()) { throw RuntimeException("Erro buscando produto: " + query.lastError().text()); }

  if (not query.first()) { throw RuntimeException("Dados não encontrados para o produto com id: '" + ui->itemBoxProduto->getId().toString() + "'"); }

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
  } catch (std::exception &) {
    setConnections();
    throw;
  }

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
  } catch (std::exception &) {
    setConnections();
    throw;
  }

  setConnections();
}

void WidgetCompraPendentes::on_pushButtonExcel_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const QString idVenda = modelProduto.data(selection.first().row(), "idVenda").toString();

  Excel excel(idVenda, Excel::Tipo::Venda, this);
  excel.mostrarRT = true;
  excel.gerarExcel();
}

void WidgetCompraPendentes::on_pushButtonPDF_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhum item selecionado!", this); }

  const QString idVenda = modelProduto.data(selection.first().row(), "idVenda").toString();

  PDF impressao(idVenda, PDF::Tipo::Venda, this);
  impressao.mostrarRT = true;
  impressao.gerarPdf();
}

void WidgetCompraPendentes::on_pushButtonFollowup_clicked() {
  const auto selection = ui->table->selectionModel()->selectedRows();

  if (selection.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  const QString idVenda = modelProduto.data(selection.first().row(), "idVenda").toString();

  auto *followup = new FollowUp(idVenda, FollowUp::Tipo::Venda, this);
  followup->setAttribute(Qt::WA_DeleteOnClose);
  followup->show();
}

void WidgetCompraPendentes::ajustarGroupBoxStatus() {
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

// TODO: [Conrado] quando for vendido produto_estoque marcar status como 'PRÉ-ESTOQUE' ou algo do tipo para
// o Conrado confirmar, apenas com um botão de ok ou cancelar.
// TODO: pegar a tableResumo e colocar em uma aba separada no widgetCompra
// TODO: não listar produto_estoque no SearchDialog da compra avulsa
// TODO: adicionar coluna mostrando observacao do followup
