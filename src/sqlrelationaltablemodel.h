#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlRelationalTableModel>

class SqlRelationalTableModel final : public QSqlRelationalTableModel {
  Q_OBJECT

public:
  explicit SqlRelationalTableModel(QObject *parent = 0);
  [[nodiscard]] bool setData(const int row, const int column, const QVariant &value);
  [[nodiscard]] bool setData(const int row, const QString &column, const QVariant &value);
  bool setHeaderData(const QString &column, const QVariant &value);
  QVariant data(const int row, const int column) const;
  QVariant data(const int row, const QString &column) const;
  Qt::DropActions supportedDropActions() const final;
  Qt::ItemFlags flags(const QModelIndex &index) const final;
  void setLimit(int value);

private:
  using QSqlRelationalTableModel::data;
  using QSqlRelationalTableModel::setData;
  using QSqlRelationalTableModel::setHeaderData;

protected:
  QString selectStatement() const final;
  // REFAC: put limit in constructor and set const
  int limit = 0;
};

#endif // SQLTABLEMODEL_H
