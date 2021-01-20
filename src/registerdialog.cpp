#include "registerdialog.h"

#include "application.h"
#include "itembox.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QRadioButton>
#include <QShortcut>

RegisterDialog::RegisterDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent) : QDialog(parent), primaryKey(primaryKeyStr), parent(parent) {
  setWindowModality(Qt::NonModal);
  setWindowFlags(Qt::Window);

  model.setTable(table);

  mapper.setModel(&model);
  mapper.setSubmitPolicy(QDataWidgetMapper::AutoSubmit);

  connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this), &QShortcut::activated, this, &QWidget::close);
  connect(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S), this), &QShortcut::activated, [&]() { save(); });
}

bool RegisterDialog::viewRegisterById(const QVariant &id) {
  if (model.tableName() == "profissional" and id == "1") { // não permitir alterar o cadastro do 'NÃO HÁ'
    newRegister();
    return false;
  }

  primaryId = id.toString();

  if (primaryId.isEmpty()) { throw RuntimeException("primaryId vazio!", this); }

  model.setFilter(primaryKey + " = '" + primaryId + "'");

  model.select();

  if (model.rowCount() == 0) {
    close();
    throw RuntimeException("Item não encontrado!", this);
  }

  isDirty = false;

  return viewRegister();
}

bool RegisterDialog::viewRegister() {
  if (not confirmationMessage()) { return false; }

  tipo = Tipo::Atualizar;

  clearFields();
  updateMode();

  currentRow = 0;
  mapper.toFirst();

  return true;
}

void RegisterDialog::setForeignKey(SqlTableModel &secondaryModel) {
  for (int row = 0, rowCount = secondaryModel.rowCount(); row < rowCount; ++row) { secondaryModel.setData(row, primaryKey, primaryId); }
}

void RegisterDialog::setData(const QString &key, const QVariant &value) { return model.setData(currentRow, key, value); }

QVariant RegisterDialog::data(const QString &key) { return model.data(currentRow, key); }

void RegisterDialog::addMapping(QWidget *widget, const QString &key, const QByteArray &propertyName) {
  if (model.fieldIndex(key) == -1) { throw RuntimeException("Chave " + key + " não encontrada na tabela " + model.tableName(), this); }

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

void RegisterDialog::verifyRequiredField(const QLineEdit &line) {
  if (not line.styleSheet().contains(requiredStyle())) { return; }
  if (not line.isVisible()) { return; }

  const bool isEmpty = (line.text().isEmpty());
  const bool isZero = (line.text() == "0,00");
  const bool isSymbols = (line.text() == "../-");
  const bool isLessMask = (line.text().size() < line.inputMask().remove(";").remove(">").remove("_").size());
  const bool isLessPlaceHolder = (line.text().size() < line.placeholderText().size() - 1);

  if (isEmpty or isZero or isSymbols or isLessMask or isLessPlaceHolder) { throw RuntimeError("Um campo obrigatório não foi preenchido:\n" + line.accessibleName()); }
}

bool RegisterDialog::confirmationMessage() {
  if (isDirty) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Deseja salvar as alterações?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
    msgBox.setButtonText(QMessageBox::Yes, "Salvar");
    msgBox.setButtonText(QMessageBox::No, "Descartar");
    msgBox.setButtonText(QMessageBox::Cancel, "Cancelar");

    const int escolha = msgBox.exec();

    if (escolha == QMessageBox::Yes) { save(); }
    if (escolha == QMessageBox::No) {}
    if (escolha == QMessageBox::Cancel) { return false; }
  }

  return true;
}

bool RegisterDialog::newRegister() {
  if (not confirmationMessage()) { return false; }

  model.setFilter("0");

  currentRow = -1;
  primaryId.clear();

  tipo = Tipo::Cadastrar;

  clearFields();
  registerMode();

  return true;
}

void RegisterDialog::save(const bool silent) {
  verifyFields();

  cadastrar();

  isDirty = false;

  if (not silent) { successMessage(); }

  viewRegisterById(primaryId);
}

void RegisterDialog::clearFields() {
  const auto lineEdits = findChildren<QLineEdit *>(QRegularExpression("lineEdit"));

  for (const auto &lineEdit : lineEdits) {
    if (lineEdit->objectName().startsWith("lineEdit")) { lineEdit->clear(); }
  }

  const auto itemBoxes = findChildren<ItemBox *>(QRegularExpression("itemBox"));

  for (const auto &itemBox : itemBoxes) {
    if (itemBox->objectName().startsWith("itemBox")) { itemBox->clear(); }
  }

  const auto doubleSpinBoxes = findChildren<QDoubleSpinBox *>(QRegularExpression("doubleSpinBox"));

  for (const auto &doubleSpinBox : doubleSpinBoxes) {
    if (doubleSpinBox->objectName().startsWith("doubleSpinBox")) { doubleSpinBox->setValue(0); }
  }

  const auto spinBoxes = findChildren<QSpinBox *>(QRegularExpression("spinBox"));

  for (const auto &spinBox : spinBoxes) {
    if (spinBox->objectName().startsWith("spinBox")) { spinBox->setValue(0); }
  }

  const auto radioButtons = findChildren<QRadioButton *>(QRegularExpression("radioButton"));

  for (const auto radioButton : radioButtons) {
    if (radioButton->objectName().startsWith("radioButton")) { radioButton->setChecked(false); }
  }

  const auto checkBoxes = findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (const auto checkBox : checkBoxes) {
    if (checkBox->objectName().startsWith("checkBox")) { checkBox->setChecked(false); }
  }

  const auto comboBoxes = findChildren<QComboBox *>(QRegularExpression("comboBox"));

  for (const auto comboBox : comboBoxes) {
    if (comboBox->objectName().startsWith("comboBox")) { comboBox->setCurrentIndex(0); }
  }
}

int RegisterDialog::removeBox() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja remover?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.setButtonText(QMessageBox::Yes, "Remover");
  msgBox.setButtonText(QMessageBox::No, "Voltar");

  return msgBox.exec();
}

void RegisterDialog::remove() {
  if (removeBox() == QMessageBox::Yes) {
    setData("desativado", true);

    model.submitAll();

    isDirty = false;

    newRegister();
  }
}

bool RegisterDialog::validaCNPJ(const QString &text) {
  const QString cnpj = QString(text).remove(".").remove("/").remove("-");

  if (cnpj.size() != 14) { return false; }

  const QString sub = cnpj.left(12);

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

  if (digito1 != cnpj.at(12).digitValue() or digito2 != cnpj.at(13).digitValue()) { throw RuntimeError("CNPJ inválido!", this); }

  return true;
}

bool RegisterDialog::validaCPF(const QString &text) {
  const QString cpf = QString(text).remove(".").remove("-");

  if (cpf.size() != 11) { return false; }

  if (cpf == "00000000000" or cpf == "11111111111" or cpf == "22222222222" or cpf == "33333333333" or cpf == "44444444444" or cpf == "55555555555" or cpf == "66666666666" or cpf == "77777777777" or
      cpf == "88888888888" or cpf == "99999999999") {
    throw RuntimeError("CPF inválido!", this);
  }

  const QString sub = cpf.left(9);

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

  if (digito1 != cpf.at(9).digitValue() or digito2 != cpf.at(10).digitValue()) { throw RuntimeError("CPF inválido!", this); }

  return true;
}

// TODO: no lugar de qualquer alteracao em lineEdit sair marcando dirty fazer uma analise tela por tela do que pode ser considerado edicao
void RegisterDialog::marcarDirty() { isDirty = true; }
