#pragma once

#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetDevolucao;
}

class WidgetDevolucao final : public QWidget {
  Q_OBJECT

public:
  explicit WidgetDevolucao(QWidget *parent = nullptr);
  ~WidgetDevolucao() final;
  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel model;
  Ui::WidgetDevolucao *ui;
  // methods
  auto delayFiltro() -> void;
  auto montaFiltro() -> void;
  auto on_pushButtonGerarNFe_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
