#pragma once

#include <QAbstractProxyModel>
#include <QSqlQueryModel>

class SqlQueryModel final : public QSqlQueryModel {
  Q_OBJECT

public:
  explicit SqlQueryModel(QObject *parent);
  explicit SqlQueryModel();

  auto data(const QModelIndex &index, const QString &column) const -> QVariant;
  auto data(const int row, const QString &column) const -> QVariant;
  auto data(const int row, const int column) const -> QVariant;
  auto fieldIndex(const QString &fieldName, const bool silent = false) const -> int;
  auto select() -> void;
  auto setHeaderData(const QString &column, const QVariant &value) -> bool;
  auto setQuery(const QString &query, const QSqlDatabase &db = QSqlDatabase()) -> void;
  auto sort(QString column, Qt::SortOrder order = Qt::AscendingOrder) -> void;
  auto sort(int column, Qt::SortOrder order = Qt::AscendingOrder) -> void override;

  QAbstractProxyModel *proxyModel = nullptr;

private:
  using QSqlQueryModel::data;
  using QSqlQueryModel::setHeaderData;
  using QSqlQueryModel::setQuery;
  using QSqlQueryModel::sort;

  // attributes
  QString base_query; // to be changed only by setQuery
  QString last_query; // to be used for refreshing tables
  // methods
  auto select(const QString &query) -> void;
};
