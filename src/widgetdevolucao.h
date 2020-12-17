#pragma once

#include "sqltablemodel.h"

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
  SqlTableModel model;
  Ui::WidgetDevolucao *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_pushButtonGerarNFe_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
