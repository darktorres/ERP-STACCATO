#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

#include "application.h"
#include "cadastrofornecedor.h"
#include "cadastroproduto.h"
#include "ui_cadastroproduto.h"
#include "usersession.h"

CadastroProduto::CadastroProduto(QWidget *parent) : RegisterDialog("produto", "idProduto", parent), ui(new Ui::CadastroProduto) {
  ui->setupUi(this);

  connect(ui->doubleSpinBoxCusto, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastroProduto::on_doubleSpinBoxCusto_valueChanged);
  connect(ui->doubleSpinBoxVenda, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &CadastroProduto::on_doubleSpinBoxVenda_valueChanged);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonAtualizar_clicked);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonCadastrar_clicked);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonNovoCad_clicked);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonRemover_clicked);

  Q_FOREACH (const QLineEdit *line, findChildren<QLineEdit *>()) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  ui->lineEditCodBarras->setInputMask("9999999999999;_");
  ui->lineEditNCM->setInputMask("99999999;_");

  ui->comboBoxOrigem->addItem("0 - Nacional", 0);
  ui->comboBoxOrigem->addItem("1 - Imp. Direta", 1);
  ui->comboBoxOrigem->addItem("2 - Merc. Interno", 2);

  setupMapper();
  newRegister();

  ui->itemBoxFornecedor->setSearchDialog(SearchDialog::fornecedor(this));

  sdProduto = SearchDialog::produto(true, true, this);
  connect(sdProduto, &SearchDialog::itemSelected, this, &CadastroProduto::viewRegisterById);
  connect(ui->pushButtonBuscar, &QAbstractButton::clicked, sdProduto, &SearchDialog::show);

  ui->itemBoxFornecedor->setRegisterDialog(new CadastroFornecedor(this));

  if (UserSession::tipoUsuario() != "ADMINISTRADOR") { ui->pushButtonRemover->setDisabled(true); }

  if (UserSession::tipoUsuario() == "VENDEDOR") {
    ui->pushButtonCadastrar->setVisible(false);
    ui->pushButtonNovoCad->setVisible(false);
  }

  ui->groupBox->hide();
  ui->groupBox_4->hide();
  ui->groupBox_5->hide();
}

CadastroProduto::~CadastroProduto() { delete ui; }

void CadastroProduto::clearFields() {
  Q_FOREACH (const auto &line, findChildren<QLineEdit *>()) { line->clear(); }

  Q_FOREACH (const auto &spinBox, findChildren<QDoubleSpinBox *>()) { spinBox->clear(); }

  ui->itemBoxFornecedor->clear();

  ui->radioButtonDesc->setChecked(false);
  ui->radioButtonLote->setChecked(false);

  ui->dateEditValidade->setDate(QDate(1900, 1, 1));

  ui->textEditObserv->clear();
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
  Q_FOREACH (const auto &line, findChildren<QLineEdit *>()) {
    if (not verifyRequiredField(line)) { return false; }
  }

  if (ui->comboBoxUn->currentText().isEmpty()) {
    qApp->enqueueError("Faltou preencher unidade!", this);
    ui->comboBoxUn->setFocus();
    return false;
  }

  if (ui->dateEditValidade->date().toString("dd-MM-yyyy") == "01-01-1900") {
    qApp->enqueueError("Faltou preencher validade!", this);
    ui->dateEditValidade->setFocus();
    return false;
  }

  if (qFuzzyIsNull(ui->doubleSpinBoxCusto->value())) {
    qApp->enqueueError("Custo inválido!", this);
    ui->doubleSpinBoxCusto->setFocus();
    return false;
  }

  if (qFuzzyIsNull(ui->doubleSpinBoxVenda->value())) {
    qApp->enqueueError("Preço inválido!", this);
    ui->doubleSpinBoxVenda->setFocus();
    return false;
  }

  if (ui->itemBoxFornecedor->getId().isNull()) {
    qApp->enqueueError("Faltou preencher fornecedor!", this);
    ui->itemBoxFornecedor->setFocus();
    return false;
  }

  if (tipo == Tipo::Cadastrar) {
    QSqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE fornecedor = :fornecedor AND codComercial = :codComercial");
    query.bindValue(":fornecedor", ui->itemBoxFornecedor->text());
    query.bindValue(":codComercial", ui->lineEditCodComer->text());

    if (not query.exec()) { return qApp->enqueueError(false, "Erro verificando se produto já cadastrado!", this); }

    if (query.first()) { return qApp->enqueueError(false, "Código comercial já cadastrado!", this); }
  }

  return true;
}

void CadastroProduto::setupMapper() {
  addMapping(ui->comboBoxCST, "cst");
  addMapping(ui->comboBoxOrigem, "origem");
  addMapping(ui->comboBoxUn, "un");
  addMapping(ui->dateEditValidade, "validade");
  addMapping(ui->doubleSpinBoxComissao, "comissao");
  addMapping(ui->doubleSpinBoxCusto, "custo");
  addMapping(ui->doubleSpinBoxEstoque, "estoqueRestante");
  addMapping(ui->doubleSpinBoxIPI, "ipi");
  addMapping(ui->doubleSpinBoxKgCx, "kgcx");
  addMapping(ui->doubleSpinBoxM2Cx, "m2cx");
  addMapping(ui->doubleSpinBoxMarkup, "markup");
  addMapping(ui->doubleSpinBoxPcCx, "pccx");
  addMapping(ui->doubleSpinBoxQtePallet, "qtdPallet");
  addMapping(ui->doubleSpinBoxST, "st");
  addMapping(ui->doubleSpinBoxVenda, "precoVenda");
  addMapping(ui->itemBoxFornecedor, "idFornecedor", "id");
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

void CadastroProduto::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Produto cadastrado com sucesso!", this); }

bool CadastroProduto::savingProcedures() {
  if (not setData("codBarras", ui->lineEditCodBarras->text())) { return false; }
  if (not setData("codComercial", ui->lineEditCodComer->text())) { return false; }
  if (not setData("colecao", ui->lineEditColecao->text())) { return false; }
  if (not setData("comissao", ui->doubleSpinBoxComissao->value())) { return false; }
  if (not setData("cst", ui->comboBoxCST->currentText())) { return false; }
  if (not setData("custo", ui->doubleSpinBoxCusto->value())) { return false; }
  if (not setData("descricao", ui->lineEditDescricao->text())) { return false; }
  if (not setData("estoqueRestante", ui->doubleSpinBoxEstoque->value())) { return false; }
  if (not setData("formComercial", ui->lineEditFormComer->text())) { return false; }
  if (not setData("Fornecedor", ui->itemBoxFornecedor->text())) { return false; }
  if (not setData("icms", ui->lineEditICMS->text())) { return false; }
  if (not setData("idFornecedor", ui->itemBoxFornecedor->getId())) { return false; }
  if (not setData("ipi", ui->doubleSpinBoxIPI->value())) { return false; }
  if (not setData("kgcx", ui->doubleSpinBoxKgCx->value())) { return false; }
  if (not setData("m2cx", ui->doubleSpinBoxM2Cx->value())) { return false; }
  if (not setData("markup", ui->doubleSpinBoxMarkup->value())) { return false; }
  if (not setData("ncm", ui->lineEditNCM->text())) { return false; }
  if (not setData("observacoes", ui->textEditObserv->toPlainText())) { return false; }
  if (not setData("origem", ui->comboBoxOrigem->currentData())) { return false; }
  if (not setData("pccx", ui->doubleSpinBoxPcCx->value())) { return false; }
  if (not setData("precoVenda", ui->doubleSpinBoxVenda->value())) { return false; }
  if (not setData("qtdPallet", ui->doubleSpinBoxQtePallet->value())) { return false; }
  if (not setData("st", ui->doubleSpinBoxST->value())) { return false; }
  if (not setData("temLote", ui->radioButtonLote->isChecked() ? "SIM" : "NÃO")) { return false; }
  if (not setData("ui", ui->lineEditUI->text().isEmpty() ? "0" : ui->lineEditUI->text())) { return false; }
  if (not setData("un", ui->comboBoxUn->currentText())) { return false; }
  if (not setData("validade", ui->dateEditValidade->date())) { return false; }

  QSqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE idFornecedor = :idFornecedor");
  query.bindValue(":idFornecedor", ui->itemBoxFornecedor->getId());

  if (not query.exec() or not query.first()) { return qApp->enqueueError(false, "Erro verificando se fornecedor é representacao: " + query.lastError().text(), this); }

  const bool representacao = query.value("representacao").toBool();

  if (not setData("representacao", representacao)) { return false; }
  if (not setData("descontinuado", ui->dateEditValidade->date() < QDate::currentDate())) { return false; }

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
  if (tipo == Tipo::Cadastrar) {
    currentRow = model.rowCount();
    model.insertRow(currentRow);
  }

  if (not savingProcedures()) { return false; }

  if (not columnsToUpper(model, currentRow)) { return false; }

  if (not model.submitAll()) { return false; }

  primaryId = (tipo == Tipo::Atualizar) ? data(currentRow, primaryKey).toString() : model.query().lastInsertId().toString();

  if (primaryId.isEmpty()) { return qApp->enqueueError(false, "Id vazio!", this); }

  return true;
}

// TODO: 3poder alterar nesta tela a quantidade minima/multiplo dos produtos
// REFAC: 5verificar se estou usando corretamente a tabela 'produto_has_preco'
// me parece que ela só é preenchida na importacao de tabela e nao na modificacao manual de produtos
// REFAC: 4verificar se posso remover 'un2' de produto
// TODO: colocar logica para trabalhar a tabela produto_has_preco para que os produtos nao sejam descontinuados com validade ativa
// TODO: validar entrada do campo icms para apenas numeros
// REFAC: verificar para que era usado o campo 'un2' e remove-lo caso nao seja mais usado
// TODO: verificar se vendedor deve mesmo poder alterar cadastro do produto
