#ifndef SQLTABLEMODEL_H
#define SQLTABLEMODEL_H

#include <QSqlRelationalTableModel>

class SqlRelationalTableModel final : public QSqlRelationalTableModel {
  Q_OBJECT

public:
  explicit SqlRelationalTableModel(const int limit = 0, QObject *parent = nullptr);
  [[nodiscard]] auto setData(const int row, const int column, const QVariant &value) -> bool;
  [[nodiscard]] auto setData(const int row, const QString &column, const QVariant &value) -> bool;
  auto setHeaderData(const QString &column, const QVariant &value) -> bool;
  auto data(const int row, const int column) const -> QVariant;
  auto data(const int row, const QString &column) const -> QVariant;
  auto match(const QString &column, const QVariant &value, int hits = 1, Qt::MatchFlags flags = Qt::MatchFlags(Qt::MatchStartsWith | Qt::MatchWrap)) const -> QModelIndexList;
  auto supportedDropActions() const -> Qt::DropActions final;
  auto flags(const QModelIndex &index) const -> Qt::ItemFlags final;

signals:
  void errorSignal(const QString &error) const;

private:
  using QSqlRelationalTableModel::data;
  using QSqlRelationalTableModel::match;
  using QSqlRelationalTableModel::setData;
  using QSqlRelationalTableModel::setHeaderData;

protected:
  // attributes
  const int limit;
  // methods
  auto selectStatement() const -> QString final;
};

#endif // SQLTABLEMODEL_H
