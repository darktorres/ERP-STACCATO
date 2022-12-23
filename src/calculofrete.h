#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class CalculoFrete;
}

class CalculoFrete : public QDialog {
  Q_OBJECT

public:
  explicit CalculoFrete(QWidget *parent = nullptr);
  ~CalculoFrete();

  auto getDistancia() -> double;
  auto getFrete() -> double;
  auto setCliente(const QVariant &idCliente) -> void;
  auto setOrcamento(const QVariant idEndereco, const double pesoSul, const double pesoTotal) -> void;

private:
  // attributes
  bool returnZero = false;
  SqlTableModel modelItem;
  Ui::CalculoFrete *ui;
  // methods
  auto on_comboBoxOrcamento_currentTextChanged(const QString &orcamento) -> void;
  auto on_comboBoxVenda_currentTextChanged(const QString &venda) -> void;
  auto on_itemBoxCliente_textChanged(const QString &text) -> void;
  auto on_itemBoxDestino_textChanged(const QString &text) -> void;
  auto on_pushButtonCalcular_clicked() -> void;
  auto qualp() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
