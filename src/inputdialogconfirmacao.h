#ifndef INPUTDIALOGCONFIRMACAO_H
#define INPUTDIALOGCONFIRMACAO_H

#include "dialog.h"
#include "sqlrelationaltablemodel.h"

namespace Ui {
class InputDialogConfirmacao;
}

class InputDialogConfirmacao final : public Dialog {
  Q_OBJECT

public:
  enum class Tipo { Recebimento, Entrega, Representacao };

  explicit InputDialogConfirmacao(const Tipo tipo, QWidget *parent = nullptr);
  ~InputDialogConfirmacao() final;
  auto getDateTime() const -> QDateTime;
  auto getEntregou() const -> QString;
  auto getNextDateTime() const -> QDateTime;
  auto getRecebeu() const -> QString;
  auto setFilter(const QString &id, const QString &idEvento) -> bool;
  auto setFilter(const QStringList &ids) -> bool;

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
  auto cadastrar() -> bool;
  auto criarConsumo(const int row) -> bool;
  auto criarReposicaoCliente() -> bool;
  auto desfazerConsumo(const int idEstoque) -> bool;
  auto gerarCreditoCliente() -> bool;
  auto processarQuebra(const int row) -> bool;
  auto quebraEntrega(const int row) -> bool;
  auto quebraRecebimento(const int row) -> bool;
  auto quebrarLinha(const int row, const int caixas) -> bool;
  auto setupTables() -> void;
  void on_dateEditEvento_dateChanged(const QDate &date);
  void on_pushButtonFaltando_clicked();
  void on_pushButtonQuebrado_clicked();
  void on_pushButtonSalvar_clicked();
};

#endif // INPUTDIALOGCONFIRMACAO_H
