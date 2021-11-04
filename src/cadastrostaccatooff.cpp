#include "cadastrostaccatooff.h"
#include "ui_cadastroStaccatoOff.h"

#include "application.h"
#include "produtoproxymodel.h"
#include "sqlquery.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlRecord>

CadastroStaccatoOff::CadastroStaccatoOff(QWidget *parent) : QDialog(parent), ui(new Ui::CadastroStaccatoOff) {
  ui->setupUi(this);

  setWindowTitle("Gerenciar promoção");
  setWindowFlags(Qt::Window);

  ui->itemBoxFornecedor->setSearchDialog(SearchDialog::fornecedor(this));

  ui->dateEditValidade->setDate(qApp->serverDate());

  setupTables();

  setConnections();
}

CadastroStaccatoOff::~CadastroStaccatoOff() { delete ui; }

void CadastroStaccatoOff::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->itemBoxFornecedor, &ItemBox::textChanged, this, &CadastroStaccatoOff::on_itemBoxFornecedor_textChanged, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroStaccatoOff::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonDescadastrar, &QPushButton::clicked, this, &CadastroStaccatoOff::on_pushButtonDescadastrar_clicked, connectionType);
  connect(ui->pushButtonLimparFiltroFornecedor, &QPushButton::clicked, this, &CadastroStaccatoOff::on_pushButtonLimparFiltroFornecedor_clicked, connectionType);
  connect(ui->radioButtonEstoque, &QRadioButton::toggled, this, &CadastroStaccatoOff::on_radioButtonEstoque_toggled, connectionType);
  connect(ui->radioButtonStaccatoOFF, &QRadioButton::toggled, this, &CadastroStaccatoOff::on_radioButtonStaccatoOFF_toggled, connectionType);
  connect(ui->radioButtonTodos, &QRadioButton::toggled, this, &CadastroStaccatoOff::on_radioButtonTodos_toggled, connectionType);
}

void CadastroStaccatoOff::setupTables() {
  model.setTable("view_produto");

  model.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  model.select();

  model.setHeaderData("fornecedor", "Fornecedor");
  model.setHeaderData("statusEstoque", "Estoque");
  model.setHeaderData("descricao", "Descrição");
  model.setHeaderData("estoqueRestante", "Estoque Disp.");
  model.setHeaderData("estoqueCaixa", "Estoque Cx.");
  model.setHeaderData("lote", "Lote");
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

  model.proxyModel = new ProdutoProxyModel(&model, this);

  ui->tableView->setModel(&model);

  ui->tableView->hideColumn("idProduto");
  ui->tableView->hideColumn("estoque");
  ui->tableView->hideColumn("promocao");
  ui->tableView->hideColumn("descontinuado");
  ui->tableView->hideColumn("desativado");
  ui->tableView->hideColumn("representacao");

  for (int column = 0, columnCount = model.columnCount(); column < columnCount; ++column) {
    if (model.record().fieldName(column).endsWith("Upd")) { ui->tableView->setColumnHidden(column, true); }
  }
}

void CadastroStaccatoOff::on_itemBoxFornecedor_textChanged(const QString &text) {
  model.setFilter("fornecedor = '" + text + "' AND estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");

  model.select();
}

void CadastroStaccatoOff::on_radioButtonTodos_toggled(const bool checked) {
  if (not checked) { return; }

  const QString fornecedor = ui->itemBoxFornecedor->text().isEmpty() ? "" : " AND fornecedor = '" + ui->itemBoxFornecedor->text() + "'";

  model.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE" + fornecedor);

  model.select();
}

void CadastroStaccatoOff::on_radioButtonStaccatoOFF_toggled(const bool checked) {
  if (not checked) { return; }

  const QString fornecedor = ui->itemBoxFornecedor->text().isEmpty() ? "" : " AND fornecedor = '" + ui->itemBoxFornecedor->text() + "'";

  model.setFilter("estoque = TRUE AND promocao = 2 AND descontinuado = FALSE AND desativado = FALSE" + fornecedor);

  model.select();
}

void CadastroStaccatoOff::on_radioButtonEstoque_toggled(const bool checked) {
  if (not checked) { return; }

  const QString fornecedor = ui->itemBoxFornecedor->text().isEmpty() ? "" : " AND fornecedor = '" + ui->itemBoxFornecedor->text() + "'";

  model.setFilter("estoque = TRUE AND promocao = 0 AND descontinuado = FALSE AND desativado = FALSE" + fornecedor);

  model.select();
}

void CadastroStaccatoOff::on_pushButtonCadastrar_clicked() {
  auto list = ui->tableView->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  if (qFuzzyIsNull(ui->doubleSpinBoxDesconto->value())) { throw RuntimeError("Selecione um desconto!", this); }

  for (auto index : list) {
    if (model.data(index.row(), "promocao").toInt() == 2) { throw RuntimeError("Linha com promoção selecionada!", this); }
  }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja cadastrar promoção?", QMessageBox::Yes | QMessageBox::Cancel, this);
  msgBox.setButtonText(QMessageBox::Yes, "Cadastrar");
  msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

  const int escolha = msgBox.exec();

  if (escolha == QMessageBox::Cancel) { return; }

  //--------------------------------------------

  for (auto index : list) {
    const QString idProduto = model.data(index.row(), "idProduto").toString();

    SqlQuery query;

    if (not query.exec("UPDATE produto SET oldPrecoVenda = precoVenda, precoVenda = precoVenda * " + QString::number(1 - (ui->doubleSpinBoxDesconto->value() / 100)) +
                       ", promocao = 2, descricao = CONCAT(descricao, ' (PROMOÇÃO STACCATO OFF " + QString::number(ui->doubleSpinBoxDesconto->value()) + "%)'), validade = '" +
                       ui->dateEditValidade->date().toString("yyyy-MM-dd") + "' WHERE idProduto = " + idProduto)) {
      throw RuntimeError("Erro alterando estoque: " + query.lastError().text(), this);
    }
  }

  model.select();
  qApp->enqueueInformation("Dados salvos com sucesso!", this);
}

void CadastroStaccatoOff::on_pushButtonDescadastrar_clicked() {
  auto list = ui->tableView->selectionModel()->selectedRows();

  if (list.isEmpty()) { throw RuntimeError("Nenhuma linha selecionada!", this); }

  for (auto index : list) {
    if (model.data(index.row(), "promocao").toInt() != 2) { throw RuntimeError("Linha sem promoção selecionada!", this); }
  }

  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja descadastrar promoção?", QMessageBox::Yes | QMessageBox::Cancel, this);
  msgBox.setButtonText(QMessageBox::Yes, "Descadastrar");
  msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

  const int escolha = msgBox.exec();

  if (escolha == QMessageBox::Cancel) { return; }

  //--------------------------------------------

  for (auto index : list) {
    const QString idProduto = model.data(index.row(), "idProduto").toString();

    SqlQuery query;

    if (not query.exec(
            "UPDATE produto SET precoVenda = oldPrecoVenda, oldPrecoVenda = NULL, promocao = 0, descricao = LEFT(descricao, CHAR_LENGTH(descricao) - 28), validade = NULL WHERE idProduto = " +
            idProduto)) {
      throw RuntimeError("Erro alterando estoque: " + query.lastError().text(), this);
    }
  }

  model.select();
  qApp->enqueueInformation("Dados salvos com sucesso!", this);
}

void CadastroStaccatoOff::on_pushButtonLimparFiltroFornecedor_clicked() {
  ui->itemBoxFornecedor->clear();

  model.setFilter("estoque = TRUE AND descontinuado = FALSE AND desativado = FALSE");
}
