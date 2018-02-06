#ifndef WIDGETLOGISTICACALENDARIO_H
#define WIDGETLOGISTICACALENDARIO_H

#include "widget.h"

namespace Ui {
class WidgetLogisticaCalendario;
}

class WidgetLogisticaCalendario final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogisticaCalendario(QWidget *parent = nullptr);
  ~WidgetLogisticaCalendario();
  auto updateTables() -> bool;

private:
  // attributes
  bool setup = false;
  Ui::WidgetLogisticaCalendario *ui;
  // methods
  auto on_calendarWidget_selectionChanged() -> void;
  auto on_checkBoxMostrarFiltros_toggled(bool checked) -> void;
  auto on_pushButtonAnterior_clicked() -> void;
  auto on_pushButtonProximo_clicked() -> void;
  auto updateCalendar(const QDate &startDate) -> bool;
  auto updateFilter() -> void;
};

#endif // WIDGETLOGISTICACALENDARIO_H
