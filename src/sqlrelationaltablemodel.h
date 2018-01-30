#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlRelationalTableModel>

class SqlRelationalTableModel final : public QSqlRelationalTableModel {
  Q_OBJECT

public:
  explicit SqlRelationalTableModel(const int limit = 0, QObject *parent = nullptr);
  [[nodiscard]] bool setData(const int row, const int column, const QVariant &value);
  [[nodiscard]] bool setData(const int row, const QString &column, const QVariant &value);
  bool setHeaderData(const QString &column, const QVariant &value);
  QVariant data(const int row, const int column) const;
  QVariant data(const int row, const QString &column) const;
  Qt::DropActions supportedDropActions() const final;
  Qt::ItemFlags flags(const QModelIndex &index) const final;

signals:
  void errorSignal(const QString &error) const;

private:
  using QSqlRelationalTableModel::data;
  using QSqlRelationalTableModel::setData;
  using QSqlRelationalTableModel::setHeaderData;

protected:
  QString selectStatement() const final;
  const int limit;
};

#endif // SQLTABLEMODEL_H
