#pragma once

#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class WidgetCompraDevolucao;
}

class WidgetCompraDevolucao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetCompraDevolucao(QWidget *parent);
  ~WidgetCompraDevolucao();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlTableModel modelVendaProduto;
  Ui::WidgetCompraDevolucao *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_pushButtonDevolucaoFornecedor_clicked() -> void;
  auto on_pushButtonRetornarEstoque_clicked() -> void;
  auto on_radioButtonFiltroDevolvido_clicked(const bool) -> void;
  auto on_radioButtonFiltroPendente_clicked(const bool) -> void;
  auto on_table_selectionChanged() -> void;
  auto retornarEstoque(const QModelIndexList &list) -> bool;
  auto retornarFornecedor(const QModelIndexList &list) -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
