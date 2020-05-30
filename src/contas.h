#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class Contas;
}

class Contas final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Pagar, Receber };
  explicit Contas(const Tipo tipo, QWidget *parent);
  ~Contas();
  auto viewContaPagar(const QString &dataPagamento) -> void;
  auto viewContaReceber(const QString &idPagamento, const QString &contraparte) -> void;

private:
  // attributes
  const Tipo tipo;
  SqlTableModel modelPendentes;
  SqlTableModel modelProcessados;
  Ui::Contas *ui;
  // methods
  auto on_pushButtonSalvar_clicked() -> void;
  auto preencher(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto validarData(const QModelIndex &index) -> bool;
  auto verifyFields() -> bool;
};
