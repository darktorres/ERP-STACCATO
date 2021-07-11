#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QTimer>

namespace Ui {
class PrecoEstoque;
}

class PrecoEstoque final : public QDialog {
  Q_OBJECT

public:
  explicit PrecoEstoque(QWidget *parent);
  ~PrecoEstoque();

private:
  // attributes
  SqlTableModel modelProduto;
  QTimer timer;
  Ui::PrecoEstoque *ui;
  // methods
  auto delayFiltro() -> void;
  auto on_lineEditBusca_textChanged() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
