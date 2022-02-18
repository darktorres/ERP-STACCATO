#pragma once

#include <QSqlQuery>
#include <QVariant>

class SqlQuery final : public QSqlQuery {

public:
  //  auto bindValue(const QString &placeholder, const QVariant &val, const bool adjustValue = true, QSql::ParamType type = QSql::In) -> void;
  auto value(const QString &name) const -> QVariant;
  auto value(const int i) const -> QVariant;
};
