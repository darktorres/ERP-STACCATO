#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "cadastrofornecedor.h"
#include "cadastroproduto.h"
#include "ui_cadastroproduto.h"
#include "usersession.h"

CadastroProduto::CadastroProduto(QWidget *parent) : RegisterDialog("produto", "idProduto", parent), ui(new Ui::CadastroProduto) {
  ui->setupUi(this);

  ui->lineEditCodBarras->setInputMask("9999999999999;_");
  ui->lineEditNCM->setInputMask("99999999;_");

  ui->comboBoxOrigem->addItem("0 - Nacional", 0);
  ui->comboBoxOrigem->addItem("1 - Imp. Direta", 1);
  ui->comboBoxOrigem->addItem("2 - Merc. Interno", 2);

  setupMapper();
  newRegister();

  ui->itemBoxFornecedor->setSearchDialog(SearchDialog::fornecedor(this));

  sdProduto = SearchDialog::produto(true, this);
  connect(sdProduto, &SearchDialog::itemSelected, this, &CadastroProduto::viewRegisterById);
  connect(ui->pushButtonBuscar, &QAbstractButton::clicked, sdProduto, &SearchDialog::show);

  ui->itemBoxFornecedor->setRegisterDialog(new CadastroFornecedor(this));

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") ui->pushButtonRemover->setDisabled(true);

  if (UserSession::tipoUsuario() == "VENDEDOR") {
    ui->pushButtonCadastrar->setVisible(false);
    ui->pushButtonNovoCad->setVisible(false);
  }

  ui->groupBox->hide();
  ui->groupBox_4->hide();
  ui->groupBox_5->hide();

  //  model.setEditStrategy(QSqlTableModel::OnRowChange); // for avoiding reloading the entire table

  for (const QLineEdit *line : findChildren<QLineEdit *>()) connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty);
}

CadastroProduto::~CadastroProduto() { delete ui; }

bool CadastroProduto::viewRegister() {
  if (not RegisterDialog::viewRegister()) return false;

  // TODO: implement necessary stuff here

  return true;
}

void CadastroProduto::clearFields() {
  for (const auto &line : findChildren<QLineEdit *>()) line->clear();

  for (const auto &spinBox : findChildren<QDoubleSpinBox *>()) spinBox->clear();

  ui->radioButtonDesc->setChecked(false);
  ui->radioButtonLote->setChecked(false);

  ui->dateEditValidade->setDate(QDate(1900, 1, 1));
}

void CadastroProduto::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonRemover->show();
}

void CadastroProduto::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonRemover->hide();
}

bool CadastroProduto::verifyFields() {
  for (const auto &line : findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) return false;
  }

  if (ui->comboBoxUn->currentText().isEmpty()) {
    ui->comboBoxUn->setFocus();
    QMessageBox::critical(this, "Erro!", "Faltou preencher unidade!");
    return false;
  }

  if (ui->dateEditValidade->date().toString("dd-MM-yyyy") == "01-01-1900") {
    ui->dateEditValidade->setFocus();
    QMessageBox::critical(this, "Erro!", "Faltou preencher validade!");
    return false;
  }

  if (ui->doubleSpinBoxCusto->value() == 0.) {
    ui->doubleSpinBoxCusto->setFocus();
    QMessageBox::critical(this, "Erro!", "Custo inválido!");
    return false;
  }

  if (ui->doubleSpinBoxVenda->value() == 0.) {
    ui->doubleSpinBoxVenda->setFocus();
    QMessageBox::critical(this, "Erro!", "Preço inválido!");
    return false;
  }

  if (ui->itemBoxFornecedor->getValue().isNull()) {
    ui->itemBoxFornecedor->setFocus();
    QMessageBox::critical(this, "Erro!", "Faltou preencher fornecedor!");
    return false;
  }

  if (ui->lineEditICMS->text().isEmpty()) {
    ui->lineEditICMS->setFocus();
    QMessageBox::critical(this, "Erro!", "Faltou preencher ICMS!");
    return false;
  }

  if (ui->lineEditCodComer->text().isEmpty()) {
    ui->lineEditCodComer->setFocus();
    QMessageBox::critical(this, "Erro!", "Faltou preencher Código comercial!");
    return false;
  }

  if (tipo == Tipo::Cadastrar) {
    QSqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE fornecedor = :fornecedor AND codComercial = :codComercial");
    query.bindValue(":fornecedor", ui->itemBoxFornecedor->text());
    query.bindValue(":codComercial", ui->lineEditCodComer->text());

    if (not query.exec()) {
      QMessageBox::critical(this, "Erro!", "Erro verificando se produto já cadastrado!");
      return false;
    }

    if (query.first()) {
      QMessageBox::critical(this, "Erro!", "Código comercial já cadastrado!");
      return false;
    }
  }

  return true;
}

void CadastroProduto::setupMapper() {
  addMapping(ui->comboBoxCST, "cst");
  addMapping(ui->comboBoxOrigem, "origem");
  addMapping(ui->comboBoxUn, "un");
  addMapping(ui->dateEditValidade, "validade");
  addMapping(ui->doubleSpinBoxComissao, "comissao", "value");
  addMapping(ui->doubleSpinBoxCusto, "custo", "value");
  addMapping(ui->doubleSpinBoxEstoque, "estoqueRestante");
  addMapping(ui->doubleSpinBoxIPI, "ipi", "value");
  addMapping(ui->doubleSpinBoxKgCx, "kgcx");
  addMapping(ui->doubleSpinBoxM2Cx, "m2cx", "value");
  addMapping(ui->doubleSpinBoxMarkup, "markup", "value");
  addMapping(ui->doubleSpinBoxPcCx, "pccx");
  addMapping(ui->doubleSpinBoxQtePallet, "qtdPallet", "value");
  addMapping(ui->doubleSpinBoxST, "st", "value");
  addMapping(ui->doubleSpinBoxVenda, "precoVenda", "value");
  addMapping(ui->itemBoxFornecedor, "idFornecedor", "value");
  addMapping(ui->lineEditCodBarras, "codBarras");
  addMapping(ui->lineEditCodComer, "codComercial");
  addMapping(ui->lineEditColecao, "colecao");
  addMapping(ui->lineEditDescricao, "descricao");
  addMapping(ui->lineEditFormComer, "formComercial");
  addMapping(ui->lineEditICMS, "icms");
  addMapping(ui->lineEditNCM, "ncm");
  addMapping(ui->lineEditUI, "ui");
  addMapping(ui->radioButtonDesc, "descontinuado");
  addMapping(ui->radioButtonLote, "temLote");
  addMapping(ui->textEditObserv, "observacoes", "plainText");
  addMapping(ui->checkBoxEstoque, "estoque");
  addMapping(ui->checkBoxPromocao, "promocao");
}

void CadastroProduto::successMessage() { QMessageBox::information(this, "Atenção!", tipo == Tipo::Atualizar ? "Cadastro atualizado!" : "Produto cadastrado com sucesso!"); }

bool CadastroProduto::savingProcedures() {
  if (not setData("codBarras", ui->lineEditCodBarras->text())) return false;
  if (not setData("codComercial", ui->lineEditCodComer->text())) return false;
  if (not setData("colecao", ui->lineEditColecao->text())) return false;
  if (not setData("comissao", ui->doubleSpinBoxComissao->value())) return false;
  if (not setData("cst", ui->comboBoxCST->currentText())) return false;
  if (not setData("custo", ui->doubleSpinBoxCusto->value())) return false;
  if (not setData("descricao", ui->lineEditDescricao->text())) return false;
  if (not setData("estoqueRestante", ui->doubleSpinBoxEstoque->value())) return false;
  if (not setData("formComercial", ui->lineEditFormComer->text())) return false;
  if (not setData("Fornecedor", ui->itemBoxFornecedor->text())) return false;
  if (not setData("icms", ui->lineEditICMS->text())) return false;
  if (not setData("idFornecedor", ui->itemBoxFornecedor->getValue())) return false;
  if (not setData("ipi", ui->doubleSpinBoxIPI->value())) return false;
  if (not setData("kgcx", ui->doubleSpinBoxKgCx->value())) return false;
  if (not setData("m2cx", ui->doubleSpinBoxM2Cx->value())) return false;
  if (not setData("markup", ui->doubleSpinBoxMarkup->value())) return false;
  if (not setData("ncm", ui->lineEditNCM->text())) return false;
  if (not setData("observacoes", ui->textEditObserv->toPlainText())) return false;
  if (not setData("origem", ui->comboBoxOrigem->currentData())) return false;
  if (not setData("pccx", ui->doubleSpinBoxPcCx->value())) return false;
  if (not setData("precoVenda", ui->doubleSpinBoxVenda->value())) return false;
  if (not setData("qtdPallet", ui->doubleSpinBoxQtePallet->value())) return false;
  if (not setData("st", ui->doubleSpinBoxST->value())) return false;
  if (not setData("temLote", ui->radioButtonLote->isChecked() ? "SIM" : "NÃO")) return false;
  if (not setData("ui", ui->lineEditUI->text().isEmpty() ? "0" : ui->lineEditUI->text())) return false;
  if (not setData("un", ui->comboBoxUn->currentText())) return false;
  if (not setData("validade", ui->dateEditValidade->date())) return false;

  QSqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE idFornecedor = :idFornecedor");
  query.bindValue(":idFornecedor", ui->itemBoxFornecedor->getValue());

  if (not query.exec() or not query.first()) {
    emit errorSignal("Erro verificando se fornecedor é representacao: " + query.lastError().text());
    return false;
  }

  const bool representacao = query.value("representacao").toBool();

  if (not setData("representacao", representacao)) return false;
  if (not setData("descontinuado", ui->dateEditValidade->date() < QDate::currentDate())) return false;

  return true;
}

void CadastroProduto::on_pushButtonCadastrar_clicked() { save(); }

void CadastroProduto::on_pushButtonAtualizar_clicked() { save(); }

void CadastroProduto::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroProduto::on_pushButtonRemover_clicked() { remove(); }

void CadastroProduto::on_doubleSpinBoxVenda_valueChanged(const double &) { calcularMarkup(); }

void CadastroProduto::on_doubleSpinBoxCusto_valueChanged(const double &) { calcularMarkup(); }

void CadastroProduto::calcularMarkup() {
  const double markup = ((ui->doubleSpinBoxVenda->value() / ui->doubleSpinBoxCusto->value()) - 1.) * 100.;
  ui->doubleSpinBoxMarkup->setValue(markup);
}

bool CadastroProduto::cadastrar() {
  currentRow = tipo == Tipo::Atualizar ? mapper.currentIndex() : model.rowCount();

  if (currentRow == -1) {
    emit errorSignal("Erro linha -1");
    return false;
  }

  if (tipo == Tipo::Cadastrar and not model.insertRow(currentRow)) {
    emit errorSignal("Erro inserindo linha na tabela: " + model.lastError().text());
    return false;
  }

  if (not savingProcedures()) return false;

  for (int column = 0; column < model.rowCount(); ++column) {
    const QVariant dado = model.data(currentRow, column);
    if (dado.type() == QVariant::String) {
      if (not model.setData(currentRow, column, dado.toString().toUpper())) return false;
    }
  }

  if (not model.submitAll()) {
    emit errorSignal("Erro salvando dados na tabela " + model.tableName() + ": " + model.lastError().text());
    return false;
  }

  primaryId = data(currentRow, primaryKey).isValid() ? data(currentRow, primaryKey).toString() : getLastInsertId().toString();

  if (primaryId.isEmpty()) {
    emit errorSignal("Id vazio!");
    return false;
  }

  return true;
}

bool CadastroProduto::save() {
  if (not verifyFields()) return false;

  emit transactionStarted();

  QSqlQuery("SET SESSION TRANSACTION ISOLATION LEVEL SERIALIZABLE").exec();
  QSqlQuery("START TRANSACTION").exec();

  if (not cadastrar()) {
    QSqlQuery("ROLLBACK").exec();
    emit transactionEnded();
    return false;
  }

  QSqlQuery("COMMIT").exec();

  emit transactionEnded();

  isDirty = false;

  viewRegisterById(primaryId);

  successMessage();

  return true;
}

// TODO: 3poder alterar nesta tela a quantidade minima/multiplo dos produtos
// TODO: 5verificar se estou usando corretamente a tabela 'produto_has_preco'
// me parece que ela só é preenchida na importacao de tabela e nao na modificacao manual de produtos
// TODO: 4verificar se posso remover 'un2' de produto
// TODO: colocar logica para trabalhar a tabela produto_has_preco para que os produtos nao sejam descontinuados com validade ativa
