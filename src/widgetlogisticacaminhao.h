#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetLogisticaCaminhao;
}

class WidgetLogisticaCaminhao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaCaminhao(QWidget *parent);
  ~WidgetLogisticaCaminhao();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelCaminhao;
  SqlTableModel modelCarga;
  Ui::WidgetLogisticaCaminhao *ui;
  // methods
  auto on_checkBoxDesativados_toggled(const bool checked) -> void;
  auto on_table_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
