#pragma once

#include <QAbstractProxyModel>
#include <QSqlTableModel>

class Condition {
public:
  Condition(const QString &column, const QVariant &value, const bool equal = true) : column(column), value(value), equal(equal) {}

  QString column;
  QVariant value;
  bool equal;
};

class SqlTableModel final : public QSqlTableModel {
  Q_OBJECT

public:
  explicit SqlTableModel(const int limit, QObject *parent);
  explicit SqlTableModel(const int limit);
  explicit SqlTableModel();

  auto data(const int row, const QString &column) const -> QVariant;
  auto data(const int row, const int column) const -> QVariant;
  auto fieldIndex(const QString &fieldName, const bool silent = false) const -> int;
  auto insertRowAtEnd() -> int;
  auto match(const QString &column, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const -> QModelIndexList;
  auto multiMatch(const QVector<Condition> conditions, bool allHits = true) const -> QVector<int>;
  auto removeSelection(const QModelIndexList &selection) -> void;
  auto select() -> bool;
  auto setData(const int row, const QString &column, const QVariant &value) -> void;
  auto setData(const int row, const int column, const QVariant &value) -> void;
  auto setFilter(const QString &filter) -> void final;
  auto setHeaderData(const QString &column, const QVariant &value) -> bool;
  auto setSort(const QString &column, Qt::SortOrder order = Qt::AscendingOrder) -> void;
  auto setTable(const QString &tableName) -> void final;
  auto submitAll() -> void;
  auto supportedDropActions() const -> Qt::DropActions final;

  QAbstractProxyModel *proxyModel = nullptr;

private:
  using QSqlTableModel::data;
  using QSqlTableModel::match;
  using QSqlTableModel::setData;
  using QSqlTableModel::setHeaderData;
  using QSqlTableModel::setSort;

protected:
  // attributes
  int const limit = 0;
  // methods
  auto selectStatement() const -> QString final;
};

// TODO: add a readOnly attribute for when table is a view?
