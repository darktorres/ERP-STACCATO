#ifndef INPUTDIALOGCONFIRMACAO_H
#define INPUTDIALOGCONFIRMACAO_H

#include <QDialog>

#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialogConfirmacao;
}

class InputDialogConfirmacao final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Recebimento, Entrega, Representacao };

  explicit InputDialogConfirmacao(const Tipo &tipo, QWidget *parent = 0);
  ~InputDialogConfirmacao();
  QDateTime getDateTime() const;
  QDateTime getNextDateTime() const;
  QString getRecebeu() const;
  QString getEntregou() const;
  bool setFilter(const QStringList &ids);
  // TODO: convert functions to trailing return syntax?
  //  auto setFilter(const QStringList &ids) -> bool;
  bool setFilter(const QString &id, const QString &idEvento);

signals:
  void errorSignal(const QString &error);
  void transactionEnded();
  void transactionStarted();

private slots:
  void on_dateEditEvento_dateChanged(const QDate &date);
  void on_pushButtonFaltando_clicked();
  void on_pushButtonQuebrado_clicked();
  void on_pushButtonSalvar_clicked();

private:
  // attributes
  const Tipo tipo;
  SqlRelationalTableModel model; // REFAC: separate this into 2 models? one for each table
  SqlRelationalTableModel modelCliente;
  SqlRelationalTableModel modelVenda;
  Ui::InputDialogConfirmacao *ui;
  // temp
  int choice;
  int caixasDefeito;
  double unCaixa;
  //

  // methods
  bool cadastrar();
  bool criarConsumo(const int row);
  bool criarReposicaoCliente();
  bool desfazerConsumo(const int idEstoque);
  bool gerarCreditoCliente();
  bool processarQuebra(const int row);
  bool quebraEntrega(const int row);
  bool quebraRecebimento(const int row);
  bool quebrarLinha(const int row, const int caixas);
  void setupTables();
};

#endif // INPUTDIALOGCONFIRMACAO_H
