#ifndef WIDGETGRAFICOS_H
#define WIDGETGRAFICOS_H

#include <QWidget>

namespace Ui {
class WidgetGraficos;
}

class WidgetGraficos : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGraficos(QWidget *parent = nullptr);
  ~WidgetGraficos();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  bool isSet = false;
  Ui::WidgetGraficos *ui;
};

#endif // WIDGETGRAFICOS_H
