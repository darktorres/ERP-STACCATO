#ifndef FOLLOWUP_H
#define FOLLOWUP_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class FollowUp;
}

class FollowUp final : public Dialog {
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

#endif // FOLLOWUP_H
