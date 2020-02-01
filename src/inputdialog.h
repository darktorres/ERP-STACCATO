#pragma once

#include "sqlrelationaltablemodel.h"

#include <QDialog>

namespace Ui {
class InputDialog;
}

class InputDialog final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Carrinho, Faturamento, AgendarColeta, Coleta, AgendarRecebimento, AgendarEntrega, ReagendarPedido };

  explicit InputDialog(const Tipo &tipo, QWidget *parent = nullptr);
  ~InputDialog();
  auto getDate() const -> QDate;
  auto getNextDate() const -> QDate;
  auto getObservacao() const -> QString;

private:
  // attributes
  const Tipo tipo;
  Ui::InputDialog *ui;
  // methods
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_pushButtonSalvar_clicked() -> void;
};
