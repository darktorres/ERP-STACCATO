#include <QDate>
#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

#include "doubledelegate.h"
#include "excel.h"
#include "financeiroproxymodel.h"
#include "impressao.h"
#include "inputdialog.h"
#include "produtospendentes.h"
#include "reaisdelegate.h"
#include "ui_widgetcomprapendentes.h"
#include "widgetcomprapendentes.h"

WidgetCompraPendentes::WidgetCompraPendentes(QWidget *parent) : Widget(parent), ui(new Ui::WidgetCompraPendentes) {
  ui->setupUi(this);

  connect(ui->doubleSpinBoxQuantAvulso, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxQuantAvulso_valueChanged);
  connect(ui->doubleSpinBoxQuantAvulsoCaixas, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidgetCompraPendentes::on_doubleSpinBoxQuantAvulsoCaixas_valueChanged);
  connect(ui->groupBoxStatus, &QGroupBox::toggled, this, &WidgetCompraPendentes::on_groupBoxStatus_toggled);
  connect(ui->pushButtonComprarAvulso, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked);
  connect(ui->pushButtonExcel, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonExcel_clicked);
  connect(ui->pushButtonPDF, &QPushButton::clicked, this, &WidgetCompraPendentes::on_pushButtonPDF_clicked);
  connect(ui->table, &TableView::activated, this, &WidgetCompraPendentes::on_table_activated);
  connect(ui->table, &TableView::entered, this, &WidgetCompraPendentes::on_table_entered);

  ui->itemBoxProduto->setSearchDialog(SearchDialog::produto(false, this));
  ui->itemBoxProduto->getSearchDialog()->setRepresentacao(" AND representacao = FALSE");

  connect(ui->itemBoxProduto, &QLineEdit::textChanged, this, &WidgetCompraPendentes::setarDadosAvulso);

  ui->checkBoxFiltroPendentes->setChecked(true);
  ui->checkBoxFiltroRepoEntrega->setChecked(true);
  ui->checkBoxFiltroRepoReceb->setChecked(true);
}

WidgetCompraPendentes::~WidgetCompraPendentes() { delete ui; }

void WidgetCompraPendentes::setarDadosAvulso() {
  if (ui->itemBoxProduto->getValue().isNull()) {
    ui->doubleSpinBoxQuantAvulso->setValue(0);
    ui->doubleSpinBoxQuantAvulsoCaixas->setValue(0);
    ui->doubleSpinBoxQuantAvulso->setSuffix("");

    return;
  }

  QSqlQuery query;
  query.prepare("SELECT UPPER(un) AS un, m2cx, pccx FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getValue());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando produto: " + query.lastError().text());
    return;
  }

  const QString un = query.value("un").toString();

  ui->doubleSpinBoxQuantAvulso->setSingleStep(query.value(un == "M2" or un == "M²" or un == "ML" ? "m2cx" : "pccx").toDouble());

  ui->doubleSpinBoxQuantAvulso->setValue(0);
  ui->doubleSpinBoxQuantAvulsoCaixas->setValue(0);

  ui->doubleSpinBoxQuantAvulso->setSuffix(" " + un);
}

void WidgetCompraPendentes::makeConnections() {
  connect(ui->checkBoxFiltroPendentes, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroIniciados, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroCompra, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroFaturamento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroColeta, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroRecebimento, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEstoque, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEmEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEntregaAgend, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroEntregue, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroRepoEntrega, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroRepoReceb, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &WidgetCompraPendentes::montaFiltro);
  connect(ui->checkBoxFiltroSul, &QCheckBox::toggled, this, &WidgetCompraPendentes::montaFiltro);
}

bool WidgetCompraPendentes::updateTables() {
  if (modelViewVendaProduto.tableName().isEmpty()) {
    setupTables();
    montaFiltro();
    makeConnections();
  }

  if (not modelViewVendaProduto.select()) {
    emit errorSignal("Erro lendo tabela produtos pendentes: " + modelViewVendaProduto.lastError().text());
    return false;
  }

  ui->table->resizeColumnsToContents();

  return true;
}

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

  ui->table->sortByColumn("idVenda");
  ui->table->setItemDelegateForColumn("quant", new DoubleDelegate(this));
  ui->table->setItemDelegateForColumn("total", new ReaisDelegate(this));
  ui->table->resizeColumnsToContents();
}

void WidgetCompraPendentes::on_table_activated(const QModelIndex &index) {
  const QString status = modelViewVendaProduto.data(index.row(), "status").toString();

  if (status != "PENDENTE" and status != "REPO. ENTREGA" and status != "REPO. RECEB.") {
    emit errorSignal("Produto não está PENDENTE!");
    return;
  }

  const QString financeiro = modelViewVendaProduto.data(index.row(), "statusFinanceiro").toString();
  const QString codComercial = modelViewVendaProduto.data(index.row(), "codComercial").toString();
  const QString idVenda = modelViewVendaProduto.data(index.row(), "idVenda").toString();

  if (financeiro == "PENDENTE") {
    QMessageBox msgBox(QMessageBox::Question, "Pendente!", "Financeiro não liberou! Continuar?", QMessageBox::Yes | QMessageBox::No, this);
    msgBox.setButtonText(QMessageBox::Yes, "Continuar");
    msgBox.setButtonText(QMessageBox::No, "Voltar");

    if (msgBox.exec() == QMessageBox::No) return;
  }

  auto *produtos = new ProdutosPendentes(this);
  produtos->setAttribute(Qt::WA_DeleteOnClose);
  produtos->viewProduto(codComercial, idVenda);
}

void WidgetCompraPendentes::on_groupBoxStatus_toggled(bool enabled) {
  Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    child->setEnabled(true);
    child->setChecked(enabled);
  }
}

void WidgetCompraPendentes::montaFiltro() {
  QString filtroCheck;

  Q_FOREACH (const auto &child, ui->groupBoxStatus->findChildren<QCheckBox *>()) {
    if (child->isChecked()) filtroCheck += QString(filtroCheck.isEmpty() ? "" : " OR ") + "status = '" + child->text().toUpper() + "'";
  }

  filtroCheck = filtroCheck.isEmpty() ? "" : "(" + filtroCheck + ")";

  const QString textoBusca = ui->lineEditBusca->text();

  const QString textoSul = ui->checkBoxFiltroSul->isChecked() ? "" : " AND idVenda NOT LIKE 'CAMB-%'";

  const QString filtroBusca = textoBusca.isEmpty() ? ""
                                                   : QString(filtroCheck.isEmpty() ? "" : " AND ") + "((idVenda LIKE '%" + textoBusca + "%') OR (fornecedor LIKE '%" + textoBusca +
                                                         "%') OR (produto LIKE '%" + textoBusca + "%') OR (`codComercial` LIKE '%" + textoBusca + "%'))";

  const QString filtroStatus = QString((filtroCheck + filtroBusca).isEmpty() ? "" : " AND ") + "status != 'CANCELADO'";

  modelViewVendaProduto.setFilter(filtroCheck + filtroBusca + filtroStatus + " AND quant > 0" + textoSul);

  if (not modelViewVendaProduto.select()) emit errorSignal("Erro na busca: " + modelViewVendaProduto.lastError().text());

  ui->table->resizeColumnsToContents();
}

void WidgetCompraPendentes::on_pushButtonComprarAvulso_clicked() {
  if (ui->itemBoxProduto->text().isEmpty()) {
    emit errorSignal("Nenhum produto selecionado!");
    return;
  }

  if (qFuzzyIsNull(ui->doubleSpinBoxQuantAvulso->value())) {
    emit errorSignal("Deve escolher uma quantidade!");
    return;
  }

  InputDialog inputDlg(InputDialog::Tipo::Carrinho);

  if (inputDlg.exec() != InputDialog::Accepted) return;

  const QDate dataPrevista = inputDlg.getNextDate();

  insere(dataPrevista) ? emit informationSignal("Produto enviado para compras com sucesso!") : emit errorSignal("Erro ao enviar produto para compras!");

  ui->itemBoxProduto->clear();
}

bool WidgetCompraPendentes::insere(const QDate &dataPrevista) {
  QSqlQuery query;
  query.prepare("SELECT fornecedor, idProduto, descricao, colecao, un, un2, custo, kgcx, formComercial, codComercial, codBarras FROM produto WHERE idProduto = :idProduto");
  query.bindValue(":idProduto", ui->itemBoxProduto->getValue());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro buscando produto: " + query.lastError().text());
    return false;
  }

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

  if (not query2.exec()) {
    emit errorSignal("Erro inserindo dados em pedido_fornecedor_has_produto: " + query2.lastError().text());
    return false;
  }

  return true;
}

void WidgetCompraPendentes::on_doubleSpinBoxQuantAvulsoCaixas_valueChanged(const double value) { ui->doubleSpinBoxQuantAvulso->setValue(value * ui->doubleSpinBoxQuantAvulso->singleStep()); }

void WidgetCompraPendentes::on_doubleSpinBoxQuantAvulso_valueChanged(const double value) { ui->doubleSpinBoxQuantAvulsoCaixas->setValue(value / ui->doubleSpinBoxQuantAvulso->singleStep()); }

void WidgetCompraPendentes::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }

void WidgetCompraPendentes::on_pushButtonExcel_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  Excel excel(modelViewVendaProduto.data(list.first().row(), "idVenda").toString());
  excel.gerarExcel();
}

void WidgetCompraPendentes::on_pushButtonPDF_clicked() {
  const auto list = ui->table->selectionModel()->selectedRows();

  if (list.isEmpty()) {
    emit errorSignal("Nenhum item selecionado!");
    return;
  }

  Impressao impressao(modelViewVendaProduto.data(list.first().row(), "idVenda").toString());
  impressao.print();
}

// TODO: [Conrado] quando for vendido produto_estoque marcar status como 'PRÉ-ESTOQUE' ou algo do tipo para o Conrado confirmar, apenas com um botão de ok ou cancelar.
// TODO: pegar a tableResumo e colocar em uma aba separada no widgetCompra
