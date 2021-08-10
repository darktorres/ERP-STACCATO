#pragma once

#include "sqltablemodel.h"

#include <QDataWidgetMapper>
#include <QDialog>
#include <QLineEdit>

class RegisterDialog : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Cadastrar, Atualizar };
  Q_ENUM(Tipo)

  explicit RegisterDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent);
  ~RegisterDialog() override = default;

  auto setReadOnly(const bool isReadOnly) -> void;
  auto show() -> void;
  virtual auto viewRegisterById(const QVariant &id) -> bool;

signals:
  void registerUpdated(const QVariant &idCliente);

protected:
  // attributes
  bool isDirty = false;
  bool readOnly = false;
  int currentRow = -1;
  QString primaryId;
  QString primaryKey;
  SqlTableModel model;
  Tipo tipo = Tipo::Cadastrar;
  // methods
  auto addMapping(QWidget *widget, const QString &key, const QByteArray &propertyName = QByteArray()) -> void;
  auto confirmationMessage() -> bool;
  auto connectLineEditsToDirty() -> void;
  auto data(const QString &key) const -> QVariant;
  auto marcarDirty() -> void;
  auto remove() -> void;
  auto removeBox() -> int;
  auto setConnections() -> void;
  auto setData(const QString &key, const QVariant &value, const bool adjustValue = true) -> void;
  auto setForeignKey(SqlTableModel &secondaryModel) -> void;
  auto validaCNPJ(const QString &text) -> bool;
  auto validaCPF(const QString &text) -> bool;
  virtual auto clearFields() -> void = 0;
  virtual auto newRegister() -> bool;
  virtual auto save(const bool silent = false) -> void final;
  virtual auto verifyRequiredField(const QLineEdit &line) -> void;
  virtual auto viewRegister() -> bool;

private:
  // attributes
  QStringList textKeys;
  QDataWidgetMapper mapper;
  QWidget *parent = nullptr;
  // methods
  auto getTextKeys() const -> QStringList;
  auto requiredStyle() -> QString;
  auto setTextKeys(const QStringList &value) -> void;
  virtual auto cadastrar() -> void = 0;
  virtual auto closeEvent(QCloseEvent *event) -> void final;
  virtual auto keyPressEvent(QKeyEvent *event) -> void final;
  virtual auto registerMode() -> void = 0;
  virtual auto savingProcedures() -> void = 0;
  virtual auto setupMapper() -> void = 0;
  virtual auto successMessage() -> void = 0;
  virtual auto updateMode() -> void = 0;
  virtual auto verifyFields() -> void = 0;
};
