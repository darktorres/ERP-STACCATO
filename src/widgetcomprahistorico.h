#pragma once

#include "sqltablemodel.h"
#include "sqltreemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetCompraHistorico;
}

class WidgetCompraHistorico final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraHistorico(QWidget *parent);
  ~WidgetCompraHistorico();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  SqlTableModel modelCompras;
  SqlTableModel modelFinanceiro;
  SqlTableModel modelNFe;
  SqlTableModel modelProdutos2;
  SqlTableModel modelProdutos;
  SqlTreeModel modelTree;
  Ui::WidgetCompraHistorico *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonAlteraCodForn_clicked() -> void;
  auto on_pushButtonDanfe_clicked() -> void;
  auto on_pushButtonFollowup_clicked() -> void;
  auto on_tablePedidos_selectionChanged() -> void;
  auto on_treeView_doubleClicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setTreeView() -> void;
  auto setupTables() -> void;
};
