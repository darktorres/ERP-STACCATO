#ifndef PRODUTOSPENDENTES_H
#define PRODUTOSPENDENTES_H

#include <QDialog>

#include "sqlquerymodel.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class ProdutosPendentes;
}

class ProdutosPendentes final : public QDialog {
  Q_OBJECT

public:
  explicit ProdutosPendentes(const QString &codComercial, const QString &idVenda, QWidget *parent = nullptr);
  ~ProdutosPendentes();

private:
  // attributes
  SqlRelationalTableModel modelProdutos;
  SqlRelationalTableModel modelViewProdutos;
  SqlQueryModel modelEstoque;
  Ui::ProdutosPendentes *ui;
  // methods
  auto atualizarVenda(const int row) -> bool;
  auto comprar(const QModelIndexList &list, const QDate &dataPrevista) -> bool;
  auto consumirEstoque(const int rowProduto, const int rowEstoque, const double quantConsumir, const double quantVenda) -> bool;
  auto enviarExcedenteParaCompra(const int row, const QDate &dataPrevista) -> bool;
  auto enviarProdutoParaCompra(const int row, const QDate &dataPrevista) -> bool;
  auto insere(const QDateTime &dataPrevista) -> bool;
  auto on_pushButtonComprar_clicked() -> void;
  auto on_pushButtonConsumirEstoque_clicked() -> void;
  auto on_tableProdutos_entered(const QModelIndex &) -> void;
  auto dividirVenda(const double quantConsumir, const double quantVenda, const int rowProduto) -> bool;
  auto recalcularQuantidade() -> void;
  auto recarregarTabelas() -> void;
  auto setupTables() -> void;
  auto viewProduto(const QString &codComercial, const QString &idVenda) -> void;
};

#endif // PRODUTOSPENDENTES_H
