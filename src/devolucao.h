#pragma once

#include <QDataWidgetMapper>
#include <QDialog>

#include "sqlrelationaltablemodel.h"

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
  QDataWidgetMapper mapperItem;
  QString idDevolucao;
  SqlRelationalTableModel modelCliente;
  SqlRelationalTableModel modelDevolvidos;
  SqlRelationalTableModel modelPagamentos;
  SqlRelationalTableModel modelProdutos;
  SqlRelationalTableModel modelVenda;
  SqlRelationalTableModel modelConsumos;
  Ui::Devolucao *ui;
  // methods
  auto atualizarDevolucao() -> bool;
  auto calcPrecoItemTotal() -> void;
  auto criarContas() -> bool;
  auto criarDevolucao() -> bool;
  auto determinarIdDevolucao() -> std::optional<bool>;
  auto devolverItem(const int currentRow, const bool createNewId, const int novoIdVendaProduto) -> bool;
  auto inserirItens(const int currentRow, const int novoIdVendaProduto) -> bool;
  auto limparCampos() -> void;
  auto on_doubleSpinBoxCaixas_valueChanged(const double caixas) -> void;
  auto on_doubleSpinBoxQuant_editingFinished() -> void;
  auto on_doubleSpinBoxQuant_valueChanged(double) -> void;
  auto on_doubleSpinBoxTotalItem_valueChanged(double value) -> void;
  auto on_pushButtonDevolverItem_clicked() -> void;
  auto on_tableProdutos_clicked(const QModelIndex &index) -> void;
  auto reservarIdVendaProduto() -> std::optional<int>;
  auto salvarCredito() -> bool;
  auto setupTables() -> void;
};
