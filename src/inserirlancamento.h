#ifndef INSERIRLANCAMENTO_H
#define INSERIRLANCAMENTO_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

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
  auto openPersistentEditor() -> void;
  auto setupTables() -> void;
  auto verifyFields() -> bool;
};

#endif // INSERIRLANCAMENTO_H
