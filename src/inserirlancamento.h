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
  explicit InserirLancamento(const Tipo tipo, QWidget *parent = 0);
  ~InserirLancamento();

private slots:
  void on_pushButtonCriarLancamento_clicked();
  void on_pushButtonDuplicarLancamento_clicked();
  void on_pushButtonSalvar_clicked();

private:
  // attributes
  const Tipo tipo;
  SqlRelationalTableModel model;
  Ui::InserirLancamento *ui;
  // methods
  void setupTables();
  bool verifyFields();
  void openPersistentEditor();
};

#endif // INSERIRLANCAMENTO_H
