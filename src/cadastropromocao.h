#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CadastroPromocao;
}

class CadastroPromocao : public QDialog {
  Q_OBJECT

public:
  explicit CadastroPromocao(QWidget *parent = nullptr);
  ~CadastroPromocao();

private slots:
  void on_pushButtonDescadastrar_clicked();

  void on_radioButtonTodos_toggled(bool checked);

  void on_radioButtonStaccatoOFF_toggled(bool checked);

  void on_radioButtonEstoque_toggled(bool checked);

private:
  // attributes
  Ui::CadastroPromocao *ui;
  SqlTableModel model;
  // methods
  auto on_itemBoxFornecedor_textChanged(const QString &text) -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto setupTables() -> void;
};
