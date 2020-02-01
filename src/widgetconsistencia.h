#pragma once

#include "sqlrelationaltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetConsistencia;
}

class WidgetConsistencia : public QWidget {
  Q_OBJECT

public:
  explicit WidgetConsistencia(QWidget *parent = nullptr);
  ~WidgetConsistencia();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  bool modelIsSet = false;
  SqlRelationalTableModel model1;
  SqlRelationalTableModel model2;
  SqlRelationalTableModel model3;
  SqlRelationalTableModel model4;
  SqlRelationalTableModel model5;
  SqlRelationalTableModel model6;
  SqlRelationalTableModel model7;
  SqlRelationalTableModel model8;

private:
  // attributes
  Ui::WidgetConsistencia *ui;
  // methods
  auto setupTables() -> void;
};
