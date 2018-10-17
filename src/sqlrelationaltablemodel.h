#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlRelationalTableModel>

class SqlRelationalTableModel final : public QSqlRelationalTableModel {
  Q_OBJECT

public:
  explicit SqlRelationalTableModel(const int limit = 0, QObject *parent = nullptr);
  [[nodiscard]] auto select() -> bool final;
  [[nodiscard]] auto setData(const int row, const QString &column, const QVariant &value) -> bool;
  [[nodiscard]] auto setData(const int row, const int column, const QVariant &value) -> bool;
  [[nodiscard]] auto submitAll() -> bool;
  auto data(const int row, const QString &column) const -> QVariant;
  auto data(const int row, const int column) const -> QVariant;
  auto match(const QString &column, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const -> QModelIndexList;
  auto setHeaderData(const QString &column, const QVariant &value) -> bool;
  auto supportedDropActions() const -> Qt::DropActions final;

private:
  using QSqlRelationalTableModel::data;
  using QSqlRelationalTableModel::match;
  using QSqlRelationalTableModel::select;
  using QSqlRelationalTableModel::setData;
  using QSqlRelationalTableModel::setHeaderData;
  using QSqlRelationalTableModel::submitAll;

protected:
  // attributes
  const int limit;
  // methods
  auto selectStatement() const -> QString final;
};

#endif // SQLTABLEMODEL_H
