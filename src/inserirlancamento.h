#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QStack>

namespace Ui {
class InserirLancamento;
}

class InserirLancamento final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Pagar, Receber };
  Q_ENUM(Tipo)

  explicit InserirLancamento(const Tipo tipo, QWidget *parent);
  ~InserirLancamento();

private:
  // attributes
  QStack<int> blockingSignals;
  SqlTableModel modelContaPagamento;
  Tipo const tipo;
  Ui::InserirLancamento *ui;
  // methods
  auto on_pushButtonCriarLancamento_clicked() -> void;
  auto on_pushButtonDuplicarLancamento_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto preencher(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto somarSelecao() -> void;
  auto unsetConnections() -> void;
  auto verifyFields() -> void;
};
