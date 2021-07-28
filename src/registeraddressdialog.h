#pragma once

#include <QSqlRecord>

#include "registerdialog.h"

class RegisterAddressDialog : public RegisterDialog {
  Q_OBJECT

public:
  explicit RegisterAddressDialog(const QString &table, const QString &primaryKeyStr, QWidget *parent);

protected:
  // attributes
  int currentRowEnd = -1;
  QDataWidgetMapper mapperEnd;
  QList<QSqlRecord> backupEndereco;
  SqlTableModel modelEnd;
  // methods
  auto dataEnd(const QString &key) const -> QVariant;
  auto getCodigoUF(const QString &uf) const -> int;
  auto newRegister() -> bool override;
  auto setDataEnd(const QString &key, const QVariant &value) -> void;
  auto verificaEndereco(const QString &cidade, const QString &uf) -> void;

private:
  // methods
  auto setupTables(const QString &table) -> void;
};
