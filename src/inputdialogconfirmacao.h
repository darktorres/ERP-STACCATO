#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class InputDialogConfirmacao;
}

class InputDialogConfirmacao final : public QDialog {
  Q_OBJECT

public:
  enum class Tipo { Recebimento, Entrega, Representacao };

  explicit InputDialogConfirmacao(const Tipo tipo, QWidget *parent);
  ~InputDialogConfirmacao() final;
  auto getDate() const -> QDate;
  auto getEntregou() const -> QString;
  auto getNextDateTime() const -> QDate;
  auto getRecebeu() const -> QString;
  auto setFilterEntrega(const QString &id, const QString &idEvento) -> bool;
  auto setFilterRecebe(const QStringList &ids) -> bool;

private:
  // attributes
  const Tipo tipo;
  SqlTableModel modelEstoque;
  SqlTableModel modelVeiculo;
  Ui::InputDialogConfirmacao *ui;
  // methods
  auto cadastrar() -> bool;
  auto criarConsumoQuebrado(const int idEstoque, const double caixasDefeito, const double quantCaixa) -> bool;
  auto criarReposicaoCliente(SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa, const QString obs, const int novoIdVendaProduto2) -> bool;
  auto desfazerConsumo(const int idEstoque, const double caixasDefeito) -> bool;
  auto dividirCompra(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) -> bool;
  auto dividirConsumo(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) -> bool;
  auto dividirEntrega(const int row, const int choice, const double caixasDefeito, const QString obs, const int novoIdVendaProduto2) -> bool;
  auto dividirRecebimento(const int row, const double caixasDefeito, const double quantCaixa) -> bool;
  auto dividirVeiculo(const int row, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) -> bool;
  auto dividirVenda(SqlTableModel &modelVendaProduto, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) -> bool;
  auto gerarCreditoCliente(const SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa) -> bool;
  auto getCaixasDefeito(const int row) -> std::optional<double>;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_pushButtonFoto_clicked() -> void;
  auto on_pushButtonQuebradoEntrega_clicked() -> void;
  auto on_pushButtonQuebradoReceb_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setupTables() -> void;
};
