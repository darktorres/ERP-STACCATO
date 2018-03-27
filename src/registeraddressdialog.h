#ifndef REGISTERADDRESSDIALOG_H
#define REGISTERADDRESSDIALOG_H

#include "registerdialog.h"

class RegisterAddressDialog : public RegisterDialog {

public:
  explicit RegisterAddressDialog(const QString &table, const QString &primaryKey, QWidget *parent);
  auto viewRegisterById(const QVariant &id) -> bool final;

protected:
  // attributes
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

#endif // REGISTERADDRESSDIALOG_H
