#ifndef WIDGETLOGISTICACALENDARIO_H
#define WIDGETLOGISTICACALENDARIO_H

#include <QWidget>

namespace Ui {
class WidgetLogisticaCalendario;
}

class WidgetLogisticaCalendario final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetLogisticaCalendario(QWidget *parent = nullptr);
  ~WidgetLogisticaCalendario();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  Ui::WidgetLogisticaCalendario *ui;
  // methods
  auto listarVeiculos() -> void;
  auto on_calendarWidget_selectionChanged() -> void;
  auto on_checkBoxMostrarFiltros_toggled(bool checked) -> void;
  auto on_pushButtonAnterior_clicked() -> void;
  auto on_pushButtonProximo_clicked() -> void;
  auto setConnections() -> void;
  auto updateCalendar(const QDate &startDate) -> void;
  auto updateFilter() -> void;
};

#endif // WIDGETLOGISTICACALENDARIO_H
