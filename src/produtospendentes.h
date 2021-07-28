#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class ProdutosPendentes;
}

class ProdutosPendentes final : public QDialog {
  Q_OBJECT

public:
  explicit ProdutosPendentes(QWidget *parent);
  ~ProdutosPendentes();

  auto viewProduto(const QString &fornecedor, const QString &codComercial, const QString &idVenda) -> void;

private:
  // attributes
  SqlQueryModel modelEstoque;
  SqlTableModel modelCompra;
  SqlTableModel modelProdutos;
  SqlTableModel modelViewProdutos;
  Ui::ProdutosPendentes *ui;
  // methods
  auto atualizarVenda(const int rowProduto) -> void;
  auto comprar(const QModelIndexList &list, const QDate dataPrevista) -> void;
  auto consumirCompra(const int rowProduto, const int rowCompra, const double quantConsumir, const double quantVenda) -> void;
  auto consumirEstoque(const int rowProduto, const int rowEstoque, const double quantConsumir, const double quantVenda) -> void;
  auto dividirCompra(const double quantSeparar, const double quantCompra, const int rowCompra) -> void;
  auto dividirVenda(const double quantSeparar, const double quantVenda, const int rowProduto) -> void;
  auto enviarExcedenteParaCompra(const int row, const QDate dataPrevista) -> void;
  auto enviarProdutoParaCompra(const int row, const QDate dataPrevista) -> void;
  auto on_pushButtonComprar_clicked() -> void;
  auto on_pushButtonConsumirCompra_clicked() -> void;
  auto on_pushButtonConsumirEstoque_clicked() -> void;
  auto recalcularQuantidade() -> void;
  auto recarregarTabelas() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
