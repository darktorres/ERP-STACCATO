#ifndef PRECOESTOQUE_H
#define PRECOESTOQUE_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class PrecoEstoque;
}

class PrecoEstoque final : public Dialog {
  Q_OBJECT

public:
  explicit PrecoEstoque(QWidget *parent = nullptr);
  ~PrecoEstoque();

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::PrecoEstoque *ui;
  // methods
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_table_entered(const QModelIndex &) -> void;
  auto setupTables() -> void;
};

#endif // PRECOESTOQUE_H
