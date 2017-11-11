#ifndef INSERIRTRANSFERENCIA_H
#define INSERIRTRANSFERENCIA_H

#include "sqlrelationaltablemodel.h"

#include <QDialog>

namespace Ui {
class InserirTransferencia;
}

class InserirTransferencia final : public QDialog {
  Q_OBJECT

public:
  explicit InserirTransferencia(QWidget *parent = 0);
  ~InserirTransferencia();

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_pushButtonSalvar_clicked();
  void on_pushButtonCancelar_clicked();

private:
  SqlRelationalTableModel modelDe;
  SqlRelationalTableModel modelPara;
  Ui::InserirTransferencia *ui;
  bool verifyFields();
  void setupTables();
  bool cadastrar();
};

#endif // INSERIRTRANSFERENCIA_H
