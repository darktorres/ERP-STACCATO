#pragma once

#include "sqlrelationaltablemodel.h"

#include <QDialog>

namespace Ui {
class InserirTransferencia;
}

class InserirTransferencia final : public QDialog {
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
