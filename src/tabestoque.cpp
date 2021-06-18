#include "tabestoque.h"
#include "ui_tabestoque.h"

#include <QDebug>

TabEstoque::TabEstoque(QWidget *parent) : QWidget(parent), ui(new Ui::TabEstoque) {
  ui->setupUi(this);
  setConnections();
}

TabEstoque::~TabEstoque() { delete ui; }

void TabEstoque::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &TabEstoque::on_tabWidget_currentChanged, connectionType);
}

void TabEstoque::on_tabWidget_currentChanged(const int &) { updateTables(); }

void TabEstoque::updateTables() {
  const QString currenTab = ui->tabWidget->tabText(ui->tabWidget->currentIndex());

  if (currenTab == "Estoques") { ui->widgetEstoques->updateTables(); }
  if (currenTab == "Produtos") { ui->widgetProdutos->updateTables(); }
  if (currenTab == "Peso") { ui->widgetPeso->updateTables(); }
}

void TabEstoque::resetTables() {
  ui->widgetEstoques->resetTables();
  ui->widgetProdutos->resetTables();
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
