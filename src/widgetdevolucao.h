#pragma once

#include "sqltablemodel.h"

#include <QStack>
#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetDevolucao;
}

class WidgetDevolucao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetDevolucao(QWidget *parent);
  ~WidgetDevolucao() final;

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  QStack<int> blockingSignals;
  SqlTableModel model;
  Ui::WidgetDevolucao *ui;
  // methods
  auto ajustarGroupBoxStatus() -> void;
  auto montaFiltro() -> void;
  auto on_pushButtonGerarNFe_clicked() -> void;
  auto on_table_doubleClicked(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
