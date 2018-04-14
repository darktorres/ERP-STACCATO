#ifndef WIDGETNFEENTRADA_H
#define WIDGETNFEENTRADA_H

#include "sqlrelationaltablemodel.h"
#include "widget.h"

namespace Ui {
class WidgetNfeEntrada;
}

class WidgetNfeEntrada final : public Widget {
  Q_OBJECT

public:
  explicit WidgetNfeEntrada(QWidget *parent = nullptr);
  ~WidgetNfeEntrada();
  auto updateTables() -> bool;

private:
  // attributes
  SqlRelationalTableModel modelViewNFeEntrada;
  Ui::WidgetNfeEntrada *ui;
  // methods
  auto cancelar(const int row) -> bool;
  auto setupTables() -> void;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonCancelarNFe_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto on_table_entered(const QModelIndex &) -> void;
};

#endif // WIDGETNFEENTRADA_H
