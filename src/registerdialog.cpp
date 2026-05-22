#include "registerdialog.h"

#include "application.h"
#include "itembox.h"
#include "validators.h"

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

  setConnections();
}

void RegisterDialog::setReadOnly(const bool isReadOnly) { readOnly = isReadOnly; }

void RegisterDialog::setConnections() {
  const auto connectionType = static_cast<Qt::ConnectionType>(Qt::AutoConnection | Qt::UniqueConnection);

  connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this), &QShortcut::activated, this, &QWidget::close, connectionType);
  connect(
      new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this), &QShortcut::activated, this, [&]() { save(); }, connectionType);
}

bool RegisterDialog::viewRegisterById(const QVariant &id) {
  if (model.tableName() == "profissional" and id.toString() == "1") { // não permitir alterar o cadastro do 'NÃO HÁ'
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
  if (not askSaveBeforeClosing()) { return false; }

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

void RegisterDialog::setData(const QString &key, const QVariant &value, const bool adjustValue) { model.setData(currentRow, key, value, adjustValue); }

QVariant RegisterDialog::data(const QString &key) const { return model.data(currentRow, key); }

void RegisterDialog::addMapping(QWidget *widget, const QString &key, const QByteArray &propertyName) {
  if (model.fieldIndex(key) == -1) { throw RuntimeException("Chave " + key + " não encontrada na tabela " + model.tableName(), this); }

  propertyName.isNull() ? mapper.addMapping(widget, model.fieldIndex(key)) : mapper.addMapping(widget, model.fieldIndex(key), propertyName);
}

QString RegisterDialog::requiredStyle() { return (QString("background-color: rgb(255, 255, 127)")); }

void RegisterDialog::closeEvent(QCloseEvent *event) { askSaveBeforeClosing() ? event->accept() : event->ignore(); }

void RegisterDialog::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    event->accept();
    close();
    return;
  }

  QDialog::keyPressEvent(event);
}

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

bool RegisterDialog::askSaveBeforeClosing() {
  if (isDirty) {
    QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Deseja salvar as alterações?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, this);
    msgBox.button(QMessageBox::Yes)->setText("Salvar");
    msgBox.button(QMessageBox::No)->setText("Descartar");
    msgBox.button(QMessageBox::Cancel)->setText("Cancelar");

    const int escolha = msgBox.exec();

    if (escolha == QMessageBox::Yes) { save(); }
    if (escolha == QMessageBox::No) {}
    if (escolha == QMessageBox::Cancel) { return false; }
  }

  return true;
}

bool RegisterDialog::newRegister() {
  if (not askSaveBeforeClosing()) { return false; }

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

  for (auto *const lineEdit : lineEdits) {
    if (lineEdit->objectName().startsWith("lineEdit")) { lineEdit->clear(); }
  }

  const auto itemBoxes = findChildren<ItemBox *>(QRegularExpression("itemBox"));

  for (auto *const itemBox : itemBoxes) {
    if (itemBox->objectName().startsWith("itemBox")) { itemBox->clear(); }
  }

  const auto doubleSpinBoxes = findChildren<QDoubleSpinBox *>(QRegularExpression("doubleSpinBox"));

  for (auto *const doubleSpinBox : doubleSpinBoxes) {
    if (doubleSpinBox->objectName().startsWith("doubleSpinBox")) { doubleSpinBox->setValue(0); }
  }

  const auto spinBoxes = findChildren<QSpinBox *>(QRegularExpression("spinBox"));

  for (auto *const spinBox : spinBoxes) {
    if (spinBox->objectName().startsWith("spinBox")) { spinBox->setValue(0); }
  }

  const auto radioButtons = findChildren<QRadioButton *>(QRegularExpression("radioButton"));

  for (auto *const radioButton : radioButtons) {
    if (radioButton->objectName().startsWith("radioButton")) { radioButton->setChecked(false); }
  }

  const auto checkBoxes = findChildren<QCheckBox *>(QRegularExpression("checkBox"));

  for (auto *const checkBox : checkBoxes) {
    if (checkBox->objectName().startsWith("checkBox")) { checkBox->setChecked(false); }
  }

  const auto comboBoxes = findChildren<QComboBox *>(QRegularExpression("comboBox"));

  for (auto *const comboBox : comboBoxes) {
    if (comboBox->objectName().startsWith("comboBox")) { comboBox->setCurrentIndex(0); }
  }
}

int RegisterDialog::removeBox() {
  QMessageBox msgBox(QMessageBox::Question, "Atenção!", "Tem certeza que deseja desativar?", QMessageBox::Yes | QMessageBox::No, this);
  msgBox.button(QMessageBox::Yes)->setText("Desativar");
  msgBox.button(QMessageBox::No)->setText("Voltar");

  return msgBox.exec();
}

void RegisterDialog::remove() {
  if (removeBox() == QMessageBox::Yes) {
    qApp->startTransaction("RegisterDialog::remove");

    setData("desativado", true);

    model.submitAll();

    qApp->endTransaction();

    qApp->enqueueInformation("Cadastro desativado com sucesso!");

    isDirty = false;

    newRegister();
  }
}

bool RegisterDialog::validaCNPJ(const QString &text) {
  // Preserve legacy contract: malformed length → return false (caller asks
  // user to keep typing); bad checksum → throw so the QMessageBox surfaces.
  const QString cnpj = QString(text).remove(".").remove("/").remove("-");

  if (cnpj.size() != 14) { return false; }

  if (not validators::cnpjValido(text)) { throw RuntimeError("CNPJ inválido!", this); }

  return true;
}

bool RegisterDialog::validaCPF(const QString &text) {
  // Same contract as validaCNPJ — see comment there.
  const QString cpf = QString(text).remove(".").remove("-");

  if (cpf.size() != 11) { return false; }

  if (not validators::cpfValido(text)) { throw RuntimeError("CPF inválido!", this); }

  return true;
}

// TODO: no lugar de qualquer alteracao em lineEdit sair marcando dirty fazer uma analise tela por tela do que pode ser considerado edicao
void RegisterDialog::marcarDirty() { isDirty = true; }
