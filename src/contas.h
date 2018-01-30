#ifndef CONTAS_H
#define CONTAS_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class Contas;
}

class Contas final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Pagar, Receber };
  explicit Contas(const Tipo tipo, QWidget *parent = nullptr);
  ~Contas();
  void viewConta(const QString &idPagamento, const QString &contraparte);

private slots:
  void on_pushButtonSalvar_clicked();
  void on_tablePendentes_entered(const QModelIndex &);
  void on_tableProcessados_entered(const QModelIndex &);

private:
  // attributes
  const Tipo tipo;
  SqlRelationalTableModel modelPendentes;
  SqlRelationalTableModel modelProcessados;
  Ui::Contas *ui;
  // methods
  bool verifyFields();
  void preencher(const QModelIndex &index);
  void setupTables();
  void validarData(const QModelIndex &index);
};

#endif // CONTAS_H
