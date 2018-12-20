#ifndef WIDGETNFEENTRADA_H
#define WIDGETNFEENTRADA_H

#include <QWidget>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class WidgetNfeEntrada;
}

class WidgetNfeEntrada final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfeEntrada(QWidget *parent = nullptr);
  ~WidgetNfeEntrada();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  SqlRelationalTableModel modelViewNFeEntrada;
  Ui::WidgetNfeEntrada *ui;
  // methods
  auto cancelar(const int row) -> bool;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonRemoverNFe_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};

#endif // WIDGETNFEENTRADA_H
