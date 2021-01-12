#include "cadastroproduto.h"
#include "ui_cadastroproduto.h"

#include "application.h"
#include "cadastrofornecedor.h"
#include "usersession.h"

#include <QDebug>
#include <QMessageBox>
#include <QSqlError>

CadastroProduto::CadastroProduto(QWidget *parent) : RegisterDialog("produto", "idProduto", parent), ui(new Ui::CadastroProduto) {
  ui->setupUi(this);

  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const QLineEdit *line : children) { connect(line, &QLineEdit::textEdited, this, &RegisterDialog::marcarDirty); }

  setupUi();

  ui->comboBoxOrigem->addItem("0 - Nacional", 0);
  ui->comboBoxOrigem->addItem("1 - Imp. Direta", 1);
  ui->comboBoxOrigem->addItem("2 - Merc. Interno", 2);

  setupMapper();
  newRegister();

  ui->itemBoxFornecedor->setSearchDialog(SearchDialog::fornecedor(this));

  sdProduto = SearchDialog::produto(true, true, true, false, this);
  connect(sdProduto, &SearchDialog::itemSelected, this, &CadastroProduto::viewRegisterById);
  connect(ui->pushButtonBuscar, &QAbstractButton::clicked, sdProduto, &SearchDialog::show);

  ui->itemBoxFornecedor->setRegisterDialog(new CadastroFornecedor(this));

  if (UserSession::tipoUsuario != "ADMINISTRADOR" and UserSession::tipoUsuario != "ADMINISTRATIVO") { ui->pushButtonRemover->setDisabled(true); }

  setConnections();
}

CadastroProduto::~CadastroProduto() { delete ui; }

void CadastroProduto::clearFields() {
  unsetConnections();

  try {
    RegisterDialog::clearFields();

    ui->comboBoxCST->setCurrentIndex(7);
    ui->dateEditValidade->setDate(QDate(1900, 1, 1));
    ui->textEditObserv->clear();

    setupUi();
  } catch (std::exception &e) {}

  setConnections();
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

void CadastroProduto::verifyFields() {
  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { verifyRequiredField(*line); }

  if (ui->comboBoxUn->currentText().isEmpty()) { throw RuntimeError("Faltou preencher unidade!"); }

  if (ui->dateEditValidade->date().toString("dd-MM-yyyy") == "01-01-1900") { throw RuntimeError("Faltou preencher validade!"); }

  if (qFuzzyIsNull(ui->doubleSpinBoxCusto->value())) { throw RuntimeError("Custo inválido!"); }

  if (qFuzzyIsNull(ui->doubleSpinBoxVenda->value())) { throw RuntimeError("Preço inválido!"); }

  if (ui->itemBoxFornecedor->getId().isNull()) { throw RuntimeError("Faltou preencher fornecedor!"); }

  if (tipo == Tipo::Cadastrar) {
    SqlQuery query;
    query.prepare("SELECT idProduto FROM produto WHERE fornecedor = :fornecedor AND codComercial = :codComercial");
    query.bindValue(":fornecedor", ui->itemBoxFornecedor->text());
    query.bindValue(":codComercial", ui->lineEditCodComer->text());

    if (not query.exec()) { throw RuntimeException("Erro verificando se produto já cadastrado!"); }

    if (query.first()) { throw RuntimeError("Código comercial já cadastrado!"); }
  }
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
  addMapping(ui->textEditObserv, "observacoes", "plainText");
  addMapping(ui->checkBoxEstoque, "estoque");
  addMapping(ui->checkBoxPromocao, "promocao");
}

void CadastroProduto::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Produto cadastrado com sucesso!", this); }

void CadastroProduto::savingProcedures() {
  // TODO: verificar aonde estou salvando 'estoque'/'promocao' e não deixar marcar os 2

  setData("codBarras", ui->lineEditCodBarras->text());
  setData("codComercial", ui->lineEditCodComer->text());
  setData("colecao", ui->lineEditColecao->text());
  setData("comissao", ui->doubleSpinBoxComissao->value());
  setData("cst", ui->comboBoxCST->currentText());
  setData("custo", ui->doubleSpinBoxCusto->value());
  setData("descricao", ui->lineEditDescricao->text());
  setData("estoqueRestante", ui->doubleSpinBoxEstoque->value());
  setData("formComercial", ui->lineEditFormComer->text());
  setData("Fornecedor", ui->itemBoxFornecedor->text().split(" - ").first());
  setData("icms", ui->lineEditICMS->text());
  setData("idFornecedor", ui->itemBoxFornecedor->getId());
  setData("ipi", ui->doubleSpinBoxIPI->value());
  setData("kgcx", ui->doubleSpinBoxKgCx->value());
  setData("m2cx", ui->doubleSpinBoxM2Cx->value());
  setData("markup", ui->doubleSpinBoxMarkup->value());
  setData("ncm", ui->lineEditNCM->text());
  setData("observacoes", ui->textEditObserv->toPlainText());
  setData("origem", ui->comboBoxOrigem->currentData());
  setData("pccx", ui->doubleSpinBoxPcCx->value());
  setData("precoVenda", ui->doubleSpinBoxVenda->value());
  setData("qtdPallet", ui->doubleSpinBoxQtePallet->value());
  setData("st", ui->doubleSpinBoxST->value());
  setData("ui", ui->lineEditUI->text().isEmpty() ? "0" : ui->lineEditUI->text());

  const QString un = ui->comboBoxUn->currentText();
  const double m2cx = ui->doubleSpinBoxM2Cx->value();
  const double pccx = ui->doubleSpinBoxPcCx->value();
  const double quantCaixa = (un == "M2" or un == "M²" or un == "ML") ? m2cx : pccx;

  setData("quantCaixa", quantCaixa);
  setData("un", ui->comboBoxUn->currentText());
  setData("validade", ui->checkBoxValidade->isChecked() ? ui->dateEditValidade->date() : QVariant());

  SqlQuery query;
  query.prepare("SELECT representacao FROM fornecedor WHERE idFornecedor = :idFornecedor");
  query.bindValue(":idFornecedor", ui->itemBoxFornecedor->getId());

  if (not query.exec() or not query.first()) { throw RuntimeException("Erro verificando se fornecedor é representacao: " + query.lastError().text()); }

  const bool representacao = query.value("representacao").toBool();

  setData("representacao", representacao);
  setData("descontinuado", ui->dateEditValidade->date() < qApp->serverDate());
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

void CadastroProduto::cadastrar() {
  try {
    qApp->startTransaction("CadastroProduto::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    qApp->endTransaction();
  } catch (std::exception &e) {
    qApp->rollbackTransaction();
    model.select();

    throw;
  }
}

void CadastroProduto::on_checkBoxValidade_stateChanged(const int state) { ui->dateEditValidade->setEnabled(state); }

void CadastroProduto::setupUi() {
  ui->lineEditCodBarras->setInputMask("9999999999999;_");
  ui->lineEditNCM->setInputMask("99999999;_");
}

void CadastroProduto::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(ui->checkBoxValidade, &QCheckBox::stateChanged, this, &CadastroProduto::on_checkBoxValidade_stateChanged, connectionType);
  connect(ui->doubleSpinBoxCusto, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastroProduto::on_doubleSpinBoxCusto_valueChanged, connectionType);
  connect(ui->doubleSpinBoxVenda, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastroProduto::on_doubleSpinBoxVenda_valueChanged, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonAtualizar_clicked, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonNovoCad_clicked, connectionType);
  connect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonRemover_clicked, connectionType);
}

void CadastroProduto::unsetConnections() {
  disconnect(ui->checkBoxValidade, &QCheckBox::stateChanged, this, &CadastroProduto::on_checkBoxValidade_stateChanged);
  disconnect(ui->doubleSpinBoxCusto, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastroProduto::on_doubleSpinBoxCusto_valueChanged);
  disconnect(ui->doubleSpinBoxVenda, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &CadastroProduto::on_doubleSpinBoxVenda_valueChanged);
  disconnect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonAtualizar_clicked);
  disconnect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonCadastrar_clicked);
  disconnect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonNovoCad_clicked);
  disconnect(ui->pushButtonRemover, &QPushButton::clicked, this, &CadastroProduto::on_pushButtonRemover_clicked);
}

// TODO: 3poder alterar nesta tela a quantidade minima/multiplo dos produtos
// TODO: 5verificar se estou usando corretamente a tabela 'produto_has_preco'
// me parece que ela só é preenchida na importacao de tabela e nao na modificacao manual de produtos
// TODO: 4verificar se posso remover 'un2' de produto
// TODO: colocar logica para trabalhar a tabela produto_has_preco para que os produtos nao sejam descontinuados com validade ativa
// TODO: validar entrada do campo icms para apenas numeros
// TODO: verificar para que era usado o campo 'un2' e remove-lo caso nao seja mais usado
// TODO: verificar se vendedor deve mesmo poder alterar cadastro do produto

// TODO: change 'icms' from lineEdit to doubleSpinBox
