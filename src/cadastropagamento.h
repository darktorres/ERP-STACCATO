#pragma once

#include "sqltablemodel.h"

#include <QDataWidgetMapper>
#include <QDialog>

namespace Ui {
class CadastroPagamento;
}

class CadastroPagamento final : public QDialog {
  Q_OBJECT

public:
  explicit CadastroPagamento(QWidget *parent);
  ~CadastroPagamento();

private:
  // attributes
  QDataWidgetMapper mapperPagamento;
  SqlTableModel modelAssocia1;
  SqlTableModel modelAssocia2;
  SqlTableModel modelPagamentos;
  SqlTableModel modelTaxas;
  Ui::CadastroPagamento *ui;
  // methods
  auto adicionarPagamento() -> bool;
  auto atualizarPagamento() -> bool;
  auto limparSelecao() -> void;
  auto on_itemBoxLoja_idChanged(const QVariant &id) -> void;
  auto on_pushButtonAdicionaAssociacao_clicked() -> void;
  auto on_pushButtonAdicionarPagamento_clicked() -> void;
  auto on_pushButtonAtualizarPagamento_clicked() -> void;
  auto on_pushButtonAtualizarTaxas_clicked() -> void;
  auto on_pushButtonLimparSelecao_clicked() -> void;
  auto on_pushButtonRemoveAssociacao_clicked() -> void;
  auto on_pushButtonRemoverPagamento_clicked() -> void;
  auto on_tablePagamentos_clicked(const QModelIndex &index) -> void;
  auto setupMapper() -> void;
  auto setupTables() -> void;
  auto updateTables() -> void;
};
