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
  model.setTable("produto");
  model.setEditStrategy(SqlRelationalTableModel::OnManualSubmit);

  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("descricao", "Descrição");
  model.setHeaderData("estoqueRestante", "Estoque Disp.");
  model.setHeaderData("un", "Un.");
  model.setHeaderData("un2", "Un.2");
  model.setHeaderData("colecao", "Coleção");
  model.setHeaderData("tipo", "Tipo");
  model.setHeaderData("minimo", "Mínimo");
  model.setHeaderData("multiplo", "Múltiplo");
  model.setHeaderData("m2cx", "M/Cx.");
  model.setHeaderData("pccx", "Pç./Cx.");
  model.setHeaderData("kgcx", "Kg./Cx.");
  model.setHeaderData("formComercial", "Form. Com.");
  model.setHeaderData("codComercial", "Cód. Com.");
  model.setHeaderData("precoVenda", "R$");
  model.setHeaderData("validade", "Validade");
  model.setHeaderData("ui", "UI");

  model.setFilter("estoque = TRUE AND estoqueRestante > 0");

  if (not model.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela produto: " + model.lastError().text());

  ui->table->setModel(&model);
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

  for (int column = 0, columnCount = model.columnCount(); column < columnCount; ++column) {
    if (model.record().fieldName(column).endsWith("Upd")) ui->table->setColumnHidden(column, true);
  }

  ui->table->setItemDelegate(new NoEditDelegate(this));
  ui->table->setItemDelegateForColumn("precoVenda", new ReaisDelegate(this));
}

void PrecoEstoque::on_pushButtonSalvar_clicked() {
  if (not model.submitAll()) {
    QMessageBox::critical(this, "Erro!", "Erro salvando dados: " + model.lastError().text());
    return;
  }

  QMessageBox::information(this, "Aviso!", "Dados atualizados!");
  close();
}

void PrecoEstoque::on_pushButtonCancelar_clicked() { close(); }

void PrecoEstoque::on_lineEditBusca_textChanged(const QString &text) {
  model.setFilter(text.isEmpty() ? "estoque = TRUE AND estoqueRestante > 0"
                                 : "MATCH(fornecedor, descricao, codComercial, colecao) AGAINST('+" + text + "*' IN BOOLEAN MODE) AND estoque = TRUE AND estoqueRestante > 0");

  if (not model.select()) QMessageBox::critical(this, "Erro!", "Erro lendo tabela produto: " + model.lastError().text());
}

void PrecoEstoque::on_table_entered(const QModelIndex &) { ui->table->resizeColumnsToContents(); }
