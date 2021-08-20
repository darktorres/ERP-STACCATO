#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CadastroNCM;
}

class CadastroNCM final : public QDialog {
  Q_OBJECT

public:
  explicit CadastroNCM(QWidget *parent);
  ~CadastroNCM();

private:
  // attributes
  SqlTableModel model;
  Ui::CadastroNCM *ui;
  // methods
  auto on_lineEditBusca_textChanged(const QString &text) -> void;
  auto on_pushButtonAdicionar_clicked() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonRemover_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto verificaNCM(const QModelIndex &index) -> void;
  auto verifyFields() -> void;
};
