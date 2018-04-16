#ifndef WIDGETCOMPRADEVOLUCAO_H
#define WIDGETCOMPRADEVOLUCAO_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetCompraDevolucao;
}

class WidgetCompraDevolucao final : public Widget {
  Q_OBJECT

public:
  explicit WidgetCompraDevolucao(QWidget *parent = nullptr);
  ~WidgetCompraDevolucao();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelVendaProduto;
  Ui::WidgetCompraDevolucao *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_pushButtonDevolucaoFornecedor_clicked() -> void;
  auto on_pushButtonRetornarEstoque_clicked() -> void;
  auto on_radioButtonFiltroDevolvido_toggled(const bool) -> void;
  auto on_radioButtonFiltroPendente_toggled(const bool) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto retornarEstoque(const QModelIndexList &list) -> bool;
  auto setupTables() -> void;
  auto setConnections() -> void;
};

#endif // WIDGETCOMPRADEVOLUCAO_H
