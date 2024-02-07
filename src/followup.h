#pragma once

#include "sqlquerymodel.h"
#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class FollowUp;
}

class FollowUp final : public QDialog {
  Q_OBJECT

public:
  // TODO: adicionar um modo misto para mostrar Venda/Compra na mesma tela?
  enum class Tipo { Orcamento, Venda, Compra, Estoque, NFe };
  Q_ENUM(Tipo)

  explicit FollowUp(const QString &id, const Tipo tipo, QWidget *parent);
  ~FollowUp();

private:
  // attributes
  QString const id;
  SqlQueryModel modelMisto;
  SqlTableModel modelFollowup;
  SqlTableModel modelOrcamento;
  Tipo const tipo;
  Ui::FollowUp *ui;
  // methods
  auto on_dateFollowup_dateChanged(const QDate date) -> void;
  auto on_pushButtonCancelar_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
  auto verifyFields() -> void;
};
