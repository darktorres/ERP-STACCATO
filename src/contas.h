#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QStack>

namespace Ui {
class Contas;
}

class Contas final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Pagar, Receber };
  Q_ENUM(Tipo)

  explicit Contas(const Tipo tipo, QWidget *parent);
  ~Contas() final;

  auto viewContaPagarContraparte(const QString &contraparte) -> void;
  auto viewContaPagarData(const QString &dataPagamento) -> void;
  auto viewContaPagarOrdemCompra(const QString &ordemCompra) -> void;
  auto viewContaPagarPgt(const QString &idPagamento) -> void;
  auto viewContaReceber(const QString &idPagamento, const QString &contraparte) -> void;
  auto viewContaReceberContraparte(const QString &contraparte) -> void;
  auto viewContaReceberPgt(const QString &idPagamento) -> void;

private:
  // attributes
  QStack<int> blockingSignals;
  SqlTableModel modelPendentes;
  SqlTableModel modelProcessados;
  Tipo const tipo;
  Ui::Contas *ui;
  // methods
  auto on_checkBoxMostrarCancelados_toggled(const bool checked) -> void;
  auto on_pushButtonCriarLancamento_clicked() -> void;
  auto on_pushButtonDuplicarLancamento_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto preencher(const QModelIndex &index) -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto somarPendentes() -> void;
  auto somarProcessados() -> void;
  auto unsetConnections() -> void;
  auto validarData(const QModelIndex &index) -> void;
  auto verifyFields() -> void;
};
