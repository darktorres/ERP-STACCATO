#pragma once

#include "cnab.h"
#include "sqltablemodel.h"

#include <QTimer>
#include <QWidget>

namespace Ui {
class WidgetGare;
}

class WidgetGare : public QWidget {
  Q_OBJECT

public:
  explicit WidgetGare(QWidget *parent);
  ~WidgetGare();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QTimer timer;
  SqlTableModel model;
  Ui::WidgetGare *ui;
  // methods
  auto delayFiltro() -> void;
  auto habilitarBotoes() -> void;
  auto montaFiltro() -> void;
  auto montarGare(const QModelIndexList &selection) -> QVector<CNAB::Gare>;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonDarBaixaItau_clicked() -> void;
  auto on_pushButtonRemessaItau_clicked() -> void;
  auto on_pushButtonRetornoItau_clicked() -> void;
  auto on_tableSelection_changed() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
