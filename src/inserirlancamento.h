#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class InserirLancamento;
}

class InserirLancamento final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Pagar, Receber };
  explicit InserirLancamento(const Tipo tipo, QWidget *parent);
  ~InserirLancamento();

private:
  // attributes
  const Tipo tipo;
  SqlTableModel modelContaPagamento;
  Ui::InserirLancamento *ui;
  // methods
  auto on_pushButtonCriarLancamento_clicked() -> void;
  auto on_pushButtonDuplicarLancamento_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto preencher(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto verifyFields() -> bool;
};
