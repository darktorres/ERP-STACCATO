#pragma once

#include "sqltablemodel.h"

#include <QDialog>

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
  Ui::PrecoEstoque *ui;
  // methods
  auto on_lineEditBusca_textChanged(const QString &) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
