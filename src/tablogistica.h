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
  SqlTableModel modelViewLogistica;
  Ui::TabLogistica *ui;
  // methods
  auto on_pushButtonLimparFiltro_clicked() -> void;
  auto on_tabWidgetLogistica_currentChanged() -> void;
  auto on_tableForn_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
