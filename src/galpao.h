#pragma once

#include "sqltablemodel.h"

#include <QGraphicsScene>
#include <QWidget>

namespace Ui {
class Galpao;
}

class Galpao : public QWidget {
  Q_OBJECT

public:
  explicit Galpao(QWidget *parent = nullptr);
  ~Galpao();

  auto resetTables() -> void;
  auto updateTables() -> void;

private:
  // attributes
  bool isSet = false;
  bool modelIsSet = false;
  QGraphicsScene *scene;
  SqlTableModel modelTranspAgend;
  Ui::Galpao *ui;
  // methods
  auto carregarPallets() -> void;
  auto on_dateTimeEdit_dateChanged(const QDate &) -> void;
  auto on_groupBoxEdicao_toggled(bool checked) -> void;
  auto on_itemBoxVeiculo_textChanged(const QString &) -> void;
  auto on_pushButtonCriarPallet_clicked() -> void;
  auto on_pushButtonRemoverPallet_clicked() -> void;
  auto on_table_selectionChanged() -> void;
  auto resizeEvent(QResizeEvent *event) -> void override;
  auto salvarPallets() -> void;
  auto setConnections() -> void;
  auto setFilter() -> void;
  auto setupTables() -> void;
  auto unselectOthers() -> void;
};
