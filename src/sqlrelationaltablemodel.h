#pragma once

#include <QAbstractProxyModel>
#include <QSqlRelationalTableModel>

class Condition {
public:
  Condition(const QString &column, const QVariant &condition, const bool equal = true) : column(column), condition(condition), equal(equal) {}

  QString column;
  QVariant condition;
  bool equal;
};

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
  auto fieldIndex(const QString &fieldName, const bool silent = false) const -> int;
  auto match(const QString &column, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const -> QModelIndexList;
  auto multiMatch(const QVector<Condition> conditions, bool allHits = true) const -> QVector<int>;
  auto setFilter(const QString &filter) -> void final;
  auto setHeaderData(const QString &column, const QVariant &value) -> bool;
  auto setSort(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;
  auto setTable(const QString &tableName) -> void final;
  auto supportedDropActions() const -> Qt::DropActions final;
  auto insertRowAtEnd() -> int;

  QAbstractProxyModel *proxyModel = nullptr;

private:
  using QSqlRelationalTableModel::data;
  using QSqlRelationalTableModel::match;
  using QSqlRelationalTableModel::setData;
  using QSqlRelationalTableModel::setHeaderData;
  using QSqlRelationalTableModel::setSort;

protected:
  // attributes
  const int limit;
  // methods
  auto selectStatement() const -> QString final;
};

// TODO: add a readOnly attribute for when table is a view?
