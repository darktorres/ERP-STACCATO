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
#include <QSqlQuery>

WidgetCompraPendentes::WidgetCompraPendentes(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetCompraPendentes) { ui->setupUi(this); }

WidgetCompraPendentes::~WidgetCompraPendentes() { delete ui; }

void WidgetCompraPendentes::setarDadosAvulso() {
  if (ui->itemBoxProduto->getId().isNull()) {
    ui->doubleSpinBoxQuantAvulso->setValue(0);
    ui->doubleSpinBoxQuantAvulsoCaixas->setValue(0);
    ui->doubleSpinBoxQuantAvulso->setSuffix("");

    return;
  }

  QSqlQuery query;
  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueException("Erro buscando produto: " + query.lastError().text(), this); }

  // TODO: change this to use quantCaixa?
  const QString un = query.value("un").toString();
  const QString un2 = (un == "M2") or (un == "M²") or (un == "ML") ? "m2cx" : "pccx";

  ui->doubleSpinBoxQuantAvulso->setSingleStep(query.value(un2).toDouble());

  ui->doubleSpinBoxQuantAvulso->setValue(0);
  ui->doubleSpinBoxQuantAvulsoCaixas->setValue(0);

  ui->doubleSpinBoxQuantAvulso->setSuffix(" " + un);
}

void WidgetCompraPendentes::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

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
  connect(ui->doubleSpinBoxQuantAvulso, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxQuantAvulso_valueChanged, connectionType);
  connect(ui->doubleSpinBoxQuantAvulsoCaixas, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxQuantAvulsoCaixas_valueChanged, connectionType);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled, connectionType);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraPendentes::montaFiltro, connectionType);
  connect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked, connectionType);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked, connectionType);
  connect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked, connectionType);
  connect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated, connectionType);
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

  if (not modelViewVendaProduto.select()) { return; }
}

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

  if (status != "PENDENTE" and status != "REPO. ENTREGA" and status != "REPO. RECEB.") { return qApp->enqueueError("Produto não está 'PENDENTE/REPO. ENTREGA/REPO. RECEB.'!", this); }

  const QString financeiro = modelViewVendaProduto.data(row, "statusFinanceiro").toString();
  const QString codComercial = modelViewVendaProduto.data(row, "codComercial").toString();
  const QString idVenda = modelViewVendaProduto.data(row, "idVenda").toString();

  if (financeiro == "PENDENTE") {
    QMessageBox msgBox(QMessageBox::Question, "Pendente!", "Financeiro não liberou! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) { return; }
  }

  auto *produtos = new ProdutosPendentes(codComercial, idVenda, this);
  produtos->setAttribute(Qt::WA_DeleteOnClose);
}

void WidgetCompraPendentes::on_groupBoxStatus_toggled(bool enabled) {
  const auto container = ui->groupBoxStatus->findChildren<QCheckBox *>();

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

  const auto children = ui->groupBoxStatus->findChildren<QCheckBox *>();

  for (const auto &child : children) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();
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
  if (ui->itemBoxProduto->text().isEmpty()) { return qApp->enqueueError("Nenhum produto selecionado!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuantAvulso->value())) { return qApp->enqueueError("Deve escolher uma quantidade!", this); }

  InputDialog inputDlg(InputDialog::Tipo::Carrinho, this);

  if (inputDlg.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = inputDlg.getNextDate();

  insere(dataPrevista) ? qApp->enqueueInformation("Produto enviado para compras com sucesso!", this) : qApp->enqueueException("Erro ao enviar produto para compras!", this);

  ui->itemBoxProduto->clear();
}

bool WidgetCompraPendentes::insere(const QDate &dataPrevista) {
  SqlTableModel model;
  model.setTable("pedido_fornecedor_has_produto");

  const int newRow = model.insertRowAtEnd();

  QSqlQuery query;
  query.prepare("SELECT fornecedor, idProduto, descricao, colecao, un, un2, custo, kgcx, formComercial, codComercial, codBarras FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueException(false, "Erro buscando produto: " + query.lastError().text(), this); }

  if (not model.setData(newRow, "fornecedor", query.value("fornecedor"))) { return false; }
  if (not model.setData(newRow, "idProduto", query.value("idProduto"))) { return false; }
  if (not model.setData(newRow, "descricao", query.value("descricao"))) { return false; }
  if (not model.setData(newRow, "colecao", query.value("colecao"))) { return false; }
  if (not model.setData(newRow, "quant", ui->doubleSpinBoxQuantAvulso->value())) { return false; }
  if (not model.setData(newRow, "un", query.value("un"))) { return false; }
  if (not model.setData(newRow, "un2", query.value("un2"))) { return false; }
  if (not model.setData(newRow, "caixas", ui->doubleSpinBoxQuantAvulsoCaixas->value())) { return false; }
  if (not model.setData(newRow, "prcUnitario", query.value("custo").toDouble())) { return false; }
  if (not model.setData(newRow, "preco", query.value("custo").toDouble() * ui->doubleSpinBoxQuantAvulso->value())) { return false; }
  if (not model.setData(newRow, "kgcx", query.value("kgcx"))) { return false; }
  if (not model.setData(newRow, "formComercial", query.value("formComercial"))) { return false; }
  if (not model.setData(newRow, "codComercial", query.value("codComercial"))) { return false; }
  if (not model.setData(newRow, "codBarras", query.value("codBarras"))) { return false; }
  if (not model.setData(newRow, "dataPrevCompra", dataPrevista)) { return false; }

  if (not model.submitAll()) { return qApp->enqueueException(false, "Erro inserindo dados em pedido_fornecedor_has_produto: " + model.lastError().text(), this); }

  return true;
}

void WidgetCompraPendentes::on_doubleSpinBoxQuantAvulsoCaixas_valueChanged(const double value) { ui->doubleSpinBoxQuantAvulso->setValue(value * ui->doubleSpinBoxQuantAvulso->singleStep()); }

void WidgetCompraPendentes::on_doubleSpinBoxQuantAvulso_valueChanged(const double value) { ui->doubleSpinBoxQuantAvulsoCaixas->setValue(value / ui->doubleSpinBoxQuantAvulso->singleStep()); }

void WidgetCompraPendentes::on_pushButtonExcel_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  const QString idVenda = modelViewVendaProduto.data(list.first().row(), "idVenda").toString();

  Excel excel(idVenda, Excel::Tipo::Venda);
  excel.gerarExcel();
}

void WidgetCompraPendentes::on_pushButtonPDF_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  const QString idVenda = modelViewVendaProduto.data(list.first().row(), "idVenda").toString();

  PDF impressao(idVenda, PDF::Tipo::Venda, this);
  impressao.gerarPdf();
}

// TODO: [Conrado] quando for vendido produto_estoque marcar status como 'PRÉ-ESTOQUE' ou algo do tipo para
// o Conrado confirmar, apenas com um botão de ok ou cancelar.
// TODO: pegar a tableResumo e colocar em uma aba separada no widgetCompra
// TODO: não listar produto_estoque no SearchDialog da compra avulsa
