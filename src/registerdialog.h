#pragma once

#include "sqltablemodel.h"

#include <QDataWidgetMapper>
#include <QDialog>
#include <QLineEdit>

class RegisterDialog : public QDialog {
  Q_OBJECT

public:
  explicit RegisterDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent);
  ~RegisterDialog() override = default;

  virtual auto viewRegisterById(const QVariant &id) -> bool;

  auto marcarDirty() -> void;
  auto show() -> void;

signals:
  void registerUpdated(const QVariant &idCliente);

protected:
  // attributes
  enum class Tipo { Cadastrar, Atualizar } tipo = Tipo::Cadastrar;
  bool isDirty = false;
  int currentRow = -1;
  QString primaryId;
  QString primaryKey;
  QStringList textKeys;
  SqlTableModel model;
  QDataWidgetMapper mapper;
  QWidget *parent;
  // methods
  virtual auto cadastrar() -> void = 0;
  virtual auto clearFields() -> void = 0;
  virtual auto closeEvent(QCloseEvent *event) -> void final;
  virtual auto keyPressEvent(QKeyEvent *event) -> void final;
  virtual auto newRegister() -> bool;
  virtual auto registerMode() -> void = 0;
  virtual auto save(const bool silent = false) -> void final;
  virtual auto savingProcedures() -> void = 0;
  virtual auto setupMapper() -> void = 0;
  virtual auto successMessage() -> void = 0;
  virtual auto updateMode() -> void = 0;
  virtual auto verifyFields() -> void = 0;
  virtual auto verifyRequiredField(QLineEdit &line) -> void;
  virtual auto viewRegister() -> bool;

  auto addMapping(QWidget *widget, const QString &key, const QByteArray &propertyName = QByteArray()) -> void;
  auto confirmationMessage() -> bool;
  auto data(const QString &key) -> QVariant;
  auto getTextKeys() const -> QStringList;
  auto remove() -> void;
  auto removeBox() -> int;
  auto requiredStyle() -> QString;
  auto setData(const QString &key, const QVariant &value) -> void;
  auto setForeignKey(SqlTableModel &secondaryModel) -> void;
  auto setTextKeys(const QStringList &value) -> void;
  auto validaCNPJ(const QString &text) -> bool;
  auto validaCPF(const QString &text) -> bool;
  auto verifyFields(const QList<QLineEdit *> &list) -> void;
};
