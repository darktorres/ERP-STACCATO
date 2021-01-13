#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetNfeEntrada;
}

class WidgetNfeEntrada final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetNfeEntrada(QWidget *parent);
  ~WidgetNfeEntrada();
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel modelViewNFeEntrada;
  Ui::WidgetNfeEntrada *ui;
  // methods
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonExportar_clicked() -> void;
  auto on_pushButtonRemoverNFe_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto remover(const int row) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
