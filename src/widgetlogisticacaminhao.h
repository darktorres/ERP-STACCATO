#ifndef WIDGETLOGISTICACAMINHAO_H
#define WIDGETLOGISTICACAMINHAO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogisticaCaminhao;
}

class WidgetLogisticaCaminhao final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaCaminhao(QWidget *parent = nullptr);
  ~WidgetLogisticaCaminhao();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelCaminhao;
  SqlRelationalTableModel modelCarga;
  Ui::WidgetLogisticaCaminhao *ui;
  // methods
  auto setupTables() -> void;
  auto on_table_clicked(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
};

#endif // WIDGETLOGISTICACAMINHAO_H
