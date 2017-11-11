#ifndef SQLQUERYMODEL_H
#define SQLQUERYMODEL_H

#include <QSqlQueryModel>

class SqlQueryModel final : public QSqlQueryModel {
  Q_OBJECT

public:
  explicit SqlQueryModel(QObject *parent = 0);
  bool setHeaderData(const QString &column, const QVariant &value);
  QVariant data(const int row, const QString &column) const;

private:
  using QSqlQueryModel::data;
  using QSqlQueryModel::setHeaderData;
};

#endif // SQLQUERYMODEL_H
