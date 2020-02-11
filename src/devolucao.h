#pragma once

#include "sqltablemodel.h"

#include <QDataWidgetMapper>
#include <QDialog>

namespace Ui {
class Devolucao;
}

class Devolucao final : public QDialog {
  Q_OBJECT

public:
  explicit Devolucao(const QString &idVenda, QWidget *parent = nullptr);
  ~Devolucao();

private:
  // attributes
  const QString idVenda;
  QDataWidgetMapper mapperProdutos;
  QString idDevolucao;
  SqlTableModel modelCliente;
  SqlTableModel modelDevolvidos1;
  SqlTableModel modelPagamentos;
  SqlTableModel modelProdutos2;
  SqlTableModel modelVenda;
  SqlTableModel modelConsumos;
  Ui::Devolucao *ui;
  // methods
  auto atualizarDevolucao() -> bool;
  auto calcPrecoItemTotal() -> void;
  auto criarContas() -> bool;
  auto criarDevolucao() -> bool;
  auto determinarIdDevolucao() -> std::optional<bool>;
  auto devolverItem(const int currentRow, const bool createNewId, const int novoIdVendaProduto2) -> bool;
  auto inserirItens(const int currentRow, const int novoIdVendaProduto2) -> bool;
  auto limparCampos() -> void;
  auto on_doubleSpinBoxCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxQuant_valueChanged(const double quant) -> void;
  auto on_doubleSpinBoxTotalItem_valueChanged(double value) -> void;
  auto on_pushButtonDevolverItem_clicked() -> void;
  auto on_tableProdutos_clicked(const QModelIndex &index) -> void;
  auto salvarCredito() -> bool;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
};
