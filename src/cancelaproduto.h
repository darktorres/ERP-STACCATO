#ifndef CANCELAPRODUTO_H
#define CANCELAPRODUTO_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class CancelaProduto;
}

class CancelaProduto : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { CompraConfirmar, CompraGerar, CompraFaturamento, LogisticaColeta, LogisticaEntregues, LogisticaRecebimento, NFeEntrada };
  explicit CancelaProduto(QWidget *parent = nullptr);
  ~CancelaProduto();
  auto setFilter(const QString &ordemCompra, const Tipo &tipo) -> void;

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::CancelaProduto *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
};

#endif // CANCELAPRODUTO_H
