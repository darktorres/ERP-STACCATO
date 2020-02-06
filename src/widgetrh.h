#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetRh;
}

class WidgetRh : public QWidget {
  Q_OBJECT

public:
  explicit WidgetRh(QWidget *parent = nullptr);
  ~WidgetRh();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelRelatorio;
  SqlTableModel modelTotal;
  Ui::WidgetRh *ui;
  // methods
  auto on_dateEdit_dateChanged(const QDate &date) -> void;
  auto on_pushButtonSalvarMes_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
