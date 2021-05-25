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
  Q_ENUM(Tipo)

  explicit InputDialogConfirmacao(const Tipo tipo, QWidget *parent);
  ~InputDialogConfirmacao() final;

  auto getDate() const -> QDate;
  auto getEntregou() const -> QString;
  auto getNextDateTime() const -> QDate;
  auto getRecebeu() const -> QString;
  auto setFilterEntrega(const QString &id, const QString &idEvento) -> void;
  auto setFilterRecebe(const QStringList &ids) -> void;

private:
  // attributes
  SqlTableModel modelEstoque;
  SqlTableModel modelVeiculo;
  Tipo const tipo;
  Ui::InputDialogConfirmacao *ui;
  // methods
  auto cadastrar() -> void;
  auto criarConsumoQuebrado(const int idEstoque, const double caixasDefeito, const double quantCaixa) -> void;
  auto criarReposicaoCliente(SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa, const QString obs, const int novoIdVendaProduto2) -> void;
  auto desfazerConsumo(const int idEstoque, const double caixasDefeito) -> void;
  auto dividirCompra(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) -> void;
  auto dividirConsumo(const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2, const QString idVendaProduto2) -> void;
  auto dividirEntrega(const int row, const int choice, const double caixasDefeito, const QString obs, const int novoIdVendaProduto2) -> void;
  auto dividirRecebimento(const int row, const double caixasDefeito, const double quantCaixa) -> void;
  auto dividirVeiculo(const int row, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) -> void;
  auto dividirVenda(SqlTableModel &modelVendaProduto, const double caixas, const double caixasDefeito, const double quantCaixa, const int novoIdVendaProduto2) -> void;
  auto gerarCreditoCliente(const SqlTableModel &modelVendaProduto, const double caixasDefeito, const double quantCaixa) -> void;
  auto getCaixasDefeito(const int row) -> double;
  auto on_dateEditEvento_dateChanged(const QDate &date) -> void;
  auto on_pushButtonFoto_clicked() -> void;
  auto on_pushButtonQuebradoEntrega_clicked() -> void;
  auto on_pushButtonQuebradoReceb_clicked() -> void;
  auto on_pushButtonSalvar_clicked() -> void;
  auto setConnections() -> void;
  auto setupTables() -> void;
};
