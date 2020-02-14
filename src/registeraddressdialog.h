#pragma once

#include <QSqlRecord>

#include "registerdialog.h"

class RegisterAddressDialog : public RegisterDialog {

public:
  explicit RegisterAddressDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent = nullptr);

protected:
  // attributes
  QList<QSqlRecord> backupEndereco;
  int currentRowEnd = -1;
  QDataWidgetMapper mapperEnd;
  SqlTableModel modelEnd;
  // methods
  auto setDataEnd(const QString &key, const QVariant &value) -> bool;
  auto getCodigoUF(const QString &uf) const -> int;
  virtual auto newRegister() -> bool override;

private:
  auto setupTables(const QString &table) -> void;
};
