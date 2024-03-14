#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CompraAvulsa;
}

class CompraAvulsa : public QDialog {
  Q_OBJECT

public:
  explicit CompraAvulsa(QWidget *parent = nullptr);
  ~CompraAvulsa();  

private:
  // attributes
  QStack<int> blockingSignals;
  SqlTableModel modelCompra;
  SqlTableModel modelPagar;
  Ui::CompraAvulsa *ui;
  // methods
  auto on_itemBoxNFe_textChanged(const QString &text) -> void;
  auto on_pushButtonAdicionarPagamento_clicked() -> void;
  auto on_pushButtonAdicionarProduto_clicked() -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonRemoverLinhasPagamento_clicked() -> void;
  auto on_pushButtonRemoverLinhasProdutos_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto on_tablePagar_dataChanged(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto verifyFields() -> void;
};
