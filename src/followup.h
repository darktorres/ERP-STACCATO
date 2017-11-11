#ifndef FOLLOWUP_H
#define FOLLOWUP_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class FollowUp;
}

class FollowUp final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Orcamento, Venda };
  explicit FollowUp(const QString &id, const Tipo tipo, QWidget *parent = 0);
  ~FollowUp();

private slots:
  void on_dateFollowup_dateChanged(const QDate &date);
  void on_pushButtonCancelar_clicked();
  void on_pushButtonSalvar_clicked();

private:
  // attributes
  const QString id;
  const Tipo tipo;
  SqlRelationalTableModel model;
  Ui::FollowUp *ui;
  // methods
  bool verifyFields();
  void setupTables();
};

#endif // FOLLOWUP_H
