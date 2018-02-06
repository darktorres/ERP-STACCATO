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
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::WidgetCompraDevolucao *ui;
  // methods
  auto on_pushButtonDevolucaoFornecedor_clicked() -> void;
  auto on_pushButtonRetornarEstoque_clicked() -> void;
  auto on_radioButtonFiltroPendente_toggled(bool checked) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto retornarEstoque(const QModelIndexList &list) -> bool;
  auto setupTables() -> void;
};

#endif // WIDGETCOMPRADEVOLUCAO_H
