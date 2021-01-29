#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class InserirTransferencia;
}

class InserirTransferencia final : public QDialog {
  Q_OBJECT

public:
  explicit InserirTransferencia(QWidget *parent);
  ~InserirTransferencia();

private:
  // attributes
  SqlTableModel modelDe;
  SqlTableModel modelPara;
  Ui::InserirTransferencia *ui;
  // methods
  auto cadastrar() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto verifyFields() -> void;
};
