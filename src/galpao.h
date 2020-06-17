#pragma once

#include "graphicsscene.h"
#include "sqltablemodel.h"

#include <QWidget>

namespace Ui {
class Galpao;
}

class Galpao : public QWidget {
  Q_OBJECT

public:
  explicit Galpao(QWidget *parent = nullptr);
  ~Galpao();

private:
  // attributes
  Ui::Galpao *ui;
  GraphicsScene *scene;
  SqlTableModel modelTranspAgend;
  // methods
  auto carregarPallets() -> void;
  auto on_dateTimeEdit_dateChanged(const QDate &date) -> void;
  auto on_itemBoxVeiculo_textChanged(const QString &) -> void;
  auto salvarPallets() -> void;
  auto setupTables() -> void;
};
