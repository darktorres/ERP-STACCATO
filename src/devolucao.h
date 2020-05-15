#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class Devolucao;
}

class Devolucao final : public QDialog {
  Q_OBJECT

public:
  explicit Devolucao(const QString &idVenda, const bool isRepresentacao, QWidget *parent = nullptr);
  ~Devolucao();

private:
  // attributes
  const QString idVenda;
  const bool isRepresentacao;
  QString idDevolucao;
  SqlTableModel modelCliente;
  SqlTableModel modelDevolvidos1;
  SqlTableModel modelPagamentos;
  SqlTableModel modelProdutos2;
  SqlTableModel modelVenda;
  SqlTableModel modelCompra;
  SqlTableModel modelConsumos;
  Ui::Devolucao *ui;
  // methods
  auto alterarLinhaOriginal(const int currentRow) -> bool;
  auto atualizarDevolucao() -> bool;
  auto atualizarIdRelacionado(const int currentRow) -> bool;
  auto dividirConsumo(const int currentRow, const int novoIdVendaProduto2) -> bool;
  auto copiarProdutoParaDevolucao(const int currentRow) -> bool;
  auto criarContas() -> bool;
  auto criarDevolucao() -> bool;
  auto determinarIdDevolucao() -> bool;
  auto devolverItem(const int currentRow, const int novoIdVendaProduto2) -> bool;
  auto dividirCompra(const int currentRow, const int novoIdVendaProduto2) -> bool;
  auto dividirVenda(const int currentRow, const int novoIdVendaProduto2) -> bool;
  auto inserirItens(const int currentRow, const int novoIdVendaProduto2) -> bool;
  auto lerConsumos(const int currentRow) -> bool;
  auto limparCampos() -> void;
  auto on_doubleSpinBoxCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxCredito_valueChanged(const double credito) -> void;
  auto on_doubleSpinBoxPorcentagem_valueChanged(const double porcentagem) -> void;
  auto on_doubleSpinBoxQuant_valueChanged(const double quant) -> void;
  auto on_pushButtonDevolverItem_clicked() -> void;
  auto on_tableProdutos_clicked(const QModelIndex &index) -> void;
  auto salvarCredito() -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
