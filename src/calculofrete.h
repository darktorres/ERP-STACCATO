#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CalculoFrete;
}

class CalculoFrete : public QDialog {
  Q_OBJECT

public:
  explicit CalculoFrete(QWidget *parent);
  ~CalculoFrete();

  auto getDistancia() -> double;
  auto getFrete() -> double;
  auto setCliente(const QVariant &idCliente) -> void;
  auto setOrcamento(const QString &idOrcamento, const QString &endereco) -> void;

private:
  // attributes
  SqlTableModel modelItem;
  Ui::CalculoFrete *ui;
  // methods
  auto on_comboBoxOrcamento_currentTextChanged(const QString &orcamento) -> void;
  auto on_comboBoxVenda_currentTextChanged(const QString &venda) -> void;
  auto on_itemBoxCliente_textChanged(const QString &) -> void;
  auto on_pushButtonCalcular_clicked() -> void;
  auto qualp() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
