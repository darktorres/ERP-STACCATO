#ifndef SQLQUERYMODEL_H
#define SQLQUERYMODEL_H

#include <QSqlQueryModel>

class SqlQueryModel : public QSqlQueryModel {
  Q_OBJECT

public:
  explicit SqlQueryModel(QObject *parent = 0);
  bool setData(const int row, const int column, const QVariant &value);
  bool setData(const int row, const QString &column, const QVariant &value);
  bool setHeaderData(const QString &column, const QVariant &value);
  QVariant data(const int row, const QString &column) const;

private:
  using QSqlQueryModel::setHeaderData;
  using QSqlQueryModel::data;
};

#endif // SQLQUERYMODEL_H
