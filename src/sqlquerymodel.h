#ifndef SQLQUERYMODEL_H
#define SQLQUERYMODEL_H

#include <QSqlQueryModel>

class SqlQueryModel final : public QSqlQueryModel {
  Q_OBJECT

public:
  explicit SqlQueryModel(QObject *parent = nullptr);
  bool setHeaderData(const QString &column, const QVariant &value);
  QVariant data(const int row, const QString &column) const;
  virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private:
  using QSqlQueryModel::data;
  using QSqlQueryModel::setHeaderData;
};

#endif // SQLQUERYMODEL_H
