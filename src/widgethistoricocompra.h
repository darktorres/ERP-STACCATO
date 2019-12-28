#pragma once

#include <QWidget>

#include "sqlrelationaltablemodel.h"
#include "sqltreemodel.h"

namespace Ui {
class WidgetHistoricoCompra;
}

class WidgetHistoricoCompra final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetHistoricoCompra(QWidget *parent = nullptr);
  ~WidgetHistoricoCompra();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewComprasFinanceiro;
  SqlRelationalTableModel modelProdutos;
  SqlRelationalTableModel modelProdutos2;
  SqlRelationalTableModel modelNFe;
  SqlTreeModel modelTree;
  Ui::WidgetHistoricoCompra *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonDanfe_clicked() -> void;
  auto on_tablePedidos_clicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setTreeView() -> void;
  auto setupTables() -> void;
};
