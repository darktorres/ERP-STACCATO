#include <QCloseEvent>
#include <QDebug>
#include <QMessageBox>
#include <QShortcut>
#include <QSqlError>
#include <QSqlQuery>

#include "application.h"
#include "registerdialog.h"

RegisterDialog::RegisterDialog(const QString &table, QString primaryKey, QWidget *parent = nullptr) : QDialog(parent), primaryKey(std::move(primaryKey)) {
  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  model.setTable(table);
  model.setEditStrategy(QSqlTableModel::OnManualSubmit);
  model.setFilter("0");

  if (not model.select()) { return; }

  mapper.setModel(&model);
  mapper.setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

  connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this), &QShortcut::activated, this, &QWidget::close);
  connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this), &QShortcut::activated, [&]() { save(); });
}

bool RegisterDialog::viewRegisterById(const QVariant &id) {
  primaryId = id.toString();

  if (primaryId.isEmpty()) { return qApp->enqueueError(false, "primaryId vazio!"); }

  model.setFilter(primaryKey + " = '" + primaryId + "'");

  if (not model.select()) { return false; }

  if (model.rowCount() == 0) {
    close();
    return qApp->enqueueError(false, "Item não encontrado.");
  }

  isDirty = false;

  return viewRegister();
}

bool RegisterDialog::viewRegister() {
  if (not confirmationMessage()) { return false; }

  tipo = Tipo::Atualizar;

  clearFields();
  updateMode();

  if (not primaryId.isEmpty()) {
    model.setFilter(primaryKey + " = '" + primaryId + "'");

    if (not model.select()) { return false; }
  }

  mapper.setCurrentIndex(0);

  return true;
}

bool RegisterDialog::verifyFields(const QList<QLineEdit *> &list) {
  for (const auto &line : list) {
    if (not verifyRequiredField(line)) { return false; }
  }

  return true;
}

bool RegisterDialog::setData(const QString &key, const QVariant &value) {
  if (value.isNull() or (value.type() == QVariant::String and value.toString().isEmpty())) { return true; }
  if (value.type() == QVariant::String and value.toString().remove(".").remove("/").remove("-").isEmpty()) { return true; }
  if (value.type() == QVariant::Date and value.toString() == "1900-01-01") { return true; }

  const int row = currentRow != -1 ? currentRow : mapper.currentIndex();

  return model.setData(row, key, value);
}

QVariant RegisterDialog::data(const QString &key) { return model.data(mapper.currentIndex(), key); }

QVariant RegisterDialog::data(const int row, const QString &key) { return model.data(row, key); }

void RegisterDialog::addMapping(QWidget *widget, const QString &key, const QByteArray &propertyName) {
  if (model.fieldIndex(key) == -1) { return qApp->enqueueError("Chave " + key + " não encontrada na tabela " + model.tableName()); }

  propertyName.isNull() ? mapper.addMapping(widget, model.fieldIndex(key)) : mapper.addMapping(widget, model.fieldIndex(key), propertyName);
}

QString RegisterDialog::requiredStyle() { return (QString("background-color: rgb(255, 255, 127)")); }

void RegisterDialog::closeEvent(QCloseEvent *event) { confirmationMessage() ? event->accept() : event->ignore(); }

void RegisterDialog::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    event->accept();
    close();
    return;
  }

  QDialog::keyPressEvent(event);
}

QStringList RegisterDialog::getTextKeys() const { return textKeys; }

void RegisterDialog::setTextKeys(const QStringList &value) { textKeys = value; }

void RegisterDialog::show() {
  QWidget::show();
  adjustSize();
}

bool RegisterDialog::verifyRequiredField(QLineEdit *line, const bool silent) {
  if (not line->styleSheet().contains(requiredStyle())) { return true; }
  if (not line->isVisible()) { return true; }

  if ((line->text().isEmpty()) or (line->text() == "0,00") or (line->text() == "../-") or (line->text().size() < line->inputMask().remove(";").remove(">").remove("_").size()) or
      (line->text().size() < line->placeholderText().size() - 1)) {
    if (not silent) {
      line->setFocus();
      qApp->enqueueError("Você não preencheu um campo obrigatório: " + line->accessibleName());
    }

    return false;
  }

  return true;
}

bool RegisterDialog::confirmationMessage() {
  if (isDirty) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Deseja salvar as alterações?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
    msgBox.setButtonText(QMessageBox::Yes, "Salvar");
    msgBox.setButtonText(QMessageBox::No, "Descartar");
    msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

    const int escolha = msgBox.exec();

    if (escolha == QMessageBox::Yes) { return save(); }
    if (escolha == QMessageBox::No) { return true; }
    if (escolha == QMessageBox::Cancel) { return false; }
  }

  return true;
}

bool RegisterDialog::newRegister() {
  if (not confirmationMessage()) { return false; }

  model.setFilter("0");

  if (not model.select()) { return false; }

  tipo = Tipo::Cadastrar;

  clearFields();
  registerMode();

  return true;
}

bool RegisterDialog::save(const bool silent) {
  if (not verifyFields()) { return false; }

  if (not qApp->startTransaction()) { return false; }

  if (not cadastrar()) {
    qApp->rollbackTransaction();
    void(model.select());
    mapper.toLast();
    return false;
  }

  if (not qApp->endTransaction()) { return false; }

  isDirty = false;

  if (not silent) { successMessage(); }

  viewRegisterById(primaryId);

  return true;
}

void RegisterDialog::clearFields() {
  Q_FOREACH (const auto &line, findChildren<QLineEdit *>()) {
    if (not line->isReadOnly()) { line->clear(); }
  }
}

void RegisterDialog::remove() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  if (msgBox.exec() == QMessageBox::Yes) {
    if (not setData("desativado", true)) { return; }

    if (not model.submitAll()) { return; }

    newRegister();
  }
}

bool RegisterDialog::validaCNPJ(const QString &text) {
  if (text.size() != 14) { return false; }

  const QString sub = text.left(12);

  QVector<int> sub2;

  for (const auto &i : sub) { sub2.push_back(i.digitValue()); }

  const QVector<int> multiplicadores = {5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma = 0;

  for (int i = 0; i < 12; ++i) { soma += sub2.at(i) * multiplicadores.at(i); }

  const int resto = soma % 11;

  const int digito1 = resto < 2 ? 0 : 11 - resto;

  sub2.push_back(digito1);

  const QVector<int> multiplicadores2 = {6, 5, 4, 3, 2, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma2 = 0;

  for (int i = 0; i < 13; ++i) { soma2 += sub2.at(i) * multiplicadores2.at(i); }

  const int resto2 = soma2 % 11;

  const int digito2 = resto2 < 2 ? 0 : 11 - resto2;

  if (digito1 != text.at(12).digitValue() or digito2 != text.at(13).digitValue()) { return qApp->enqueueError(false, "CNPJ inválido!"); }

  return true;
}

bool RegisterDialog::validaCPF(const QString &text) {
  if (text.size() != 11) { return false; }

  if (text == "00000000000" or text == "11111111111" or text == "22222222222" or text == "33333333333" or text == "44444444444" or text == "55555555555" or text == "66666666666" or
      text == "77777777777" or text == "88888888888" or text == "99999999999") {
    return qApp->enqueueError(false, "CPF inválido!");
  }

  const QString sub = text.left(9);

  QVector<int> sub2;

  for (const auto &i : sub) { sub2.push_back(i.digitValue()); }

  const QVector<int> multiplicadores = {10, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma = 0;

  for (int i = 0; i < 9; ++i) { soma += sub2.at(i) * multiplicadores.at(i); }

  const int resto = soma % 11;

  const int digito1 = resto < 2 ? 0 : 11 - resto;

  sub2.push_back(digito1);

  const QVector<int> multiplicadores2 = {11, 10, 9, 8, 7, 6, 5, 4, 3, 2};

  int soma2 = 0;

  for (int i = 0; i < 10; ++i) { soma2 += sub2.at(i) * multiplicadores2.at(i); }

  const int resto2 = soma2 % 11;

  const int digito2 = resto2 < 2 ? 0 : 11 - resto2;

  if (digito1 != text.at(9).digitValue() or digito2 != text.at(10).digitValue()) { return qApp->enqueueError(false, "CPF inválido!"); }

  return true;
}

// TODO: no lugar de qualquer alteracao em lineEdit sair marcando dirty fazer uma analise tela por tela do que pode ser considerado edicao
void RegisterDialog::marcarDirty() { isDirty = true; }
