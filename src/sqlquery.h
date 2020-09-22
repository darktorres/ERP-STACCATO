#pragma once

#include <QSqlQuery>
#include <QVariant>

class SqlQuery final : public QSqlQuery {

public:
  explicit SqlQuery();

  auto value(const QString &name) const -> QVariant;
  auto value(int i) const -> QVariant;
};
