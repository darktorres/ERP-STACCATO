#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QSqlTableModel>

class SortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  explicit SortFilterProxyModel(QSqlTableModel *model, QObject *parent = nullptr);
  explicit SortFilterProxyModel(QSqlQueryModel *model, QObject *parent = nullptr);
  auto data(const int row, const int column, int role = Qt::DisplayRole) const -> QVariant;
  auto data(const int row, const QString &column, int role = Qt::DisplayRole) const -> QVariant;

private:
  using QSortFilterProxyModel::data;

protected:
  const QSqlTableModel *const tableModel = nullptr;
  const QSqlQueryModel *const queryModel = nullptr;
};

#endif // SORTFILTERPROXYMODEL_H
