#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CadastroStaccatoOff;
}

class CadastroStaccatoOff : public QDialog {
  Q_OBJECT

public:
  explicit CadastroStaccatoOff(QWidget *parent = nullptr);
  ~CadastroStaccatoOff();

private:
  // attributes
  Ui::CadastroStaccatoOff *ui;
  SqlTableModel model;
  // methods
  auto on_itemBoxFornecedor_textChanged(const QString &text) -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonDescadastrar_clicked() -> void;
  auto on_radioButtonEstoque_toggled(bool checked) -> void;
  auto on_radioButtonStaccatoOFF_toggled(bool checked) -> void;
  auto on_radioButtonTodos_toggled(bool checked) -> void;
  auto setupTables() -> void;
};
