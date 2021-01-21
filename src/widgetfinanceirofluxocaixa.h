#pragma once

#include "sqlquerymodel.h"

#include <QWidget>

namespace Ui {
class WidgetFinanceiroFluxoCaixa;
}

class WidgetFinanceiroFluxoCaixa final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetFinanceiroFluxoCaixa(QWidget *parent);
  ~WidgetFinanceiroFluxoCaixa();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlQueryModel modelCaixa;
  SqlQueryModel modelCaixa2;
  SqlQueryModel modelFuturo;
  Ui::WidgetFinanceiroFluxoCaixa *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_groupBoxCaixa1_toggled(const bool checked) -> void;
  auto on_groupBoxCaixa2_toggled(const bool checked) -> void;
  auto on_tableCaixa2_activated(const QModelIndex &index) -> void;
  auto on_tableCaixa_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
};
