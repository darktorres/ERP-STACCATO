#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CadastroFuncionario;
}

class CadastroFuncionario : public QDialog {
  Q_OBJECT

public:
  explicit CadastroFuncionario(QWidget *parent = nullptr);
  ~CadastroFuncionario();

private:
  // attributes
  SqlTableModel model;
  Ui::CadastroFuncionario *ui;
  // methods
  auto fillComboBoxRegime() -> void;
  auto on_checkBoxDesativado_toggled(const bool checked) -> void;
  auto on_comboBoxRegime_currentTextChanged(const QString &text) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
