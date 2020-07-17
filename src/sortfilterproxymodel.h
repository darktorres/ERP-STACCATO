#pragma once

#include <QSortFilterProxyModel>

class SortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  explicit SortFilterProxyModel(QAbstractItemModel *model, QObject *parent);
};
