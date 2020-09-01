#pragma once

#include "cnab.h"
#include "sqltablemodel.h"

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
  SqlTableModel model;
  Ui::WidgetGare *ui;
  // methods
  auto montaFiltro() -> void;
  auto montarGare(const QModelIndexList selection) -> QVector<CNAB::Gare>;
  auto on_pushButtonDarBaixaItau_clicked() -> void;
  auto on_pushButtonRemessaItau_clicked() -> void;
  auto on_pushButtonRetornoItau_clicked() -> void;
  auto on_tableSelection_changed() -> void;
  auto on_table_activated(const QModelIndex &index) -> void;
  auto setupTables() -> void;
};
