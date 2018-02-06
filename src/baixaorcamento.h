#ifndef BAIXAORCAMENTO_H
#define BAIXAORCAMENTO_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class BaixaOrcamento;
}

class BaixaOrcamento final : public Dialog {
  Q_OBJECT

public:
  explicit BaixaOrcamento(const QString &idOrcamento, QWidget *parent = nullptr);
  ~BaixaOrcamento();

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::BaixaOrcamento *ui;
  // methods
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
};

#endif // BAIXAORCAMENTO_H
