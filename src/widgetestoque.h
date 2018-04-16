#ifndef WIDGETESTOQUE_H
#define WIDGETESTOQUE_H

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetEstoque;
}

class WidgetEstoque final : public Widget {
  Q_OBJECT

public:
  explicit WidgetEstoque(QWidget *parent = nullptr);
  ~WidgetEstoque();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlQueryModel model;
  Ui::WidgetEstoque *ui;
  // methods
  auto gerarExcel(const QString &arquivoModelo, const QString &fileName, const SqlQueryModel &modelContabil) -> bool;
  auto montaFiltro() -> void;
  auto on_pushButtonRelatorio_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETESTOQUE_H
