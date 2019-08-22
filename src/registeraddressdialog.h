#pragma once

#include "registerdialog.h"

#include <QSqlRecord>

class RegisterAddressDialog : public RegisterDialog {

public:
  explicit RegisterAddressDialog(const QString &table, const QString &primaryKey, QWidget *parent = nullptr);

protected:
  // attributes
  QList<QSqlRecord> backupEndereco;
  int currentRowEnd = -1;
  QDataWidgetMapper mapperEnd;
  SqlRelationalTableModel modelEnd;
  // methods
  auto setDataEnd(const QString &key, const QVariant &value) -> bool;
  auto getCodigoUF(QString uf) const -> int;
  virtual auto newRegister() -> bool override;

private:
  auto setupTables(const QString &table) -> void;
};
