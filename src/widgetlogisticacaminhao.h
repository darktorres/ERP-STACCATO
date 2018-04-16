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
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelCaminhao;
  SqlRelationalTableModel modelCarga;
  Ui::WidgetLogisticaCaminhao *ui;
  // methods
  auto on_table_clicked(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETLOGISTICACAMINHAO_H
