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
  bool updateTables();

private slots:
  void on_checkBoxMostrarFiltros_toggled(bool checked);
  void on_pushButtonProximo_clicked();
  void on_pushButtonAnterior_clicked();
  void on_calendarWidget_selectionChanged();

private:
  // attributes
  bool setup = false;
  Ui::WidgetLogisticaCalendario *ui;
  // methods
  bool updateCalendar(const QDate &startDate);
  void updateFilter();
};

#endif // WIDGETLOGISTICACALENDARIO_H
