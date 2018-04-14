#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

#include "noeditdelegate.h"
#include "precoestoque.h"
#include "reaisdelegate.h"
#include "ui_precoestoque.h"

PrecoEstoque::PrecoEstoque(QWidget *parent) : Dialog(parent), ui(new Ui::PrecoEstoque) {
  ui->setupUi(this);

  connect(ui->lineEditBusca, &QLineEdit::textChanged, this, &PrecoEstoque::on_lineEditBusca_textChanged);
  connect(ui->pushButtonCancelar, &QPushButton::clicked, this, &PrecoEstoque::on_pushButtonCancelar_clicked);
  connect(ui->pushButtonSalvar, &QPushButton::clicked, this, &PrecoEstoque::on_pushButtonSalvar_clicked);
  connect(ui->table, &TableView::entered, this, &PrecoEstoque::on_table_entered);

  setWindowFlags(Qt::Window);

  setupTables();
}

PrecoEstoque::~PrecoEstoque() { delete ui; }

void PrecoEstoque::setupTables() {
  modelProduto.setTable("produto");
  modelProduto.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  modelProduto.setHeaderData("fornecedor", "Fornecedor");
  modelProduto.setHeaderData("descricao", "Descrição");
  modelProduto.setHeaderData("estoqueRestante", "Estoque Disp.");
  modelProduto.setHeaderData("un", "Un.");
  modelProduto.setHeaderData("un2", "Un.2");
  modelProduto.setHeaderData("colecao", "Coleção");
  modelProduto.setHeaderData("tipo", "Tipo");
  modelProduto.setHeaderData("minimo", "Mínimo");
  modelProduto.setHeaderData("multiplo", "Múltiplo");
  modelProduto.setHeaderData("m2cx", "M/Cx.");
  modelProduto.setHeaderData("pccx", "Pç./Cx.");
  modelProduto.setHeaderData("kgcx", "Kg./Cx.");
  modelProduto.setHeaderData("formComercial", "Form. Com.");
  modelProduto.setHeaderData("codComercial", "Cód. Com.");
  modelProduto.setHeaderData("precoVenda", "R$");
  modelProduto.setHeaderData("validade", "Validade");
  modelProduto.setHeaderData("ui", "UI");

  modelProduto.setFilter("estoque = TRUE AND estoqueRestante > 0");

  if (not modelProduto.select()) { return; }

  ui->table->setModel(&modelProduto);
  ui->table->hideColumn("idEstoque");
  ui->table->hideColumn("atualizarTabelaPreco");
  ui->table->hideColumn("cfop");
  ui->table->hideColumn("codBarras");
  ui->table->hideColumn("comissao");
  ui->table->hideColumn("cst");
  ui->table->hideColumn("custo");
  ui->table->hideColumn("minimo");
  ui->table->hideColumn("multiplo");
  ui->table->hideColumn("desativado");
  ui->table->hideColumn("descontinuado");
  ui->table->hideColumn("estoque");
  ui->table->hideColumn("promocao");
  ui->table->hideColumn("icms");
  ui->table->hideColumn("idFornecedor");
  ui->table->hideColumn("idProduto");
  ui->table->hideColumn("idProdutoRelacionado");
  ui->table->hideColumn("ipi");
  ui->table->hideColumn("markup");
  ui->table->hideColumn("ncm");
  ui->table->hideColumn("ncmEx");
  ui->table->hideColumn("observacoes");
  ui->table->hideColumn("origem");
  ui->table->hideColumn("qtdPallet");
  ui->table->hideColumn("representacao");
  ui->table->hideColumn("st");
  ui->table->hideColumn("temLote");

  for (int column = 0, columnCount = modelProduto.columnCount(); column < columnCount; ++column) {
    if (modelProduto.record().fieldName(column).endsWith("Upd")) { ui->table->setColumnHidden(column, true); }
  }

  ui->table->setItemDelegate(new NoEditDelegate(this));
  ui->table->setItemDelegateForColumn("precoVenda", new ReaisDelegate(this));
}

void PrecoEstoque::on_pushButtonSalvar_clicked() {
  if (not modelProduto.submitAll()) { return; }

  emit informationSignal("Dados atualizados!");
  close();
}

void PrecoEstoque::on_pushButtonCancelar_clicked() { close(); }

void PrecoEstoque::on_lineEditBusca_textChanged(const QString &text) {
  modelProduto.setFilter(text.isEmpty() ? "estoque = TRUE AND estoqueRestante > 0"
                                        : "MATCH(fornecedor, descricao, codComercial, colecao) AGAINST('+" + text + "*' IN BOOLEAN MODE) AND estoque = TRUE AND estoqueRestante > 0");

  if (not modelProduto.select()) { return; }
}

void PrecoEstoque::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }
