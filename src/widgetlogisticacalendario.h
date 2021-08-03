#pragma once

#include <QStack>
#include <QWidget>

namespace Ui {
class WidgetLogisticaCalendario;
}

class WidgetLogisticaCalendario final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaCalendario(QWidget *parent);
  ~WidgetLogisticaCalendario();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QStack<int> blockingSignals;
  Ui::WidgetLogisticaCalendario *ui;
  // methods
  auto listarVeiculos() -> void;
  auto on_calendarWidget_selectionChanged() -> void;
  auto on_checkBoxMostrarFiltros_toggled(const bool checked) -> void;
  auto on_groupBoxVeiculos_toggled(const bool enabled) -> void;
  auto on_pushButtonAnterior_clicked() -> void;
  auto on_pushButtonProximo_clicked() -> void;
  auto setConnections() -> void;
  auto unsetConnections() -> void;
  auto updateCalendar(const QDate startDate) -> void;
  auto updateFilter() -> void;
};
