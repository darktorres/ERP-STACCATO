#ifndef BAIXAORCAMENTO_H
#define BAIXAORCAMENTO_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class BaixaOrcamento;
}

class BaixaOrcamento final : public QDialog {
  Q_OBJECT

public:
  explicit BaixaOrcamento(const QString &idOrcamento, QWidget *parent = nullptr);
  ~BaixaOrcamento();

private:
  // attributes
  SqlRelationalTableModel modelOrcamento;
  Ui::BaixaOrcamento *ui;
  // methods
  auto setupTables(const QString &idOrcamento) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
};

#endif // BAIXAORCAMENTO_H
