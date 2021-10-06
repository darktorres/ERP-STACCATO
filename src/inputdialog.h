#pragma once

#include <QDialog>

namespace Ui {
class InputDialog;
}

class InputDialog final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Carrinho, ReagendarFaturamento, AgendarColeta, Coleta, ReagendarRecebimento, ReagendarPedido, ReagendarEntrega };
  Q_ENUM(Tipo)

  explicit InputDialog(const Tipo tipo, QWidget *parent);
  ~InputDialog();

  auto getDataVeiculo() const -> QDateTime;
  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto getObservacao() const -> QString;
  auto getVeiculo() const -> int;
  auto setVeiculo(const int idVeiculo) -> void;

private:
  // attributes
  Tipo const tipo;
  Ui::InputDialog *ui;
  // methods
  auto on_dateEditEvento_dateChanged(const QDate date) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
};
