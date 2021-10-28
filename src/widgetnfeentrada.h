#pragma once

#include "sqltablemodel.h"

#include <QStack>
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
  QStack<int> blockingSignals;
  QTimer timer;
  SqlTableModel model;
  Ui::WidgetNfeEntrada *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto delayFiltro() -> void;
  auto inutilizar(const int row) -> void;
  auto montaFiltro() -> void;
  auto on_pushButtonExportar_clicked() -> void;
  auto on_pushButtonInutilizarNFe_clicked() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
