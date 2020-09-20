#pragma once

#include <QSqlQuery>
#include <QVariant>

class SqlQuery final : public QSqlQuery {

public:
  explicit SqlQuery();
  auto exec() -> bool;
  auto exec(const QString &query) -> bool;
  auto value(const QString &name) -> QVariant const;
};
