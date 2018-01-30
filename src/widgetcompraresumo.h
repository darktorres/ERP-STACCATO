#ifndef WIDGETCOMPRARESUMO_H
#define WIDGETCOMPRARESUMO_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetCompraResumo;
}

class WidgetCompraResumo : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraResumo(QWidget *parent = nullptr);
  ~WidgetCompraResumo();
  bool updateTables();

signals:
  void errorSignal(const QString &error);

private:
  // attributes
  Ui::WidgetCompraResumo *ui;
  SqlRelationalTableModel modelResumo;
  // methods
  void setupTables();
};

#endif // WIDGETCOMPRARESUMO_H
