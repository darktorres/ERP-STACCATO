#ifndef SEARCHDIALOGPROXY_H
#define SEARCHDIALOGPROXY_H

#include "sortfilterproxymodel.h"
#include "sqlrelationaltablemodel.h"

class SearchDialogProxyModel final : public SortFilterProxyModel {

public:
  explicit SearchDialogProxyModel(SqlRelationalTableModel *model, QObject *parent = nullptr);
  ~SearchDialogProxyModel() final = default;
  // TODO: copy the helper 'data' function with 'row, column'
  auto data(const QModelIndex &proxyIndex, int role) const -> QVariant final;

private:
  const int estoqueColumn;
  const int promocaoColumn;
  const int descontinuadoColumn;
  const int validadeColumn;
};

#endif // SEARCHDIALOGPROXY_H
