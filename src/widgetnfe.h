#ifndef WIDGETNFE_H
#define WIDGETNFE_H

#include "widget.h"

namespace Ui {
class WidgetNfe;
}

class WidgetNfe final : public Widget {
  Q_OBJECT

public:
  explicit WidgetNfe(QWidget *parent = 0);
  ~WidgetNfe();
  bool updateTables();

private slots:
  void on_tabWidgetNfe_currentChanged(const int);

private:
  // attributes
  Ui::WidgetNfe *ui;
  // methods
  void setConnections();
};

#endif // WIDGETNFE_H
