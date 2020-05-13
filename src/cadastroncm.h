#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CadastroNCM;
}

class CadastroNCM final : public QDialog {
  Q_OBJECT

public:
  explicit CadastroNCM(QWidget *parent = nullptr);
  ~CadastroNCM();

private:
  // attributes
  Ui::CadastroNCM *ui;
  SqlTableModel model;
  // methods
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonAdicionar_clicked() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
};
