#pragma once

#include "sqltablemodel.h"
#include "sqltreemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetHistoricoCompra;
}

class WidgetHistoricoCompra final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetHistoricoCompra(QWidget *parent);
  ~WidgetHistoricoCompra();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelViewComprasFinanceiro;
  SqlTableModel modelProdutos;
  SqlTableModel modelProdutos2;
  SqlTableModel modelNFe;
  SqlTreeModel modelTree;
  Ui::WidgetHistoricoCompra *ui;
  // methods
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonDanfe_clicked() -> void;
  auto on_tablePedidos_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setTreeView() -> void;
  auto setupTables() -> void;
};
