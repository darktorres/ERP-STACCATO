#include "cadastrofornecedor.h"
#include "ui_cadastrofornecedor.h"

#include "application.h"
#include "cepcompleter.h"
#include "checkboxdelegate.h"
#include "user.h"

#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QSqlError>

CadastroFornecedor::CadastroFornecedor(QWidget *parent) : RegisterAddressDialog("fornecedor", "idFornecedor", parent), ui(new Ui::CadastroFornecedor) {
  ui->setupUi(this);

  connectLineEditsToDirty();
  setupUi();
  setupTables();
  setupMapper();
  newRegister();

  if (not User::isAdministrativo()) {
    ui->pushButtonDesativar->setDisabled(true);
    ui->pushButtonDesativarEnd->setDisabled(true);
  }

  setConnections();
}

CadastroFornecedor::~CadastroFornecedor() { delete ui; }

void CadastroFornecedor::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(sdFornecedor, &SearchDialog::itemSelected, this, &CadastroFornecedor::viewRegisterById, connectionType);
  connect(ui->checkBoxMostrarInativos, &QCheckBox::clicked, this, &CadastroFornecedor::on_checkBoxMostrarInativos_clicked, connectionType);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroFornecedor::on_lineEditCEP_textChanged, connectionType);
  connect(ui->lineEditCNPJ, &QLineEdit::textEdited, this, &CadastroFornecedor::on_lineEditCNPJ_textEdited, connectionType);
  connect(ui->lineEditCNPJBancario, &QLineEdit::textEdited, this, &CadastroFornecedor::on_lineEditCNPJBancario_textEdited, connectionType);
  connect(ui->lineEditContatoCPF, &QLineEdit::textEdited, this, &CadastroFornecedor::on_lineEditContatoCPF_textEdited, connectionType);
  connect(ui->pushButtonAdicionarEnd, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonAdicionarEnd_clicked, connectionType);
  connect(ui->pushButtonAtualizar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonAtualizar_clicked, connectionType);
  connect(ui->pushButtonAtualizarEnd, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonAtualizarEnd_clicked, connectionType);
  connect(ui->pushButtonBuscar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonBuscar_clicked, connectionType);
  connect(ui->pushButtonCadastrar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonCadastrar_clicked, connectionType);
  connect(ui->pushButtonDesativar, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonDesativar_clicked, connectionType);
  connect(ui->pushButtonDesativarEnd, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonDesativarEnd_clicked, connectionType);
  connect(ui->pushButtonNovoCad, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonNovoCad_clicked, connectionType);
  connect(ui->pushButtonSalvarPrazos, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonSalvarPrazos_clicked, connectionType);
  connect(ui->pushButtonValidade, &QPushButton::clicked, this, &CadastroFornecedor::on_pushButtonValidade_clicked, connectionType);
  connect(ui->tabWidget, &QTabWidget::currentChanged, this, &CadastroFornecedor::on_tabWidget_currentChanged, connectionType);
  connect(ui->tableEndereco, &TableView::clicked, this, &CadastroFornecedor::on_tableEndereco_clicked, connectionType);
}

void CadastroFornecedor::setupTables() {
  ui->tableEndereco->setModel(&modelEnd);

  ui->tableEndereco->hideColumn("idEndereco");
  ui->tableEndereco->hideColumn("idFornecedor");
  ui->tableEndereco->hideColumn("codUF");

  ui->tableEndereco->setItemDelegateForColumn("desativado", new CheckBoxDelegate(true, this));

  ui->tableEndereco->setPersistentColumns({"desativado"});
}

void CadastroFornecedor::setupUi() {
  // dados
  ui->lineEditContatoCPF->setInputMask("999.999.999-99;_");
  ui->lineEditIdNextel->setInputMask("99*9999999*99999;_");
  ui->lineEditCNPJ->setInputMask("99.999.999/9999-99;_");
  ui->lineEditUF->setInputMask(">AA;_");

  // endereco
  ui->lineEditCEP->setInputMask("99999-999;_");
  ui->lineEditUF->setInputMask(">AA;_");

  // bancario
  ui->lineEditAgencia->setInputMask("9999-9;_");
  ui->lineEditCNPJBancario->setInputMask("99.999.999/9999-99;_");
}

void CadastroFornecedor::clearEndereco() {
  ui->lineEditBairro->clear();
  ui->lineEditCEP->clear();
  ui->lineEditCidade->clear();
  ui->lineEditComplemento->clear();
  ui->lineEditLogradouro->clear();
  ui->lineEditNumero->clear();
  ui->lineEditUF->clear();
}

void CadastroFornecedor::novoEndereco() {
  ui->pushButtonAtualizarEnd->hide();
  ui->pushButtonAdicionarEnd->show();
  ui->pushButtonDesativarEnd->hide();
  ui->tableEndereco->clearSelection();
  clearEndereco();
}

void CadastroFornecedor::verifyFields() {
  const auto children = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { verifyRequiredField(*line); }

  validaCNPJ(ui->lineEditCNPJ->text());
}

void CadastroFornecedor::savingProcedures() {
  setData("aliquotaSt", ui->doubleSpinBoxAliquotaSt->value());
  setData("cnpj", ui->lineEditCNPJ->text().remove(".").remove("/").remove("-"));
  setData("comissao1", ui->doubleSpinBoxComissaoVendedor->value());
  setData("comissao2", ui->doubleSpinBoxComissaoEspecial->value());
  setData("comissaoLoja", ui->doubleSpinBoxComissaoLoja->value());
  setData("contatoApelido", ui->lineEditContatoApelido->text());
  setData("contatoCPF", ui->lineEditContatoCPF->text().remove(".").remove("-"));
  setData("contatoNome", ui->lineEditContatoNome->text());
  setData("contatoRG", ui->lineEditContatoRG->text());
  setData("email", ui->lineEditEmail->text());
  setData("especialidade", ui->comboBoxEspecialidade->currentText().left(1).toInt());
  setData("inscEstadual", ui->lineEditInscEstadual->text());
  setData("nextel", ui->lineEditNextel->text());
  setData("nomeFantasia", ui->lineEditNomeFantasia->text());
  setData("razaoSocial", ui->lineEditFornecedor->text());
  setData("st", ui->comboBoxSt->currentText());
  setData("tel", ui->lineEditTel->text());
  setData("telCel", ui->lineEditTel_Cel->text());
  setData("telCom", ui->lineEditTel_Com->text());
  setData("vemDoSul", ui->checkBoxVemSul->isChecked());

  // Dados bancários

  setData("nomeBanco", ui->lineEditNomeBancario->text());
  setData("cnpjBanco", (ui->lineEditCNPJBancario->text() == "../-") ? "" : ui->lineEditCNPJBancario->text());
  setData("banco", ui->lineEditBanco->text());
  setData("agencia", (ui->lineEditAgencia->text() == "-") ? "" : ui->lineEditAgencia->text());
  setData("cc", ui->lineEditCC->text());
  setData("poupanca", ui->checkBoxPoupanca->isChecked());

  QSqlQuery queryRep;

  const QString isRep = ui->checkBoxRepresentacao->isChecked() ? "TRUE" : "FALSE";

  if (not queryRep.exec("UPDATE produto SET fornecedor = '" + data("razaoSocial").toString() + "', representacao = " + isRep + " WHERE idFornecedor = " + data("idFornecedor").toString())) {
    throw RuntimeException("Erro atualizando representação: " + queryRep.lastError().text());
  }
}

void CadastroFornecedor::clearFields() {
  RegisterDialog::clearFields();
  novoEndereco();
  setupUi();
}

void CadastroFornecedor::setupMapper() {
  addMapping(ui->checkBoxRepresentacao, "representacao", "checked");
  addMapping(ui->checkBoxVemSul, "vemDoSul");
  addMapping(ui->comboBoxSt, "st");
  addMapping(ui->doubleSpinBoxAliquotaSt, "aliquotaSt");
  addMapping(ui->doubleSpinBoxComissaoEspecial, "comissao2");
  addMapping(ui->doubleSpinBoxComissaoLoja, "comissaoLoja");
  addMapping(ui->doubleSpinBoxComissaoVendedor, "comissao1");
  addMapping(ui->lineEditCNPJ, "cnpj");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoApelido, "contatoApelido");
  addMapping(ui->lineEditContatoCPF, "contatoCPF");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoNome, "contatoNome");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditContatoRG, "contatoRG");
  addMapping(ui->lineEditEmail, "email");
  addMapping(ui->lineEditFornecedor, "razaoSocial");
  addMapping(ui->lineEditIdNextel, "idNextel");
  addMapping(ui->lineEditInscEstadual, "inscEstadual");
  addMapping(ui->lineEditNextel, "nextel");
  addMapping(ui->lineEditNomeFantasia, "nomeFantasia");
  addMapping(ui->lineEditTel, "tel");
  addMapping(ui->lineEditTel_Cel, "telCel");
  addMapping(ui->lineEditTel_Com, "telCom");

  addMapping(ui->checkBoxPoupanca, "poupanca");
  addMapping(ui->lineEditAgencia, "agencia");
  addMapping(ui->lineEditBanco, "banco");
  addMapping(ui->lineEditCC, "cc");
  addMapping(ui->lineEditCNPJBancario, "cnpjBanco");
  addMapping(ui->lineEditNomeBancario, "nomeBanco");

  mapperEnd.addMapping(ui->comboBoxTipoEnd, modelEnd.fieldIndex("descricao"));
  mapperEnd.addMapping(ui->lineEditBairro, modelEnd.fieldIndex("bairro"));
  mapperEnd.addMapping(ui->lineEditCEP, modelEnd.fieldIndex("CEP"));
  mapperEnd.addMapping(ui->lineEditCidade, modelEnd.fieldIndex("cidade"));
  mapperEnd.addMapping(ui->lineEditComplemento, modelEnd.fieldIndex("complemento"));
  mapperEnd.addMapping(ui->lineEditLogradouro, modelEnd.fieldIndex("logradouro"));
  mapperEnd.addMapping(ui->lineEditNumero, modelEnd.fieldIndex("numero"));
  mapperEnd.addMapping(ui->lineEditUF, modelEnd.fieldIndex("uf"));
}

void CadastroFornecedor::registerMode() {
  ui->pushButtonCadastrar->show();
  ui->pushButtonAtualizar->hide();
  ui->pushButtonDesativar->hide();
  ui->pushButtonValidade->hide();

  ui->pushButtonDesativarEnd->hide();
}

void CadastroFornecedor::updateMode() {
  ui->pushButtonCadastrar->hide();
  ui->pushButtonAtualizar->show();
  ui->pushButtonDesativar->show();

  if (readOnly) {
    ui->pushButtonBuscar->hide();
    ui->pushButtonNovoCad->hide();
  }
}

void CadastroFornecedor::cadastrar() {
  try {
    qApp->startTransaction("CadastroFornecedor::cadastrar");

    if (tipo == Tipo::Cadastrar) { currentRow = model.insertRowAtEnd(); }

    savingProcedures();

    model.submitAll();

    primaryId = (tipo == Tipo::Atualizar) ? data(primaryKey).toString() : model.query().lastInsertId().toString();

    if (primaryId.isEmpty()) { throw RuntimeException("Id vazio!"); }

    // -------------------------------------------------------------------------

    setForeignKey(modelEnd);

    modelEnd.submitAll();

    qApp->endTransaction();

    backupEndereco.clear();

    model.setFilter(primaryKey + " = '" + primaryId + "'");

    modelEnd.setFilter(primaryKey + " = '" + primaryId + "'");
  } catch (std::exception &) {
    qApp->rollbackTransaction();
    model.select();
    modelEnd.select();

    for (auto &record : backupEndereco) { modelEnd.insertRecord(-1, record); }

    throw;
  }
}

void CadastroFornecedor::on_pushButtonCadastrar_clicked() { save(); }

void CadastroFornecedor::on_pushButtonAtualizar_clicked() { save(); }

void CadastroFornecedor::on_pushButtonBuscar_clicked() {
  if (not confirmationMessage()) { return; }

  sdFornecedor->show();
}

void CadastroFornecedor::on_pushButtonNovoCad_clicked() { newRegister(); }

void CadastroFornecedor::on_pushButtonDesativar_clicked() { remove(); }

void CadastroFornecedor::on_lineEditCNPJ_textEdited(const QString &text) {
  ui->lineEditCNPJ->setStyleSheet(validaCNPJ(text) ? "background-color: rgb(255, 255, 127);color: rgb(0, 190, 0)" : "background-color: rgb(255, 255, 127);color: rgb(255, 0, 0)");
}

void CadastroFornecedor::on_lineEditContatoCPF_textEdited(const QString &text) { ui->lineEditContatoCPF->setStyleSheet(validaCPF(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroFornecedor::on_pushButtonAdicionarEnd_clicked() {
  cadastrarEndereco();
  novoEndereco();
}

void CadastroFornecedor::cadastrarEndereco(const Tipo tipoEndereco) {
  verificaEndereco();

  if (tipoEndereco == Tipo::Cadastrar) { currentRowEnd = modelEnd.insertRowAtEnd(); }

  setDataEnd("descricao", ui->comboBoxTipoEnd->currentText());
  setDataEnd("cep", ui->lineEditCEP->text());
  setDataEnd("logradouro", ui->lineEditLogradouro->text());
  setDataEnd("numero", ui->lineEditNumero->text());
  setDataEnd("complemento", ui->lineEditComplemento->text());
  setDataEnd("bairro", ui->lineEditBairro->text());
  setDataEnd("cidade", ui->lineEditCidade->text());
  setDataEnd("uf", ui->lineEditUF->text());
  setDataEnd("codUF", getCodigoUF(ui->lineEditUF->text()));
  setDataEnd("desativado", false);

  if (tipoEndereco == Tipo::Cadastrar) { backupEndereco.append(modelEnd.record(currentRowEnd)); }

  isDirty = true;

  if (tipo == Tipo::Atualizar) { save(true); }
}

void CadastroFornecedor::on_lineEditCEP_textChanged(const QString &cep) {
  if (not ui->lineEditCEP->isValid()) { return; }

  ui->lineEditNumero->clear();
  ui->lineEditComplemento->clear();

  CepCompleter cc;
  cc.buscaCEP(cep, this);

  ui->lineEditLogradouro->setText(cc.getEndereco());
  ui->lineEditComplemento->setText(cc.getComplemento());
  ui->lineEditBairro->setText(cc.getBairro());
  ui->lineEditCidade->setText(cc.getCidade());
  ui->lineEditUF->setText(cc.getUf());
}

void CadastroFornecedor::on_pushButtonAtualizarEnd_clicked() {
  cadastrarEndereco(Tipo::Atualizar);
  novoEndereco();
}

void CadastroFornecedor::on_tableEndereco_clicked(const QModelIndex &index) {
  if (not index.isValid()) { return novoEndereco(); }

  currentRowEnd = index.row();

  const bool desativado = dataEnd("desativado").toBool();

  ui->pushButtonAtualizarEnd->setEnabled(desativado);
  ui->pushButtonDesativarEnd->setDisabled(desativado);

  ui->pushButtonAtualizarEnd->show();
  ui->pushButtonAdicionarEnd->hide();
  ui->pushButtonDesativarEnd->show();

  //------------------------------------------
  disconnect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroFornecedor::on_lineEditCEP_textChanged);
  mapperEnd.setCurrentModelIndex(index);
  connect(ui->lineEditCEP, &LineEditCEP::textChanged, this, &CadastroFornecedor::on_lineEditCEP_textChanged);
  //------------------------------------------
}

bool CadastroFornecedor::viewRegister() {
  if (not RegisterDialog::viewRegister()) { return false; }

  //---------------------------------------------------

  const bool inativos = ui->checkBoxMostrarInativos->isChecked();
  modelEnd.setFilter("idFornecedor = " + data("idFornecedor").toString() + (inativos ? "" : " AND desativado = FALSE"));

  modelEnd.select();

  //---------------------------------------------------

  ui->pushButtonValidade->show();

  ui->comboBoxEspecialidade->setCurrentIndex(data("especialidade").toString().left(1).toInt());

  return true;
}

void CadastroFornecedor::on_pushButtonDesativarEnd_clicked() {
  if (removeBox() == QMessageBox::Yes) {
    setDataEnd("desativado", true);

    modelEnd.submitAll();

    novoEndereco();
  }
}

void CadastroFornecedor::successMessage() { qApp->enqueueInformation((tipo == Tipo::Atualizar) ? "Cadastro atualizado!" : "Fornecedor cadastrado com sucesso!", this); }

void CadastroFornecedor::ajustarValidade(const int novaValidade) {
  const QString fornecedor = data("razaoSocial").toString();

  SqlQuery query;
  query.prepare("UPDATE produto SET validade = :novaValidade, descontinuado = FALSE WHERE fornecedor = :fornecedor AND estoque = FALSE AND promocao = FALSE");
  query.bindValue(":novaValidade", qApp->serverDate().addDays(novaValidade));
  query.bindValue(":fornecedor", fornecedor);

  if (not query.exec()) { throw RuntimeException("Erro atualizando validade nos produtos: " + query.lastError().text()); }

  SqlQuery query2;
  query2.prepare("UPDATE fornecedor SET validadeProdutos = :novaValidade WHERE razaoSocial = :fornecedor");
  query2.bindValue(":novaValidade", qApp->serverDate().addDays(novaValidade));
  query2.bindValue(":fornecedor", fornecedor);

  if (not query2.exec()) { throw RuntimeException("Erro atualizando validade no fornecedor: " + query2.lastError().text()); }

  SqlQuery query3;

  if (not query3.exec("CALL invalidar_produtos_expirados()")) { throw RuntimeException("Erro executando invalidar_produtos_expirados: " + query3.lastError().text()); }
}

void CadastroFornecedor::on_pushButtonValidade_clicked() {
  bool ok = false;

  const int novaValidade = QInputDialog::getInt(this, "Validade", "Quantos dias de validade para os produtos: ", 0, -1, 1000, 1, &ok);

  if (not ok) { return; }

  qApp->startTransaction("CadastroFornecedor::on_pushButtonValidade");

  ajustarValidade(novaValidade);

  qApp->endTransaction();

  qApp->enqueueInformation("Validade alterada com sucesso!", this);
}

void CadastroFornecedor::on_checkBoxMostrarInativos_clicked(const bool checked) {
  if (currentRow == -1) { return; }

  modelEnd.setFilter("idFornecedor = " + data("idFornecedor").toString() + (checked ? "" : " AND desativado = FALSE"));

  modelEnd.select();
}

void CadastroFornecedor::on_lineEditCNPJBancario_textEdited(const QString &text) { ui->lineEditCNPJBancario->setStyleSheet(validaCNPJ(text) ? "color: rgb(0, 190, 0)" : "color: rgb(255, 0, 0)"); }

void CadastroFornecedor::on_pushButtonSalvarPrazos_clicked() {
  const QString especialidade1 = QString::number(0) + ", '" + ui->labelEspecialidade1->text() + "', " + QString::number(ui->spinBoxPrazoEspecialidade1->value());
  const QString especialidade2 = QString::number(1) + ", '" + ui->labelEspecialidade2->text() + "', " + QString::number(ui->spinBoxPrazoEspecialidade2->value());
  const QString especialidade3 = QString::number(2) + ", '" + ui->labelEspecialidade3->text() + "', " + QString::number(ui->spinBoxPrazoEspecialidade3->value());
  const QString especialidade4 = QString::number(3) + ", '" + ui->labelEspecialidade4->text() + "', " + QString::number(ui->spinBoxPrazoEspecialidade4->value());
  const QString especialidade5 = QString::number(4) + ", '" + ui->labelEspecialidade5->text() + "', " + QString::number(ui->spinBoxPrazoEspecialidade5->value());

  QSqlQuery query;

  if (not query.exec("INSERT INTO fornecedor_has_prazo (especialidadeIndex, especialidade, prazo) VALUES (" + especialidade1 + "), (" + especialidade2 + "), (" + especialidade3 + "), (" +
                     especialidade4 + "), (" + especialidade5 + ") ON DUPLICATE KEY UPDATE prazo = VALUES (prazo)")) {
    throw RuntimeException("Erro cadastrando prazos: " + query.lastError().text());
  }

  qApp->enqueueInformation("Prazos atualizados com sucesso!", this);
}

void CadastroFornecedor::on_tabWidget_currentChanged(const int index) {
  if (ui->tabWidget->tabText(index) == "Parâmetros") {
    QSqlQuery query;

    if (not query.exec("SELECT * FROM fornecedor_has_prazo")) { throw RuntimeException("Erro buscando prazos: " + query.lastError().text()); }

    if (query.first()) {
      ui->spinBoxPrazoEspecialidade1->setValue(query.value("prazo").toInt());

      query.next();

      ui->spinBoxPrazoEspecialidade2->setValue(query.value("prazo").toInt());

      query.next();

      ui->spinBoxPrazoEspecialidade3->setValue(query.value("prazo").toInt());

      query.next();

      ui->spinBoxPrazoEspecialidade4->setValue(query.value("prazo").toInt());

      query.next();

      ui->spinBoxPrazoEspecialidade5->setValue(query.value("prazo").toInt());
    }
  }
}

void CadastroFornecedor::verificaEndereco() {
  if (not ui->lineEditCEP->isValid()) { throw RuntimeError("CEP inválido!", this); }

  if (ui->lineEditNumero->text().isEmpty()) { throw RuntimeError(R"(Número vazio! Se necessário coloque "S/N"!)", this); }

  if (ui->lineEditCidade->text().isEmpty()) { throw RuntimeError("Cidade vazio!", this); }

  if (ui->lineEditUF->text().isEmpty()) { throw RuntimeError("UF vazio!", this); }

  RegisterAddressDialog::verificaEndereco(ui->lineEditCidade->text(), ui->lineEditUF->text());
}

void CadastroFornecedor::connectLineEditsToDirty() {
  const auto children = ui->tabWidget->findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &line : children) { connect(line, &QLineEdit::textEdited, this, &CadastroFornecedor::marcarDirty); }
}

// TODO: 5poder alterar na tela 'comissao'
// TODO: quando for novo cadastro permitir marcar flag de representacao
