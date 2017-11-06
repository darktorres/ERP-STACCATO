#ifndef PRECOESTOQUE_H
#define PRECOESTOQUE_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class PrecoEstoque;
}

class PrecoEstoque : public QDialog {
  Q_OBJECT

public:
  explicit PrecoEstoque(QWidget *parent = 0);
  ~PrecoEstoque();

private slots:
  void on_lineEditBusca_textChanged(const QString &text);
  void on_pushButtonCancelar_clicked();
  void on_pushButtonSalvar_clicked();
  void on_table_entered(const QModelIndex &);

private:
  // attributes
  SqlRelationalTableModel model;
  Ui::PrecoEstoque *ui;
  // methods
  void setupTables();
};

#endif // PRECOESTOQUE_H
