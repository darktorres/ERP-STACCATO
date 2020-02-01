#pragma once

#include "sqlrelationaltablemodel.h"

#include <QDialog>

namespace Ui {
class FollowUp;
}

class FollowUp final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Orcamento, Venda };
  explicit FollowUp(const QString &id, const Tipo tipo, QWidget *parent = nullptr);
  ~FollowUp();

private:
  // attributes
  const QString id;
  const Tipo tipo;
  SqlRelationalTableModel modelViewFollowup;
  Ui::FollowUp *ui;
  // methods
  auto on_dateFollowup_dateChanged(const QDate &date) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
  auto verifyFields() -> bool;
};
