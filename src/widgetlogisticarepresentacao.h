#ifndef WIDGETLOGISTICAREPRESENTACAO_H
#define WIDGETLOGISTICAREPRESENTACAO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaRepresentacao;
}

class WidgetLogisticaRepresentacao final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaRepresentacao(QWidget *parent = nullptr);
  ~WidgetLogisticaRepresentacao();
  auto updateTables() -> bool;
  auto tableFornLogistica_activated(const QString &fornecedor) -> void;

private:
  // attributes
  SqlRelationalTableModel modelViewLogisticaRepresentacao;
  QString fornecedor;
  Ui::WidgetLogisticaRepresentacao *ui;
  // methods
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonMarcarEntregue_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto processRows(const QModelIndexList &list, const QDateTime &dataEntrega, const QString &recebeu) -> bool;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICAREPRESENTACAO_H
