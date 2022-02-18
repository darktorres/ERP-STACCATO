#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class TabLogistica;
}

class TabLogistica final : public QWidget {
  Q_OBJECT

public:
  explicit TabLogistica(QWidget *parent);
  ~TabLogistica();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  Ui::TabLogistica *ui;
  // methods
  auto on_tabWidgetLogistica_currentChanged() -> void;
  auto setConnections() -> void;
};
