#pragma once

#include <QSqlRecord>

#include "registerdialog.h"

class RegisterAddressDialog : public RegisterDialog {

public:
  explicit RegisterAddressDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent);

protected:
  // attributes
  int currentRowEnd = -1;
  QList<QSqlRecord> backupEndereco;
  QDataWidgetMapper mapperEnd;
  SqlTableModel modelEnd;
  // methods
  auto getCodigoUF(const QString &uf) const -> int;
  auto dataEnd(const QString &key) const -> QVariant;
  auto setDataEnd(const QString &key, const QVariant &value) -> void;
  auto verificaEndereco(const QString &cidade, const QString &uf) -> void;
  virtual auto newRegister() -> bool override;

private:
  // methods
  auto setupTables(const QString &table) -> void;
};
