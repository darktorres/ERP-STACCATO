#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CancelaProduto;
}

class CancelaProduto : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { CompraConfirmar, CompraFaturamento, LogisticaColeta, LogisticaRecebimento, LogisticaEntregues, NFeEntrada };
  explicit CancelaProduto(const Tipo &tipo, QWidget *parent = nullptr);
  ~CancelaProduto();
  auto setFilter(const QString &ordemCompra) -> void;

private:
  // attributes
  const Tipo tipo;
  SqlTableModel model;
  Ui::CancelaProduto *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> bool;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
};
