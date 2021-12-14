#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class BaixaOrcamento;
}

class BaixaOrcamento final : public QDialog {
  Q_OBJECT

public:
  explicit BaixaOrcamento(const QString &idOrcamento, QWidget *parent);
  ~BaixaOrcamento() final;

private:
  // attributes
  SqlTableModel modelOrcamento;
  Ui::BaixaOrcamento *ui;
  // methods
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables(const QString &idOrcamento) -> void;
};
