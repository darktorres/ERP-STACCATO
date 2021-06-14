#pragma once

#include "sqltablemodel.h"

#include <QDialog>
#include <QStack>
#include <QTimer>

namespace Ui {
class AnteciparRecebimento;
}

class AnteciparRecebimento final : public QDialog {
  Q_OBJECT

public:
  explicit AnteciparRecebimento(QWidget *parent);
  ~AnteciparRecebimento();

private:
  // attributes
  QStack<int> blockingSignals;
  QTimer timer;
  SqlTableModel modelContaReceber;
  Ui::AnteciparRecebimento *ui;
  // methods
  auto cadastrar(const QModelIndexList &list) -> void;
  auto calcularTotais() -> void;
  auto delayFiltro() -> void;
  auto fillComboBoxLoja() -> void;
  auto fillComboBoxPagamento() -> void;
  auto montaFiltro() -> void;
  auto on_comboBoxPagamento_currentTextChanged(const QString &text) -> void;
  auto on_doubleSpinBoxValorPresente_valueChanged(double) -> void;
  auto on_pushButtonGerar_clicked() -> void;
  auto selecionarTaxa() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto unsetConnections() -> void;
  auto verifyFields(const QModelIndexList &list) -> void;
};
