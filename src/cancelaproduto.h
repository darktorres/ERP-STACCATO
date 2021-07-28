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
  Q_ENUM(Tipo)

  explicit CancelaProduto(const Tipo tipo, QWidget *parent);
  ~CancelaProduto();

  auto setFilter(const QString &ordemCompra) -> void;

private:
  // attributes
  SqlTableModel model;
  Tipo const tipo;
  Ui::CancelaProduto *ui;
  // methods
  auto cancelar(const QModelIndexList &list) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_pushButtonVoltar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
