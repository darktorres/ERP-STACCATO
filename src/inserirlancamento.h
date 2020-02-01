#pragma once

#include "sqlrelationaltablemodel.h"

#include <QDialog>

namespace Ui {
class InserirLancamento;
}

class InserirLancamento final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Pagar, Receber };
  explicit InserirLancamento(const Tipo tipo, QWidget *parent = nullptr);
  ~InserirLancamento();

private:
  // attributes
  const Tipo tipo;
  SqlRelationalTableModel modelContaPagamento;
  Ui::InserirLancamento *ui;
  // methods
  auto on_pushButtonCriarLancamento_clicked() -> void;
  auto on_pushButtonDuplicarLancamento_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
  auto verifyFields() -> bool;
};
