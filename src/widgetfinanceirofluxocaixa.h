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
  SqlQueryModel modelCaixa;
  SqlQueryModel modelCaixa2;
  SqlQueryModel modelFuturo;
  QString filtroData;
  Ui::WidgetFinanceiroFluxoCaixa *ui;
  // methods
  auto alterarData() -> void;
  auto montaFiltro() -> void;
  auto montaTabela1() -> void;
  auto montaTabela2() -> void;
  auto montaTabela3() -> void;
  auto on_groupBoxCaixa1_toggled(const bool checked) -> void;
  auto on_groupBoxCaixa2_toggled(const bool checked) -> void;
  auto on_tableCaixa1_activated(const QModelIndex &index) -> void;
  auto on_tableCaixa2_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
};
