#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "doubledelegate.h"
#include "excel.h"
#include "financeiroproxymodel.h"
#include "impressao.h"
#include "inputdialog.h"
#include "produtospendentes.h"
#include "reaisdelegate.h"
#include "ui_widgetcomprapendentes.h"
#include "widgetcomprapendentes.h"

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

  if (not query.exec() or not query.first()) { return qApp->enqueueError("Erro buscando produto: " + query.lastError().text(), this); }

  const QString un = query.value("un").toString();
  const QString un2 = un == "M2" or un == "M²" or un == "ML" ? "m2cx" : "pccx";

  ui->doubleSpinBoxQuantAvulso->setSingleStep(query.value(un2).toDouble());

  ui->doubleSpinBoxQuantAvulso->setValue(0);
  ui->doubleSpinBoxQuantAvulsoCaixas->setValue(0);

  ui->doubleSpinBoxQuantAvulso->setSuffix(" " + un);
}

void WidgetCompraPendentes::setConnections() {
  connect(ui->checkBoxFiltroColeta, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroCompra, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEmEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEntregaAgend, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEntregue, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEstoque, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroFaturamento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroIniciados, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroPendentes, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroRecebimento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroRepoEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroRepoReceb, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->doubleSpinBoxQuantAvulso, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxQuantAvulso_valueChanged);
  connect(ui->doubleSpinBoxQuantAvulsoCaixas, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxQuantAvulsoCaixas_valueChanged);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked);
  connect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated);
}

void WidgetCompraPendentes::updateTables() {
  if (not isSet) {
    setConnections();

    ui->itemBoxProduto->setSearchDialog(SearchDialog::produto(false, this));
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

  ui->table->resizeColumnsToContents();
}

void WidgetCompraPendentes::resetTables() { modelIsSet = false; }

void WidgetCompraPendentes::setupTables() {
  modelViewVendaProduto.setTable("view_venda_produto");

  modelViewVendaProduto.setHeaderData("data", "Data");
  modelViewVendaProduto.setHeaderData("fornecedor", "Fornecedor");
  modelViewVendaProduto.setHeaderData("idVenda", "Venda");
  modelViewVendaProduto.setHeaderData("produto", "Produto");
  modelViewVendaProduto.setHeaderData("caixas", "Caixas");
  modelViewVendaProduto.setHeaderData("quant", "Quant.");
  modelViewVendaProduto.setHeaderData("un", "Un.");
  modelViewVendaProduto.setHeaderData("total", "Total");
  modelViewVendaProduto.setHeaderData("codComercial", "Cód. Com.");
  modelViewVendaProduto.setHeaderData("formComercial", "Form. Com.");
  modelViewVendaProduto.setHeaderData("status", "Status");
  modelViewVendaProduto.setHeaderData("statusFinanceiro", "Financeiro");
  modelViewVendaProduto.setHeaderData("dataFinanceiro", "Data Fin.");
  modelViewVendaProduto.setHeaderData("obs", "Obs.");

  ui->table->setModel(new FinanceiroProxyModel(&modelViewVendaProduto, this));

  ui->table->setItemDelegateForColumn("quant", new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("total", new ReaisDelegate(this));
}

void WidgetCompraPendentes::on_table_activated(const QModelIndex &index) {
  const QString status = modelViewVendaProduto.data(index.row(), "status").toString();

  if (status != "PENDENTE" and status != "REPO. ENTREGA" and status != "REPO. RECEB.") { return qApp->enqueueError("Produto não está PENDENTE!", this); }

  const QString financeiro = modelViewVendaProduto.data(index.row(), "statusFinanceiro").toString();
  const QString codComercial = modelViewVendaProduto.data(index.row(), "codComercial").toString();
  const QString idVenda = modelViewVendaProduto.data(index.row(), "idVenda").toString();

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

  Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    if (child->isChecked()) { filtroCheck << "'" + child->text().toUpper() + "'"; }
  }

  if (not filtroCheck.isEmpty()) { filtros << "status IN (" + filtroCheck.join(", ") + ")"; }

  //-------------------------------------

  const QString textoBusca = ui->lineEditBusca->text();

  if (not textoBusca.isEmpty()) {
    const QString filtroBusca =
        "((idVenda LIKE '%" + textoBusca + "%') OR (fornecedor LIKE '%" + textoBusca + "%') OR (produto LIKE '%" + textoBusca + "%') OR (`codComercial` LIKE '%" + textoBusca + "%'))";
    filtros << filtroBusca;
  }

  //-------------------------------------

  filtros << "quant > 0";

  modelViewVendaProduto.setFilter(filtros.join(" AND "));

  if (not modelViewVendaProduto.select()) { return; }

  ui->table->sortByColumn("idVenda");
  ui->table->resizeColumnsToContents();
}

void WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked() {
  if (ui->itemBoxProduto->text().isEmpty()) { return qApp->enqueueError("Nenhum produto selecionado!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuantAvulso->value())) { return qApp->enqueueError("Deve escolher uma quantidade!", this); }

  InputDialog inputDlg(InputDialog::Tipo::Carrinho);

  if (inputDlg.exec() != InputDialog::Accepted) { return; }

  const QDate dataPrevista = inputDlg.getNextDate();

  insere(dataPrevista) ? qApp->enqueueInformation("Produto enviado para compras com sucesso!", this) : qApp->enqueueError("Erro ao enviar produto para compras!", this);

  ui->itemBoxProduto->clear();
}

bool WidgetCompraPendentes::insere(const QDate &dataPrevista) {
  QSqlQuery query;
  query.prepare("SELECT fornecedor, idProduto, descricao, colecao, un, un2, custo, kgcx, formComercial, codComercial, codBarras FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro buscando produto: " + query.lastError().text(), this); }

  QSqlQuery query2;
  query2.prepare(
      "INSERT INTO pedido_fornecedor_has_produto (fornecedor, idProduto, descricao, colecao, quant, un, un2, caixas, prcUnitario, preco, kgcx, formComercial, codComercial, codBarras, "
      "dataPrevCompra) VALUES (:fornecedor, :idProduto, :descricao, :colecao, :quant, :un, :un2, :caixas, :prcUnitario, :preco, :kgcx, :formComercial, :codComercial, :codBarras, :dataPrevCompra)");
  query2.bindValue(":fornecedor", query.value("fornecedor"));
  query2.bindValue(":idProduto", query.value("idProduto"));
  query2.bindValue(":descricao", query.value("descricao"));
  query2.bindValue(":colecao", query.value("colecao"));
  query2.bindValue(":quant", ui->doubleSpinBoxQuantAvulso->value());
  query2.bindValue(":un", query.value("un"));
  query2.bindValue(":un2", query.value("un2"));
  query2.bindValue(":caixas", ui->doubleSpinBoxQuantAvulsoCaixas->value());
  query2.bindValue(":prcUnitario", query.value("custo").toDouble());
  query2.bindValue(":preco", query.value("custo").toDouble() * ui->doubleSpinBoxQuantAvulso->value());
  query2.bindValue(":kgcx", query.value("kgcx"));
  query2.bindValue(":formComercial", query.value("formComercial"));
  query2.bindValue(":codComercial", query.value("codComercial"));
  query2.bindValue(":codBarras", query.value("codBarras"));
  query2.bindValue(":dataPrevCompra", dataPrevista);

  if (not query2.exec()) { return qApp->enqueueError(false, "Erro inserindo dados em pedido_fornecedor_has_produto: " + query2.lastError().text(), this); }

  return true;
}

void WidgetCompraPendentes::on_doubleSpinBoxQuantAvulsoCaixas_valueChanged(const double value) { ui->doubleSpinBoxQuantAvulso->setValue(value * ui->doubleSpinBoxQuantAvulso->singleStep()); }

void WidgetCompraPendentes::on_doubleSpinBoxQuantAvulso_valueChanged(const double value) { ui->doubleSpinBoxQuantAvulsoCaixas->setValue(value / ui->doubleSpinBoxQuantAvulso->singleStep()); }

void WidgetCompraPendentes::on_pushButtonExcel_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  Excel excel(modelViewVendaProduto.data(list.first().row(), "idVenda").toString());
  excel.gerarExcel();
}

void WidgetCompraPendentes::on_pushButtonPDF_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) { return qApp->enqueueError("Nenhum item selecionado!", this); }

  Impressao impressao(modelViewVendaProduto.data(list.first().row(), "idVenda").toString());
  impressao.print();
}

// TODO: [Conrado] quando for vendido produto_estoque marcar status como 'PRÉ-ESTOQUE' ou algo do tipo para o Conrado confirmar, apenas com um botão de ok ou cancelar.
// TODO: pegar a tableResumo e colocar em uma aba separada no widgetCompra
