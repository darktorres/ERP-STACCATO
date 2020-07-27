#pragma once

#include "sqltablemodel.h"

#include <QDialog>

namespace Ui {
class Estoque;
}

class Estoque final : public QDialog {
  Q_OBJECT

public:
  // TODO: turn showWindow into a enum
  explicit Estoque(const QString &idEstoque, const bool showWindow = true, QWidget *parent = nullptr);
  ~Estoque();
  auto criarConsumo(const int idVendaProduto2, const double quant = 0) -> bool;
  static auto desfazerConsumo(const int idVendaProduto2) -> bool;

private:
  // attributes
  const QString idEstoque; // TODO: change this to int?
  SqlTableModel modelEstoque;
  SqlTableModel modelConsumo;
  SqlTableModel modelViewConsumo;
  Ui::Estoque *ui;

  enum class FieldColors {
    White = 0,     // Não processado
    Green = 1,     // Ok
    Yellow = 2,    // Quant difere
    Red = 3,       // Não encontrado
    DarkGreen = 4, // Consumo
    Cyan = 5       // Devolução
  };

  // methods
  auto buscarRestante() -> void;
  auto dividirCompra(const int idVendaProduto2, const double quant) -> bool;
  auto exibirNota() -> void;
  auto on_pushButtonExibirNfe_clicked() -> void;
  auto setupTables() -> void;
  auto viewRegisterById(const bool showWindow) -> bool;
};
