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
  explicit ProdutosPendentes(QWidget *parent = 0);
  ~ProdutosPendentes();
  void viewProduto(const QString &codComercial, const QString &idVenda);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_pushButtonComprar_clicked();
  void on_pushButtonConsumirEstoque_clicked();
  void on_tableProdutos_entered(const QModelIndex &);

private:
  // attributes
  QString codComercial;
  SqlRelationalTableModel modelProdutos;
  SqlRelationalTableModel modelViewProdutos;
  SqlQueryModel modelEstoque;
  Ui::ProdutosPendentes *ui;
  // methods
  bool atualizarVenda(const int row, const QDate &dataPrevista);
  bool comprar(const QModelIndexList &list, const QDate &dataPrevista);
  bool consumirEstoque(const int rowProduto, const int rowEstoque, const double quantConsumir, const double quantVenda);
  bool enviarExcedenteParaCompra(const int row, const QDate &dataPrevista);
  bool enviarProdutoParaCompra(const int row, const QDate &dataPrevista);
  bool insere(const QDateTime &dataPrevista);
  bool quebrarVenda(const double quantConsumir, const double quantVenda, const int rowProduto);
  void recalcularQuantidade();
  void recarregarTabelas();
  void setupTables();
};

#endif // PRODUTOSPENDENTES_H
