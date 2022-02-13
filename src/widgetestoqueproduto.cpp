#include "widgetestoqueproduto.h"
#include "ui_widgetestoqueproduto.h"

#include "application.h"
#include "produtoproxymodel.h"

#include <QDebug>

WidgetEstoqueProduto::WidgetEstoqueProduto(QWidget *parent) : QWidget(parent), ui(new Ui::WidgetEstoqueProduto) { ui->setupUi(this); }

WidgetEstoqueProduto::~WidgetEstoqueProduto() { delete ui; }

void WidgetEstoqueProduto::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->lineEditBusca, &LineEdit::delayedTextChanged, this, &WidgetEstoqueProduto::montaFiltro, connectionType);
  connect(ui->radioButtonEstoque, &QRadioButton::toggled, this, &WidgetEstoqueProduto::on_radioButtonEstoque_toggled, connectionType);
  connect(ui->radioButtonStaccatoOFF, &QRadioButton::toggled, this, &WidgetEstoqueProduto::on_radioButtonStaccatoOFF_toggled, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &WidgetEstoqueProduto::on_radioButtonTodos_toggled, connectionType);
}

void WidgetEstoqueProduto::setupTables() {
  modelProdutos.setTable("view_produto");

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.proxyModel = new ProdutoProxyModel(&modelProdutos, this);

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

void WidgetEstoqueProduto::updateTables() {
  if (not isSet) {
    ui->lineEditBusca->setDelayed();
    setupTables();
    setConnections();
    isSet = true;
  }

  modelProdutos.select();
}

void WidgetEstoqueProduto::resetTables() { setupTables(); }

void WidgetEstoqueProduto::montaFiltro() {
  const QString text = ui->lineEditBusca->text();

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE AND (fornecedor LIKE '%" + text + "%' OR descricao LIKE '%" + text + "%' OR codComercial LIKE '%" + text +
                          "%')");
}

void WidgetEstoqueProduto::on_radioButtonTodos_toggled(const bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

void WidgetEstoqueProduto::on_radioButtonStaccatoOFF_toggled(const bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND promocao = 2 AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}

void WidgetEstoqueProduto::on_radioButtonEstoque_toggled(const bool checked) {
  if (not checked) { return; }

  modelProdutos.setFilter("estoque = TRUE AND promocao = 0 AND descontinuado = FALSE AND desativado = FALSE");

  modelProdutos.select();
}
