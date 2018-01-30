#ifndef WIDGETLOGISTICA_H
#define WIDGETLOGISTICA_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetLogistica;
}

class WidgetLogistica final : public Widget {
  Q_OBJECT

public:
  explicit WidgetLogistica(QWidget *parent = nullptr);
  ~WidgetLogistica();
  bool updateTables();

private slots:
  void on_tableForn_activated(const QModelIndex &index);
  void on_tabWidgetLogistica_currentChanged(const int);

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::WidgetLogistica *ui;
  // methods
  void setConnections();
};

#endif // WIDGETLOGISTICA_H
