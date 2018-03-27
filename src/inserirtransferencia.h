#ifndef INSERIRTRANSFERENCIA_H
#define INSERIRTRANSFERENCIA_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class InserirTransferencia;
}

class InserirTransferencia final : public Dialog {
  Q_OBJECT

public:
  explicit InserirTransferencia(QWidget *parent = nullptr);
  ~InserirTransferencia();

private:
  // attributes
  SqlRelationalTableModel modelDe;
  SqlRelationalTableModel modelPara;
  Ui::InserirTransferencia *ui;
  // methods
  auto cadastrar() -> bool;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
  auto verifyFields() -> bool;
};

#endif // INSERIRTRANSFERENCIA_H
