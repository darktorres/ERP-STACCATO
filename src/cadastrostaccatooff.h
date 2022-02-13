#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QStack>

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
  QStack<int> blockingSignals;
  SqlTableModel model;
  Ui::CadastroStaccatoOff *ui;
  // methods
  auto montaFiltro() -> void;
  auto on_pushButtonCadastrar_clicked() -> void;
  auto on_pushButtonDescadastrar_clicked() -> void;
  auto on_pushButtonLimparFiltros_clicked() -> void;
  auto on_radioButtonEstoque_toggled(const bool checked) -> void;
  auto on_radioButtonStaccatoOFF_toggled(const bool checked) -> void;
  auto on_radioButtonTodos_toggled(const bool checked) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
